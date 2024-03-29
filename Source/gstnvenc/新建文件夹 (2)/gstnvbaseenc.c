/* GStreamer NVENC plugin
 * Copyright (C) 2015 Centricular Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstnvbaseenc.h"

#include <gst/pbutils/codec-utils.h>

#include <string.h>

#if HAVE_NVENC_GST_GL
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <gst/gl/gl.h>
#endif

/* TODO:
 *  - reset last_flow on FLUSH_STOP (seeking)
 */

#define N_BUFFERS_PER_FRAME 1
#define SUPPORTED_GL_APIS GST_GL_API_OPENGL3

/* magic pointer value we can put in the async queue to signal shut down */
#define SHUTDOWN_COOKIE ((gpointer)GINT_TO_POINTER (1))

#define parent_class gst_nv_base_enc_parent_class
G_DEFINE_ABSTRACT_TYPE (GstNvBaseEnc, gst_nv_base_enc, GST_TYPE_VIDEO_ENCODER);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " "format = (string) NV12, " // TODO: I420, YV12, Y444 support
        "width = (int) [ 16, 4096 ], height = (int) [ 16, 2160 ], "
        "framerate = (fraction) [0, MAX],"
        "interlace-mode = { progressive, mixed, interleaved } "
// #if HAVE_NVENC_GST_GL
//         ";"
//         "video/x-raw(memory:GLMemory), "
//         "format = (string) { NV12, Y444 }, "
//         "width = (int) [ 16, 4096 ], height = (int) [ 16, 2160 ], "
//         "framerate = (fraction) [0, MAX],"
//         "interlace-mode = { progressive, mixed, interleaved } "
// #endif
    ));

enum
{
  PROP_0,
  PROP_DEVICE_ID,
};

#if HAVE_NVENC_GST_GL
struct gl_input_resource
{
  GstGLMemory *gl_mem[GST_VIDEO_MAX_PLANES];
  struct cudaGraphicsResource *cuda_texture;
  gpointer cuda_plane_pointers[GST_VIDEO_MAX_PLANES];
  gpointer cuda_pointer;
  gsize cuda_stride;
  gsize cuda_num_bytes;
  NV_ENC_REGISTER_RESOURCE nv_resource;
  NV_ENC_MAP_INPUT_RESOURCE nv_mapped_resource;
};
#endif

struct frame_state
{
  gint n_buffers;
  gpointer in_bufs[N_BUFFERS_PER_FRAME];
  gpointer out_bufs[N_BUFFERS_PER_FRAME];
};

static gboolean gst_nv_base_enc_open (GstVideoEncoder * enc);
static gboolean gst_nv_base_enc_close (GstVideoEncoder * enc);
static gboolean gst_nv_base_enc_start (GstVideoEncoder * enc);
static gboolean gst_nv_base_enc_stop (GstVideoEncoder * enc);
static void gst_nv_base_enc_set_context (GstElement * element,
    GstContext * context);
static gboolean gst_nv_base_enc_sink_query (GstVideoEncoder * enc,
    GstQuery * query);
static gboolean gst_nv_base_enc_set_format (GstVideoEncoder * enc,
    GstVideoCodecState * state);
static GstFlowReturn gst_nv_base_enc_handle_frame (GstVideoEncoder * enc,
    GstVideoCodecFrame * frame);
static void gst_nv_base_enc_free_buffers (GstNvBaseEnc * nvenc);
static GstFlowReturn gst_nv_base_enc_finish (GstVideoEncoder * enc);
static void gst_nv_base_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_nv_base_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_nv_base_enc_finalize (GObject * obj);
static GstCaps *gst_nv_base_enc_getcaps (GstVideoEncoder * enc,
    GstCaps * filter);

static void
gst_nv_base_enc_class_init (GstNvBaseEncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoEncoderClass *videoenc_class = GST_VIDEO_ENCODER_CLASS (klass);

  gobject_class->set_property = gst_nv_base_enc_set_property;
  gobject_class->get_property = gst_nv_base_enc_get_property;
  gobject_class->finalize = gst_nv_base_enc_finalize;

  element_class->set_context = GST_DEBUG_FUNCPTR (gst_nv_base_enc_set_context);

  videoenc_class->open = GST_DEBUG_FUNCPTR (gst_nv_base_enc_open);
  videoenc_class->close = GST_DEBUG_FUNCPTR (gst_nv_base_enc_close);

  videoenc_class->start = GST_DEBUG_FUNCPTR (gst_nv_base_enc_start);
  videoenc_class->stop = GST_DEBUG_FUNCPTR (gst_nv_base_enc_stop);

  videoenc_class->set_format = GST_DEBUG_FUNCPTR (gst_nv_base_enc_set_format);
  videoenc_class->getcaps = GST_DEBUG_FUNCPTR (gst_nv_base_enc_getcaps);
  videoenc_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_nv_base_enc_handle_frame);
  videoenc_class->finish = GST_DEBUG_FUNCPTR (gst_nv_base_enc_finish);
  videoenc_class->sink_query = GST_DEBUG_FUNCPTR (gst_nv_base_enc_sink_query);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  g_object_class_install_property (gobject_class, PROP_DEVICE_ID,
      g_param_spec_uint ("cuda-device-id",
          "Cuda Device ID",
          "Set the GPU device to use for operations",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gboolean
_get_supported_input_formats (GstNvBaseEnc * nvenc)
{
  GstNvBaseEncClass *nvenc_class = GST_NV_BASE_ENC_GET_CLASS (nvenc);
  guint64 format_mask = 0;
  uint32_t i, num = 0;
  NV_ENC_BUFFER_FORMAT formats[64];
  GValue list = G_VALUE_INIT;
  GValue val = G_VALUE_INIT;

  GstNvEncGetInputFormats(nvenc->encoder, nvenc_class->codec_id, formats,
      G_N_ELEMENTS (formats), &num);

  for (i = 0; i < num; ++i) {
    GST_INFO_OBJECT (nvenc, "input format: 0x%08x", formats[i]);
    /* Apparently we can just ignore the tiled formats and can feed
     * it the respective untiled planar format instead ?! */
    switch (formats[i]) {
      case NV_ENC_BUFFER_FORMAT_NV12_PL:
      case NV_ENC_BUFFER_FORMAT_NV12_TILED16x16:
      case NV_ENC_BUFFER_FORMAT_NV12_TILED64x16:
        format_mask |= (1 << GST_VIDEO_FORMAT_NV12);
        break;
      case NV_ENC_BUFFER_FORMAT_YV12_PL:
      case NV_ENC_BUFFER_FORMAT_YV12_TILED16x16:
      case NV_ENC_BUFFER_FORMAT_YV12_TILED64x16:
        format_mask |= (1 << GST_VIDEO_FORMAT_YV12);
        break;
      case NV_ENC_BUFFER_FORMAT_IYUV_PL:
      case NV_ENC_BUFFER_FORMAT_IYUV_TILED16x16:
      case NV_ENC_BUFFER_FORMAT_IYUV_TILED64x16:
        format_mask |= (1 << GST_VIDEO_FORMAT_I420);
        break;
      case NV_ENC_BUFFER_FORMAT_YUV444_PL:
      case NV_ENC_BUFFER_FORMAT_YUV444_TILED16x16:
      case NV_ENC_BUFFER_FORMAT_YUV444_TILED64x16:{
        NV_ENC_CAPS_PARAM caps_param = { 0, };
        int yuv444_supported = 0;

        caps_param.version = NV_ENC_CAPS_PARAM_VER;
        caps_param.capsToQuery = NV_ENC_CAPS_SUPPORT_YUV444_ENCODE;

        if (GstNvEncGetEncodeCaps (nvenc->encoder, nvenc_class->codec_id,
                &caps_param, &yuv444_supported) != NV_ENC_SUCCESS)
          yuv444_supported = 0;

        if (yuv444_supported)
          format_mask |= (1 << GST_VIDEO_FORMAT_Y444);
        break;
      }
      default:
        GST_FIXME ("unmapped input format: 0x%08x", formats[i]);
        break;
    }
  }

  if (format_mask == 0)
    return FALSE;

  /* process a second time so we can add formats in the order we want */
  g_value_init (&list, GST_TYPE_LIST);
  g_value_init (&val, G_TYPE_STRING);
  if ((format_mask & (1 << GST_VIDEO_FORMAT_NV12))) {
    g_value_set_static_string (&val, "NV12");
    gst_value_list_append_value (&list, &val);
  }
  if ((format_mask & (1 << GST_VIDEO_FORMAT_YV12))) {
    g_value_set_static_string (&val, "YV12");
    gst_value_list_append_value (&list, &val);
  }
  if ((format_mask & (1 << GST_VIDEO_FORMAT_I420))) {
    g_value_set_static_string (&val, "I420");
    gst_value_list_append_value (&list, &val);
  }
  if ((format_mask & (1 << GST_VIDEO_FORMAT_Y444))) {
    g_value_set_static_string (&val, "Y444");
    gst_value_list_append_value (&list, &val);
  }
  g_value_unset (&val);

  GST_OBJECT_LOCK (nvenc);
  g_free (nvenc->input_formats);
  nvenc->input_formats = g_memdup (&list, sizeof (GValue));
  GST_OBJECT_UNLOCK (nvenc);

  return TRUE;
}

static gboolean
gst_nv_base_enc_open (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  nvenc->cuda_ctx = gst_nvenc_create_cuda_context (nvenc->cuda_device_id);
  if (nvenc->cuda_ctx == NULL) {
    GST_ELEMENT_ERROR (enc, LIBRARY, INIT, (NULL),
        ("Failed to create CUDA context, perhaps CUDA is not supported."));
    return FALSE;
  }

  {
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = { 0, };
    NVENCSTATUS nv_ret;

    params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    params.apiVersion = NVENCAPI_VERSION;
    params.device = nvenc->cuda_ctx;
    params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
	nv_ret = GstNvEncOpenEncodeSessionEx(&params, &nvenc->encoder);
    if (nv_ret != NV_ENC_SUCCESS) {
      GST_ERROR ("Failed to create NVENC encoder session, ret=%d", nv_ret);
      if (gst_nvenc_destroy_cuda_context (nvenc->cuda_ctx))
        nvenc->cuda_ctx = NULL;
      return FALSE;
    }
    GST_INFO ("created NVENC encoder %p", nvenc->encoder);
  }

  /* query supported input formats */
  if (!_get_supported_input_formats (nvenc)) {
    GST_WARNING_OBJECT (nvenc, "No supported input formats");
    gst_nv_base_enc_close (enc);
    return FALSE;
  }

  return TRUE;
}

static void
gst_nv_base_enc_set_context (GstElement * element, GstContext * context)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (element);

#if HAVE_NVENC_GST_GL
  gst_gl_handle_set_context (element, context,
      (GstGLDisplay **) & nvenc->display,
      (GstGLContext **) & nvenc->other_context);
  if (nvenc->display)
    gst_gl_display_filter_gl_api (GST_GL_DISPLAY (nvenc->display),
        SUPPORTED_GL_APIS);
#endif

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

static gboolean
gst_nv_base_enc_sink_query (GstVideoEncoder * enc, GstQuery * query)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  switch (GST_QUERY_TYPE (query)) {
#if HAVE_NVENC_GST_GL
    case GST_QUERY_CONTEXT:{
      gboolean ret;

      ret = gst_gl_handle_context_query ((GstElement *) nvenc, query,
          (GstGLDisplay **) & nvenc->display,
          (GstGLContext **) & nvenc->other_context);
      if (nvenc->display)
        gst_gl_display_filter_gl_api (GST_GL_DISPLAY (nvenc->display),
            SUPPORTED_GL_APIS);

      if (ret)
        return ret;
      break;
    }
#endif
    default:
      break;
  }

  return GST_VIDEO_ENCODER_CLASS (parent_class)->sink_query (enc, query);
}

static gboolean
gst_nv_base_enc_start (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  nvenc->bitstream_pool = g_async_queue_new ();
  nvenc->bitstream_queue = g_async_queue_new ();
  nvenc->in_bufs_pool = g_async_queue_new ();

  nvenc->last_flow = GST_FLOW_OK;

#if HAVE_NVENC_GST_GL
  {
    gst_gl_ensure_element_data (GST_ELEMENT (nvenc),
        (GstGLDisplay **) & nvenc->display,
        (GstGLContext **) & nvenc->other_context);
    if (nvenc->display)
      gst_gl_display_filter_gl_api (GST_GL_DISPLAY (nvenc->display),
          SUPPORTED_GL_APIS);
  }
#endif

  return TRUE;
}

static gboolean
gst_nv_base_enc_stop (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  gst_nv_base_enc_free_buffers (nvenc);

  if (nvenc->bitstream_pool) {
    g_async_queue_unref (nvenc->bitstream_pool);
    nvenc->bitstream_pool = NULL;
  }
  if (nvenc->bitstream_queue) {
    g_async_queue_unref (nvenc->bitstream_queue);
    nvenc->bitstream_queue = NULL;
  }
  if (nvenc->in_bufs_pool) {
    g_async_queue_unref (nvenc->in_bufs_pool);
    nvenc->in_bufs_pool = NULL;
  }
  if (nvenc->display) {
    gst_object_unref (nvenc->display);
    nvenc->display = NULL;
  }
  if (nvenc->other_context) {
    gst_object_unref (nvenc->other_context);
    nvenc->other_context = NULL;
  }

  return TRUE;
}

static GValue *
_get_interlace_modes (GstNvBaseEnc * nvenc)
{
  GstNvBaseEncClass *nvenc_class = GST_NV_BASE_ENC_GET_CLASS (nvenc);
  NV_ENC_CAPS_PARAM caps_param = { 0, };
  GValue *list = g_new0 (GValue, 1);
  GValue val = G_VALUE_INIT;

  g_value_init (list, GST_TYPE_LIST);
  g_value_init (&val, G_TYPE_STRING);

  g_value_set_static_string (&val, "progressive");
  gst_value_list_append_value (list, &val);

  caps_param.version = NV_ENC_CAPS_PARAM_VER;
  caps_param.capsToQuery = NV_ENC_CAPS_SUPPORT_FIELD_ENCODING;

  if (GstNvEncGetEncodeCaps (nvenc->encoder, nvenc_class->codec_id,
          &caps_param, &nvenc->interlace_modes) != NV_ENC_SUCCESS)
    nvenc->interlace_modes = 0;

  if (nvenc->interlace_modes >= 1) {
    g_value_set_static_string (&val, "interleaved");
    gst_value_list_append_value (list, &val);
    g_value_set_static_string (&val, "mixed");
    gst_value_list_append_value (list, &val);
  }
  /* TODO: figure out what nvenc frame based interlacing means in gst terms */

  return list;
}

static GstCaps *
gst_nv_base_enc_getcaps (GstVideoEncoder * enc, GstCaps * filter)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);
  GstCaps *supported_incaps = NULL;
  GstCaps *template_caps, *caps;

  GST_OBJECT_LOCK (nvenc);

  if (nvenc->input_formats != NULL) {
    GValue *val;

    template_caps = gst_pad_get_pad_template_caps (enc->sinkpad);
    supported_incaps = gst_caps_copy (template_caps);
    gst_caps_set_value (supported_incaps, "format", nvenc->input_formats);

    val = _get_interlace_modes (nvenc);
    gst_caps_set_value (supported_incaps, "interlace-mode", val);
    g_free (val);

    GST_LOG_OBJECT (enc, "codec input caps %" GST_PTR_FORMAT, supported_incaps);
    GST_LOG_OBJECT (enc, "   template caps %" GST_PTR_FORMAT, template_caps);
    caps = gst_caps_intersect (template_caps, supported_incaps);
    gst_caps_unref (template_caps);
    gst_caps_unref (supported_incaps);
    supported_incaps = caps;
    GST_LOG_OBJECT (enc, "  supported caps %" GST_PTR_FORMAT, supported_incaps);
  }

  GST_OBJECT_UNLOCK (nvenc);

  caps = gst_video_encoder_proxy_getcaps (enc, supported_incaps, filter);

  if (supported_incaps)
    gst_caps_unref (supported_incaps);

  GST_DEBUG_OBJECT (nvenc, "  returning caps %" GST_PTR_FORMAT, caps);

  return caps;
}

static gboolean
gst_nv_base_enc_close (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  if (nvenc->encoder) {
	  if (GstNvEncDestroyEncoder(nvenc->encoder) != NV_ENC_SUCCESS)
      return FALSE;
    nvenc->encoder = NULL;
  }

  if (nvenc->cuda_ctx) {
    if (!gst_nvenc_destroy_cuda_context (nvenc->cuda_ctx))
      return FALSE;
    nvenc->cuda_ctx = NULL;
  }

  GST_OBJECT_LOCK (nvenc);
  g_free (nvenc->input_formats);
  nvenc->input_formats = NULL;
  GST_OBJECT_UNLOCK (nvenc);

  if (nvenc->input_state) {
    gst_video_codec_state_unref (nvenc->input_state);
    nvenc->input_state = NULL;
  }

  if (nvenc->bitstream_pool != NULL) {
    g_assert (g_async_queue_length (nvenc->bitstream_pool) == 0);
    g_async_queue_unref (nvenc->bitstream_pool);
    nvenc->bitstream_pool = NULL;
  }

  return TRUE;
}

static void
gst_nv_base_enc_init (GstNvBaseEnc * nvenc)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (nvenc);

  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
}

static void
gst_nv_base_enc_finalize (GObject * obj)
{
  G_OBJECT_CLASS (gst_nv_base_enc_parent_class)->finalize (obj);
}

static GstVideoCodecFrame *
_find_frame_with_output_buffer (GstNvBaseEnc * nvenc, NV_ENC_OUTPUT_PTR out_buf)
{
  GList *l = gst_video_encoder_get_frames (GST_VIDEO_ENCODER (nvenc));
  gint i;

  for (; l; l = l->next) {
    GstVideoCodecFrame *frame = (GstVideoCodecFrame *) l->data;
    struct frame_state *state = frame->user_data;

    for (i = 0; i < N_BUFFERS_PER_FRAME; i++) {
      if (!state->out_bufs[i])
        break;

      if (state->out_bufs[i] == out_buf)
        return frame;
    }
  }

  return NULL;
}

static gpointer
gst_nv_base_enc_bitstream_thread (gpointer user_data)
{
  GstVideoEncoder *enc = user_data;
  GstNvBaseEnc *nvenc = user_data;

  /* overview of operation:
   * 1. retreive the next buffer submitted to the bitstream pool
   * 2. wait for that buffer to be ready from nvenc (LockBitsream)
   * 3. retreive the GstVideoCodecFrame associated with that buffer
   * 4. for each buffer in the frame
   * 4.1 (step 2): wait for that buffer to be ready from nvenc (LockBitsream)
   * 4.2 create an output GstBuffer from the nvenc buffers
   * 4.3 unlock the nvenc bitstream buffers UnlockBitsream
   * 5. finish_frame()
   * 6. cleanup
   */
  do {
    GstBuffer *buffers[N_BUFFERS_PER_FRAME];
    struct frame_state *state = NULL;
    GstVideoCodecFrame *frame = NULL;
    NVENCSTATUS nv_ret;
    GstFlowReturn flow = GST_FLOW_OK;
    gint i;

    {
      NV_ENC_LOCK_BITSTREAM lock_bs = { 0, };
      NV_ENC_OUTPUT_PTR out_buf;

      for (i = 0; i < N_BUFFERS_PER_FRAME; i++) {
        /* get and lock bitstream buffers */
        GstVideoCodecFrame *tmp_frame;

        if (state && i >= state->n_buffers)
          break;

        GST_LOG_OBJECT (enc, "wait for bitstream buffer..");

        /* assumes buffers are submitted in order */
        out_buf = g_async_queue_pop (nvenc->bitstream_queue);
        if ((gpointer) out_buf == SHUTDOWN_COOKIE)
          break;

        GST_LOG_OBJECT (nvenc, "waiting for output buffer %p to be ready",
            out_buf);

        lock_bs.version = NV_ENC_LOCK_BITSTREAM_VER;
        lock_bs.outputBitstream = out_buf;
        lock_bs.doNotWait = 0;

        /* FIXME: this would need to be updated for other slice modes */
        lock_bs.sliceOffsets = NULL;

        nv_ret = GstNvEncLockBitstream (nvenc->encoder, &lock_bs);
        if (nv_ret != NV_ENC_SUCCESS) {
          /* FIXME: what to do here? */
          GST_ELEMENT_ERROR (nvenc, STREAM, ENCODE, (NULL),
              ("Failed to lock bitstream buffer %p, ret %d",
                  lock_bs.outputBitstream, nv_ret));
          out_buf = SHUTDOWN_COOKIE;
          break;
        }

        GST_LOG_OBJECT (nvenc, "picture type %d", lock_bs.pictureType);

        tmp_frame = _find_frame_with_output_buffer (nvenc, out_buf);
        g_assert (tmp_frame != NULL);
        if (frame)
          g_assert (frame == tmp_frame);
        frame = tmp_frame;

        state = frame->user_data;
        g_assert (state->out_bufs[i] == out_buf);

        /* copy into output buffer */
        buffers[i] =
            gst_buffer_new_allocate (NULL, lock_bs.bitstreamSizeInBytes, NULL);
        gst_buffer_fill (buffers[i], 0, lock_bs.bitstreamBufferPtr,
            lock_bs.bitstreamSizeInBytes);

        if (lock_bs.pictureType == NV_ENC_PIC_TYPE_IDR) {
          GST_DEBUG_OBJECT (nvenc, "This is a keyframe");
          GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
        }

        /* TODO: use lock_bs.outputTimeStamp and lock_bs.outputDuration */
        /* TODO: check pts/dts is handled properly if there are B-frames */

        nv_ret = GstNvEncUnlockBitstream (nvenc->encoder, state->out_bufs[i]);
        if (nv_ret != NV_ENC_SUCCESS) {
          /* FIXME: what to do here? */
          GST_ELEMENT_ERROR (nvenc, STREAM, ENCODE, (NULL),
              ("Failed to unlock bitstream buffer %p, ret %d",
                  lock_bs.outputBitstream, nv_ret));
          state->out_bufs[i] = SHUTDOWN_COOKIE;
          break;
        }

        GST_LOG_OBJECT (nvenc, "returning bitstream buffer %p to pool",
            state->out_bufs[i]);
        g_async_queue_push (nvenc->bitstream_pool, state->out_bufs[i]);
      }

      if (out_buf == SHUTDOWN_COOKIE)
        break;
    }

    {
      GstBuffer *output_buffer = gst_buffer_new ();

      for (i = 0; i < state->n_buffers; i++)
        output_buffer = gst_buffer_append (output_buffer, buffers[i]);

      frame->output_buffer = output_buffer;
    }

    for (i = 0; i < state->n_buffers; i++) {
      void *in_buf = state->in_bufs[i];
      g_assert (in_buf != NULL);

#if HAVE_NVENC_GST_GL
      if (nvenc->gl_input) {
        struct gl_input_resource *in_gl_resource = in_buf;

        nv_ret =
            GstNvEncUnmapInputResource (nvenc->encoder,
            in_gl_resource->nv_mapped_resource.mappedResource);
        if (nv_ret != NV_ENC_SUCCESS) {
          GST_ERROR_OBJECT (nvenc, "Failed to unmap input resource %p, ret %d",
              in_gl_resource, nv_ret);
          break;
        }

        memset (&in_gl_resource->nv_mapped_resource, 0,
            sizeof (in_gl_resource->nv_mapped_resource));
      }
#endif

      g_async_queue_push (nvenc->in_bufs_pool, in_buf);
    }

    flow = gst_video_encoder_finish_frame (enc, frame);
    frame = NULL;

    if (flow != GST_FLOW_OK) {
      GST_INFO_OBJECT (enc, "got flow %s", gst_flow_get_name (flow));
      g_atomic_int_set (&nvenc->last_flow, flow);
      break;
    }
  }
  while (TRUE);

  GST_INFO_OBJECT (nvenc, "exiting thread");

  return NULL;
}

static gboolean
gst_nv_base_enc_start_bitstream_thread (GstNvBaseEnc * nvenc)
{
  gchar *name = g_strdup_printf ("%s-read-bits", GST_OBJECT_NAME (nvenc));

  g_assert (nvenc->bitstream_thread == NULL);

  g_assert (g_async_queue_length (nvenc->bitstream_queue) == 0);

  nvenc->bitstream_thread =
      g_thread_try_new (name, gst_nv_base_enc_bitstream_thread, nvenc, NULL);

  g_free (name);

  if (nvenc->bitstream_thread == NULL)
    return FALSE;

  GST_INFO_OBJECT (nvenc, "started thread to read bitstream");
  return TRUE;
}

static gboolean
gst_nv_base_enc_stop_bitstream_thread (GstNvBaseEnc * nvenc)
{
  gpointer out_buf;

  if (nvenc->bitstream_thread == NULL)
    return TRUE;

  /* FIXME */
  GST_FIXME_OBJECT (nvenc, "stop bitstream reading thread properly");
  g_async_queue_lock (nvenc->bitstream_queue);
  g_async_queue_lock (nvenc->bitstream_pool);
  while ((out_buf = g_async_queue_try_pop_unlocked (nvenc->bitstream_queue))) {
    GST_INFO_OBJECT (nvenc, "stole bitstream buffer %p from queue", out_buf);
    g_async_queue_push_unlocked (nvenc->bitstream_pool, out_buf);
  }
  g_async_queue_push_unlocked (nvenc->bitstream_queue, SHUTDOWN_COOKIE);
  g_async_queue_unlock (nvenc->bitstream_pool);
  g_async_queue_unlock (nvenc->bitstream_queue);

  /* temporary unlock, so other thread can find and push frame */
  GST_VIDEO_ENCODER_STREAM_UNLOCK (nvenc);
  g_thread_join (nvenc->bitstream_thread);
  GST_VIDEO_ENCODER_STREAM_LOCK (nvenc);

  nvenc->bitstream_thread = NULL;
  return TRUE;
}

static void
gst_nv_base_enc_reset_queues (GstNvBaseEnc * nvenc, gboolean refill)
{
  gpointer ptr;
  gint i;

  GST_INFO_OBJECT (nvenc, "clearing queues");

  while ((ptr = g_async_queue_try_pop (nvenc->bitstream_queue))) {
    /* do nothing */
  }
  while ((ptr = g_async_queue_try_pop (nvenc->bitstream_pool))) {
    /* do nothing */
  }
  while ((ptr = g_async_queue_try_pop (nvenc->in_bufs_pool))) {
    /* do nothing */
  }

  if (refill) {
    GST_INFO_OBJECT (nvenc, "refilling buffer pools");
    for (i = 0; i < nvenc->n_bufs; ++i) {
      g_async_queue_push (nvenc->bitstream_pool, nvenc->input_bufs[i]);
      g_async_queue_push (nvenc->in_bufs_pool, nvenc->output_bufs[i]);
    }
  }
}

static void
gst_nv_base_enc_free_buffers (GstNvBaseEnc * nvenc)
{
  NVENCSTATUS nv_ret;
  guint i;

  if (nvenc->encoder == NULL)
    return;

  gst_nv_base_enc_reset_queues (nvenc, FALSE);

  for (i = 0; i < nvenc->n_bufs; ++i) {
    NV_ENC_OUTPUT_PTR out_buf = nvenc->output_bufs[i];

#if HAVE_NVENC_GST_GL
    if (nvenc->gl_input) {
      struct gl_input_resource *in_gl_resource = nvenc->input_bufs[i];

      cuCtxPushCurrent (nvenc->cuda_ctx);
      nv_ret =
          GstNvEncUnregisterResource (nvenc->encoder,
          in_gl_resource->nv_resource.registeredResource);
      if (nv_ret != NV_ENC_SUCCESS)
        GST_ERROR_OBJECT (nvenc, "Failed to unregister resource %p, ret %d",
            in_gl_resource, nv_ret);

      g_free (in_gl_resource);
      cuCtxPopCurrent (NULL);
    } else
#endif
    {
      NV_ENC_INPUT_PTR in_buf = (NV_ENC_INPUT_PTR) nvenc->input_bufs[i];

      GST_DEBUG_OBJECT (nvenc, "Destroying input buffer %p", in_buf);
      nv_ret = GstNvEncDestroyInputBuffer (nvenc->encoder, in_buf);
      if (nv_ret != NV_ENC_SUCCESS) {
        GST_ERROR_OBJECT (nvenc, "Failed to destroy input buffer %p, ret %d",
            in_buf, nv_ret);
      }
    }

    GST_DEBUG_OBJECT (nvenc, "Destroying output bitstream buffer %p", out_buf);
    nv_ret = GstNvEncDestroyBitstreamBuffer (nvenc->encoder, out_buf);
    if (nv_ret != NV_ENC_SUCCESS) {
      GST_ERROR_OBJECT (nvenc, "Failed to destroy output buffer %p, ret %d",
          out_buf, nv_ret);
    }
  }

  nvenc->n_bufs = 0;
  g_free (nvenc->output_bufs);
  nvenc->output_bufs = NULL;
  g_free (nvenc->input_bufs);
  nvenc->input_bufs = NULL;
}

static inline guint
_get_plane_width (GstVideoInfo * info, guint plane)
{
  if (GST_VIDEO_INFO_IS_YUV (info))
    /* For now component width and plane width are the same and the
     * plane-component mapping matches
     */
    return GST_VIDEO_INFO_COMP_WIDTH (info, plane);
  else                          /* RGB, GRAY */
    return GST_VIDEO_INFO_WIDTH (info);
}

static inline guint
_get_plane_height (GstVideoInfo * info, guint plane)
{
  if (GST_VIDEO_INFO_IS_YUV (info))
    /* For now component width and plane width are the same and the
     * plane-component mapping matches
     */
    return GST_VIDEO_INFO_COMP_HEIGHT (info, plane);
  else                          /* RGB, GRAY */
    return GST_VIDEO_INFO_HEIGHT (info);
}

static inline gsize
_get_frame_data_height (GstVideoInfo * info)
{
  gsize ret = 0;
  gint i;

  for (i = 0; i < GST_VIDEO_INFO_N_PLANES (info); i++) {
    ret += _get_plane_height (info, i);
  }

  return ret;
}

void
gst_nv_base_enc_set_max_encode_size (GstNvBaseEnc * nvenc, guint max_width,
    guint max_height)
{
  nvenc->max_encode_width = max_width;
  nvenc->max_encode_height = max_height;
}

void
gst_nv_base_enc_get_max_encode_size (GstNvBaseEnc * nvenc, guint * max_width,
    guint * max_height)
{
  *max_width = nvenc->max_encode_width;
  *max_height = nvenc->max_encode_height;
}

static gboolean
gst_nv_base_enc_set_format (GstVideoEncoder * enc, GstVideoCodecState * state)
{
  GstNvBaseEncClass *nvenc_class = GST_NV_BASE_ENC_GET_CLASS (enc);
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);
  GstVideoInfo *info = &state->info;
  GstVideoCodecState *old_state = nvenc->input_state;
  NVENCSTATUS nv_ret;

  g_assert (nvenc_class->initialize_encoder);
  if (!nvenc_class->initialize_encoder (nvenc, old_state, state)) {
    GST_ERROR_OBJECT (enc, "Subclass failed to reconfigure encoder");
    return FALSE;
  }

  if (!nvenc->max_encode_width && !nvenc->max_encode_height) {
    gst_nv_base_enc_set_max_encode_size (nvenc, GST_VIDEO_INFO_WIDTH (info),
        GST_VIDEO_INFO_HEIGHT (info));
  }

  if (!old_state) {
    nvenc->input_info = *info;
    nvenc->gl_input = FALSE;
  }

  if (nvenc->input_state)
    gst_video_codec_state_unref (nvenc->input_state);
  nvenc->input_state = gst_video_codec_state_ref (state);
  GST_INFO_OBJECT (nvenc, "configured encoder");

  /* now allocate some buffers only on first configuration */
  if (!old_state) {
#if HAVE_NVENC_GST_GL
    GstCapsFeatures *features;
#endif
    guint num_macroblocks, i;
    guint input_width, input_height;

    input_width = GST_VIDEO_INFO_WIDTH (info);
    input_height = GST_VIDEO_INFO_HEIGHT (info);

    num_macroblocks = (GST_ROUND_UP_16 (input_width) >> 4)
        * (GST_ROUND_UP_16 (input_height) >> 4);
    nvenc->n_bufs = (num_macroblocks >= 8160) ? 32 : 48;

    /* input buffers */
    nvenc->input_bufs = g_new0 (gpointer, nvenc->n_bufs);

#if HAVE_NVENC_GST_GL
    features = gst_caps_get_features (state->caps, 0);
    if (gst_caps_features_contains (features,
            GST_CAPS_FEATURE_MEMORY_GL_MEMORY)) {
      guint pixel_depth = 0;
      nvenc->gl_input = TRUE;

      for (i = 0; i < GST_VIDEO_INFO_N_COMPONENTS (info); i++) {
        pixel_depth += GST_VIDEO_INFO_COMP_DEPTH (info, i);
      }

      cuCtxPushCurrent (nvenc->cuda_ctx);
      for (i = 0; i < nvenc->n_bufs; ++i) {
        struct gl_input_resource *in_gl_resource =
            g_new0 (struct gl_input_resource, 1);
        CUresult cu_ret;

        memset (&in_gl_resource->nv_resource, 0,
            sizeof (in_gl_resource->nv_resource));
        memset (&in_gl_resource->nv_mapped_resource, 0,
            sizeof (in_gl_resource->nv_mapped_resource));

        /* scratch buffer for non-contigious planer into a contigious buffer */
        cu_ret =
            cuMemAllocPitch ((CUdeviceptr *) & in_gl_resource->cuda_pointer,
            &in_gl_resource->cuda_stride, input_width,
            _get_frame_data_height (info), 16);
        if (cu_ret != CUDA_SUCCESS) {
          const gchar *err;

          cuGetErrorString (cu_ret, &err);
          GST_ERROR_OBJECT (nvenc, "failed to alocate cuda scratch buffer "
              "ret %d error :%s", cu_ret, err);
          g_assert_not_reached ();
        }

        in_gl_resource->nv_resource.version = NV_ENC_REGISTER_RESOURCE_VER;
        in_gl_resource->nv_resource.resourceType =
            NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
        in_gl_resource->nv_resource.width = input_width;
        in_gl_resource->nv_resource.height = input_height;
        in_gl_resource->nv_resource.pitch = in_gl_resource->cuda_stride;
        in_gl_resource->nv_resource.bufferFormat =
            gst_nvenc_get_nv_buffer_format (GST_VIDEO_INFO_FORMAT (info));
        in_gl_resource->nv_resource.resourceToRegister =
            in_gl_resource->cuda_pointer;

        nv_ret =
            GstNvEncRegisterResource (nvenc->encoder,
            &in_gl_resource->nv_resource);
        if (nv_ret != NV_ENC_SUCCESS)
          GST_ERROR_OBJECT (nvenc, "Failed to register resource %p, ret %d",
              in_gl_resource, nv_ret);

        nvenc->input_bufs[i] = in_gl_resource;
        g_async_queue_push (nvenc->in_bufs_pool, nvenc->input_bufs[i]);
      }

      cuCtxPopCurrent (NULL);
    } else
#endif
    {
      for (i = 0; i < nvenc->n_bufs; ++i) {
        NV_ENC_CREATE_INPUT_BUFFER cin_buf = { 0, };

        cin_buf.version = NV_ENC_CREATE_INPUT_BUFFER_VER;

        cin_buf.width = GST_ROUND_UP_32 (input_width);
        cin_buf.height = GST_ROUND_UP_32 (input_height);

        cin_buf.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;
        cin_buf.bufferFmt =
            gst_nvenc_get_nv_buffer_format (GST_VIDEO_INFO_FORMAT (info));

        nv_ret = GstNvEncCreateInputBuffer (nvenc->encoder, &cin_buf);

        if (nv_ret != NV_ENC_SUCCESS) {
          GST_WARNING_OBJECT (enc, "Failed to allocate input buffer: %d",
              nv_ret);
          /* FIXME: clean up */
          return FALSE;
        }

        nvenc->input_bufs[i] = cin_buf.inputBuffer;

        GST_INFO_OBJECT (nvenc, "allocated  input buffer %2d: %p", i,
            nvenc->input_bufs[i]);

        g_async_queue_push (nvenc->in_bufs_pool, nvenc->input_bufs[i]);
      }
    }

    /* output buffers */
    nvenc->output_bufs = g_new0 (NV_ENC_OUTPUT_PTR, nvenc->n_bufs);
    for (i = 0; i < nvenc->n_bufs; ++i) {
      NV_ENC_CREATE_BITSTREAM_BUFFER cout_buf = { 0, };

      cout_buf.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;

      /* 1 MB should be large enough to hold most output frames.
       * NVENC will automatically increase this if it's not enough. */
      cout_buf.size = 1024 * 1024;
      cout_buf.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

      nv_ret = GstNvEncCreateBitstreamBuffer (nvenc->encoder, &cout_buf);
      if (nv_ret != NV_ENC_SUCCESS) {
        GST_WARNING_OBJECT (enc, "Failed to allocate input buffer: %d", nv_ret);
        /* FIXME: clean up */
        return FALSE;
      }

      nvenc->output_bufs[i] = cout_buf.bitstreamBuffer;

      GST_INFO_OBJECT (nvenc, "allocated output buffer %2d: %p", i,
          nvenc->output_bufs[i]);

      g_async_queue_push (nvenc->bitstream_pool, nvenc->output_bufs[i]);
    }

#if 0
    /* Get SPS/PPS */
    {
      NV_ENC_SEQUENCE_PARAM_PAYLOAD seq_param = { 0 };
      uint32_t seq_size = 0;

      seq_param.version = NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
      seq_param.spsppsBuffer = g_alloca (1024);
      seq_param.inBufferSize = 1024;
      seq_param.outSPSPPSPayloadSize = &seq_size;

      nv_ret = GstNvEncGetSequenceParams (nvenc->encoder, &seq_param);
      if (nv_ret != NV_ENC_SUCCESS) {
        GST_WARNING_OBJECT (enc, "Failed to retrieve SPS/PPS: %d", nv_ret);
        return FALSE;
      }

      /* FIXME: use SPS/PPS */
      GST_MEMDUMP_OBJECT (enc, "SPS/PPS", seq_param.spsppsBuffer, seq_size);
    }
#endif
  }

  g_assert (nvenc_class->set_src_caps);
  if (!nvenc_class->set_src_caps (nvenc, state)) {
    GST_ERROR_OBJECT (nvenc, "Subclass failed to set output caps");
    /* FIXME: clean up */
    return FALSE;
  }

  return TRUE;
}

static inline guint
_plane_get_n_components (GstVideoInfo * info, guint plane)
{
  switch (GST_VIDEO_INFO_FORMAT (info)) {
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_AYUV:
      return 4;
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
      return 3;
    case GST_VIDEO_FORMAT_GRAY16_BE:
    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
      return 2;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      return plane == 0 ? 1 : 2;
    case GST_VIDEO_FORMAT_GRAY8:
    case GST_VIDEO_FORMAT_Y444:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
      return 1;
    default:
      g_assert_not_reached ();
      return 1;
  }
}

#if HAVE_NVENC_GST_GL
struct map_gl_input
{
  GstNvBaseEnc *nvenc;
  GstVideoCodecFrame *frame;
  GstVideoInfo *info;
  struct gl_input_resource *in_gl_resource;
};

static void
_map_gl_input_buffer (GstGLContext * context, struct map_gl_input *data)
{
  cudaError_t cuda_ret;
  guint8 *data_pointer;
  guint i;

  cuCtxPushCurrent (data->nvenc->cuda_ctx);
  data_pointer = data->in_gl_resource->cuda_pointer;
  for (i = 0; i < GST_VIDEO_INFO_N_PLANES (data->info); i++) {
    guint plane_n_components;
    GstGLBaseBuffer *gl_buf_obj;
    GstGLMemory *gl_mem;
    guint src_stride, dest_stride;

    gl_mem =
        (GstGLMemory *) gst_buffer_peek_memory (data->frame->input_buffer, i);
    g_assert (gst_is_gl_memory ((GstMemory *) gl_mem));
    data->in_gl_resource->gl_mem[i] = gl_mem;
    plane_n_components = _plane_get_n_components (data->info, i);

    gl_buf_obj = (GstGLBaseBuffer *) gl_mem;

    /* get the texture into the PBO */
    gst_gl_memory_upload_transfer (gl_mem);
    gst_gl_memory_download_transfer (gl_mem);

    GST_LOG_OBJECT (data->nvenc, "attempting to copy texture %u into cuda",
        gl_mem->tex_id);

    cuda_ret =
        cudaGraphicsGLRegisterBuffer (&data->in_gl_resource->cuda_texture,
        gl_buf_obj->id, cudaGraphicsRegisterFlagsReadOnly);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to register GL texture %u to cuda "
          "ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    cuda_ret =
        cudaGraphicsMapResources (1, &data->in_gl_resource->cuda_texture, 0);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to map GL texture %u into cuda "
          "ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    cuda_ret =
        cudaGraphicsResourceGetMappedPointer (&data->in_gl_resource->
        cuda_plane_pointers[i], &data->in_gl_resource->cuda_num_bytes,
        data->in_gl_resource->cuda_texture);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to get mapped pointer of map GL "
          "texture %u in cuda ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    src_stride = GST_VIDEO_INFO_PLANE_STRIDE (data->info, i);
    dest_stride = data->in_gl_resource->cuda_stride;

    /* copy into scratch buffer */
    cuda_ret =
        cudaMemcpy2D (data_pointer, dest_stride,
        data->in_gl_resource->cuda_plane_pointers[i], src_stride,
        _get_plane_width (data->info, i) * plane_n_components,
        _get_plane_height (data->info, i), cudaMemcpyDeviceToDevice);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to copy GL texture %u into cuda "
          "ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    cuda_ret =
        cudaGraphicsUnmapResources (1, &data->in_gl_resource->cuda_texture, 0);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to unmap GL texture %u from cuda "
          "ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    cuda_ret =
        cudaGraphicsUnregisterResource (data->in_gl_resource->cuda_texture);
    if (cuda_ret != cudaSuccess) {
      GST_ERROR_OBJECT (data->nvenc, "failed to unregister GL texture %u from "
          "cuda ret :%d", gl_mem->tex_id, cuda_ret);
      g_assert_not_reached ();
    }

    data_pointer =
        data_pointer +
        data->in_gl_resource->cuda_stride *
        _get_plane_height (&data->nvenc->input_info, i);
  }
  cuCtxPopCurrent (NULL);
}
#endif

static GstFlowReturn
_acquire_input_buffer (GstNvBaseEnc * nvenc, gpointer * input)
{
  g_assert (input);

  GST_LOG_OBJECT (nvenc, "acquiring input buffer..");
  GST_VIDEO_ENCODER_STREAM_UNLOCK (nvenc);
  *input = g_async_queue_pop (nvenc->in_bufs_pool);
  GST_VIDEO_ENCODER_STREAM_LOCK (nvenc);

  return GST_FLOW_OK;
}

static GstFlowReturn
_submit_input_buffer (GstNvBaseEnc * nvenc, GstVideoCodecFrame * frame,
    GstVideoFrame * vframe, void *inputBuffer, void *inputBufferPtr,
    NV_ENC_BUFFER_FORMAT bufferFormat, void *outputBufferPtr)
{
  GstNvBaseEncClass *nvenc_class = GST_NV_BASE_ENC_GET_CLASS (nvenc);
  NV_ENC_PIC_PARAMS pic_params = { 0, };
  NVENCSTATUS nv_ret;

  GST_LOG_OBJECT (nvenc, "%u: input buffer %p, output buffer %p, "
      "pts %" GST_TIME_FORMAT, frame->system_frame_number, inputBuffer,
      outputBufferPtr, GST_TIME_ARGS (frame->pts));

  pic_params.version = NV_ENC_PIC_PARAMS_VER;
  pic_params.inputBuffer = inputBufferPtr;
  pic_params.bufferFmt = bufferFormat;

  pic_params.inputWidth = GST_VIDEO_FRAME_WIDTH (vframe);
  pic_params.inputHeight = GST_VIDEO_FRAME_HEIGHT (vframe);
  pic_params.outputBitstream = outputBufferPtr;
  pic_params.completionEvent = NULL;
  if (GST_VIDEO_FRAME_IS_INTERLACED (vframe)) {
    if (GST_VIDEO_FRAME_IS_TFF (vframe))
      pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM;
    else
      pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP;
  } else {
    pic_params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
  }
  pic_params.inputTimeStamp = frame->pts;
  pic_params.inputDuration =
      GST_CLOCK_TIME_IS_VALID (frame->duration) ? frame->duration : 0;
  pic_params.frameIdx = frame->system_frame_number;

  if (GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME (frame))
    pic_params.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR;
  else
    pic_params.encodePicFlags = 0;

  if (nvenc_class->set_pic_params
      && !nvenc_class->set_pic_params (nvenc, frame, &pic_params)) {
    GST_ERROR_OBJECT (nvenc, "Subclass failed to submit buffer");
    return GST_FLOW_ERROR;
  }

  nv_ret = GstNvEncEncodePicture (nvenc->encoder, &pic_params);
  if (nv_ret == NV_ENC_SUCCESS) {
    GST_LOG_OBJECT (nvenc, "Encoded picture");
  } else if (nv_ret == NV_ENC_ERR_NEED_MORE_INPUT) {
    /* FIXME: we should probably queue pending output buffers here and only
     * submit them to the async queue once we got sucess back */
    GST_DEBUG_OBJECT (nvenc, "Encoded picture (encoder needs more input)");
  } else {
    GST_ERROR_OBJECT (nvenc, "Failed to encode picture: %d", nv_ret);
    GST_DEBUG_OBJECT (nvenc, "re-enqueueing input buffer %p", inputBuffer);
    g_async_queue_push (nvenc->in_bufs_pool, inputBuffer);
    GST_DEBUG_OBJECT (nvenc, "re-enqueueing output buffer %p", outputBufferPtr);
    g_async_queue_push (nvenc->bitstream_pool, outputBufferPtr);

    return GST_FLOW_ERROR;
  }

  g_async_queue_push (nvenc->bitstream_queue, outputBufferPtr);

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_nv_base_enc_handle_frame (GstVideoEncoder * enc, GstVideoCodecFrame * frame)
{
  gpointer input_buffer = NULL;
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);
  NV_ENC_OUTPUT_PTR out_buf;
  NVENCSTATUS nv_ret;
  GstVideoFrame vframe;
  GstVideoInfo *info = &nvenc->input_state->info;
  GstFlowReturn flow = GST_FLOW_OK;
  GstMapFlags in_map_flags = GST_MAP_READ;
  struct frame_state *state;
  guint frame_n = 0;

  g_assert (nvenc->encoder != NULL);

#if HAVE_NVENC_GST_GL
  if (nvenc->gl_input)
    in_map_flags |= GST_MAP_GL;
#endif

  if (!gst_video_frame_map (&vframe, info, frame->input_buffer, in_map_flags))
    return GST_FLOW_ERROR;

  /* make sure our thread that waits for output to be ready is started */
  if (nvenc->bitstream_thread == NULL) {
    if (!gst_nv_base_enc_start_bitstream_thread (nvenc))
      goto error;
  }

  flow = _acquire_input_buffer (nvenc, &input_buffer);
  if (flow != GST_FLOW_OK)
    return flow;
  if (input_buffer == NULL)
    return GST_FLOW_ERROR;

  state = frame->user_data;
  if (!state)
    state = g_new0 (struct frame_state, 1);
  state->n_buffers = 1;

#if HAVE_NVENC_GST_GL
  if (nvenc->gl_input) {
    struct gl_input_resource *in_gl_resource = input_buffer;
    struct map_gl_input data;

    GST_LOG_OBJECT (enc, "got input buffer %p", in_gl_resource);

    in_gl_resource->gl_mem[0] =
        (GstGLMemory *) gst_buffer_peek_memory (frame->input_buffer, 0);
    g_assert (gst_is_gl_memory ((GstMemory *) in_gl_resource->gl_mem[0]));

    data.nvenc = nvenc;
    data.frame = frame;
    data.info = &vframe.info;
    data.in_gl_resource = in_gl_resource;

    gst_gl_context_thread_add (in_gl_resource->gl_mem[0]->mem.context,
        (GstGLContextThreadFunc) _map_gl_input_buffer, &data);

    in_gl_resource->nv_mapped_resource.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    in_gl_resource->nv_mapped_resource.registeredResource =
        in_gl_resource->nv_resource.registeredResource;

    nv_ret =
        GstNvEncMapInputResource (nvenc->encoder,
        &in_gl_resource->nv_mapped_resource);
    if (nv_ret != NV_ENC_SUCCESS) {
      GST_ERROR_OBJECT (nvenc, "Failed to map input resource %p, ret %d",
          in_gl_resource, nv_ret);
      goto error;
    }

    out_buf = g_async_queue_try_pop (nvenc->bitstream_pool);
    if (out_buf == NULL) {
      GST_DEBUG_OBJECT (nvenc, "wait for output buf to become available again");
      out_buf = g_async_queue_pop (nvenc->bitstream_pool);
    }

    state->in_bufs[frame_n] = in_gl_resource;
    state->out_bufs[frame_n++] = out_buf;

    frame->user_data = state;
    frame->user_data_destroy_notify = (GDestroyNotify) g_free;

    flow =
        _submit_input_buffer (nvenc, frame, &vframe, in_gl_resource,
        in_gl_resource->nv_mapped_resource.mappedResource,
        in_gl_resource->nv_mapped_resource.mappedBufferFmt, out_buf);

    /* encoder will keep frame in list internally, we'll look it up again later
     * in the thread where we get the output buffers and finish it there */
    gst_video_codec_frame_unref (frame);
    frame = NULL;
  }
#endif

  if (!nvenc->gl_input) {
    NV_ENC_LOCK_INPUT_BUFFER in_buf_lock = { 0, };
    NV_ENC_INPUT_PTR in_buf = input_buffer;
    guint8 *src, *dest;
    guint src_stride, dest_stride;
    guint height, width;
    guint y;

    GST_LOG_OBJECT (enc, "got input buffer %p", in_buf);

    in_buf_lock.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
    in_buf_lock.inputBuffer = in_buf;

    nv_ret = GstNvEncLockInputBuffer (nvenc->encoder, &in_buf_lock);
    if (nv_ret != NV_ENC_SUCCESS) {
      GST_ERROR_OBJECT (nvenc, "Failed to lock input buffer: %d", nv_ret);
      /* FIXME: post proper error message */
      goto error;
    }
    GST_LOG_OBJECT (nvenc, "Locked input buffer %p", in_buf);

    width = GST_VIDEO_FRAME_WIDTH (&vframe);
    height = GST_VIDEO_FRAME_HEIGHT (&vframe);

    // FIXME: this only works for NV12
    g_assert (GST_VIDEO_FRAME_FORMAT (&vframe) == GST_VIDEO_FORMAT_NV12);

    /* copy Y plane */
    src = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
    src_stride = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0);
    dest = in_buf_lock.bufferDataPtr;
    dest_stride = in_buf_lock.pitch;
    for (y = 0; y < height; ++y) {
      memcpy (dest, src, width);
      dest += dest_stride;
      src += src_stride;
    }

    /* copy UV plane */
    src = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 1);
    src_stride = GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 1);
    dest =
        (guint8 *) in_buf_lock.bufferDataPtr +
        GST_ROUND_UP_32 (GST_VIDEO_INFO_HEIGHT (&nvenc->input_info)) *
        in_buf_lock.pitch;
    dest_stride = in_buf_lock.pitch;
    for (y = 0; y < GST_ROUND_UP_2 (height) / 2; ++y) {
      memcpy (dest, src, width);
      dest += dest_stride;
      src += src_stride;
    }

    nv_ret = GstNvEncUnlockInputBuffer (nvenc->encoder, in_buf);
    if (nv_ret != NV_ENC_SUCCESS) {
      GST_ERROR_OBJECT (nvenc, "Failed to unlock input buffer: %d", nv_ret);
      goto error;
    }

    out_buf = g_async_queue_try_pop (nvenc->bitstream_pool);
    if (out_buf == NULL) {
      GST_DEBUG_OBJECT (nvenc, "wait for output buf to become available again");
      out_buf = g_async_queue_pop (nvenc->bitstream_pool);
    }

    state->in_bufs[frame_n] = in_buf;
    state->out_bufs[frame_n++] = out_buf;
    frame->user_data = state;
    frame->user_data_destroy_notify = (GDestroyNotify) g_free;

    flow =
        _submit_input_buffer (nvenc, frame, &vframe, in_buf, in_buf,
        gst_nvenc_get_nv_buffer_format (GST_VIDEO_INFO_FORMAT (info)), out_buf);

    /* encoder will keep frame in list internally, we'll look it up again later
     * in the thread where we get the output buffers and finish it there */
    gst_video_codec_frame_unref (frame);
    frame = NULL;
  }

  if (flow != GST_FLOW_OK)
    goto out;

  flow = g_atomic_int_get (&nvenc->last_flow);

out:

  gst_video_frame_unmap (&vframe);

  return flow;

error:
  flow = GST_FLOW_ERROR;
  goto out;
}

static gboolean
gst_nv_base_enc_drain_encoder (GstNvBaseEnc * nvenc)
{
  NV_ENC_PIC_PARAMS pic_params = { 0, };
  NVENCSTATUS nv_ret;

  GST_INFO_OBJECT (nvenc, "draining encoder");

  if (nvenc->input_state == NULL) {
    GST_DEBUG_OBJECT (nvenc, "no input state, nothing to do");
    return TRUE;
  }

  pic_params.version = NV_ENC_PIC_PARAMS_VER;
  pic_params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;

  nv_ret = GstNvEncEncodePicture (nvenc->encoder, &pic_params);
  if (nv_ret != NV_ENC_SUCCESS) {
    GST_LOG_OBJECT (nvenc, "Failed to drain encoder, ret %d", nv_ret);
    return FALSE;
  }

  return TRUE;
}

static GstFlowReturn
gst_nv_base_enc_finish (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);

  GST_FIXME_OBJECT (enc, "implement finish");

  gst_nv_base_enc_drain_encoder (nvenc);

  /* wait for encoder to output the remaining buffers */
  gst_nv_base_enc_stop_bitstream_thread (nvenc);

  return GST_FLOW_OK;
}

#if 0
static gboolean
gst_nv_base_enc_flush (GstVideoEncoder * enc)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (enc);
  GST_INFO_OBJECT (nvenc, "done flushing encoder");
  return TRUE;
}
#endif

static void
gst_nv_base_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (object);

  switch (prop_id) {
    case PROP_DEVICE_ID:
      nvenc->cuda_device_id = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_nv_base_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstNvBaseEnc *nvenc = GST_NV_BASE_ENC (object);

  switch (prop_id) {
    case PROP_DEVICE_ID:
      g_value_set_uint (value, nvenc->cuda_device_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

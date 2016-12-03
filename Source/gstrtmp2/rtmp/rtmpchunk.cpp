/* GStreamer RTMP Library
 * Copyright (C) 2013 David Schleef <ds@schleef.org>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#include <gst/gst.h>

#include "rtmpchunk.h"
#include "rtmputils.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC(gst_rtmp_chunk_debug_category);
#define GST_CAT_DEFAULT gst_rtmp_chunk_debug_category

/* prototypes */


static void gst_rtmp_chunk_set_property(GObject * object,
	guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_rtmp_chunk_get_property(GObject * object,
	guint property_id, GValue * value, GParamSpec * pspec);
static void gst_rtmp_chunk_dispose(GObject * object);
static void gst_rtmp_chunk_finalize(GObject * object);


enum
{
	PROP_0
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstRtmpChunk, gst_rtmp_chunk, G_TYPE_OBJECT,
	GST_DEBUG_CATEGORY_INIT(gst_rtmp_chunk_debug_category, "rtmpchunk", 0,
	"debug category for rtmpchunk element"));

static void
gst_rtmp_chunk_class_init(GstRtmpChunkClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = gst_rtmp_chunk_set_property;
	gobject_class->get_property = gst_rtmp_chunk_get_property;
	gobject_class->dispose = gst_rtmp_chunk_dispose;
	gobject_class->finalize = gst_rtmp_chunk_finalize;

}

static void
gst_rtmp_chunk_init(GstRtmpChunk * rtmpchunk)
{
}

void
gst_rtmp_chunk_set_property(GObject * object, guint property_id,
const GValue * value, GParamSpec * pspec)
{
	GstRtmpChunk *rtmpchunk = GST_RTMP_CHUNK(object);

	GST_DEBUG_OBJECT(rtmpchunk, "set_property");

// 	switch (property_id) {
// 	default:
// 		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
// 		break;
// 	}
}

void
gst_rtmp_chunk_get_property(GObject * object, guint property_id,
GValue * value, GParamSpec * pspec)
{
	GstRtmpChunk *rtmpchunk = GST_RTMP_CHUNK(object);

	GST_DEBUG_OBJECT(rtmpchunk, "get_property");

// 	switch (property_id) {
// 	default:
// 		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
// 		break;
// 	}
}

void
gst_rtmp_chunk_dispose(GObject * object)
{
// 	GstRtmpChunk *rtmpchunk = GST_RTMP_CHUNK(object);

// 	GST_DEBUG_OBJECT(rtmpchunk, "dispose");

	/* clean up as possible.  may be called multiple times */

	G_OBJECT_CLASS(gst_rtmp_chunk_parent_class)->dispose(object);
}

void
gst_rtmp_chunk_finalize(GObject * object)
{
	GstRtmpChunk *rtmpchunk = GST_RTMP_CHUNK(object);

// 	GST_DEBUG_OBJECT(rtmpchunk, "finalize");

	/* clean up object here */
	if (rtmpchunk->payload) {
		g_bytes_unref(rtmpchunk->payload);
	}

	G_OBJECT_CLASS(gst_rtmp_chunk_parent_class)->finalize(object);
}

GstRtmpChunk *
gst_rtmp_chunk_new(void)
{
	return (GstRtmpChunk *)g_object_new(GST_TYPE_RTMP_CHUNK, NULL);
}

gboolean
gst_rtmp_chunk_parse_header1(GstRtmpChunkHeader * header, GBytes * bytes)
{
	const guint8 *data;
	const gsize sizes[4] = { 12, 8, 4, 1 };
	int chunk_stream_id;
	gsize size;

	data = (const guint8 *)g_bytes_get_data(bytes, &size);
	header->format = data[0] >> 6;
	header->header_size = sizes[header->format];

	chunk_stream_id = data[0] & 0x3f;
	if (chunk_stream_id == GST_RTMP_CHUNK_STREAM_TWOBYTE) {
		if (size >= 2)
			header->chunk_stream_id = 64 + data[1];
		header->header_size += 1;
	}
	else if (chunk_stream_id == GST_RTMP_CHUNK_STREAM_THREEBYTE) {
		if (size >= 3)
			header->chunk_stream_id = 64 + data[1] + (data[2] << 8);
		header->header_size += 2;
	}
	else {
		header->chunk_stream_id = chunk_stream_id;
	}

	return (header->header_size <= size);
}

gboolean
gst_rtmp_chunk_parse_header2(GstRtmpChunkHeader * header, GBytes * bytes,
GstRtmpChunkHeader * previous_header)
{
	int offset;
	const guint8 *data;
	gsize size;

	data = (const guint8 *)g_bytes_get_data(bytes, &size);

	header->format = data[0] >> 6;
	header->chunk_stream_id = data[0] & 0x3f;
	offset = 1;
	if (header->chunk_stream_id == GST_RTMP_CHUNK_STREAM_TWOBYTE) {
		header->chunk_stream_id = 64 + data[1];
		offset = 2;
	}
	else if (header->chunk_stream_id == GST_RTMP_CHUNK_STREAM_THREEBYTE) {
		header->chunk_stream_id = 64 + GST_READ_UINT16_LE(data + 1);
		offset = 3;
	}
	if (header->format == 0) {
		header->timestamp = GST_READ_UINT24_BE(data + offset);
		header->message_length = GST_READ_UINT24_BE(data + offset + 3);
		header->message_type_id = data[offset + 6];
		/* SRSLY:  "Message stream ID is stored in little-endian format." */
		header->stream_id = GST_READ_UINT32_LE(data + offset + 7);
		offset += 11;
		if (header->timestamp == 0xffffff) {
			GST_ERROR("untested long timestamp");
			header->timestamp = GST_READ_UINT32_BE(data + offset);
			offset += 4;
		}
	}
	else {
		header->timestamp = previous_header->timestamp;
		header->message_length = previous_header->message_length;
		header->message_type_id = previous_header->message_type_id;
		header->stream_id = previous_header->stream_id;

		if (header->format == 1) {
			header->timestamp += GST_READ_UINT24_BE(data + offset);
			header->message_length = GST_READ_UINT24_BE(data + offset + 3);
			header->message_type_id = data[offset + 6];
			offset += 7;
		}
		else if (header->format == 2) {
			header->timestamp += GST_READ_UINT24_BE(data + offset);
			offset += 3;
		}
		else {
			/* ok */
		}
	}

	header->header_size = offset;

	return (header->header_size <= size);
}

GBytes *
gst_rtmp_chunk_serialize(GstRtmpChunk * chunk,
GstRtmpChunkHeader * previous_header, gsize max_chunk_size)
{
	guint8 *data;
	const guint8 *chunkdata;
	gsize chunksize;
	int header_fmt;
	//guint32 timestamp;
	int offset;
	int i;

	chunkdata = (const guint8 *)g_bytes_get_data(chunk->payload, &chunksize);
	if (chunk->message_length != chunksize) {
		GST_ERROR("message_length wrong (%" G_GSIZE_FORMAT " should be %"
			G_GSIZE_FORMAT ")", chunk->message_length, chunksize);
	}

	g_assert(chunk->chunk_stream_id < 64);
	data = (guint8 *)g_malloc(chunksize + 12 + (chunksize / max_chunk_size));

	/* FIXME this is incomplete and inefficient */
	header_fmt = 0;
#if 0
	if (previous_header->message_length > 0) {
		header_fmt = 1;
		timestamp = chunk->timestamp - previous_header->timestamp;
	}
#endif

	g_assert(chunk->chunk_stream_id < 64);
	data[0] = (guint8)((header_fmt << 6) | (chunk->chunk_stream_id));
	if (header_fmt == 0) {
		g_assert(chunk->timestamp < 0xffffff);
		GST_WRITE_UINT24_BE(data + 1, chunk->timestamp);
		GST_WRITE_UINT24_BE(data + 4, chunk->message_length);
		data[7] = (guint8)(chunk->message_type_id);
		/* SRSLY:  "Message stream ID is stored in little-endian format." */
		GST_WRITE_UINT32_LE(data + 8, chunk->stream_id);
		offset = 12;
	}
	else {
		GST_WRITE_UINT24_BE(data + 1, chunk->timestamp);
		GST_WRITE_UINT24_BE(data + 4, chunk->message_length);
		data[7] = (guint8)chunk->message_type_id;
		offset = 8;
	}
	for (i = 0; i < (int)chunksize; i += (int)max_chunk_size) {
		if (i != 0) {
			data[offset] = (guint8)( 0xc0 | chunk->chunk_stream_id);
			offset++;
		}
		memcpy(data + offset, chunkdata + i, MIN(chunksize - i, max_chunk_size));
		offset += (int)(MIN(chunksize - i, max_chunk_size));
	}
	GST_DEBUG("type: %d in: %" G_GSIZE_FORMAT " out: %d", chunk->message_type_id,
		chunksize, offset);

	return g_bytes_new_take(data, offset);
}


GBytes *
gst_rtmp_chunk_serialize_nemo(GstRtmpChunk * chunk,
GstRtmpChunkHeader * previous_header, gsize max_chunk_size)
{
	guint8 *data;
	guint8 *phead;
	const guint8 *chunkdata;
	gsize chunksize;
	//guint32 timestamp;
	int offset;
	int i;
	GstRtmpChunkStream basic_head_size_type = GST_RTMP_CHUNK_STREAM_TWOBYTE;
	int basic_head_size = 1;
	GstRtmpChunkMessgeHeadType message_header_fmt = GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_LARGE;
	int message_head_size = 11;
	int extend_timestamp_size = 0;
	guint32 deta_timestmp = 0;

	chunkdata = (const guint8 *)g_bytes_get_data(chunk->payload, &chunksize);
	if (chunk->message_length != chunksize) {
		GST_ERROR("message_length wrong (%" G_GSIZE_FORMAT " should be %"
			G_GSIZE_FORMAT ")", chunk->message_length, chunksize);
	}

	// chunk basic head type and size

	if (chunk->chunk_stream_id > 319)
	{
		basic_head_size_type = GST_RTMP_CHUNK_STREAM_THREEBYTE;
		basic_head_size = 3;
	}
	else if (chunk->chunk_stream_id > 63)
	{
		basic_head_size_type = GST_RTMP_CHUNK_STREAM_TWOBYTE;
		basic_head_size = 2;
	}
	else
	{
		basic_head_size_type = GST_RTMP_CHUNK_STREAM_PROTOCOL;
		basic_head_size = 1;
	}

	// header format type

	if ((chunk->message_type_id == GST_RTMP_MESSAGE_TYPE_AUDIO || chunk->message_type_id == GST_RTMP_MESSAGE_TYPE_VIDEO) && chunk->timestamp)
	{
		message_header_fmt = GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_MEDIUM;
		message_head_size = 7;
	}
	else
	{
		message_header_fmt = GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_LARGE;
		message_head_size = 11;
	}

	if (message_header_fmt != GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_LARGE)
	{
		if (previous_header->message_type_id != -1)
		{
			if (previous_header->message_length == chunk->message_length
				&& previous_header->message_type_id == chunk->message_type_id)
			{
				message_header_fmt = GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_SMALL;
				message_head_size = 3;
			}
			if (previous_header->timestamp == chunk->timestamp
				&& message_header_fmt == GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_SMALL)
			{
				message_header_fmt = GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_NONE;
				message_head_size = 0;
			}

			deta_timestmp = chunk->timestamp - previous_header->timestamp;
			g_assert(chunk->timestamp >= previous_header->timestamp);
		}
	}
	
	if (deta_timestmp >= 0xffffff)
	{
		extend_timestamp_size = 4;
	}

	data = (guint8 *)g_malloc(chunksize + basic_head_size + message_head_size + extend_timestamp_size + chunksize / max_chunk_size);
	phead = data;
	/* FIXME this is incomplete and inefficient */
	// basic header
	phead[0] = (guint8)(message_header_fmt << 6);
	
	if (basic_head_size_type == GST_RTMP_CHUNK_STREAM_PROTOCOL)
	{
		phead[0] |= (chunk->chunk_stream_id);
	}
	else 
	{
		int tmp = chunk->chunk_stream_id - 64;
		phead[1] = tmp & 0xFF;
		if (basic_head_size_type == GST_RTMP_CHUNK_STREAM_THREEBYTE)
		{
			phead[0] |= 1;
			phead[2] = (guint8)(tmp >> 8);
		}
	}
	// message header
	phead = data + basic_head_size;
	if (message_header_fmt < GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_NONE)
	{
		GST_WRITE_UINT24_BE(phead, deta_timestmp > 0xffffff ? 0xffffff : deta_timestmp);
	}
	if (message_header_fmt < GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_SMALL)
	{
		GST_WRITE_UINT24_BE(phead + 3, chunk->message_length);
		phead[6] = (guint8)chunk->message_type_id;
	}
	if (message_header_fmt < GST_RTMP_CHUNK_MESSAGE_HEAD_TYPE_MEDIUM)
	{
		GST_WRITE_UINT32_LE(phead + 7, chunk->stream_id);
	}

	if (extend_timestamp_size > 0)
	{
		phead = data + basic_head_size + message_head_size;
		GST_WRITE_UINT32_BE(phead, deta_timestmp);
	}

	offset = basic_head_size + message_head_size + extend_timestamp_size;

	GST_DEBUG("type = %d, hSize = %d,t = %d, pkt->t = %d", chunk->message_type_id, offset, deta_timestmp, chunk->timestamp);

	for (i = 0; i < (int)chunksize; i += (int)max_chunk_size) {
		if (i != 0) {
			data[offset] = (guint8)( 0xc0 | chunk->chunk_stream_id);
			offset++;
		}
		memcpy(data + offset, chunkdata + i, MIN(chunksize - i, max_chunk_size));
		offset += (int)(MIN(chunksize - i, max_chunk_size));
	}
// 	GST_DEBUG("type: %d in: %" G_GSIZE_FORMAT " out: %d", chunk->message_type_id,
// 		chunksize, offset);

	return g_bytes_new_take(data, offset);
}

void
gst_rtmp_chunk_set_chunk_stream_id(GstRtmpChunk * chunk,
guint32 chunk_stream_id)
{
	chunk->chunk_stream_id = chunk_stream_id;
}

void
gst_rtmp_chunk_set_timestamp(GstRtmpChunk * chunk, guint32 timestamp)
{
	chunk->timestamp = timestamp;
}

void
gst_rtmp_chunk_set_payload(GstRtmpChunk * chunk, GBytes * payload)
{
	if (chunk->payload) {
		g_bytes_unref(chunk->payload);
	}
	chunk->payload = payload;
}

guint32
gst_rtmp_chunk_get_chunk_stream_id(GstRtmpChunk * chunk)
{
	return chunk->chunk_stream_id;
}

guint32
gst_rtmp_chunk_get_timestamp(GstRtmpChunk * chunk)
{
	return chunk->timestamp;
}

GBytes *
gst_rtmp_chunk_get_payload(GstRtmpChunk * chunk)
{
	return chunk->payload;
}

/* chunk cache */

GstRtmpChunkCache *
gst_rtmp_chunk_cache_new(void)
{
	return g_array_new(FALSE, TRUE, sizeof (GstRtmpChunkCacheEntry));
}

void
gst_rtmp_chunk_cache_free(GstRtmpChunkCache * cache)
{
	g_array_free(cache, TRUE);

}

GstRtmpChunkCacheEntry *
gst_rtmp_chunk_cache_get(GstRtmpChunkCache * cache, guint32 chunk_stream_id)
{
	int i;
	GstRtmpChunkCacheEntry *entry;
	for (i = 0; i < (int)cache->len; i++) {
		entry = &g_array_index(cache, GstRtmpChunkCacheEntry, i);
		if (entry->previous_header.chunk_stream_id == chunk_stream_id)
			return entry;
	}
	g_array_set_size(cache, cache->len + 1);
	entry = &g_array_index(cache, GstRtmpChunkCacheEntry, cache->len - 1);
	entry->previous_header.chunk_stream_id = chunk_stream_id;
	entry->previous_header.message_type_id = -1;
	return entry;
}

void
gst_rtmp_chunk_cache_update(GstRtmpChunkCacheEntry * entry,
GstRtmpChunk * chunk)
{
	entry->previous_header.timestamp = chunk->timestamp;
	entry->previous_header.message_length = chunk->message_length;
	entry->previous_header.message_type_id = chunk->message_type_id;
	entry->previous_header.stream_id = chunk->stream_id;
}

gboolean
gst_rtmp_chunk_parse_message(GstRtmpChunk * chunk, char **command_name,
double *transaction_id, GstAmfNode ** command_object,
GstAmfNode ** optional_args)
{
	gsize n_parsed;
	const guint8 *data;
	gsize size;
	int offset;
	GstAmfNode *n1, *n2, *n3, *n4;

	offset = 0;
	data = (const guint8 *)g_bytes_get_data(chunk->payload, &size);
	n1 = gst_amf_node_new_parse(data + offset, size - offset, &n_parsed);
	offset += (int)n_parsed;
	n2 = gst_amf_node_new_parse(data + offset, size - offset, &n_parsed);
	offset += (int)n_parsed;
	n3 = gst_amf_node_new_parse(data + offset, size - offset, &n_parsed);
	offset += (int)n_parsed;
	if (offset < (int)size) {
		n4 = gst_amf_node_new_parse(data + offset, size - offset, &n_parsed);
	}
	else {
		n4 = NULL;
	}

	if (command_name) {
		*command_name = g_strdup(gst_amf_node_get_string(n1));
	}
	gst_amf_node_free(n1);

	if (transaction_id) {
		*transaction_id = gst_amf_node_get_number(n2);
	}
	gst_amf_node_free(n2);

	if (command_object) {
		*command_object = n3;
	}
	else {
		gst_amf_node_free(n3);
	}

	if (optional_args) {
		*optional_args = n4;
	}
	else {
		if (n4)
			gst_amf_node_free(n4);
	}

	return TRUE;
}

#pragma warning(pop)

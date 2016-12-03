
#include "helpFunctions.h"
#include "errorCode.h"
#include <windows.h>
#include "CrashRpt.h"


//////////////////////////////////////////////////////////////////////////

static BOOL WINAPI CrashCallback(LPVOID lpvState)
{
	UNREFERENCED_PARAMETER(lpvState);

	// Crash happened!

	return TRUE;
}


bool instalCrashReport()
{
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = ProjectInfo::projectName.toWideCharPointer(); // Define application name.
	info.pszAppVersion = ProjectInfo::versionString.toWideCharPointer();     // Define application version.
	info.pszEmailSubject = L" Error Report"; // Define subject for email.

	info.pszEmailTo = L"shidonglin@daseasy.com";
	info.pszSmtpLogin = L"shidonglin@daseasy.com";
	info.pszSmtpPassword = L"1234qwer";
	info.pszSmtpProxy = L"smtp.mxhichina.com";

	info.pfnCrashCallback = CrashCallback; // Define crash callback function.   
	// Define delivery methods priorities. 
	info.uPriorities[CR_HTTP] = CR_NEGATIVE_PRIORITY;         // Use HTTP the first.
	info.uPriorities[CR_SMTP] = 1;         // Use SMTP the second.
	info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;        // Use Simple MAPI the last.  
	info.dwFlags = 0;
	info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers.    
	//info.dwFlags |= CR_INST_APP_RESTART;            // Restart the application on crash.  
	//info.dwFlags |= CR_INST_NO_MINIDUMP;          // Do not include minidump.
	//info.dwFlags |= CR_INST_NO_GUI;               // Don't display GUI.
	//info.dwFlags |= CR_INST_DONT_SEND_REPORT;     // Don't send report immediately, just queue for later delivery.
	//info.dwFlags |= CR_INST_STORE_ZIP_ARCHIVES;   // Store ZIP archives along with uncompressed files (to be used with CR_INST_DONT_SEND_REPORT)
	//info.dwFlags |= CR_INST_SEND_MANDATORY;         // Remove "Close" and "Other actions..." buttons from Error Report dialog.
	//info.dwFlags |= CR_INST_SHOW_ADDITIONAL_INFO_FIELDS; //!< Make "Your E-mail" and "Describe what you were doing when the problem occurred" fields of Error Report dialog always visible.
	info.dwFlags |= CR_INST_ALLOW_ATTACH_MORE_FILES; //!< Adds an ability for user to attach more files to crash report by clicking "Attach More File(s)" item from context menu of Error Report Details dialog.
	//info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS;    // Send reports that were failed to send recently.	
	//info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS; 
	info.pszDebugHelpDLL = NULL;                    // Search for dbghelp.dll using default search sequence.
	// 	info.uMiniDumpType = MiniDumpNormal;            // Define minidump size.
	info.uMiniDumpType = (MINIDUMP_TYPE)(MiniDumpNormal
		| MiniDumpWithHandleData
		| MiniDumpWithUnloadedModules
		| MiniDumpWithIndirectlyReferencedMemory
		| MiniDumpScanMemory
		| MiniDumpWithProcessThreadData
		| MiniDumpWithThreadInfo);
	// Define privacy policy URL.
	info.pszErrorReportSaveDir = NULL;       // Save error reports to the default location.
	//info.pszRestartCmdLine = _T("/restart"); // Command line for automatic app restart.
	//info.pszLangFilePath = _T("D:\\");       // Specify custom dir or filename for language file.
	//info.pszCustomSenderIcon = _T("C:\\WINDOWS\\System32\\user32.dll, 1"); // Specify custom icon for CrashRpt dialogs.
	//info.nRestartTimeout = 50;

	int nResult = crInstall(&info);
	if (nResult != 0)
	{
		return false;
	}
	nResult = crAddScreenshot2(CR_AS_PROCESS_WINDOWS | CR_AS_USE_JPEG_FORMAT | CR_AS_ALLOW_DELETE, 50);
	if (nResult != 0)
	{
		return false;
	}

	return true;
}

void uninstallCrashReport()
{
	crUninstall();
}


bool gstHelpInitEnviroment()
{
#ifdef WIN32
	// set work-path for the plugins load dll file;
	wchar_t workPath[4096];
	String strPluginsPath = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getFullPathName() + L"\\plugins";
	File(strPluginsPath).createDirectory();

	GetEnvironmentVariable(L"path", workPath, 4096);
	String strWorkPath = workPath;
	strWorkPath += L";";
	strWorkPath += strPluginsPath;
	SetEnvironmentVariable(L"path", strWorkPath.toWideCharPointer());

	// set environment "GST_PLUGIN_PATH";
	SetEnvironmentVariable(L"GST_PLUGIN_PATH", strPluginsPath.toWideCharPointer());
	// GST_PLUGIN_SYSTEM_PATH
	SetEnvironmentVariable(L"GST_PLUGIN_SYSTEM_PATH", strPluginsPath.toWideCharPointer());
	//GSTREAMER_1_0_ROOT_X86_64
	String strDebugPath = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getFullPathName() + L"\\logs";
	SetEnvironmentVariable(L"GST_DEBUG_DUMP_DOT_DIR", strDebugPath.toWideCharPointer());
	File(strDebugPath).createDirectory();

	// GST_DEBUG_FILE 
	String strLogFullPath = strDebugPath + L"\\gstLogs.log";
	SetEnvironmentVariable(L"GST_DEBUG_FILE", strLogFullPath.toWideCharPointer());

//#ifdef _DEBUG
	gst_debug_set_default_threshold(GST_LEVEL_WARNING);
	gst_debug_set_threshold_for_name("bus", GST_LEVEL_DEBUG);
// 	gst_debug_set_threshold_for_name("decklinkaudiosink", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("decklinkvideosink", GST_LEVEL_LOG);
	// 	gst_debug_set_threshold_for_name("bin", GST_LEVEL_DEBUG);
	// 	gst_debug_set_threshold_for_name("rtmpsink", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmp", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmp2sink", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmpconnection", GST_LEVEL_WARNING);
// 	gst_debug_set_threshold_for_name("rtmpchunk", GST_LEVEL_LOG);
	// 	gst_debug_set_threshold_for_name("flvdemux", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmp2src", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmpsrc", GST_LEVEL_LOG);
//	gst_debug_set_threshold_for_name("rtmpconnection", GST_LEVEL_LOG);
// 	gst_debug_set_threshold_for_name("rtmp2sink", GST_LEVEL_LOG);
//	gst_debug_set_threshold_for_name("videorate", GST_LEVEL_LOG);
// #else
// #endif

#endif
	return true;
}
#include "gstrtmp2/gstrtmp2.h"
#include "decklink/gstdecklink.h"
#include "gstmfxenc/gstmfxenc.h"
//#include "gstnvenc/gstnvenc.h"

void gstHelpReigsterRtmp()
{
	gst_plugin_register_static(
		GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		"rtmp2",
		"Private elements of rtmp2",
		GstPluginInitFunc(gst_rtmp2_plugin_init),
		"1.4.5",
		"LGPL",
		"rtmp2",
		"rtmp2",
		"http://www.tsingmedia.com/");

	gst_plugin_register_static(
		GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		"decklink",
		"Private elements of decklink",
		GstPluginInitFunc(gst_decklink_plugin_init),
		"1.4.5",
		"LGPL",
		"decklink",
		"decklink",
		"http://www.tsingmedia.com/");

	gst_plugin_register_static(
		GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		"mfxenc",
		"Private elements of mfxenc",
		GstPluginInitFunc(gst_mfx_encoder_plugin_init),
		"1.4.5",
		"LGPL",
		"mfxenc",
		"mfxenc",
		"http://www.tsingmedia.com/");

// 	gst_plugin_register_static(
// 		GST_VERSION_MAJOR,
// 		GST_VERSION_MINOR,
// 		"nvenc",
// 		"Private elements of nvenc",
// 		GstPluginInitFunc(gst_nv_encoder_plugin_init),
// 		"1.4.5",
// 		"LGPL",
// 		"nvenc",
// 		"nvenc",
// 		"http://www.tsingmedia.com/");
}

int gstHelpString2DecklinkModeIndex(String strModeKey)
{
	String strSpace = L" ";
	String strKey;
	int t = strModeKey.indexOf(0, StringRef(strSpace));
	if (t >= 0)
	{
		strKey = strModeKey.substring(0, t);
		t++;
		if (t < strModeKey.length())
		{
			strKey += strModeKey.substring(t);
		}
	}
	else
	{
		strKey = strModeKey;
	}

	static StringArray decklinkModeKeys = StringArray::fromLines(
		"AUTO\nNTSC\nNTSC2398\nPAL\nNTSC_P\nPAL_P\n1080p2398\n1080p24\n1080p25\n1080p2997\n1080p30\n1080i50\n1080i5994\n1080i60\n1080p50\n1080p5994\n1080p60\n720p50\n720p5994\n720p60");

	int i = decklinkModeKeys.indexOf(StringRef(strKey), true);
	if (i < 0)
	{
		int numKeys = decklinkModeKeys.size();
		for (int n = 0; n < numKeys; ++n)
		{
			if (decklinkModeKeys[n].containsIgnoreCase(StringRef(strKey)))
			{
				i = n;
				break;
			}
		}

		if (i < 0)
			i = 0;
	}
	return i;
}

PropertySet* readPropertFromMemIni(String memFilePath)
{
	File tmpFile = File(memFilePath);
	if (!tmpFile.existsAsFile())
	{
		return nullptr;
	}
	PropertySet* newSetting = new PropertySet(true);
	String fileFullPath = tmpFile.getFullPathName();

	TCHAR szTXT[1024] = { 0 };

	if (GetPrivateProfileString(L"mem-video", L"channelname", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) <= 0)
	{
		deleteAndZero(newSetting);
		return nullptr;
	}
	newSetting->setValue(KEY_MEM_VIDEO_NAME, String(szTXT));

	if (GetPrivateProfileString(L"mem-audio", L"channelname", L"", szTXT, 255, fileFullPath.toWideCharPointer()) <= 0)
	{
		deleteAndZero(newSetting);
		return nullptr;
	}
	newSetting->setValue(KEY_MEM_AUDIO_NAME, String(szTXT));
	return newSetting;
}
#ifdef UNICODE
#define strToDouble _wtof
#else
#define strToDouble atof
#endif
PropertySet* readPropertiesFromIni(String iniFilePath, String memIniFilePath, vOutputCfg** ppvOutputCfg)
{
	if (iniFilePath.isNotEmpty() && !File::isAbsolutePath(iniFilePath))
	{
		return nullptr;
	}
	if (memIniFilePath.isNotEmpty() && !File::isAbsolutePath(memIniFilePath))
	{
		return nullptr;
	}
	File tmpFile = File(iniFilePath);
	File memIniFile = File(memIniFilePath);
	if (!tmpFile.existsAsFile() && !memIniFile.existsAsFile())
	{
		return nullptr;
	}

	PropertySet* newSetting = new PropertySet(true);
	String fileFullPath = tmpFile.getFullPathName();
	String tmpFileFullPath = fileFullPath;
	String memFileFullPath = memIniFile.getFullPathName();

	TCHAR szTXT[1024];

	// process global event name
	if (GetPrivateProfileString(L"event", L"stseventname", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
	{
		newSetting->setValue(KEY_PROCESS_EVENT_NAME, String(szTXT));
	}

	if (memFileFullPath.isNotEmpty())
	{
		fileFullPath = memFileFullPath;
	}

	if (GetPrivateProfileString(L"common", L"acceleration", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
	{
		newSetting->setValue(KEY_ACCELERATION_TYPE, String(szTXT));
	}

	// mem-video
	if (GetPrivateProfileString(L"mem-video", L"channelname", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) <= 0)
	{
		deleteAndZero(newSetting);
		return nullptr;
	}
	newSetting->setValue(KEY_MEM_VIDEO_NAME, String(szTXT));

	GetPrivateProfileString(L"common", L"info", L"", szTXT, 1024, fileFullPath.toWideCharPointer());
	newSetting->setValue(KEY_OUT_DATA_INFO, (int)String(szTXT).equalsIgnoreCase("enable"));
	newSetting->setValue(KEY_OUT_DATA_INFO_HZ, (int)GetPrivateProfileInt(L"common", L"infoHZ", 10, fileFullPath.toWideCharPointer()));

	newSetting->setValue(
		KEY_MEM_VIDEO_WIDTH,
		(int)GetPrivateProfileInt(L"mem-video", L"width", 480, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_MEM_VIDEO_HEIGHT,
		(int)GetPrivateProfileInt(L"mem-video", L"height", 270, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_MEM_VIDEO_BUFFERS,
		(int)GetPrivateProfileInt(L"mem-video", L"buffers", 1, fileFullPath.toWideCharPointer()));

	// mem-audio
	if (GetPrivateProfileString(L"mem-audio", L"channelname", L"", szTXT, 255, fileFullPath.toWideCharPointer()) <= 0)
	{
		deleteAndZero(newSetting);
		return nullptr;
	}

	newSetting->setValue(KEY_MEM_AUDIO_NAME, String(szTXT));

	newSetting->setValue(
		KEY_MEM_AUDIO_CHS,
		(int)GetPrivateProfileInt(L"mem-audio", L"channels", 2, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_MEM_AUDIO_FREQ,
		(int)GetPrivateProfileInt(L"mem-audio", L"frequency", 48000, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_MEM_AUDIO_BLOCKSIZE,
		(int)GetPrivateProfileInt(L"mem-audio", L"buffersize", 5000, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_MEM_AUDIO_BUFFERS,
		(int)GetPrivateProfileInt(L"mem-audio", L"buffers", 5, fileFullPath.toWideCharPointer()));

	GetPrivateProfileString(L"mem-audio", L"sampleformat", L"S16LE", szTXT, 255, fileFullPath.toWideCharPointer());
	newSetting->setValue(KEY_MEM_AUDIO_SAMPLE_FORMAT, String(szTXT));

	// out-video
	if (tmpFileFullPath.isNotEmpty())
	{
		fileFullPath = tmpFileFullPath;
	}
	else
	{
		return newSetting;
	}
	newSetting->setValue(
		KEY_OUT_VIDEO_WIDTH,
		(int)GetPrivateProfileInt(L"output-video", L"width", 480, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_HEIGHT,
		(int)GetPrivateProfileInt(L"output-video", L"height", 270, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_BITRATE,
		(int)GetPrivateProfileInt(L"output-video", L"bitrate", 80000, fileFullPath.toWideCharPointer()));
/*
#pragma region
	newSetting->setValue(
		KEY_OUT_VIDEO_ANALYSE,
		(int)GetPrivateProfileInt(L"output-video", L"analyse", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_AUD,
		(int)GetPrivateProfileInt(L"output-video", L"aud", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_B_ADAPT,
		(int)GetPrivateProfileInt(L"output-video", L"b-adapt", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_B_PYRAMID,
		(int)GetPrivateProfileInt(L"output-video", L"b-pyramid", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_B_FRAMES,
		(int)GetPrivateProfileInt(L"output-video", L"bframes", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_CABAC,
		(int)GetPrivateProfileInt(L"output-video", L"cabac", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_DCT8X8,
		(int)GetPrivateProfileInt(L"output-video", L"dct8x8", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_INTERLACED,
		(int)GetPrivateProfileInt(L"output-video", L"interlaced", 0, fileFullPath.toWideCharPointer()));
	GetPrivateProfileString(L"output-video", L"ip-factor", L"1.4", szTXT, 255, fileFullPath.toWideCharPointer());
	newSetting->setValue(KEY_OUT_VIDEO_IP_FACTOR, strToDouble(szTXT));
	newSetting->setValue(
		KEY_OUT_VIDEO_GOP,
		(int)GetPrivateProfileInt(L"output-video", L"key-int-max", 60, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_ME,
		(int)GetPrivateProfileInt(L"output-video", L"me", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_NOISE_REDUCTION,
		(int)GetPrivateProfileInt(L"output-video", L"noise-reduction", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_PASS,
		(int)GetPrivateProfileInt(L"output-video", L"pass", 0, fileFullPath.toWideCharPointer()));
	GetPrivateProfileString(L"output-video", L"pb-factor", L"1.3", szTXT, 255, fileFullPath.toWideCharPointer());
	newSetting->setValue(KEY_OUT_VIDEO_PB_FACTOR, strToDouble(szTXT));
	newSetting->setValue(
		KEY_OUT_VIDEO_QP_MAX,
		(int)GetPrivateProfileInt(L"output-video", L"qp-max", 51, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_QP_MIN,
		(int)GetPrivateProfileInt(L"output-video", L"qp-min", 10, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_QP_STEP,
		(int)GetPrivateProfileInt(L"output-video", L"qp-step", 4, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_QUANTIZER,
		(int)GetPrivateProfileInt(L"output-video", L"quantizer", 21, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_REF,
		(int)GetPrivateProfileInt(L"output-video", L"ref", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_SPS_ID,
		(int)GetPrivateProfileInt(L"output-video", L"sps-id", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_SUBME,
		(int)GetPrivateProfileInt(L"output-video", L"subme", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_THREADS,
		(int)GetPrivateProfileInt(L"output-video", L"threads", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_TRELLIS,
		(int)GetPrivateProfileInt(L"output-video", L"trellis", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_VBV_BUF_CAPACITY,
		(int)GetPrivateProfileInt(L"output-video", L"vbv-buf-capacity", 600, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_WEIGHTB,
		(int)GetPrivateProfileInt(L"output-video", L"weightb", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_INTRA_REFRESH,
		(int)GetPrivateProfileInt(L"output-video", L"intra-refresh", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_MB_TREE,
		(int)GetPrivateProfileInt(L"output-video", L"mb-tree", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_RC_LOOKAHEAD,
		(int)GetPrivateProfileInt(L"output-video", L"rc-lookahead", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_SLICED_THREADS,
		(int)GetPrivateProfileInt(L"output-video", L"sliced-threads", 1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_SYNC_LOOKAHEAD,
		(int)GetPrivateProfileInt(L"output-video", L"sync-lookahead", -1, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_PSY_TUNE,
		(int)GetPrivateProfileInt(L"output-video", L"psy-tune", 0, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_VIDEO_TUNE,
		(int)GetPrivateProfileInt(L"output-video", L"tune", 4, fileFullPath.toWideCharPointer()));
#pragma endregion
	newSetting->setValue(
		KEY_OUT_VIDEO_PRESET,
		(int)GetPrivateProfileInt(L"output-video", L"preset", 3, fileFullPath.toWideCharPointer()));
*/
	int iLen = GetPrivateProfileString(L"output-video", NULL, NULL, szTXT, 1024, fileFullPath.toWideCharPointer());
	vOutputCfg* pvOutputCfg = new vOutputCfg;
	pvOutputCfg->isHaveFloatCfg = false;
	TCHAR* p = szTXT;
	for (int i = 0; i < iLen;)
	{
		String strCfg(p);
		if (strCfg.equalsIgnoreCase("ip-factor") || strCfg.equalsIgnoreCase("pb-factor"))
		{
			TCHAR pCfg[1024] = { 0 };
			pvOutputCfg->isHaveFloatCfg = true;
			GetPrivateProfileString(L"output-video", strCfg.toWideCharPointer(), L"1.4", pCfg, 255, fileFullPath.toWideCharPointer());
			pvOutputCfg->mapFloatCfgs.insert(make_pair(strCfg, strToDouble(pCfg)));
		}
		else if (!strCfg.equalsIgnoreCase("profile"))
		{
			pvOutputCfg->mapIntCfgs.insert(
				make_pair(strCfg, 
					GetPrivateProfileInt(L"output-video", strCfg.toWideCharPointer(), 0, fileFullPath.toWideCharPointer()))
			);
		}
		i += strCfg.length() + 1;
		p = szTXT + i;
	}
	GetPrivateProfileString(L"output-video", L"profile", L"main", szTXT, 255, fileFullPath.toWideCharPointer());
	pvOutputCfg->strProfile = szTXT;
	if (ppvOutputCfg != nullptr)
	{
		*ppvOutputCfg = pvOutputCfg;
	}
	//newSetting->setValue(KEY_OUT_VIDEO_PROFILE, String(szTXT));


	// out-audio
	newSetting->setValue(
		KEY_OUT_AUDIO_CHS,
		(int)GetPrivateProfileInt(L"output-audio", L"channels", 2, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_AUDIO_FREQ,
		(int)GetPrivateProfileInt(L"output-audio", L"frequency", 48000, fileFullPath.toWideCharPointer()));
	newSetting->setValue(
		KEY_OUT_AUDIO_BITRATE,
		(int)GetPrivateProfileInt(L"output-audio", L"bitrate", 8000, fileFullPath.toWideCharPointer()));

	// uri
	// rtp/rtmp

	String strUri;
	if (GetPrivateProfileString(L"rtp", L"url", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
	{
		strUri = szTXT;
	}
	else
	{
		// file
		if (GetPrivateProfileString(L"file", L"fullpath", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
		{
			strUri = szTXT;
			juce::File file(strUri);
			juce::File Directory = file.getParentDirectory();
			if (!Directory.exists())
			{
				file.createDirectory();
			}
		}
	}

	if (strUri.isNotEmpty())
	{
		newSetting->setValue(KEY_OUT_URI, strUri);

		return newSetting;
	}

	// HW
	if (GetPrivateProfileString(L"hardware", L"card", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
	{
		newSetting->setValue(KEY_OUT_HW_CARD, String(szTXT));
	}
	newSetting->setValue(
		KEY_OUT_HW_INDEX,
		(int)GetPrivateProfileInt(L"hardware", L"deviceindex", 0, fileFullPath.toWideCharPointer()));
	if (GetPrivateProfileString(L"hardware", L"modekeyword", L"", szTXT, 1024, fileFullPath.toWideCharPointer()) > 0)
	{
		newSetting->setValue(KEY_OUT_HW_KEYWORD, String(szTXT));
	}

	return newSetting;
}

void writePropertiesInIniFile(String iniFilePath, int errorCode, void* handleEvent, String errorMsg)
{
	if (iniFilePath.isEmpty() || !File::isAbsolutePath(iniFilePath))
	{
		return;
	}
	WritePrivateProfileString(L"states", L"stscode", String(errorCode).toWideCharPointer(), iniFilePath.toWideCharPointer());
	if (errorMsg.isEmpty())
	{
		switch (errorCode)
		{
		case SE_ERROR_CODE_SUCCESS:
			errorMsg = L"³É¹¦";
			break;
		case SE_ERROR_CODE_STOPPED:
			errorMsg = L"ÒÑÍ£Ö¹";
			break;
		case SE_ERROR_CODE_FAILED:
			errorMsg = L"Ê§°Ü";
			break;
		case SE_ERROR_CODE_INI_FILE_FAILED:
			errorMsg = L"ÅäÖÃÎÄ¼þ»ñÈ¡Ê§°Ü";
			break;
		case SE_ERROR_CODE_SHARED_MEMEORY_VIDEO_FAILED:
			errorMsg = L"ÊÓÆµ¹²ÏíÄÚ´æ·ÃÎÊÊ§°Ü";
			break;
		case SE_ERROR_CODE_SHARED_MEMEORY_AUDIO_FAILED:
			errorMsg = L"ÒôÆµ¹²ÏíÄÚ´æ·ÃÎÊÊ§°Ü";
			break;
		case SE_ERROR_CODE_CODER_EXIST:
			errorMsg = L"±àÂëÆ÷Ãû³ÆÒÑ´æÔÚ";
			break;
		case SE_ERROR_CODE_CODER_NOT_FOUND_TYPE:
			errorMsg = L"Î´Öª±àÂëÆ÷ÀàÐÍ";
			break;
		case SE_ERROR_CODE_PLAY_FAILED:
			errorMsg = L"±àÂëÆ÷ÔËÐÐÊ§°Ü";
			break;
		default:
			errorMsg = L"Î´Öª´íÎó";
			break;
		}
	}
	WritePrivateProfileString(L"states", L"stsmsg", errorMsg.toWideCharPointer(), iniFilePath.toWideCharPointer());

	if (handleEvent)
	{
		::SetEvent(handleEvent);
	}
}

String bytesToString(guint64 bytes)
{ 
	if (bytes > BYTES_M)
	{
		return String::formatted(L"%.02f MB", (double)(bytes) / BYTES_M);
	}
	else if (bytes > BYTES_K)
	{
		return String::formatted(L"%.02f KB", (double)(bytes) / BYTES_K);
	}
	else
	{
		return String::formatted(L"%d B", bytes);
	}

	//return String::empty;
}

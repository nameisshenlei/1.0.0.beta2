#ifndef NEMO_HELP_FUNCTIONS_H
#define NEMO_HELP_FUNCTIONS_H
#include "map"
#include "publicHeader.h"
using namespace std;

typedef struct{
	String strProfile;
	bool isHaveFloatCfg;
	map<String, float>	mapFloatCfgs;
	map<String, int>	mapIntCfgs;
}vOutputCfg;

// crash report
bool instalCrashReport();
void uninstallCrashReport();

bool gstHelpInitEnviroment();
void gstHelpReigsterRtmp();

int gstHelpString2DecklinkModeIndex(String strModeKey);

PropertySet* readPropertFromMemIni(String memFilePath);
PropertySet* readPropertiesFromIni(String iniFilePath, String memIniFilePath = L"", vOutputCfg** ppvOutputCfg = nullptr);
void writePropertiesInIniFile(String iniFilePath, int errorCode, void* handleEvent = nullptr, String errorMsg = String::empty);

String bytesToString(guint64 bytes);
//////////////////////////////////////////////////////////////////////////

#endif
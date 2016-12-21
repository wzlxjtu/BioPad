//#define GPPAPI_PLUGIN
#define GCAPI_DIRECT
#include "gcapi.h"

GCDAPI_Load gcdapi_Load = NULL;
GCDAPI_Unload gcdapi_Unload = NULL;
GCAPI_IsConnected gcapi_IsConnected = NULL;
GCAPI_GetFWVer gcapi_GetFWVer = NULL;
GCAPI_Read gcapi_Read = NULL;
GCAPI_Write gcapi_Write = NULL;
GCAPI_GetTimeVal gcapi_GetTimeVal = NULL;
GCAPI_CalcPressTime gcapi_CalcPressTime = NULL;

HINSTANCE CronusInst = NULL;

#ifndef APP_H
#define APP_H

#include "Arduino.h"
#include "globals.h"
#include "typedefs.h"

#define APP_SYS_PARAM_CONFIG_FILE_PATH "/config1.txt"
#define APP_SYS_PARAM_2_CONFIG_FILE_PATH "/config2.txt"

void AppInit(void);
bool AppMakeDefaultConfigSystemParams(void);
bool AppSetConfigSysParams(SYSTEM_PARAM *pSysParam);
bool AppGetConfigSysParams(SYSTEM_PARAM *pSysParam, uint32_t *pReadLen);
bool AppMakeDefaultSysParams2(void);
bool AppSetConfigSysParams2(SYSTEM_PARAM_2 *pPumpCalcParam);
bool AppGetConfigSysParams2(SYSTEM_PARAM_2 *pPumpCalcParam, uint32_t *pReadLen);

#endif
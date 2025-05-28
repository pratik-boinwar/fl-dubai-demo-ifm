#ifndef _GLOBAL_VARS_H_
#define _GLOBAL_VARS_H_

#include "typedefs.h"

// Enable this line to test logger
// #define LOGGER_TEST

#ifndef LOGGER_TEST
typedef uint32_t LOG_UNIX_TIMESTAMP;

typedef union
{
    unsigned char array[sizeof(LOG_UNIX_TIMESTAMP) + (sizeof(float) * 6)];

    struct
    {
        LOG_UNIX_TIMESTAMP unixTimestamp;
        float fl1_FlowRate;
        float fl1_totalizer;
        // int fl1_highAlarm;
        // int fl1_lowAlarm;
        float fl1_dalilyFlowRate;

        float fl2_FlowRate;
        float fl2_totalizer;
        // int fl2_highAlarm;
        // int fl2_lowAlarm;
        float fl2_dalilyFlowRate;
    };
} TRANSACTION;
#endif

extern SYSTEM_PARAM gSysParam;
extern SYSTEM_PARAM_2 gSysParam2;
extern SYSTEM_FLAG gSystemFlag;
extern DISPLAY_SYSTEM_FLAG gSystemDisplayFlag;
#endif
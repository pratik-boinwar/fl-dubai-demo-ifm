#ifndef LOGGER_H
#define LOGGER_H

#include "global-vars.h"
#include "globals.h"

#define MAX_TRANSACTIONS ((4 * 1024 * 1024) / sizeof(TRANSACTION))

#ifndef LOG_DEBUG
#define LOG_DEBUG systemFlags.logDebugEnableFlag
#endif

#ifndef logDBG
#define logDBG(funcall) \
    {                   \
        if (LOG_DEBUG)  \
        {               \
            funcall     \
        }               \
    }
#endif

typedef union
{
    unsigned char array[3 * sizeof(unsigned int)];

    struct
    {
        unsigned int current;
        unsigned int start;
        unsigned int end;
    };
} LOG_INFO;

#ifdef LOGGER_TEST
typedef uint32_t UNIX_TIME;
typedef uint32_t PARAMVALUE;
typedef uint32_t RECORD_NO;

typedef union
{
    unsigned char array[sizeof(UNIX_TIME) + sizeof(PARAMVALUE) + sizeof(RECORD_NO)];
    struct
    {
        RECORD_NO logNo;
        UNIX_TIME logDT;
        PARAMVALUE logPV;
    };
} TRANSACTION;
#endif

bool LoggerInit(void);
unsigned int LoggerPush(TRANSACTION *pTransaction);
bool LoggerFlush(void);
bool LoggerIsPacketAvailable(void);
unsigned int LoggerGetPacket(TRANSACTION *pTransaction);
unsigned int LoggerGetPacketByLogNumber(TRANSACTION *pTransaction, uint32_t logNumber = 0);
bool LoggerDeletePacket(uint16_t numOfLogs = 1);
unsigned int LoggerGetCount(uint32_t *pTotalLogs);

#ifdef LOGGER_TEST
void LoggerTest(void);
void PrintMenu(void);
#endif
#endif // LOGGER_H
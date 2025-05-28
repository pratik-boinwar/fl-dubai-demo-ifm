#ifndef CMDS_H
#define CMDS_H

// #ifdef	__cplusplus
// extern "C" {
// #endif
#include "CircularBuffer.h"

#define MAX_ARGS 27

typedef enum _CMD_STATUS_
{
    CMDLINE_BAD_ARGS = -1,
    CMDLINE_BAD_CMD = -2,
    CMDLINE_BAD_PARAM = -3,
    CMDLINE_NO_REPLY = -4,
    CMDLINE_FAIL = 0,
    CMDLINE_SUCCESS,
    CMDLINE_AT_SUCCESS,
    CMDLINE_RESP
} CMD_STATUS;

typedef enum
{
    CHECK_FOR_SEQ_ID = 0,
    BYPASS_SEQ_ID
} PARSE_ID_TYPE;

#define SEPARATOR ' '

//*****************************************************************************
//
// Command line function callback type.
//
//*****************************************************************************
typedef CMD_STATUS (*pfncCmd)(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
typedef CMD_STATUS (*pfncParam)(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
typedef bool (*pfncNormalize)(int *pArgSize, char *argv[]);
//*****************************************************************************
//
// Structure for an entry in the parameter list table.
//
//*****************************************************************************

// typedef struct
// {
//     TIME time;

//     char day;

//     CALENDAR calendar;
// } tDateTime;

typedef struct
{
    //
    // A pointer to a string containing the name of the parameter.
    //
    const char *pcParam;

    //
    // A function pointer to the implementation of the parameter.
    //
    pfncParam pfnParam;

} tParamEntry;

//*****************************************************************************
//
// Structure for an entry in the command list table.
//
//*****************************************************************************

typedef struct
{
    //
    // A pointer to a string containing the name of the command.
    //
    const char *pcCmd;

    //
    // A function pointer to the implementation of the command.
    //
    pfncCmd pfnCmd;

    tParamEntry *pParamTable;

} tCmdEntry;

//*****************************************************************************
//
// This is the command table that must be provided by the application.
//
//*****************************************************************************
extern tCmdEntry cmdTable[];

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
// extern INT32 CmdLineProcess(CHAR *pcCmdLine);
// extern int CmdLineProcess(char *pcCmdLine, CIRCULAR_BUFFER *respBuf, void *userInfo = ((PARSE_ID_TYPE *)BYPASS_SEQ_ID), pfncNormalize pfnNormalize = 0);
extern int CmdLineProcess(char *pcCmdLine, CIRCULAR_BUFFER *respBuf, void *userInfo, pfncNormalize pfnNormalize = 0);

// #ifdef	__cplusplus
// }
// #endif

#endif //  CMDS_H

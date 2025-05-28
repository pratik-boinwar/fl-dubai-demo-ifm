#include <string.h>
#include <stdlib.h>
#include "cmds.h"
#include "CircularBuffer.h"
#include "helper.h"
#include <Arduino.h>
#include <FLToolbox.h>
#include "Quectel.h"

extern tCmdEntry cmdTable[];
char *msgData = NULL;

/**
 * @fn INT32 CmdLineProcess(CHAR *pcCmdLine, CIRCULAR_BUFFER *pRespBuf)
 * @brief This function processes the command as given and gives back the
 *        response to the command
 * @param jsmn_parser *parser
 * @remark
 */
int CmdLineProcess(char *pcCmdLine, CIRCULAR_BUFFER *pRespBuf, void *userInfo, pfncNormalize pfnNormalize)
{
    static char *argv[MAX_ARGS] = {NULL};
    char *pcChar = NULL;
    int argc;
    int bFindArg = 1;
    bool seqFound = 0;
    // char *cmdStr;
    // char seperatorCnt;
    tCmdEntry *pCmdEntry;
    tParamEntry *pParamEntry;
    int status;
    char *cpyData;

    cpyData = pcCmdLine;
    msgData = strtok(cpyData, "\"");
    msgData = strtok(NULL, "\"");

#ifdef GSM_AT_COMMAND_CONSOLE
    char atStr[3];
    atStr[0] = *pcCmdLine;
    atStr[1] = *(pcCmdLine + 1);
    atStr[2] = 0;
    ToUpper(atStr);
    // if 'pcCmdLine' is staring with AT or at..
    // send 'pcCmdLine' to quectel ...

    if (0 == strcmp(atStr, "AT"))
    {
        pcCmdLine += 2;
        if (IsATavailable())
        {
            // enable gsm dbg temporarily
            // char ch = systemFlags.gsmDebugEnableFlag;
            // systemFlags.gsmDebugEnableFlag = 1;
            // QuectelDebug(true);

            sendAT(pcCmdLine);

            String data;
            int ret = waitResponse(2000, data);
            if (0 == ret)
            {
                CbPushS(pRespBuf, "Retry AT command");
            }
            if (data.length())
            {
                CbPushS(pRespBuf, data.c_str());
            }
            else
            {
            }
            // restore original gsm debug state
            // systemFlags.gsmDebugEnableFlag = ch;
            // if (!systemFlags.gsmDebugEnableFlag)
            //     QuectelDebug(false);

            return CMDLINE_AT_SUCCESS;
        }

        return CMDLINE_FAIL;
    }
#endif

    //
    // Initialize the argument counter, and point to the beginning of the
    // command line string.
    //
    argc = 0;
    // seperatorCnt = 0;
    pcChar = pcCmdLine;
    //
    // Advance through the command line until a zero CHARacter is found.
    //
    while (*pcChar)
    {
        //
        // If there is a separator, then replace it with a zero, and set the flag
        // to search for the next argument.
        //
        if (*pcChar == SEPARATOR)
        {
            *pcChar = 0;
            bFindArg = 1;
        }

        //
        // Otherwise it is not a space, so it must be a CHARacter that is part
        // of an argument.
        //
        else
        {
            //
            // If bFindArg is set, then that means we are looking for the start
            // of the next argument.
            //
            if (bFindArg)
            {
                //
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                //
                if (argc < MAX_ARGS)
                {
                    argv[argc] = pcChar;
                    argc++;
                    bFindArg = 0;
                }

                //
                // The maximum number of arguments has been reached so return
                // the error.
                //
                else
                {
                    // CbPushS(pRespBuf, "TOO MANY ARGS\r");
                    return (CMDLINE_BAD_ARGS);
                }
            }
        }

        //
        // Advance to the next CHARacter in the command line.
        //
        pcChar++;
    }

    //
    // If one or more arguments was found, then process the command.
    //

    if (argc)
    {
        PARSE_ID_TYPE idType;
        idType = *((PARSE_ID_TYPE *)userInfo);

        if (CHECK_FOR_SEQ_ID == idType)
        {
            if (!pfnNormalize(&argc, argv))
            {
                seqFound = 0;
            }
            else
            {
                seqFound = 1;
                argc--;
            }
        }

        //
        // Start at the beginning of the command table, to look for a matching
        // command.
        //
        pCmdEntry = &cmdTable[0];

        //
        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        //
        while (pCmdEntry->pcCmd)
        {
            //
            // If this command entry command string matches argv[0], then call
            // the function for this command, passing the command line
            // arguments.
            //
            if (argv[0] == NULL)
            {
                CbPushS(pRespBuf, ERROR_RESP);
                return CMDLINE_FAIL;
            }
            ToUpper(argv[0]); // uppercase first argument    #author-Nivrutti Mahajan
            if (!strcmp(argv[0], pCmdEntry->pcCmd))
            {
                if (pCmdEntry->pfnCmd != NULL)
                {
                    CbClear(pRespBuf);
                    status = (pCmdEntry->pfnCmd(argc, argv, pRespBuf));
                    if (1 == seqFound)
                    {
                        CbPush(pRespBuf, ' ');
                        CbPushS(pRespBuf, argv[argc]);
                    }
                    CbPushS(pRespBuf, "\r\n");
                    return status;
                }
                else
                {
                    //
                    // Start at the beginning of the param table, to look for a matching
                    // parameter.
                    //
                    pParamEntry = &pCmdEntry->pParamTable[0];
                    systemFlags.smsCmdRx = true;
                    while (pParamEntry->pcParam)
                    {
                        //
                        // If this parameter entry param string matches argv[1], then call
                        // the function for this param, passing the command line
                        // arguments.
                        //
                        if (argv[1] == NULL)
                        {
                            CbPushS(pRespBuf, ERROR_RESP);
                            return CMDLINE_FAIL;
                        }
                        ToUpper(argv[1]); // uppercase second argument    #author-Nivrutti Mahajan
                        if (!strcmp(argv[1], pParamEntry->pcParam))
                        {
                            CbClear(pRespBuf);
                            status = pParamEntry->pfnParam(argc, argv, pRespBuf);
                            if (1 == seqFound)
                            {
                                CbPush(pRespBuf, ' ');
                                CbPushS(pRespBuf, argv[argc]);
                            }
                            CbPushS(pRespBuf, "\r\n");
                            return status;
                        }
                        //
                        // Not found, so advance to the next entry.
                        //
                        pParamEntry++;
                    }
                    //
                    // Fall through to here means that no matching param was found, so return
                    // an error.
                    //
                    CbPushS(pRespBuf, "BAD PARAM");
                    if (1 == seqFound)
                    {
                        CbPush(pRespBuf, ' ');
                        CbPushS(pRespBuf, argv[argc]);
                    }
                    CbPushS(pRespBuf, "\r\n");
                    return (CMDLINE_BAD_PARAM);
                }
            }

            //
            // Not found, so advance to the next entry.
            //
            pCmdEntry++;
        }
    }

    //
    // Fall through to here means that no matching command was found, so return
    // an error.
    //
    CbPushS(pRespBuf, "BAD CMD");
    if (1 == seqFound)
    {
        CbPush(pRespBuf, ' ');
        CbPushS(pRespBuf, argv[argc]);
    }
    CbPushS(pRespBuf, "\r\n");
    return (CMDLINE_BAD_CMD);
}
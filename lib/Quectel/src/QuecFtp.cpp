/**
 * @file       QuecFtp.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecFtp.h"
#include "QuecFile.h"

#define FTP_MAX_DWNLD_FILE_SIZE (500 * 1024U)

extern void freezeAT(void);
extern void unfreezeAT(void);

/**
 * FTP_SM states for M95 ftp client
 */
typedef enum _SM_FTPC
{
    FTP_SM_NULL = 0,

    FTP_SM_INTIATED,
    FTP_SM_CHECK_CONNECTION,
    FTP_SM_CONFIGURE_USER_CREDINTIAL,
    FTP_SM_OPEN_CONNECTION,
    FTP_SM_WAIT_FOR_CONNECTION,
    FTP_SM_CHECK_REMOTE_FILE_SIZE,
    FTP_SM_SET_LOCAL_FILENAME,
    FTP_SM_SET_REMOTE_FILEPATH,
    FTP_SM_DOWNLOAD_FILE,
    // FTP_SM_WAIT_FOR_DOWNLOAD_FILE,
    FTP_SM_UPLOAD_FILE,
    // FTP_SM_SET_REMOTE_MAKEDIR,

    FTP_SM_GENERIC_DELAY,
    //    SM_FTPC_HOME, //home, check here gprs context activation status
    //    SM_FTPC_TIMEOUT, //set ftp timeout
    FTP_SM_CLEAN_UP_SUCCESS,
    FTP_SM_CLEAN_UP_FAIL,
    FTP_SM_IDLE,

} FTP_SM;

/**
 * FtpParameters_t holds the ftp process parameters
 */
typedef struct _FtpParameters
{
    const char *ftpServerHost;          /**< IP or DNS of FTP Server */
    uint16_t ftpServerPort;             /**< FTP Server port, default=21 */
    const char *ftpUserName;            /**< user name required to log in server */
    const char *ftpUserPass;            /**< user pass required to log in server */
    const char *ftpRemoteFilePath;      /**< default is "//"root dir in server, else mentioned */
    const char *ftpRemoteFileName;      /**< file name in server to do operation on it */
    const char *ftpLocalFileName;       /**< file name to be uploaded */
    QuecFileStorage_t localFileStorage; /**< storage memory area of file to be uploaded */
    FtpCnxType_t ftpCnxType;            /**< ftp connection used for upload file, download file, etc. */
    FtpFileMode_t ftpFileMode;          /**< ftp file process mode Append or Overwrite remote file */
    //    pfnFtpEventHandlerCb ftpEventHandlerCb; // A function pointer to the implementation of the command.

} FtpParameters_t;

#if defined(USE_QUECTEL_BG96)
typedef enum _FtpCnxState
{
    FTP_CNX_IDLE = 3,
    FTP_CNX_CLOSED = 4
} FtpCnxState_t;
#else
typedef enum _FtpCnxState
{
    FTP_CNX_IDLE = 1,
    FTP_CNX_OPENING,
    FTP_CNX_OPENED,
    FTP_CNX_CLOSED,
    FTP_CNX_WORKING,
    FTP_CNX_TRANSFER,
    FTP_CNX_CLOSING,

} FtpCnxState_t;
#endif

///holds the copy of ftp parameter
static FtpParameters_t _ftpParam;
///holds the current and previous operating state
static FTP_SM _eFtpSmState = FTP_SM_IDLE, _eFtpSmPrevState = FTP_SM_IDLE;
///general ticks used in processing
static uint32_t _localGenericTick = 0, _genericDelayTick = 0;
///holds the state app purpose of calling ftp process
// static FtpCnxType_t _ftpConnectionFor = FTP_DOWNLOAD_FILE;
///holds the last error occurred notification of ftp process
FtpLastError_t _mdmFtpLastError;
///holds the current status of process
FtpStatus_t _mdmFtpStatus;
/// holds the remote file size
static int32_t _filesize = 0;
/// holds the local storage area free size in bytes.
static long _availableSize = 0;

static int8_t _FtpGetConnectionStatus(void)
{
    sendAT("+QFTPSTAT");

#if defined(USE_QUECTEL_BG96)
    /*
     AT+QFTPSTAT

     OK

     +QFTPSTAT: 0,4
     
     */

    int8_t ret = waitResponse3("+QFTPSTAT: ", GSM_ERROR);

    if (1 == ret)
    {
        streamSkipUntil(',');
        int16_t state = streamGetIntBefore('\r');
        waitResponse3(GSM_OK); // read out remaining
        if ((state == 3) || (state == 4))
            return 1;
    }
#else
    /*
     AT+QFTPSTAT

     +QFTPSTAT: <state>

     OK
     
     */

    // The 'state' are IDLE, OPENING, OPENED, WORKING, TRANSFER, CLOSING, CLOSED
    int8_t ret = waitResponse3("+QFTPSTAT: ", GSM_ERROR);

    if (ret == 1)
    {
        String state = streamAT().readStringUntil('\r');
        waitResponse3(GSM_OK); // read out remaining
        if (0 == state.compareTo("IDLE"))
            return FTP_CNX_IDLE;
        else if (0 == state.compareTo("CLOSED"))
            return FTP_CNX_CLOSED;
        else if (0 == state.compareTo("OPENED"))
            return FTP_CNX_OPENED;
        else if (0 == state.compareTo("OPENING"))
            return FTP_CNX_OPENING;
        else if (0 == state.compareTo("WORKING"))
            return FTP_CNX_WORKING;
        else if (0 == state.compareTo("TRANSFER"))
            return FTP_CNX_TRANSFER;
        else if (state.compareTo("CLOSING"))
            return FTP_CNX_CLOSING;
        else
            return 0;
    }
#endif

    return 0;
}

static int8_t _FtpSetUserCredential(const char *username, const char *password)
{
    streamAT().print("AT+QFTPUSER=\"");
    streamAT().print(username);
    streamAT().println("\"");

    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (1 != ret)
    {
        return 0;
    }

    streamAT().print("AT+QFTPPASS=\"");
    streamAT().print(password);
    streamAT().println("\"");

    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (1 != ret)
    {
        return 0;
    }

    return 1;
}

static int8_t _FtpVerifyServerHost(const char *host, uint16_t port)
{
    /*
    AT+QFTPOPEN?

    +QFTPOPEN: "<hostName>",<port>
    
    OK
    */

    sendAT("+QFTPOPEN?");
    int8_t ret = waitResponse3("+QFTPOPEN:");
    if (1 == ret)
    {
        streamSkipUntil('\"');
        String sHost = streamAT().readStringUntil('\"');
        streamSkipUntil(',');
        if (0 == sHost.compareTo(host))
        {
            // check port
            uint16_t sPort = (uint16_t)streamGetIntBefore('\r');

            waitResponse3(GSM_OK); // read reamaining

            return sPort == port ? 1 : 0;
        }
    }

    return 0;
}

static int8_t _FtpOpenConnection(const char *host, uint16_t port)
{
    streamAT().print("AT+QFTPOPEN=\"");
    streamAT().print(host);
    streamAT().print("\",");
    streamAT().println(port);

    int8_t ret = waitResponse3(GSM_OK);

    if (1 == ret)
    {
        return 1;
    }

    return 0;
}

static int8_t _FtpCloseConnection()
{
    sendAT("+QFTPCLOSE");

    if (waitResponse2(2000, "+QFTPCLOSE: 0", GSM_ERROR) == 1)
        return 1;

    return 0;
}

static int8_t _FtpSetLocalPath(const char *localFileName, QuecFileStorage_t memory)
{
    /*
    AT+QFTPCFG=4,"/RAM/" //Set the local position as RAM.
    
    OK
    
    +QFTPCFG: 0
    */
    if (localFileName == NULL)
        return -1;

    streamAT().write("AT+QFTPCFG=4,\"");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        streamAT().write("/UFS/");
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("/RAM/");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().write("/SD/");
        break;

    default:
        streamAT().print("\r\n");
        waitResponse3(GSM_ERROR);
        return -3;
        break;
    }

    streamAT().write(localFileName);
    streamAT().println("\"");

    int8_t ret = waitResponse3("+QFTPCFG:0" GSM_NL, GSM_ERROR);

    if (1 == ret)
        return 1;

    return 0;
}

static int8_t _FtpSetRemotePath(const char *remoteFilePath, bool createRemoteDir) // const char *remoteFileName,
{
    /*
    AT+QFTPPATH="/" //Set the path to download file as "/".
    
    OK
    
    +QFTPPATH: 0 //Successfully set the path.
    */

    //ToDo :  Done
    // check if remoteFilePath includes directory or not
    // if directory path is present then create first remote directory

    // if (remoteFileName == NULL)
    //     return -1;

    if (remoteFilePath[0] != '/')
        return -1;

    // set root path
    sendAT("+QFTPPATH=\"/\"");
    int8_t ret = waitResponse3("+QFTPPATH: 0" GSM_NL, GSM_ERROR);

    /*
        this flag can be false.
        1) when, only root path is required. 
        2) for downloading session, need not required to create remote directories.
        3) for ftp user only having read access. creating directory attempt going to be fail anyway.
           though we are not checking dir creating response, so trying to avoid extra commands here.
    */
    if (!createRemoteDir)
    {
        // set remote path
        streamAT().print("AT+QFTPPATH=\"");
        streamAT().print(remoteFilePath);
        streamAT().println("\"");
        ret = waitResponse3("+QFTPPATH: 0" GSM_NL, GSM_ERROR);

        if (1 == ret)
            return 1;
        else
            return 0;
    }

    if (1 == ret)
    {
        if (remoteFilePath[0] == '/' && remoteFilePath[1] != 0)
        {
            // directories are in the path. navigate through each directory and create remote directory
            // Note that we are not checking repsonse of command for success or fail.
            // we might not have given write access or folder might already present.
            String fPath = "";
            uint8_t i = 1;
            do
            {
                if ((remoteFilePath[i] == '/') || (remoteFilePath[i + 1] == 0))
                {
                    // directory found.
                    streamAT().print("AT+QFTPMKDIR=\"");
                    streamAT().print(fPath);
                    streamAT().println("\"");
                    waitResponse2(2000, "+QFTPMKDIR:", GSM_ERROR);
                    streamSkipUntil('\r');
                }
                fPath += remoteFilePath[i];
                i++;
            } while (remoteFilePath[i] != 0);

            // again set whole path with directories
            streamAT().write("AT+QFTPPATH=\"");
            streamAT().write(remoteFilePath);
            streamAT().println("\"");

            ret = waitResponse3("+QFTPPATH: 0" GSM_NL, GSM_ERROR);
            if (1 == ret)
            {
                return 1;
            }
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

static int32_t _FtpGetRemoteFileSize(const char *remoteFileName, const char *remoteFilePath)
{
    if (remoteFileName == NULL)
        return -1;

    if (remoteFilePath[0] != '/')
        return -1;

#if 1
    // set root path
    sendAT("+QFTPPATH=\"/\"");
    int8_t ret = waitResponse3("+QFTPPATH: 0" GSM_NL, GSM_ERROR);

    streamAT().write("AT+QFTPSIZE=\"");
    streamAT().write(remoteFilePath);
    streamAT().write(remoteFileName);
    streamAT().write("\"\r\n");
#else
    int8_t ret = _FtpSetRemotePath(remoteFilePath, false);
    streamAT().write("AT+QFTPSIZE=\"");
    streamAT().write(remoteFileName);
    streamAT().write("\"\r\n");
#endif

    ret = waitResponse2(10000, "+QFTPSIZE:", GSM_ERROR);
    if (1 == ret)
    {
        long size = streamGetLongIntBefore('\r');
        streamSkipUntil('\n'); // read out remaining

        if (size > 0)
        {
            return size;
        }

        // otherwise error code received
    }

    return 0;
}

/**
 * @fn static void FtpSmChangeState(FTP_SM curState)
 * @brief This function changes the ftp process state
 * @param FTP_SM curState the state to process
 * @remark
 */
static void FtpSmChangeState(FTP_SM curState)
{
    _eFtpSmPrevState = _eFtpSmState;
    _eFtpSmState = curState;
}

/**
 * @fn static void FtpSmDelay(uint32_t delayTicks, FTP_SM nextState)
 * @brief This function suspends current task for given ticks
 *        move to another state after given ticks
 * @param uint32_t delayTicks ticks to suspend
 * @param FTP_SM nextState state to process after delay
 * @remark
 */
static void FtpSmDelay(uint32_t delayTicks, FTP_SM nextState)
{
    _genericDelayTick = delayTicks;
    _localGenericTick = millis();
    FtpSmChangeState(FTP_SM_GENERIC_DELAY);
    if (FTP_SM_NULL != nextState)
        _eFtpSmPrevState = nextState;
}

/**
 * @fn void QuectelFtpClientSm(void)
 * @brief This function is handling ftp process.this function is required to be called frequently
 * @remark
 */
void QuectelFtpClientSm(void)
{
    static uint32_t _generalTimeoutTick = 0;
    static bool _ftpConnectionOpened = false;

    switch (_eFtpSmState)
    {
    case FTP_SM_GENERIC_DELAY:
        if ((millis() - _localGenericTick) > _genericDelayTick)
        {
            FtpSmChangeState(_eFtpSmPrevState);
            break;
        }
        break;

    case FTP_SM_INTIATED:
        _mdmFtpStatus = FTP_STATUS_BUSY;
        _generalTimeoutTick = millis();
        FtpSmChangeState(FTP_SM_CHECK_CONNECTION);
        break;

    case FTP_SM_CHECK_CONNECTION: // check if server connection is already made
        if (IsATavailable())
        {
            int8_t ret = _FtpGetConnectionStatus();
            if (ret > 0)
            {
                if ((ret == FTP_CNX_IDLE) || (ret == FTP_CNX_CLOSED))
                {
                    // ftp connection is closed. prepare to open it
                    _generalTimeoutTick = millis();
                    FtpSmChangeState(FTP_SM_CONFIGURE_USER_CREDINTIAL);
                }
                else if ((ret == FTP_CNX_OPENED))
                {
                    // ftp connection is still open.

                    /* 
                       ToDo: Done
                       then check the same server is opened by command AT+QFTPOPEN?
                       if not then closed it and open desired one
                    */

                    if (!_FtpVerifyServerHost(_ftpParam.ftpServerHost, _ftpParam.ftpServerPort))
                    {
                        /* 
                           opened connection and the current session's server host & port are different.
                           close the connection
                         */
                        // _FtpCloseConnection();  // closing handled in FTP_SM_CLEAN_UP state.

                        // _mdmFtpStatus = FTP_STATUS_FAILED;
                        _mdmFtpLastError = OPEN_FTP_CONNECTION_FAIL;
                        FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                        break;
                    }

                    // opened connection and the current session's server host & port are same.
                    _generalTimeoutTick = millis();
                    FtpSmChangeState(FTP_SM_CHECK_REMOTE_FILE_SIZE);
                }
                else //if ((ret == FTP_CNX_CLOSING) || (ret == FTP_CNX_OPENING))
                {
                    //check again after some time
                    FtpSmDelay(3000, FTP_SM_NULL);
                }
            }
            else
            {
                // _mdmFtpStatus = FTP_STATUS_FAILED;
                _mdmFtpLastError = OPEN_FTP_CONNECTION_FAIL;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            }
        }
        if ((millis() - _generalTimeoutTick) > 10000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_CONFIGURE_USER_CREDINTIAL:
        if (IsATavailable())
        {
            int8_t ret = _FtpSetUserCredential(_ftpParam.ftpUserName, _ftpParam.ftpUserPass);
            if (1 == ret)
            {
                _generalTimeoutTick = millis();
                FtpSmChangeState(FTP_SM_OPEN_CONNECTION);
                break;
            }
            else
            {
                // _mdmFtpStatus = FTP_STATUS_FAILED;
                _mdmFtpLastError = OPEN_FTP_CONNECTION_FAIL;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            }
        }
        if ((millis() - _generalTimeoutTick) > 5000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_OPEN_CONNECTION:
        if (IsATavailable())
        {
            if (_FtpOpenConnection(_ftpParam.ftpServerHost, _ftpParam.ftpServerPort))
            {
                _generalTimeoutTick = millis();
                FtpSmChangeState(FTP_SM_WAIT_FOR_CONNECTION);
            }
            else
            {
                // _mdmFtpStatus = FTP_STATUS_FAILED;
                _mdmFtpLastError = OPEN_FTP_CONNECTION_FAIL;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            }
        }
        if ((millis() - _generalTimeoutTick) > 5000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_WAIT_FOR_CONNECTION:
        if (waitResponse3("+QFTPOPEN:0" GSM_NL) == 1)
        {
            _generalTimeoutTick = millis();
            FtpSmChangeState(FTP_SM_CHECK_REMOTE_FILE_SIZE);
            break;
        }

        if (IsATavailable())
        {
            int8_t ret = _FtpGetConnectionStatus();
            if (ret > 0)
            {
                if ((ret == FTP_CNX_OPENED))
                {
                    _generalTimeoutTick = millis();
                    FtpSmChangeState(FTP_SM_CHECK_REMOTE_FILE_SIZE);
                    break;
                }
                else
                {
                    // ftp connection is still open.
                    // check again
                    FtpSmDelay(3000, FTP_SM_NULL);
                }
            }
        }
        if ((millis() - _generalTimeoutTick) > 15000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_CHECK_REMOTE_FILE_SIZE:
        // if we are here means ftp server connection is opened.
        _ftpConnectionOpened = true;
        if ((_ftpParam.ftpCnxType == FTP_DOWNLOAD_FILE) && (_ftpParam.localFileStorage == QUEC_FILE_MEM_RAM))
        {
            //ToDo:
            // if RAM is selected as local storage, check remote file size before downloading.
            // otherwise generate error and close this session.
            _filesize = _FtpGetRemoteFileSize(_ftpParam.ftpRemoteFileName, _ftpParam.ftpRemoteFilePath);
            if ((_filesize > 0) && (_filesize <= _availableSize))
            {
                FtpSmChangeState(FTP_SM_SET_LOCAL_FILENAME);
            }
            else
            {
                // _mdmFtpStatus = FTP_STATUS_FAILED;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                DBG("[FTP] remote file size ");
                if (_filesize > _availableSize)
                {
                    DBG("%d too big to download\n", _filesize);
                }
                else
                {
                    DBG("error %d\n", _filesize);
                }
            }
        }
        else
        {
            FtpSmChangeState(FTP_SM_SET_LOCAL_FILENAME);
        }
        break;

    case FTP_SM_SET_LOCAL_FILENAME:
        if (IsATavailable())
        {
            int8_t ret = _FtpSetLocalPath(_ftpParam.ftpLocalFileName, _ftpParam.localFileStorage);
            if (1 == ret)
            {
                FtpSmChangeState(FTP_SM_SET_REMOTE_FILEPATH);
                _generalTimeoutTick = millis();
            }
        }
        if ((millis() - _generalTimeoutTick) > 5000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_SET_REMOTE_FILEPATH:
        if (IsATavailable())
        {
            int8_t ret = _FtpSetRemotePath(_ftpParam.ftpRemoteFilePath, (_ftpParam.ftpCnxType == FTP_UPLOAD_FILE)); // _ftpParam.ftpRemoteFileName,
            if (1 == ret)
            {
                //make decision what to do on ftp connection
                if (_ftpParam.ftpCnxType == FTP_DOWNLOAD_FILE)
                {
                    streamAT().write("AT+QFTPGET=\"");
                    streamAT().write(_ftpParam.ftpRemoteFileName);
                    streamAT().write("\",");
                    streamAT().println(_filesize);

                    FtpSmDelay(5000, FTP_SM_DOWNLOAD_FILE);
                }
                else //if (_ftpParam.ftpCnxType == FTP_UPLOAD_FILE)
                {
                    streamAT().write("AT+QFTPPUT=\"");
                    streamAT().write(_ftpParam.ftpRemoteFileName);
                    streamAT().println("\",0,30");

                    FtpSmDelay(5000, FTP_SM_UPLOAD_FILE);
                }
                _generalTimeoutTick = millis();
                break;
            }
        }

        if ((millis() - _generalTimeoutTick) > 5000)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
            break;
        }
        break;

    case FTP_SM_DOWNLOAD_FILE: //  wait till download complete
        if ((millis() - _generalTimeoutTick) > 120000U)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
        }
        else
        {
            int8_t ret = waitResponse3("+QFTPGET:", "+QFTPERROR:");
            if (1 == ret)
            {
                // either downloaded size or error code received
                int32_t size = streamGetIntBefore('\r');
                if (size > 0)
                {
                    if (size != _filesize)
                    {
                        DBG("[FTP] downloaded %d out of %d\n", size, _filesize);
                        FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                    }
                    else
                    {
                        DBG("[FTP] downloaded %d bytes\n", size);
                        FtpSmChangeState(FTP_SM_CLEAN_UP_SUCCESS); //_mdmFtpStatus = FTP_STATUS_SUCCESS;
                    }
                }
                else
                {
                    DBG("[FTP] download error %d\n", size);
                    FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL); // _mdmFtpStatus = FTP_STATUS_FAILED;
                }

                // FtpSmChangeState(FTP_SM_CLEAN_UP);
                DBG("[FTP] download complete\n");

                // testing status
                _FtpGetConnectionStatus();
                break;
            }
            else if (2 == ret)
            {
                // ftp error
                int32_t errorcode = streamGetIntBefore('\r');
                DBG("[FTP] error %d\n", errorcode);

                // _mdmFtpStatus = FTP_STATUS_FAILED;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                break;
            }
            // must required to give some time to ftp processing
            FtpSmDelay(5000, FTP_SM_NULL);
        }
        break;

    case FTP_SM_UPLOAD_FILE: // wait till upload complete
        if ((millis() - _generalTimeoutTick) > 120000U)
        {
            // _mdmFtpStatus = FTP_STATUS_FAILED;
            FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
            DBG("[FTP] timeout\n");
        }
        else
        {
            int8_t ret = waitResponse3("+QFTPPUT:", "+QFTPERROR:");
            if (1 == ret)
            {
                int32_t size = streamGetIntBefore('\r');
                if (size > 0)
                {
                    if (size != _filesize)
                    {
                        FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                        DBG("[FTP] uploaded %d out of %d\n", size, _filesize);
                    }
                    else
                    {
                        DBG("[FTP] uploaded %d bytes\n", size);
                        FtpSmChangeState(FTP_SM_CLEAN_UP_SUCCESS); // _mdmFtpStatus = FTP_STATUS_SUCCESS;
                    }
                }
                else
                {
                    DBG("[FTP] upload error %d\n", size);
                    FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL); //_mdmFtpStatus = FTP_STATUS_FAILED;
                }
                // FtpSmChangeState(FTP_SM_CLEAN_UP);
                DBG("[FTP] upload complete\n");

                // testing status
                _FtpGetConnectionStatus();
                break;
            }
            else if (2 == ret)
            {
                // ftp error
                int32_t errorcode = streamGetIntBefore('\r');
                DBG("[FTP] error %d\n", errorcode);
                // _mdmFtpStatus = FTP_STATUS_FAILED;
                FtpSmChangeState(FTP_SM_CLEAN_UP_FAIL);
                break;
            }

            // must required to give some time to ftp processing
            FtpSmDelay(5000, FTP_SM_NULL);
        }
        break;

    case FTP_SM_CLEAN_UP_SUCCESS:
        ReleaseModemAccess(MDM_RUNNING_FTP);
        FtpSmChangeState(FTP_SM_IDLE);
        _mdmFtpStatus = FTP_STATUS_SUCCESS;
        waitResponse3(); // read out remaining
        break;

    case FTP_SM_CLEAN_UP_FAIL:
        ReleaseModemAccess(MDM_RUNNING_FTP);
        FtpSmChangeState(FTP_SM_IDLE);

        _mdmFtpStatus = FTP_STATUS_FAILED;

        // close the connection
        _FtpCloseConnection();
        _ftpConnectionOpened = false;
        _generalTimeoutTick = millis();

        // SetM95InCommandMode();
        // unfreezeAT();
        // streamAT().write(0x1A);
        // streamAT().flush();
        waitResponse3(); // read out remaining
        break;

    default:
        if (_ftpConnectionOpened)
        {
            if (millis() - _generalTimeoutTick > 600000)
            {
                // it is been too long that FTP is not used.
                // check connection and if opned, then close it.
                if (IsATavailable() && IsModemReady())
                {
                    _ftpConnectionOpened = false;
                    _generalTimeoutTick = millis();
                    _FtpCloseConnection();
                }
            }
        }
        break;
    }
}

/**
 * @fn FtpStatus_t FtpGetLastStatus(void)
 * @brief This function returns the ftp process current status. 
 * @return status of ftp process
 * @remark
 */
FtpStatus_t FtpGetLastStatus(void)
{
    return (_mdmFtpStatus);
}

/**
 * @fn bool IsFtpClientAvailable(void)
 * @brief This function returns the availability of ftp process
 * @return 0: not available, 1: available
 * @remark
 */
bool IsFtpClientAvailable(void)
{
    if (_eFtpSmState == FTP_SM_IDLE)
        return true;
    else
        return false;
}

/**
 * @fn int8_t FtpInitiate(const char *ftpServerHost, uint16_t ftpServerPort, const char *ftpUserName, const char *ftpPassword,
 *                        const char *localFileName, const char *remoteFileName, const char *remoteFilePath,
 *                        FtpCnxType_t ftpSessionType, FtpFileMode_t ftpFileOperation, QuecFileStorage_t localFileStorage)
 * @brief This function initiates the ftp task. It holds the information passed by calling method
 *          through out the process
 * @param const char *ftpServerHost, pointer to buffer holding ftp server IP or DNS. Max 100 characters.
 * @param uint16_t ftpServerPort, ftp server port. deault 21.
 * @param const char *ftpUserName, ftp server username. Max 30 characters.
 * @param const char *ftpPassword, ftp server password. Max 30 characters.
 * @param FtpCnxType_t ftpSessionType, ftp session type upload, download etc.
 * @param const char *localFileName, local file name. must be 8.3 DOS format.
 * @param QuecFileStorage_t localFileStorage, local file storage memory. UFS, RAM or SD. Note that RAM has limited size.
 * @param const char *remoteFileName, remote server file name. Max 50 characters.
 * @param const char *remoteFilePath, remote server file path. Max 100 characters. if root folder then filepath must be "/".
 *                                    directories must be separated by '/'. Also must start and end with '/'. 
 *                                    Example: /Main-Directory/Sub-dir/sub-sub-dir/
 * @param FtpFileMode_t ftpFileOperation, if file existed then Append or Overwritten.
 * @return int8_t, -ve number: validation fail, 0: not available, 1: success
 * @remark It is required to hold all the buffers passed to this function.
 */
int8_t FtpInitiate(const char *ftpServerHost, uint16_t ftpServerPort, const char *ftpUserName, const char *ftpPassword,
                   const char *localFileName, const char *remoteFileName, const char *remoteFilePath,
                   FtpCnxType_t ftpSessionType, FtpFileMode_t ftpFileOperation, QuecFileStorage_t localFileStorage)
{
    // check gprs connection
    if (!IsGprsConnected())
    {
        return 0;
    }

    // check modem availability
    if (!IsModemReady())
        return 0;

    // check ftp client availaibility
    if (_eFtpSmState != FTP_SM_IDLE)
    {
        return 0;
    }

    // validate parameters
    if (ftpServerHost == NULL)
        return -1;

    if (ftpServerPort == 0)
        return -2;

    if (localFileName == NULL)
        return -3;

    if (remoteFileName == NULL)
        return -4;

    if (remoteFilePath[0] != '/')
        return -5;

    // ToDo:
    // if download session is requested, then
    // get local area starage max. available size in bytes and hold it.
    // delete similar name local file from storage area.
    if (ftpSessionType == FTP_DOWNLOAD_FILE)
    {
        // delete similar name file from local storage area if present
        QuecFileDelete(localFileName, QUEC_FILE_MEM_RAM);

        QuecFileStorageDetails_t details;
        int8_t ret = QuecFileGetStorageSize(localFileStorage, &details);
        if (1 == ret)
        {
            _availableSize = details.freeSize;
            // DBG("[FTP] storage available=%ld, total=%ld, allowable=%ld\n", details.freeSize, details.maxSize, details.allowableSize);
        }
        else
        {
            // fail to read size. we are still allowing session to be carried out.
            // assigning default size.
            _availableSize = FTP_MAX_DWNLD_FILE_SIZE;
        }
    }
    else
    {
        // if upload session, get local file size.
        int8_t ret = QuecFileSize(localFileStorage, localFileName, (uint32_t *)&_filesize);
        if (1 == ret)
        {
            DBG("[FTP] local file size %d to upload\n", _filesize);
        }
        else
        {
            // we can not allow to upload file if we do not know file size.
            // we are examining local file size and uploaded file size ahead.
            DBG("[FTP] local file size not found. aborting\n");
            return 0;
        }
    }

    // get modem access
    if (!GetModemAccess(MDM_RUNNING_FTP))
        return 0;

    // we are good to go now

    _ftpParam.ftpServerHost = ftpServerHost;
    _ftpParam.ftpServerPort = ftpServerPort;
    _ftpParam.ftpUserName = ftpUserName;
    _ftpParam.ftpUserPass = ftpPassword;
    _ftpParam.ftpLocalFileName = localFileName;
    _ftpParam.localFileStorage = localFileStorage;
    _ftpParam.ftpRemoteFileName = remoteFileName;
    _ftpParam.ftpRemoteFilePath = remoteFilePath;
    _ftpParam.ftpFileMode = ftpFileOperation;
    _ftpParam.ftpCnxType = ftpSessionType;

    _mdmFtpStatus = FTP_STATUS_INTIATED;
    _eFtpSmState = FTP_SM_INTIATED;

    return 1;
}

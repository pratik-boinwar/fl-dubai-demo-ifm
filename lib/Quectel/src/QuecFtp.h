/**
 * @file       QuecFtp.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECFTP_H
#define QUECFTP_H

#include "QuecFile.h"

#define FTP_SERVERADD_LEN 100 // max. hostname can be 100
#define FTP_FILEPATH_LEN 100  // max. filepath can be 100
#define FTP_USERNAME_LEN 30   // max. username can be 30
#define FTP_PASSWORD_LEN 30   // max. password can be 30
#define FTP_FILENAME_LEN 50   // max. filename can be 50
#define FTP_SERVICE_PORT 21   // deafult port

/**
 * FtpStatus_t states current status of ftp process
 */
typedef enum _FtpStatus
{
    FTP_STATUS_IDLE,
    FTP_STATUS_INTIATED,
    FTP_STATUS_BUSY,
    FTP_STATUS_SUCCESS,
    FTP_STATUS_FAILED
} FtpStatus_t;

/**
 * FtpCnxType_t states app purpose of calling ftp process
 */
typedef enum _FtpCnxType
{
    FTP_UPLOAD_FILE = 0, // uploading file from storage area
    FTP_DOWNLOAD_FILE,   // downloading file to storage area
    // FTP_UPLOAD_ARRAY,    // uploading user provided array
    // FTP_DOWNLOAD_ARRAY,  // downloading to user provided array
    // FTP_UPLOAD_USER,     // uploading user data stream through callback
    // FTP_DOWNLOAD_USER    // downloading data stream and given to user through callback
} FtpCnxType_t;

/**
 * FtpFileMode_t states file operation mode
 */
typedef enum _FtpFileMode
{
    FTP_FILE_APPEND = 0,
    FTP_FILE_OVERWRITE
} FtpFileMode_t;

/**
 * FtpLastError_t states ftp process error
 */
typedef enum _FtpLastError
{
    FTP_ERROR_UNKNOWN = 0,
    FTP_SUCESS,
    OPEN_FTP_CONNECTION_FAIL,
    SET_FTP_LOCAL_STORAGE_FILENAME_FAIL,
    SET_FTP_REMOTE_FILEPATH_FAIL,
    DOWNLOAD_FTP_FILE_COMMAND_FAIL,
    FTP_RECEIVED_ERROR_WHILE_DOWNLOAD,
    FTP_TIMEOUT_WHILE_DOWNLOAD,
    FTP_TIMEOUT_WHILE_CONNECT,
    FTP_RECEIVED_ERROR_WHILE_CONNECT,
    FTP_CLOSE_FAIL,
    FTP_CLOSE_TIMEOUT,
    FTP_CLOSE_COMMAND_ERROR,
    FTP_GPRS_DISCONNECT
} FtpLastError_t;

/**
 * @fn FtpStatus_t FtpGetLastStatus(void)
 * @brief This function returns the ftp process current status. 
 * @return status of ftp process
 * @remark
 */
FtpStatus_t FtpGetLastStatus(void);

/**
 * @fn bool IsFtpClientAvailable(void)
 * @brief This function returns the availability of ftp process
 * @return 0: not available, 1: available
 * @remark
 */
bool IsFtpClientAvailable(void);

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
int8_t FtpInitiate(const char *ftpServerHost = NULL, uint16_t ftpServerPort = 21, const char *ftpUserName = NULL,
                   const char *ftpPassword = NULL, const char *localFileName = NULL, const char *remoteFileName = NULL,
                   const char *remoteFilePath = "/", FtpCnxType_t ftpSessionType = FTP_UPLOAD_FILE,
                   FtpFileMode_t ftpFileOperation = FTP_FILE_OVERWRITE, QuecFileStorage_t localFileStorage = QUEC_FILE_MEM_RAM);

/* IMPORTANT NOTE:
--> This library is designed to upload local file to remote ftp server and download remote file to local storage.
--> The library does not dealing in buffers for uploading or downloading.
    that means, in one go either file is uploaded or downloaded.
    can not be uploading and downloading in chunk of bytes.
--> For uploading anything, User first need to create local file and put the desired data into it
    and then start FTP Upload procedures.
--> For downloading anything, User first start FTP Download procedures and if succed
    then need to collect data from local file into which downloaded data is copied.
--> Refer "QuecFile" for file operations.
--> Note that any file in RAM storage can not be greater that 100KB Size.



*/

#endif // QUECFTP_H
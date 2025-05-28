/**
 * @file       QuecFile.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECFILE_H
#define QUECFILE_H

// note that all modules are not supporting all types of storage area.
// M95 supporting only RAM memory.
// M66 firmware M66FBRxxx supporting only RAM. This is non OpenCPU firmware.
// M66 firmware M66FARxxx supporting UFS, RAM and SD. This is OpenCPU firmware.
typedef enum _QuecFileStorage
{
    QUEC_FILE_MEM_UFS = 0, // internal NVRAM
    QUEC_FILE_MEM_RAM,     // internal RAM. Max size is depends on available free bytes in storage.
    QUEC_FILE_MEM_SD       // external attached SD Card. Note that only "Picture" directory is considered in it.
} QuecFileStorage_t;

/**
 * QuecFileStorageDetails_t, file storage area details.
 */
typedef struct _QuecFileStorageDetails
{
    long freeSize;      // free data size
    long maxSize;       // total data size
    long allowableSize; // max allowable size. applicable for RAM storage only
} QuecFileStorageDetails_t;

/**
 * QuecFileMode_t, file operation mode.
 */
typedef enum _QuecFileMode
{
    QUEC_FILE_MODE_0 = 0, // File will be opened in read-write mode. Creating new file if doesn't exist.
    QUEC_FILE_MODE_1,     // If file exist it will be re-created and old data will be cleared.
    QUEC_FILE_MODE_2      // If file exist it will be open in Read Only mode.
} QuecFileMode_t;

/**
 * QuecFileHandler, represents handle to the 'opened' file. All further operations over file will required this handle.
 */
typedef unsigned long QuecFileHandler;

/**
 * @fn int8_t QuecFileOpen(const char *fileName, QuecFileHandler *pFileHandle, QuecFileStorage_t memory, QuecFileMode_t fileMode)
 * @brief This function opens file in given storage with selected mode
 * @param char *fileName, file name to be open
 * @param QuecFileHandler *pFileHandle, file handler for further file operations
 * @param QuecFileStorage_t memory, storage area
 * @param QuecFileMode_t fileMode, file opening mode
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileOpen(const char *fileName, QuecFileHandler *pFileHandle, QuecFileStorage_t memory, QuecFileMode_t fileMode);

/**
 * @fn int8_t QuecFileClose(QuecFileHandler fileHandler)
 * @brief This function closes file from given storage
 * @param QuecFileHandler fileHandler, file handler to be close
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileClose(QuecFileHandler fileHandler);

/**
 * @fn int32_t QuecFileRead(QuecFileHandler fileHandler, char *fileData, uint16_t fileDataSize)
 * @brief This function reads data from file which handler is provided.
 * @param QuecFileHandler fileHandler, file handler to be read.
 * @param char *fileData, pointer to buffer in which data will be copied.
 * @param uint16_t fileDataSize, size of buffer in which data will be copied.
 * @return -ve number: validation fail, +ve number: success, read bytes
 * @remark
 */
int32_t QuecFileRead(QuecFileHandler fileHandler, char *fileData, uint16_t fileDataSize);

/**
 * @fn int32_t QuecFileWrite(QuecFileHandler fileHandler, const char *fileData, uint16_t fileDataSize)
 * @brief This function write data to file which handler is provided.
 * @param QuecFileHandler fileHandler, file handler to write.
 * @param const char *fileData, pointer to buffer holding data to be write.
 * @param uint16_t fileDataSize, size of data to be write.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int32_t QuecFileWrite(QuecFileHandler fileHandler, const char *fileData, uint16_t fileDataSize);

typedef enum _FileSeekPos
{
    FILE_SEEK_BEGIN = 0, // move file pointer to start position of file
    FILE_SEEK_CURRENT,   // move file pointer to current position of file
    FILE_SEEK_END        // move file pointer to end position of file
} QuecFileSeekPos_t;

/**
 * @fn int8_t QuecFileSeek(QuecFileHandler fileHandler, uint8_t offset, QuecFileSeekPos_t position)
 * @brief This function moves file pointer according to given parameters.
 * @param QuecFileHandler fileHandler, file handler to write.
 * @param uint8_t offset, Number of bytes to move the file pointer from position.
 * @param QuecFileSeekPos_t position, pointer movement mode.
 * @return -ve number: validation fail, 0: finsih with fail, 1: success.
 * @remark
 */
int8_t QuecFileSeek(QuecFileHandler fileHandler, uint8_t offset, QuecFileSeekPos_t position);

/**
 * @fn int8_t QuecFileDelete(const char *fileName, QuecFileStorage_t memory)
 * @brief This function deletes file from given storage
 * @param char *fileName file name to be delete
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileDelete(const char *fileName, QuecFileStorage_t memory);

/**
 * @fn int8_t QuecFileDeleteAll(QuecFileStorage_t memory)
 * @brief This function deletes all file from given storage
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileDeleteAll(QuecFileStorage_t memory);

/**
 * @fn int8_t QuecFileList(QuecFileStorage_t memory)
 * @brief This function lists all file from given storage
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileList(QuecFileStorage_t memory);

/**
 * @fn int8_t QuecFileSize(QuecFileStorage_t memory, const char *fileName, uint32_t *fileSize)
 * @brief This function returns size of file from given storage
 * @param QuecFileStorage_t memory, storage area
 * @param const char *fileName, name of the file in storage
 * @param uint32_t *fileSize, pointer to integer to assign file size
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileSize(QuecFileStorage_t memory, const char *fileName, uint32_t *fileSize);

/**
 * @fn int8_t QuecFileGetStorageSize(QuecFileStorage_t memory, QuecFileStorageDetails_t *details)
 * @brief This function gives details about free, total and max_allowable sizes of given storage.
 * @param QuecFileStorage_t memory, storage area
 * @param QuecFileStorageDetails_t *details, pointer to structure to assign storage size values.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileGetStorageSize(QuecFileStorage_t memory, QuecFileStorageDetails_t *details);

/**
 * @fn int8_t QuecFileMove(const char *srcFileName, const char *dstFileName, QuecFileStorage_t srcMemory, QuecFileStorage_t dstMemory,
 *                         bool delSrcFile, bool overwriteDstFile)
 * @brief This function copies file from one location to another location. 
 * @param const char *srcFileName, source file name to be move.
 * @param const char *dstFileName, destination file name of moved file.
 * @param QuecFileStorage_t srcMemory, source file memory area
 * @param QuecFileStorage_t dstMemory, destination file memory area
 * @param bool delSrcFile, true/false deleting source file after success.
 * @param bool overwriteDstFile, true/false overwrite destination file if already exists.
 * @return -ve number: validation fail, 0: finsih with fail, 1: success.
 * @remark can not move UFS or SD files to RAM.
 */
int8_t QuecFileMove(const char *srcFileName, const char *dstFileName, QuecFileStorage_t srcMemory, QuecFileStorage_t dstMemory,
                    bool delSrcFile, bool overwriteDstFile);

/**
 * @fn int8_t QuecFileMoveAll(QuecFileStorage_t srcMemory, QuecFileStorage_t dstMemory, bool delSrcFile, bool overwriteDstFile);
 * @brief This function copies all files from one location to another location. 
 * @param QuecFileStorage_t srcMemory, source files memory area
 * @param QuecFileStorage_t dstMemory, destination files memory area
 * @param bool delSrcFile, true/false deleting source files after success.
 * @param bool overwriteDstFile, true/false overwrite destination files if already exists.
 * @return -ve number: validation fail, 0: finsih with fail, 1: success.
 * @remark can not move UFS or SD files to RAM.
 */
int8_t QuecFileMoveAll(QuecFileStorage_t srcMemory, QuecFileStorage_t dstMemory, bool delSrcFile, bool overwriteDstFile);

typedef enum _FileUploadEvent
{
    FILE_UPLD_START = 0, // when desired file open and ready to write.
    FILE_UPLD_NEW_DATA,  // when new chunk of data ready to write. user need to load 'data' variables and return with size of data.
    FILE_UPLD_END        // when upload operation finish either with success or fail.
} FileUploadEvent_t;

typedef uint16_t (*QuecFileUploadCallBack)(FileUploadEvent_t fileEvent, char *fileData, uint32_t fileDataLen);

/**
 * @fn int8_t QuecFileUpload(const char *fileName, const char *fileData, uint32_t fileDataSize, QuecFileStorage_t memory,
 *                           QuecFileUploadCallBack cb)
 * @brief This function uploads data to file in given storage. This function creates new file always.
 * @param const char *fileName, uploading file name 
 * @param const char *fileData, pointer to buffer holding file data.
 * @param uint32_t fileDataSize, file data size. max size is depends on available free bytes in RAM storage.
 * @param QuecFileStorage_t memory, storage area
 * @param QuecFileUploadCallBack cb, callback function to received new data to upload.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileUpload(const char *fileName, const char *fileData, uint32_t fileDataSize, QuecFileStorage_t memory, QuecFileUploadCallBack cb);

typedef enum _FileDownloadEvent
{
    FILE_DWNLD_START = 0, // when desired file open and ready to read. it provides total no. of bytes to be downalod.
    FILE_DWNLD_NEW_DATA,  // when new chunk of data read from file and ready to deliver. it provides data and its size in bytes.
    FILE_DWNLD_END        // when download operation finish either with success or fail. it provides remaining bytes.
} FileDownloadEvent_t;

typedef void (*QuecFileDownloadCallBack)(FileDownloadEvent_t fileEvent, const uint8_t *data, uint32_t dataLen);

/**
 * @fn int8_t QuecFileDownload(const char *fileName, const uint8_t *fileData, uint32_t fileDataSize, QuecFileStorage_t memory,
 *                             QuecFileDownloadCallBack cb)
 * @brief This function downloads data from the file from the given storage. This function reads already existed file.
 * @param const char *fileName, downloading file name 
 * @param const uint8_t *fileData, pointer to buffer holding file data.
 * @param uint32_t fileDataSize, buffer size to hold read file data. tested with 5KByte buffer.
 * @param QuecFileStorage_t memory, storage area where file is present.
 * @param QuecFileDownloadCallBack cb, callback function to return downloaded chunk of data.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark two methods. 
 * 1) collect data in buffer pointed by 'fileData' upto 'fileDataSize' at once 
 *    and terminate the operation even if there are more data present in the file.
 * 2) collect data in chunk (i.e. in buffer pointed by 'fileData' upto 'fileDataSize') 
 *    and forward to user through callback. refer FileDownloadEvent_t events as well.
 * In both cases, user has to provide a adequete size buffer to carried out download operation.
 */
int8_t QuecFileDownload(const char *fileName, uint8_t *fileData, uint32_t fileDataSize, QuecFileStorage_t memory,
                        QuecFileDownloadCallBack cb);

#endif // QUECFILE_H

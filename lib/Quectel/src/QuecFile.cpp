/**
 * @file       QuecFile.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecFile.h"

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
int8_t QuecFileOpen(const char *fileName, QuecFileHandler *pFileHandle, QuecFileStorage_t memory, QuecFileMode_t fileMode)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    if (fileName == NULL)
        return -3;

    streamAT().write("AT+QFOPEN=\"");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("RAM:");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().write("SD:");
        break;

    default:
        return -3;
        break;
    }

    streamAT().write(fileName);
    streamAT().write("\"\r\n");

    int8_t ret = waitResponse3("+QFOPEN: ", GSM_ERROR);

    if (ret == 1)
    {
        QuecFileHandler fileHandle = (QuecFileHandler)streamGetLongIntBefore('\r');
        *pFileHandle = fileHandle;
        waitResponse3(GSM_OK); // read out remaining
        // DBG("[FILE] handler %ld, %ld\n", fileHandler, *pFileHandle);
        return 1;
    }

    return 0;
}

/**
 * @fn int8_t QuecFileClose(QuecFileHandler fileHandler)
 * @brief This function closes file from given storage
 * @param QuecFileHandler fileHandler, file handler to be close
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileClose(QuecFileHandler fileHandler)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().write("AT+QFCLOSE=");
    streamAT().println(fileHandler);

    //int8_t ret = waitResponse3("+QFCLOSE: ", GSM_ERROR);
    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);

    if (1 == ret)
    {
        return 1;
    }

    return 0;
}

/**
 * @fn int32_t QuecFileRead(QuecFileHandler fileHandler, char *fileData, uint16_t fileDataSize)
 * @brief This function reads data from file which handler is provided.
 * @param QuecFileHandler fileHandler, file handler to be read.
 * @param char *fileData, pointer to buffer in which data will be copied.
 * @param uint16_t fileDataSize, size of buffer in which data will be copied.
 * @return -ve number: validation fail, +ve number: success, read bytes
 * @remark
 */
int32_t QuecFileRead(QuecFileHandler fileHandler, uint8_t *fileData, uint16_t fileDataSize)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    // required buffer to copy read data
    if (fileData == NULL)
        return -3;

    // buffer size can not be 0
    if (fileDataSize == 0)
        return -3;

    *fileData = 0;

    streamAT().print("AT+QFREAD=");
    streamAT().print((long)fileHandler);
    streamAT().write(',');
    streamAT().println(fileDataSize);

    // CONNECT <number of bytes>
    int8_t ret = waitResponse3("CONNECT ", GSM_ERROR);
    if (ret != 1)
    {
        // if we missed CONNECT, safe side send escape sequence
        streamAT().write("+++");
        return -4;
    }

    int32_t receiving = streamGetIntBefore('\r');
    if (receiving <= 0)
    {
        // reach EOF. nothing to read.
        return 0;
    }
    else
    {
        streamSkipUntil('\n'); // skip till '\n'
        int32_t _readBytes = streamAT().readBytes(fileData, receiving);
        streamSkipUntil('\n'); // read out remaining "OK"
        // DBG("[FILE] read exp %d, rx %d\n", receiving, _readBytes);
        return _readBytes;
    }
}

/**
 * @fn int32_t QuecFileWrite(QuecFileHandler fileHandler, const char *fileData, uint16_t fileDataSize)
 * @brief This function write data to file which handler is provided.
 * @param QuecFileHandler fileHandler, file handler to write.
 * @param const char *fileData, pointer to buffer holding data to be write.
 * @param uint16_t fileDataSize, size of data to be write.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int32_t QuecFileWrite(QuecFileHandler fileHandler, const char *fileData, uint16_t fileDataSize)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    // required buffer to copy read data
    if (fileData == NULL)
        return -3;

    // buffer size can not be 0
    if (fileDataSize == 0)
        return -3;

    streamAT().print("AT+QFWRITE=");
    streamAT().print(fileHandler);
    streamAT().write(',');
    streamAT().println(fileDataSize);

    int8_t ret = waitResponse2(2000, "CONNECT", GSM_ERROR);
    if (1 != ret)
    {
        // if we missed CONNECT, safe side send escape sequence
        streamAT().write("+++");
        return 0;
    }
    streamSkipUntil('\n');

    //ToDo:
    // better not to write all bytes once. there might be Serial buffer overflow condition occurs.
    // we do not know Serial Tx Buffer size to hold the data.
    // or how the Serial stream is going to process such big ammount of data stream.
    // I suggest, to write small chunk of bytes at a time till all bytes are written.
    uint32_t writtenBytes = 0;

#if 0
    uint32_t bytes = 0;
    const char *data;
    data = fileData;

    uint32_t timeout = millis();
    do
    {
        bytes = (fileDataSize - writtenBytes) > 512 ? 512 : (fileDataSize - writtenBytes);
        bytes = streamAT().write(data, bytes);
        if (bytes == 0)
        {
            //something wrong.
            streamAT().write("+++");
            return 0;
        }
        delay(50);              // keep silence, to give some time to module write operation
        writtenBytes += bytes; // update written bytes count
        data += bytes;         // move pointer
    } while ((writtenBytes < fileDataSize) && (millis() - timeout < 10000));

    if ((millis() - timeout >= 10000))
    {
        DBG("[FILE] write timeout\n");
    }
#else
    writtenBytes = streamAT().write(fileData, fileDataSize);
#endif

    if (writtenBytes != fileDataSize)
    {
        // if mismatched and if module expecting more bytes. safe side send escape sequence
        streamAT().write("+++");
        DBG("[FILE] written mismatched %d != %d\n", fileDataSize, writtenBytes);
        return 0;
    }

    //+QFWRITE: <written length>,<total_length>
    ret = waitResponse2(2000, "+QFWRITE: ", GSM_ERROR);
    writtenBytes = streamGetIntBefore(',');
    waitResponse3(GSM_OK); // read out remaining

    if (writtenBytes == fileDataSize)
    {
        return 1;
    }

    return 0;
}

/**
 * @fn int8_t QuecFileSeek(QuecFileHandler fileHandler, uint8_t offset, QuecFileSeekPos_t position)
 * @brief This function moves file pointer according to given parameters.
 * @param QuecFileHandler fileHandler, file handler to write.
 * @param uint8_t offset, Number of bytes to move the file pointer from position.
 * @param QuecFileSeekPos_t position, pointer movement mode.
 * @return -ve number: validation fail, 0: finsih with fail, 1: success.
 * @remark
 */
int8_t QuecFileSeek(QuecFileHandler fileHandler, uint8_t offset, QuecFileSeekPos_t position)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().print("AT+QFSEEK=");
    streamAT().print(fileHandler);
    streamAT().write(',');
    streamAT().print(offset);
    streamAT().write(',');
    streamAT().println(position);

    int8_t ret = waitResponse2(2000, GSM_OK, GSM_ERROR);

    if (1 == ret)
        return 1;

    return 0;
}

/**
 * @fn int8_t QuecFileDelete(const char *fileName, QuecFileStorage_t memory)
 * @brief This function deletes file from given storage
 * @param char *fileName file name to be delete
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileDelete(const char *fileName, QuecFileStorage_t memory)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    if (fileName == NULL)
        return -3;

    streamAT().write("AT+QFDEL=\"");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("RAM:");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().write("SD:");
        break;

    default:
        return -3;
        break;
    }

    streamAT().write(fileName);
    streamAT().write("\"\r\n");

    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);

    if (ret == 1)
        return 1;

    return 0;
}

/**
 * @fn int8_t QuecFileDeleteAll(QuecFileStorage_t memory)
 * @brief This function deletes all file from given storage
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileDeleteAll(QuecFileStorage_t memory)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().write("AT+QFDEL=\"");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        streamAT().write("*");
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("RAM:*");
        break;

    case QUEC_FILE_MEM_SD: //Only delete files in the Picture directory of SD card, do not
                           //delete any directory or any file in the other directories
        streamAT().write("SD:*");
        break;

    default:
        return -3;
        break;
    }

    streamAT().write("\"\r\n");

    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);

    if (ret == 1)
        return 1;

    return 0;
}

/**
 * @fn int8_t QuecFileList(QuecFileStorage_t memory)
 * @brief This function lists all files from given storage
 * @param QuecFileStorage_t memory, storage area
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileList(QuecFileStorage_t memory)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().write("AT+QFLST");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("=\"RAM:*\"");
        break;

    case QUEC_FILE_MEM_SD: //Only list files in the "Picture" directory of SD card, do not
                           //list any directory and any file in the other directories.
        streamAT().write("=\"SD:*\"");
        break;

    default:
        return -3;
        break;
    }

    streamAT().write("\r\n");

    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);

    if (ret == 1)
        return 1;

    return 0;
}

/**
 * @fn int8_t QuecFileSize(QuecFileStorage_t memory, const char *fileName, uint32_t *fileSize)
 * @brief This function returns size of file from given storage
 * @param QuecFileStorage_t memory, storage area
 * @param const char *fileName, name of the file in storage
 * @param uint32_t *fileSize, pointer to integer to assign file size
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileSize(QuecFileStorage_t memory, const char *fileName, uint32_t *fileSize)
{
    /*
    AT+QFLST="RAM:test.txt"

    +QFLST: "RAM:test.txt",1379,10240

    OK
    */
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().write("AT+QFLST=\"");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("RAM:");
        break;

    case QUEC_FILE_MEM_SD: //Only list files in the "Picture" directory of SD card, do not
                           //list any directory and any file in the other directories.
        streamAT().write("SD:");
        break;

    default:
        return -3;
        break;
    }

    streamAT().print(fileName);
    streamAT().write("\"\r\n");

    if (fileSize == NULL)
    {
        waitResponse3(GSM_OK); // read remaining
        return 0;
    }

    int8_t ret = waitResponse3("+QFLST:", GSM_OK, GSM_ERROR);

    if (ret == 1)
    {
        char fname[20] = {0};
        // streamSkipUntil('\"');
        ret = streamAT().readBytesUntil(',', fname, sizeof(fname)); // '\"'
        fname[ret] = 0;

        if (0 != strstr(fname, fileName))
        {
            // streamSkipUntil(',');
            uint32_t size = streamGetIntBefore(',');
            *fileSize = size;

            waitResponse3();
            return 1;
        }
    }
    waitResponse3();
    return 0;
}

/**
 * @fn int8_t QuecFileGetStorageSize(QuecFileStorage_t memory, QuecFileStorageDetails_t *details)
 * @brief This function gives details about free, total and max_allowable sizes of given storage.
 * @param QuecFileStorage_t memory, storage area
 * @param QuecFileStorageDetails_t *details, pointer to structure to assign storage size values.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
int8_t QuecFileGetStorageSize(QuecFileStorage_t memory, QuecFileStorageDetails_t *details)
{
    /*
    AT+QFLDS="RAM"

    +QFLDS: 340168,647668,340168

    OK
    */

    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    streamAT().write("AT+QFLDS");

    switch (memory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().write("=\"RAM\"");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().write("=\"SD\"");
        break;

    default:
        return -3;
        break;
    }

    streamAT().write("\r\n");

    if (details == NULL)
    {
        waitResponse3(GSM_OK); // read remaining
        return 0;
    }

    int8_t ret = waitResponse3("+QFLDS: ", GSM_ERROR);

    details->freeSize = 0;
    details->maxSize = 0;
    details->allowableSize = 0;

    if (ret == 1)
    {
        details->freeSize = streamGetLongIntBefore(',');
        details->maxSize = streamGetLongIntBefore(',');
        if (memory == QUEC_FILE_MEM_RAM)
            details->allowableSize = streamGetLongIntBefore('\r'); // available only for RAM storage.

        waitResponse3(GSM_OK); // read remaining

        DBG("[FILE] storage available=%ld, total=%ld, allowable=%ld\n", details->freeSize, details->maxSize, details->allowableSize);
        return 1;
    }

    return 0;
}

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
                    bool delSrcFile, bool overwriteDstFile)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    // source file name required
    if (srcFileName == NULL)
        return -3;

    // destination file name required
    if (dstFileName == NULL)
        return -3;

    // unkown storage area
    if ((srcMemory > 2) || (dstMemory > 2))
        return -3;

    // can not move UFS or SD files to RAM
    if (dstMemory == QUEC_FILE_MEM_RAM)
    {
        if ((srcMemory == QUEC_FILE_MEM_UFS) || (srcMemory == QUEC_FILE_MEM_SD))
        {
            return -4;
        }
    }

    streamAT().print("AT+QFMOV=\"");

    switch (srcMemory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().print("RAM:");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().print("SD:");
        break;

    default:
        break;
    }

    streamAT().print(srcFileName);
    streamAT().write("\",\"");

    switch (dstMemory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().print("RAM:");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().print("SD:");
        break;

    default:
        break;
    }

    streamAT().print(dstFileName);
    streamAT().printf("\",%d,%d\r\n", delSrcFile, overwriteDstFile);

    int8_t ret = waitResponse2(2000, GSM_OK, GSM_ERROR);

    if (1 == ret)
        return 1;

    return 0;
}

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
int8_t QuecFileMoveAll(QuecFileStorage_t srcMemory, QuecFileStorage_t dstMemory, bool delSrcFile, bool overwriteDstFile)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    // unkown storage area
    if ((srcMemory > 2) || (dstMemory > 2))
        return -3;

    // can not move UFS or SD files to RAM
    if (dstMemory == QUEC_FILE_MEM_RAM)
    {
        if ((srcMemory == QUEC_FILE_MEM_UFS) || (srcMemory == QUEC_FILE_MEM_SD))
        {
            return -4;
        }
    }

    streamAT().print("AT+QFMOV=\"");

    switch (srcMemory)
    {
    case QUEC_FILE_MEM_UFS:
        streamAT().print("*");
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().print("RAM:*");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().print("SD:*");
        break;

    default:
        break;
    }

    streamAT().write("\",\"");

    switch (dstMemory)
    {
    case QUEC_FILE_MEM_UFS:
        break;

    case QUEC_FILE_MEM_RAM:
        streamAT().print("RAM:");
        break;

    case QUEC_FILE_MEM_SD:
        streamAT().print("SD:");
        break;

    default:
        break;
    }

    streamAT().printf("\",%d,%d\r\n", delSrcFile, overwriteDstFile);

    int8_t ret = waitResponse2(2000, GSM_OK, GSM_ERROR);

    if (1 == ret)
        return 1;

    return 0;
}

/**
 * @fn int8_t QuecFileUpload(const char *fileName, const char *fileData, uint32_t fileDataSize, QuecFileStorage_t memory,
 *                           QuecFileUploadCallBack cb)
 * @brief This function uploads data to file in given storage. This function creates new file always.
 * @param const char *fileName, uploading file name 
 * @param const char *fileData, pointer to buffer holding file data.
 * @param uint32_t fileDataSize, file data size. max size is 10240 bytes for RAM storage.
 * @param QuecFileStorage_t memory, storage area
 * @param QuecFileUploadCallBack cb, callback function to received new data to upload.
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark deleting existing file if any.
 */
int8_t QuecFileUpload(const char *fileName, const char *fileData, uint32_t fileDataSize, QuecFileStorage_t memory,
                      QuecFileUploadCallBack cb)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    // file name must be provided
    if (fileName == NULL)
        return -3;

    // if callback is not provided then following parameters required
    if (cb == NULL)
    {
        // if no data is provided
        if (fileData == NULL)
            return -3;

        // data length can not be 0
        if (fileDataSize == 0)
            return -3;

        // in case of RAM storage Max 10240 bytes can be write
        if ((memory == QUEC_FILE_MEM_RAM) && (fileDataSize > 10240L))
            return -3;
    }

    // delete existing file if any
    QuecFileDelete(fileName, memory);

    // Note: we are not using direct "AT+QFUPL" command to upload file
    // as data can be huge and Serial out buffer will not be as big as file size.
    // Also, we probably could not handle/keep all file data in one buffer.
    // better we will go traditionally, opening the file, receive data in chunks using callback
    // and then writing out.

    QuecFileHandler fileHandler = 0;
    int8_t ret = QuecFileOpen(fileName, &fileHandler, memory, QUEC_FILE_MODE_0);

    if (ret != 1)
    {
        // file open error
        return 0;
    }

    if (cb != NULL)
    {
        cb(FILE_UPLD_START, NULL, 0);
    }

    uint32_t timeout = millis();
    int32_t bytesToWrite;
    char buffer[1025] = {0}; // size of buffer must be choose according to Serial out buffer size.
    do
    {
        if (cb != NULL)
        {
            // if callback provided, then collect writing data
            // if callback function return 0 means there are no data for write.
            // terminate the uploading procedure.
            buffer[0] = 0;
            bytesToWrite = 0; // clear previous count
            bytesToWrite = cb(FILE_UPLD_NEW_DATA, buffer, sizeof(buffer) - 1);
            // DBG("[FILE] write size %d\n", bytesToWrite);
            if (bytesToWrite > sizeof(buffer))
            {
                // sorry!!, you are not following rules.
                DBG("[FILE] max upld write size can be %d at a time\n", sizeof(buffer));
                bytesToWrite = -1; // dummy, to notify failed session
                break;
            }
            if (buffer[0] == 0)
            {
                break;
            }
            if (bytesToWrite > 0)
            {
                bytesToWrite = QuecFileWrite(fileHandler, buffer, bytesToWrite);
            }
        }
        else
        {
            // user provided data in single buffer.
            bytesToWrite = QuecFileWrite(fileHandler, fileData, fileDataSize);
        }

        if (bytesToWrite < 0)
        {
            // writing fail
            DBG("[FILE] upld write < 0, %d\n", bytesToWrite);
            break;
        }

        if (cb == NULL)
        {
            // all bytes are write
            DBG("[FILE] uplded all bytes\n");
            break;
        }

    } while ((millis() - timeout) < 15000);

    QuecFileClose(fileHandler);

    if (cb != NULL)
    {
        cb(FILE_UPLD_END, NULL, 0);
    }

    if (bytesToWrite < 0)
    {
        // writing fail
        return 0;
    }

    return 1;
}

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
                        QuecFileDownloadCallBack cb)
{
    if (!IsATavailable())
        return -1;

    if (!IsModemReady())
        return -2;

    if (fileName == NULL)
        return -3;

    // required buffer to copy read data
    if (fileData == NULL)
        return -3;

    // buffer size can not be 0
    if (fileDataSize == 0)
        return -3;

    size_t fSize = fileDataSize;

    // if callback is provided then following parameters required
    if (cb != NULL)
    {
        int8_t ret = QuecFileSize(memory, fileName, (uint32_t *)&fSize);
        if (1 == ret)
        {
            DBG("[FILE] local file size %d downloading\n", fSize);
        }
        else
        {
            DBG("[FILE] local file size not found. aborting\n");
            return 0;
        }
    }

    // Note: we are not using direct "AT+QFDWL" command to download file
    // as data can be huge and module will try to dump all the data in one attempt.
    // we probably could not handle/receive all these data in one buffer.
    // better we will go traditionally, opening the file and reading out data in chunks
    // and send back to user through call back events.
    QuecFileHandler fileHandler = 0;
    int8_t ret = QuecFileOpen(fileName, &fileHandler, memory, QUEC_FILE_MODE_2);

    if (ret != 1)
    {
        // file might not present
        DBG("[FILE] download open error %d\n", ret);
        return 0;
    }

    if (cb != NULL)
    {
        cb(FILE_DWNLD_START, NULL, fSize);
    }

    //ToDo:
    // safe side always seek file from start

    uint32_t timeout = millis();
    int32_t bytesRead;

    do
    {
        bytesRead = 0;
        bytesRead = QuecFileRead(fileHandler, fileData, fileDataSize);
        if (bytesRead < 0)
        {
            // reading fail
            DBG("[FILE] download read error %d\n", bytesRead);
            break;
        }

        if (bytesRead > 0)
        {
            fSize -= bytesRead;

            if ((cb != NULL))
            {
                timeout = millis();
                cb(FILE_DWNLD_NEW_DATA, fileData, bytesRead);
            }

            if (fSize <= 0)
            {
                // all bytes are read
                QuecFileClose(fileHandler);
                DBG("[FILE] all bytes dowloaded\n");

                if (cb != NULL)
                {
                    cb(FILE_DWNLD_END, NULL, fSize);
                }
                return 1;
            }
        }

        GSM_YIELD();

    } while ((millis() - timeout) < 15000);

    QuecFileClose(fileHandler);

    if (cb != NULL)
    {
        cb(FILE_DWNLD_END, NULL, fSize);
    }

    if ((millis() - timeout) > 15000)
    {
        DBG("[FILE] download timeout\n");
    }

    return 0;
}

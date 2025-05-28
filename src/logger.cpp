#include "logger.h"
#include <Arduino.h>
#include <fs.h>
#include <SPIFFS.h>

static const char *_filePath = "/httplog.db";

/***************************************************************************
 * SPIFFS open function needs path and mode:
 * mode = "r", "w", "a", "r+", "w+", "a+" (text file) or "rb", "wb", "ab", "rb+", "wb+", "ab+" (binary)
 * where r = read, w = write, a = append
 * + means file is open for update (read and write)
 * b means the file os open for binary operations
 *
 * Returns 0 if OK else:
 * -1 = No SPIFFS file system
 * -2 = File does not exist
 * -3 = File too short
 * -4 = Checksum does not compare
 ***************************************************************************/

bool LoggerInit(void)
{
    if (!SPIFFS.begin())
    {
        logDBG(Serial.print("SPIFFS Mount Failed\r\n"););
        return false;
    }

    if (!SPIFFS.exists(_filePath))
    {
        File _file = SPIFFS.open(_filePath, FILE_WRITE);
        if (!_file)
        {
            logDBG(Serial.print("File not opened\r\n"););
            _file.close();
            return false;
        }

        LOG_INFO logInfo;
        logInfo.current = 0;
        logInfo.start = 0;
        logInfo.end = 0;

        uint32_t size = sizeof(logInfo.array);
        uint32_t numOfBytesWritten = _file.write(logInfo.array, size);
        if (numOfBytesWritten != size)
        {
            logDBG(Serial.print("File write insufficient bytes\r\n"););
            _file.close();
            return false;
        }
        _file.close();
    }
    return true;
}

unsigned int LoggerPush(TRANSACTION *pTransaction)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead, numOfBytesWritten;

    _file = SPIFFS.open(_filePath, "r+"); // for read write both, use r+
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    // Get log details
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }

    size = sizeof(logInfo.array);
    numOfBytesRead = _file.read(logInfo.array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    if (0 == logInfo.current)
    {
        logInfo.start = 0;
        logInfo.end = 0;
    }

    // Move to end of the entry to append data
    size = sizeof(pTransaction->array);
    if (!_file.seek(sizeof(logInfo.array) + (unsigned long)logInfo.end * size, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    _file.write(pTransaction->array, size);

    logInfo.end = (logInfo.end + 1) % MAX_TRANSACTIONS;
    if (logInfo.current < MAX_TRANSACTIONS)
    {
        logInfo.current++;
    }
    else
    {
        // Overwriting the oldest. Move start to next-oldest
        logInfo.start = (logInfo.start + 1) % MAX_TRANSACTIONS;
    }

    // Move to start of log file and update log info
    size = sizeof(logInfo.array);
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesWritten = _file.write(logInfo.array, size);
    if (numOfBytesWritten != size)
    {
        logDBG(Serial.print("File write insufficient bytes\r\n"););
        _file.close();
        return false;
    }
    _file.close();
    return true;
}

bool LoggerFlush(void)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead, numOfBytesWritten;

    _file = SPIFFS.open(_filePath, FILE_WRITE);
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    logInfo.current = 0;
    logInfo.start = 0;
    logInfo.end = 0;

    // Move to start of log file and update log info
    size = sizeof(logInfo.array);
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesWritten = _file.write(logInfo.array, size);
    if (numOfBytesWritten != size)
    {
        logDBG(Serial.print("File write insufficient bytes\r\n"););
        _file.close();
        return false;
    }
    _file.close();
    return true;
}

bool LoggerIsPacketAvailable(void)
{
    uint32_t logcount = 0;
    LoggerGetCount(&logcount);
    if (logcount > 0)
    {
        return true;
    }

    return false;
}

unsigned int LoggerGetPacket(TRANSACTION *pTransaction)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead, numOfBytesWritten;

    _file = SPIFFS.open(_filePath, "r+");
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    // Get log details
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    size = sizeof(logInfo.array);
    numOfBytesRead = _file.read(logInfo.array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    if (!logInfo.current)
    {
        logDBG(Serial.print("No logs\r\n"););
        _file.close();
        return false;
    }

    // Move to start of the entry to read data
    size = sizeof(pTransaction->array);
    if (!_file.seek(sizeof(logInfo.array) + (unsigned long)logInfo.start * size, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesRead = _file.read(pTransaction->array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    logInfo.start = (logInfo.start + 1) % MAX_TRANSACTIONS;
    logInfo.current--;

    // Move to start of log file and update log info
    size = sizeof(logInfo.array);
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesWritten = _file.write(logInfo.array, size);
    if (numOfBytesWritten != size)
    {
        logDBG(Serial.print("File write insufficient bytes\r\n"););
        _file.close();
        return false;
    }
    _file.close();
    return true;
}

unsigned int LoggerGetPacketByLogNumber(TRANSACTION *pTransaction, uint32_t logNum)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead;

    _file = SPIFFS.open(_filePath, "r+");
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    // Get log details
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    size = sizeof(logInfo.array);
    numOfBytesRead = _file.read(logInfo.array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    logInfo.start = (logInfo.start + logNum) % MAX_TRANSACTIONS;

    if (!logInfo.current)
    {
        logDBG(Serial.print("No logs\r\n"););
        _file.close();
        return false;
    }

    if ((logInfo.current) <= logNum)
    {
        logDBG(Serial.print("Log number out of range\r\n"););
        _file.close();
        return false;
    }

    // Move the transaction to be popped
    // read out transaction
    size = sizeof(pTransaction->array);
    if (!_file.seek(sizeof(logInfo.array) + (unsigned long)logInfo.start * size, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesRead = _file.read(pTransaction->array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }
    _file.close();
    return true;
}

bool LoggerDeletePacket(uint16_t numOfLogs)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead, numOfBytesWritten;

    _file = SPIFFS.open(_filePath, "r+");
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    // Get log details
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    size = sizeof(logInfo.array);
    numOfBytesRead = _file.read(logInfo.array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    if (0 == logInfo.current)
    {
        logInfo.start = 0;
        logInfo.end = 0;
    }

    if (!logInfo.current)
    {
        logDBG(Serial.print("No logs\r\n"););
        _file.close();
        return false;
    }

    if ((logInfo.current) < numOfLogs)
    {
        logDBG(Serial.print("Log number out of range\r\n"););
        _file.close();
        return false;
    }

    logInfo.start = (logInfo.start + numOfLogs) % MAX_TRANSACTIONS;
    logInfo.current -= numOfLogs;

    // Move to start of log file and update log info
    size = sizeof(logInfo.array);
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    numOfBytesWritten = _file.write(logInfo.array, size);
    if (numOfBytesWritten != size)
    {
        logDBG(Serial.print("File write insufficient bytes\r\n"););
        _file.close();
        return false;
    }
    _file.close();
    return true;
}

unsigned int LoggerGetCount(uint32_t *pTotalLogs)
{
    LOG_INFO logInfo;
    File _file;
    uint32_t size, numOfBytesRead;

    _file = SPIFFS.open(_filePath, FILE_READ);
    if (!_file)
    {
        // _errorCode = EFSDB_ERR_FILE_NOT_OPENED;
        logDBG(Serial.print("File not opened\r\n"););
        _file.close();
        return false;
    }

    // Get log details
    if (!_file.seek(0, SeekSet))
    {
        logDBG(Serial.print("File seek failed\r\n"););
        _file.close();
        return false;
    }
    size = sizeof(logInfo.array);
    numOfBytesRead = _file.read(logInfo.array, size);
    if (numOfBytesRead != size)
    {
        logDBG(Serial.print("File read insufficient bytes\r\n"););
        _file.close();
        return false;
    }

    *pTotalLogs = logInfo.current;
    _file.close();
    return true;
}

#ifdef LOGGER_TEST
TRANSACTION trans;
unsigned int numLog = 0;
unsigned int logcount = 0;

void PrintMenu(void)
{
    Serial.println("===Menu===");
    Serial.println("1: create record");
    Serial.println("2: Read particular record");
    Serial.println("3: read first record and delete it");
    Serial.println("4: clear all record");
    Serial.println("5: total unread records");
    Serial.println("6: total records details");
    Serial.println("7: total records details reverse manner");
    Serial.println("8: delete records");
    Serial.println("M: Menu");
}

// call this fucntion in main loop to test logger
void LoggerTest(void)
{
    // Logger test
    TRANSACTION logPacket;
    uint32_t lastMillis = 0;
    static bool executeOnlyOnce = true;

    if (true == executeOnlyOnce)
    {
        Serial.println("\n\nLogger test...\n");
        Serial.print("pushing logs started at: ");
        Serial.println(millis());
        lastMillis = millis();
        for (int i = 0; i < 20; i++)
        {
            logPacket.logNo = i;
            logPacket.logDT = i;
            logPacket.logPV = i;

            if (LoggerPush(&logPacket))
            {
                // Serial.println("log pushed");
            }
            else
            {
                Serial.println("log push error");
            }
        }
        Serial.print("pushing logs ended at: ");
        Serial.println(millis());
        Serial.print("Total time(sec): ");
        Serial.println((millis() - lastMillis) / 1000.0);

        unsigned int logcount;
        LoggerGetCount(&logcount);
        Serial.print("gettig logs started at: ");
        Serial.println(millis());
        lastMillis = millis();
        for (int i = 0; i < logcount; i++)
        {
            if (LoggerGetPacketByLogNumber(&logPacket, i))
            {
                // Serial.print(logPacket.logNo);
                // Serial.print(',');
                // Serial.print(logPacket.logDT);
                // Serial.print(',');
                // Serial.print(logPacket.logPV);
                // Serial.print('\n');

                if (logPacket.logNo != i)
                {
                    Serial.print("log read logNo error at pos: ");
                    Serial.println(i);
                }
                else if (logPacket.logDT != i)
                {
                    Serial.print("log read logDT error at pos: ");
                    Serial.println(i);
                }
                else if (logPacket.logPV != i)
                {
                    Serial.print("log read logPV error at pos: ");
                    Serial.println(i);
                }
            }
            else
            {
                Serial.println("log read error");
            }
        }
        Serial.print("gettig logs ended at: ");
        Serial.println(millis());
        Serial.print("Total time(sec): ");
        Serial.println((millis() - lastMillis) / 1000.0);
        executeOnlyOnce = false;
    }

    if (Serial.available())
    {
        char ch = Serial.read();
        switch (ch)
        {
        case '1': // create a log
            numLog++;
            trans.logNo = numLog;
            trans.logDT = numLog;
            trans.logPV = numLog;

            if (LoggerPush(&trans))
            {
                Serial.println("log pushed");
            }
            else
            {
                Serial.println("log push error");
            }
            break;

        case '2': // read a specific log
            Serial.println("\nEnter record number to be read: ");
            while (!Serial.available())
            {
                ;
            }
            if (Serial.available())
            {
                ch = Serial.read();
            }

            if (LoggerGetPacketByLogNumber(&trans, ch - '0'))
            {
                Serial.println("\nlog read");
                Serial.print(trans.logNo);
                Serial.print(',');
                Serial.print(trans.logDT);
                Serial.print(',');
                Serial.print(trans.logPV);
                Serial.print('\n');
            }
            else
            {
                Serial.println("log read error");
            }
            break;

        case '3': // read and delete first log
            if (LoggerGetPacket(&trans))
            {
                Serial.println("\nlog poped");
                Serial.print(trans.logNo);
                Serial.print(',');
                Serial.print(trans.logDT);
                Serial.print(',');
                Serial.print(trans.logPV);
                Serial.print('\n');
            }
            else
            {
                Serial.println("log pop error");
            }
            break;

        case '4': // clear all logs
            numLog = 0;
            if (LoggerFlush())
            {
                Serial.println("log clear");
            }
            else
            {
                Serial.println("log clear error");
            }
            break;

        case '5': // get number of logs
            LoggerGetCount(&logcount);
            Serial.print("Total logs: ");
            Serial.println(logcount);
            break;

        case '6':
            LoggerGetCount(&logcount);
            for (int i = 0; i < logcount; i++)
            {
                if (LoggerGetPacketByLogNumber(&trans, i))
                {
                    Serial.print(trans.logNo);
                    Serial.print(',');
                    Serial.print(trans.logDT);
                    Serial.print(',');
                    Serial.print(trans.logPV);
                    Serial.print('\n');
                }
                else
                {
                    Serial.println("log read error");
                }
            }
            break;

        case '7':
            LoggerGetCount(&logcount);
            while (logcount--)
            {
                if (LoggerGetPacketByLogNumber(&trans, logcount))
                {
                    Serial.print(trans.logNo);
                    Serial.print(',');
                    Serial.print(trans.logDT);
                    Serial.print(',');
                    Serial.print(trans.logPV);
                    Serial.print('\n');
                }
                else
                {
                    Serial.println("log read error");
                }
            }
            break;

        case '8':
            Serial.println("\nEnter number of logs to be deleted: ");
            while (!Serial.available())
            {
                ;
            }
            if (Serial.available())
            {
                ch = Serial.read();
            }

            if (LoggerDeletePacket(ch - '0'))
            {
                Serial.println("log deleted");
            }
            else
            {
                Serial.println("log delete error");
            }
            break;

        default:
            PrintMenu();
            break;
        }
    }
}
#endif
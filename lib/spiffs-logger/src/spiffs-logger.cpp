#include "spiffs-logger.h"

#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif

#define LOGDBG(...) Serial.printf(__VA_ARGS__)

Logger::Logger(String file) : logFile(file),
                              fsInit(false),
                              totalFileSize(102400){};

Logger::~Logger() {}

bool Logger::begin(void)
{
    if (!SPIFFS.begin())
    {
        if (!SPIFFS.begin(true))
        {
            LOGDBG("[LOGGER] file system not initialized\n");
            fsInit = false;
            return false;
        }
    }

    fsInit = true;

    // validation
    if (SPIFFS.exists(logFile))
    {
        String tempFile = logFile + ".tmp";
        if (SPIFFS.exists(tempFile))
        {
            // Ohh! this shuld not happen.
            // while removing any log, tempfile not being re-writed or renamed.
            // the previous remove process was not completed.
            // this file size must be less than actual/real logfile.
            LOGDBG("[LOGGER] tmp found, error 1\n");
            SPIFFS.remove(tempFile);
        }
        else
        {
            ; // looks all good then.
        }
    }
    else
    {
        // actual/real file is not present.
        // check its copy present or not.
        String tempFile = logFile + ".tmp";
        if (SPIFFS.exists(tempFile))
        {
            // yes, there is a copy.
            // may be while last removing session file got deleted.
            // and tempfile not get renamed.
            SPIFFS.rename(tempFile, logFile);
            LOGDBG("[LOGGER] tmp found, error 2\n");
        }
        else
        {
            ; // both files are not present. either user has changed logfile name or first time logger is being used.
        }
    }

    // be sure that an empty file exist.
    if (!SPIFFS.exists(logFile))
    {
        File file = SPIFFS.open(logFile, "w");
        if (file)
        {
            file.close();
        }
    }

    return true;
}

bool Logger::apend(String logPacket)
{
    if (!fsInit)
    {
        LOGDBG("[LOGGER] FS not Init\n");
        return false;
    }

    // be sure that an empty file exist.
    if (!SPIFFS.exists(logFile))
    {
        File file = SPIFFS.open(logFile, "w");
        if (file)
        {
            file.close();
        }
    }

    uint32_t sizeNow = logPacket.length() + 2; // CR+LF appending at last

    File file = SPIFFS.open(logFile, "a");
    if (file)
    {
        // file opened.

        // check file size not exceed with new messsage.
        if (sizeNow + file.size() >= totalFileSize)
        {
            LOGDBG("[LOG] file full, delete some log\n");
            file.close();
            return false;
        }

        file.println(logPacket);
        file.close();
        return true;
    }
    else
    {
        LOGDBG("[LOGGER] file not opened\n");
    }

    return false;
}

bool Logger::read(String &log)
{
    if (!fsInit)
    {
        LOGDBG("[LOGGER] FS not Init\n");
        return false;
    }

    if (!SPIFFS.exists(logFile))
        return false;

    File file = SPIFFS.open(logFile, "r");

    if (file)
    {
        log = ""; // empty the string
        log = file.readStringUntil('\r');
        file.close();
        if (log.length() > 0)
        {
            return true;
        }
        // LOGDBG("[LOGGER] no record found\n");
    }
    else
    {
        LOGDBG("[LOGGER] file not opened\n");
    }

    return false;
}

bool Logger::remove(uint32_t numOfLogs)
{
    if (!fsInit)
    {
        LOGDBG("[LOGGER] FS not Init\n");
        return false;
    }

    if (numOfLogs == 0)
        return false;

    if (!SPIFFS.exists(logFile))
        return false;

    String tempFile = logFile + ".tmp";

    File rfile = SPIFFS.open(logFile, "r");
    if (rfile)
    {
        if (rfile.size() < 2)
        {
            // nothing to delete.
            rfile.close();
            return false;
        }

        File wfile = SPIFFS.open(tempFile, "w");
        if (wfile)
        {
            //check file size

            // skip lines upto numOfLogs
            uint32_t i = 0;
            do
            {
                rfile.readStringUntil('\n');
                i++;
            } while (i < numOfLogs);

            while (rfile.available())
            {
                String line = rfile.readStringUntil('\n');
                line = line.substring(0, line.length() - 1);
                wfile.println(line);
            }

            // close read file and delete it.
            rfile.close();
            SPIFFS.remove(logFile);

            // close write file and rename it to log filename.
            wfile.close();
            SPIFFS.rename(tempFile, logFile);

            return true;
        }
        else
        {
            // write file not opened.

            rfile.close();
        }
    }
    else
    {
        LOGDBG("[LOGGER] file not opened\n");
    }

    return false;
}

void Logger::reset(void)
{
    SPIFFS.remove(logFile);

    // be sure that an empty file exist.
    if (!SPIFFS.exists(logFile))
    {
        File file = SPIFFS.open(logFile, "w");
        if (file)
        {
            file.close();
        }
    }
}

void Logger::setLoggerSize(uint32_t fileSizeLimit)
{
    totalFileSize = fileSizeLimit;
}

uint32_t Logger::getLoggerSize()
{
    return totalFileSize;
}

uint32_t Logger::getLoggerUsedSize()
{
    uint32_t size = 0;

    if (!fsInit)
    {
        LOGDBG("[LOGGER] FS not Init\n");
        return 0;
    }

    File file = SPIFFS.open(logFile, "r");
    if (file)
    {
        size = file.size();
        file.close();
    }
    return size;
}

uint32_t Logger::getLoggerCount()
{
    if (!fsInit)
    {
        LOGDBG("[LOGGER] FS not Init\n");
        return 0;
    }

    if (!SPIFFS.exists(logFile))
        return 0;

    File rfile = SPIFFS.open(logFile, "r");
    if (rfile)
    {
        uint32_t i = 0;
        String line;

        while (rfile.available())
        {
            line = rfile.readStringUntil('\n');
            if (line.length() > 2)
            {
                line = "";
                i++;
            }
            else
            {
                break;
            }
        }

        rfile.close();

        return i;
    }
    else
    {
        LOGDBG("[LOGGER] file not opened\n");
    }

    return 0;
}

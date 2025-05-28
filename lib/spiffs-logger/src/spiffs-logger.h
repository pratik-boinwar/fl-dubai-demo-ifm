#ifndef _SPIFFS_LOGGER_H
#define _SPIFFS_LOGGER_H

#include <Arduino.h>

class Logger
{
public:
    Logger(String file);
    ~Logger();

    bool begin(void);

    bool apend(String logPacket);

    bool read(String &log);
    bool remove(uint32_t numOfLogs);
    void reset(void);

    void setLoggerSize(uint32_t fileSizeLimit);
    uint32_t getLoggerSize();
    uint32_t getLoggerUsedSize();
    uint32_t getLoggerCount();

protected:
    String logFile;

    bool fsInit;

    // total file size in bytes defined by user. default is 102400 bytes.
    uint32_t totalFileSize;
};

#endif // _SPIFFS_LOGGER_H
#ifndef OTA_H
#define OTA_H

#define ARDUINO_OTA_PORT 8266 // DO NOT CHANGE
#define OTA_DEBUG_SUPPORT 1

#define OTA_DEBUG(...) \
    {                  \
        if (OTA_DEBUG_SUPPORT) \
        {              \
            Serial.print(__VA_ARGS__); \
        }              \
    }
#define OTA_IN_PROGRESS 1
#define OTA_NOT_IN_PROGRESS 0
#define OTA_FAILED -1

void OtaFromUrl(const char *otaUrl);
void OtaInit(void);
void OtaLoop(void);
extern int OtaIsInProgress();
extern int OtaGetPercentage();
extern int OtaIsPercentChanged(int *pPercentageOfOta);

#endif // OTA_H
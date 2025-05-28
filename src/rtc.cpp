/**
 * @file rtc.cpp
 * @brief This file contains the API function for RTC.
 * @author Nivrutti Mahajan
 * @date July 2021
 */

#include "rtc.h"
#include "globals.h"
#include <TimeLib.h> // include Arduino time library
#include "user.h"
#include "QuecGsmGprs.h"

MCP7940_Class MCP7940;                 /// Create an instance of the MCP7940
char inputBuffer[SPRINTF_BUFFER_SIZE]; /// buffer to store RTC timestamp

/**
 * @fn bool RtcInit(void)
 * @brief This function Initializes RTC
 * @return false: RTC not initialized, true: RTC initialized.
 * @remark info need to call this function once on boot.
 * @example
 * bool status = RtcInit();
 */
bool RtcInit(void)
{
    if (!MCP7940.begin(SDA_PIN, SCL_PIN))
    {
        systemFlags.isRtcSuccess = false;                 // Initialize RTC communications failed
        debugPrintln(F("[RTC Log] Unable to find RTC.")); // Show error and wait
        return false;
    }
    else
    {
        systemFlags.isRtcSuccess = true;
        debugPrintln(F("[RTC Log] RTC initialized."));
    }

    if (MCP7940.getPowerFail())
    { // Check for a power failure
        debugPrintln(F("[RTC Log] Power failure mode detected!\n"));
        debugPrint(F("[RTC Log] Power failed at   "));
        DateTime now = MCP7940.getPowerDown();                     // Read when the power failed
        sprintf(inputBuffer, "....-%02d-%02d %02d:%02d:..",        // Use sprintf() to pretty print
                now.month(), now.day(), now.hour(), now.minute()); // date/time with leading zeros
        debugPrintln(inputBuffer);
        debugPrint(F("[RTC Log] Power restored at "));
        now = MCP7940.getPowerUp();                                // Read when the power restored
        sprintf(inputBuffer, "....-%02d-%02d %02d:%02d:..",        // Use sprintf() to pretty print
                now.month(), now.day(), now.hour(), now.minute()); // date/time with leading zeros
        debugPrintln(inputBuffer);
        MCP7940.clearPowerFail(); // Reset the power fail switch
    }
    else
    {
        reTry(
            {
                if (!MCP7940.deviceStatus())
                { // Turn oscillator on if necessary
                    debugPrintln(F("[RTC Log] Oscillator is off, turning it on."));
                    bool deviceStatus = MCP7940.deviceStart(); // Start oscillator and return state
                    if (!deviceStatus)
                    {                                                                         // If it didn't start
                        debugPrintln(F("[RTC Log] Oscillator did not start, trying again.")); // Show error and
                        return false;
                        delay(1000); // wait for a second
                    }                // of if-then oscillator didn't start
                    else
                    {
                        break;
                    }
                } // of while the oscillator is off
            },
            3);
        // MCP7940.adjust();  // Set to library compile Date/Time
        // debugPrintln(F("[RTC Log] Enabling battery backup mode"));
        MCP7940.setBattery(true); // enable battery backup mode
        if (!MCP7940.getBattery())
        { // Check if successful
            debugPrintln(F("[RTC Log] Couldn't set Battery Backup, is this a MCP7940N?"));
        } // if-then battery mode couldn't be set
    }     // of if-then-else we have detected a priorpower failure
    return true;
}

/**
 * @fn void GetCurDateTime(char *inputBuffer, char *date, char *time, uint8_t *day, unsigned long *secondsFrom2000)
 * @brief This function gets timestamp from RTC
 * @remark info need to call this function in loop to get current RTC date time.
 * @example
 * char timeStamp[20];
 * char rtcDate[7];
 * char rtcTime[7];
 * uint8_t currday = 0;
 * unsigned long secondsSince2000
 *
 * GetCurDateTime(timeStamp, rtcDate, rtcTime, &currday, &secondsSince2000, &unixTimeStamp);
 */
void GetCurDateTime(char *inputBuffer, char *date, char *time, uint8_t *currDay, unsigned long *secondsFrom2000, uint32_t *unixTimestamp)
{
    char dateFromRTC[15];
    static uint8_t secs; // store the seconds value
    DateTime now;
    GsmDateTime_t datetime;
    tNETWORKDT datetimeWifi;

    if (false == CheckRtcCommunicationIsFailed())
    {
        if (false == systemFlags.isRtcSuccess)
        {
            // If RTC communication recovered, sync RTC with GPS or GSM
            if (gSysParam.ap4gOrWifiEn)
            {
                systemFlags.netDateTimeFix = GetNetworkTime(&datetimeWifi);
                netDateTime.day = datetimeWifi.day;
                netDateTime.month = datetimeWifi.month;
                netDateTime.year = datetimeWifi.year + 2000;
                netDateTime.hour = datetimeWifi.hour;
                netDateTime.min = datetimeWifi.min;
                netDateTime.sec = datetimeWifi.sec;
            }
            else
            {
                systemFlags.netDateTimeFix = GetGsmTime(&datetime);
                netDateTime.day = datetime.day;
                netDateTime.month = datetime.month;
                netDateTime.year = datetime.year + 2000;
                netDateTime.hour = datetime.hour;
                netDateTime.min = datetime.minute;
                netDateTime.sec = datetime.second;
            }
            if (true == systemFlags.netDateTimeFix)
            {
                MCP7940.adjust(DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec));
                MCP7940.setBattery(true);
                debugPrintln("[RTC] Updated date time from Network!");
                debugPrintln();
            }
        }

        systemFlags.isRtcSuccess = true;
        now = MCP7940.now(); // get the current time
    }
    else
    {
        debugPrintln("[RTC] RTC communication failed!");
        systemFlags.isRtcSuccess = false;
        if (gSysParam.ap4gOrWifiEn)
        {
            systemFlags.netDateTimeFix = GetNetworkTime(&datetimeWifi);
            netDateTime.day = datetimeWifi.day;
            netDateTime.month = datetimeWifi.month;
            netDateTime.year = datetimeWifi.year + 2000;
            netDateTime.hour = datetimeWifi.hour;
            netDateTime.min = datetimeWifi.min;
            netDateTime.sec = datetimeWifi.sec;
        }
        else
        {
            systemFlags.netDateTimeFix = GetGsmTime(&datetime);
            netDateTime.day = datetime.day;
            netDateTime.month = datetime.month;
            netDateTime.year = datetime.year + 2000;
            netDateTime.hour = datetime.hour;
            netDateTime.min = datetime.minute;
            netDateTime.sec = datetime.second;
        }
        if (true == systemFlags.netDateTimeFix)
        {
            now = DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec);

            debugPrintln("[RTC] Updated date time from Network!");
            debugPrintln();
        }
    }

    // if (secs != now.second())
    { // Output if seconds have changed
        snprintf(inputBuffer, 20, "%04d-%02d-%02d %02d:%02d:%02d",
                 now.year(), now.month(), now.day(), now.hour(),
                 now.minute(), now.second()); // date/time with leading zeros
        snprintf(date, 15, "%02d-%02d-%d", now.day(), now.month(), now.year());
        snprintf(time, 15, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

        snprintf(rtcDateTime.day, sizeof(rtcDateTime.day), "%02d", now.day());
        snprintf(rtcDateTime.month, sizeof(rtcDateTime.month), "%02d", now.month());
        snprintf(rtcDateTime.year, sizeof(rtcDateTime.year), "%d", now.year());
        snprintf(rtcDateTime.hour, sizeof(rtcDateTime.hour), "%02d", now.hour());
        snprintf(rtcDateTime.min, sizeof(rtcDateTime.min), "%02d", now.minute());
        snprintf(rtcDateTime.sec, sizeof(rtcDateTime.sec), "%02d", now.second());

        // for (int i = 0; i < 6; i++)
        // {
        //     date[i] = dateFromRTC[i + 2];
        // }
        // date[6] = 0;
        *currDay = now.day();
        // *min = now.minute();
        *secondsFrom2000 = now.secondstime();
        secs = now.second(); // Set the counter variable
        *unixTimestamp = now.unixtime();
    }
}

/**
 * @fn bool CalibrateOrAdjustRTCTime(void)
 * @brief This function will update current date time of RTC.
 * @return false: date-time not updated, true: date-time updated
 * @remark info need to call this function after every 1 hour.
 * @example
 * bool status = CalibrateOrAdjustRTCTime();
 */
bool CalibrateOrAdjustRTCTime(void)
{
    static uint32_t updateTimeMillis = 0;
    GsmDateTime_t datetime;
    tNETWORKDT datetimeWifi;

    if (false == systemFlags.setDTFirstTime)
    {
        if (gSysParam.ap4gOrWifiEn)
        {
            systemFlags.netDateTimeFix = GetNetworkTime(&datetimeWifi);
            netDateTime.day = datetimeWifi.day;
            netDateTime.month = datetimeWifi.month;
            netDateTime.year = datetimeWifi.year + 2000;
            netDateTime.hour = datetimeWifi.hour;
            netDateTime.min = datetimeWifi.min;
            netDateTime.sec = datetimeWifi.sec;
        }
        else
        {
            systemFlags.netDateTimeFix = GetGsmTime(&datetime);
            netDateTime.day = datetime.day;
            netDateTime.month = datetime.month;
            netDateTime.year = datetime.year + 2000;
            netDateTime.hour = datetime.hour;
            netDateTime.min = datetime.minute;
            netDateTime.sec = datetime.second;
        }
        if (true == systemFlags.netDateTimeFix)
        {
            systemFlags.setDTFirstTime = true;
            MCP7940.adjust(DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec));
            MCP7940.setBattery(true);
            debugPrintln("[RTC] Setting datetime for first time from Network..");
            debugPrintln("[RTC] Updated date time from Network!");
            debugPrintln();
            return true;
        }
        else
        {
            // debugPrintln("[RTC] No date time received from GPS and Network");
            // debugPrintln();
            return false;
        }
    }
    // Sync RTC time with GPS or GSM after every 10 minutes
    else if (millis() - updateTimeMillis > 10 * MINUTE)
    {
        updateTimeMillis = millis();
        if (gSysParam.ap4gOrWifiEn)
        {
            systemFlags.netDateTimeFix = GetNetworkTime(&datetimeWifi);
            netDateTime.day = datetimeWifi.day;
            netDateTime.month = datetimeWifi.month;
            netDateTime.year = datetimeWifi.year + 2000;
            netDateTime.hour = datetimeWifi.hour;
            netDateTime.min = datetimeWifi.min;
            netDateTime.sec = datetimeWifi.sec;
        }
        else
        {
            systemFlags.netDateTimeFix = GetGsmTime(&datetime);
            netDateTime.day = datetime.day;
            netDateTime.month = datetime.month;
            netDateTime.year = datetime.year + 2000;
            netDateTime.hour = datetime.hour;
            netDateTime.min = datetime.minute;
            netDateTime.sec = datetime.second;
        }

        debugPrintln("[RTC] Updating RTC date time...");
        if (true == systemFlags.netDateTimeFix)
        {
            MCP7940.adjust(DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec));
            MCP7940.setBattery(true);
            debugPrintln("[RTC] Updated date time from Network!");
            debugPrintln();
            return true;
        }
        else
        {
            debugPrintln("[RTC] No date time received from GPS and Network");
            debugPrintln();
            return false;
        }
    }
    else if ((atoi(rtcDateTime.hour) > 24) || (atoi(rtcDateTime.min) > 60) || (atoi(rtcDateTime.sec) > 60))
    {
        debugPrint("[RTC] Invalid RTC date time:");
        debugPrintln(timeStamp);
        systemFlags.isRtcSuccess = false;
        debugPrintln("[RTC] Updating RTC date time...");
        if (gSysParam.ap4gOrWifiEn)
        {
            GetNetworkTime(&datetimeWifi);
            netDateTime.day = datetimeWifi.day;
            netDateTime.month = datetimeWifi.month;
            netDateTime.year = datetimeWifi.year + 2000;
            netDateTime.hour = datetimeWifi.hour;
            netDateTime.min = datetimeWifi.min;
            netDateTime.sec = datetimeWifi.sec;
        }
        else
        {
            GetGsmTime(&datetime);
            netDateTime.day = datetime.day;
            netDateTime.month = datetime.month;
            netDateTime.year = datetime.year + 2000;
            netDateTime.hour = datetime.hour;
            netDateTime.min = datetime.minute;
            netDateTime.sec = datetime.second;
        }

        systemFlags.setDTFirstTime = true;
        // PCF8523_RTC.adjust(DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec));
        MCP7940.adjust(DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec));
        MCP7940.setBattery(true);
        GetCurDateTime(timeStamp, rtcDate, rtcTime, &currday, &secondsSince2000, &unixTimeStamp);
        debugPrintln("[RTC] Updated date time from Network!");
        debugPrintln();
        return true;
    }
    else
    {
    }
    return false;
}

uint32_t GetUnixTimeFromNetwork(void)
{
    GsmDateTime_t datetime;
    tNETWORKDT datetimeWifi;

    if (gSysParam.ap4gOrWifiEn)
    {
        systemFlags.netDateTimeFix = GetNetworkTime(&datetimeWifi);
        netDateTime.day = datetimeWifi.day;
        netDateTime.month = datetimeWifi.month;
        netDateTime.year = datetimeWifi.year + 2000;
        netDateTime.hour = datetimeWifi.hour;
        netDateTime.min = datetimeWifi.min;
        netDateTime.sec = datetimeWifi.sec;
    }
    else
    {
        systemFlags.netDateTimeFix = GetGsmTime(&datetime);
        netDateTime.day = datetime.day;
        netDateTime.month = datetime.month;
        netDateTime.year = datetime.year + 2000;
        netDateTime.hour = datetime.hour;
        netDateTime.min = datetime.minute;
        netDateTime.sec = datetime.second;
    }

    if (true == systemFlags.netDateTimeFix)
    {
        DateTime now = DateTime(netDateTime.year, netDateTime.month, netDateTime.day, netDateTime.hour, netDateTime.min, netDateTime.sec);
        return now.unixtime();
    }
    return 0;
}

bool CheckRtcCommunicationIsFailed(void)
{
    Wire1.beginTransmission(MCP7940_ADDRESS); // Address the MCP7940
    if (Wire1.endTransmission() == 0)         // If there a device present
    {
        return false;
    }
    return true;
}
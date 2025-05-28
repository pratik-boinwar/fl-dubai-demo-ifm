/**
 * @file gps.cpp
 * @brief This file contains the API function for GPS.
 * @author Nivrutti Mahajan
 * @date Aug 2021
 */

#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include "gps.h"
#include "globals.h"
#include "FLToolbox.h"

#include "global-vars.h"
#include "user.h"
#include "Quectel.h"
#include "app.h"
/**
 * The TinyGPS++ object
 */
TinyGPSPlus gps;

/**
 * SoftwareSerial object
 */
SoftwareSerial gpsSerial;

static uint8_t gpsFailedToReadCount = 0;
/**
 * GPS_SM states to get data from L76 Gps module
 */
typedef enum _GpsSM
{
    GPS_SM_NULL = 0,
    GPS_SM_IDLE,
    GPS_SM_GET_GPS_DATA,
    GPS_SM_CHECK_FOR_ERRORS,
    GPS_SM_RESET_GPS,
    GPS_SM_WAIT_FOR_SOMETIME,
    GPS_SM_GET_LATLONG,
    GPS_SM_GET_DATETIME,
    GPS_SM_TAKE_REST
} GpsSM;

/// holds the copy of current millis
static uint32_t _millisToGetGpsData = 0;

/// holds the current operating state
static GpsSM _gpsState = GPS_SM_IDLE;

/**
 * @fn void GpsInit(void)
 * @brief This function is for gps initialization.
 * @remark This function shall call in setup.
 */
void GpsInit(void)
{
#ifndef ENABLE_4G
    pinMode(GPS_RESET_PIN, OUTPUT);
    gpsSerial.begin(GPS_BAUD, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif
}

#ifdef ENABLE_4G
int GpsGetLocation(void)
{
    char *token;
    char *delim = ", ";
    double latitude, longitude, lat_deg, lon_deg;
    int hour, minute, second;
    int day, month, year;
    int numOfSat;

    if (IsATavailable())
    {
        sendAT("+QGPSLOC=0");

        int8_t status = waitResponse2(3000, "+QGPSLOC: ", GSM_ERROR);
        if (status != 1)
        {
            debugPrintln("Get GPS data failed!");
            // systemFlags.gpsDateTimeFix = false;
            hbParam.gps = 1;
            hbParam.gpslock = 0;
            return status;
        }

        hbParam.gps = 1;

        // received +QMTOPEN: <connectID>,<result>
        char gpsDataBuff[100];
        String gpsStr = streamReadStringUntil('\r', gpsDataBuff);
        // Serial.print("GPSDATA: ");
        // Serial.println(gpsDataBuff);

        // Parse the string
        token = strtok(gpsDataBuff, delim);
        // Extract the time from the first token
        sscanf(token, "%2d%2d%2d.", &hour, &minute, &second);
        // Advance to the next token
        token = strtok(NULL, delim);
        // while (token != NULL) {
        if (strcmp(token + strlen(token) - 1, "N") == 0)
        {
            // Token contains latitude value
            sscanf(token, "%lf", &latitude);
        }
        token = strtok(NULL, delim);
        if (strcmp(token + strlen(token) - 1, "E") == 0)
        {
            // Token contains longitude value
            sscanf(token, "%lf", &longitude);
        }
        // token = strtok(NULL, delim);
        //}
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        token = strtok(NULL, delim);
        // Extract the time from the first token
        sscanf(token, "%2d%2d%2d.", &day, &month, &year);

        token = strtok(NULL, delim);
        numOfSat = atoi(token);

        // Convert degrees to decimal degrees
        lat_deg = floor(latitude / 100);
        lat_deg += (latitude - lat_deg * 100) / 60;

        lon_deg = floor(longitude / 100);
        lon_deg += (longitude - lon_deg * 100) / 60;

        hbParam.gpslock = 1;
        hbParam.latitute = lat_deg;
        hbParam.longitude = lon_deg;
        if (false == systemFlags.firstTimeGpsLockSetDefaultLoc)
        {
            debugPrintln("[Debug] Gps locked, Default location is updated!");
            systemFlags.firstTimeGpsLockSetDefaultLoc = true;

            Ftoa(hbParam.latitute, gSysParam.defaultLat, 6);
            Ftoa(hbParam.longitude, gSysParam.defaultLong, 6);
            if (false == AppSetConfigSysParams(&gSysParam))
            {
            }
            debugPrint("[Debug] Gps Location: ");
            debugSmt(Serial.print(hbParam.latitute, 6););
            debugPrint(F(","));
            debugSmt(Serial.println(hbParam.longitude, 6););
        }
        // gpsDateTime.day = day;
        // gpsDateTime.month = month;
        // gpsDateTime.year = year;
        // gpsDateTime.hour = hour;
        // gpsDateTime.min = minute;
        // gpsDateTime.sec = second;
        // systemFlags.gpsDateTimeFix = true;

        gpsDebug(
            // Serial.printf("\nTime (HH:MM:SS): %02d:%02d:%02d\n", hour, minute, second);
            Serial.printf("\nLatitude (Decimal Degrees): %f\n", lat_deg);
            Serial.printf("Longitude (Decimal Degrees): %f\n", lon_deg);
            //  Serial.printf("Date (DD:MM:YY): %02d:%02d:%02d\n", day, month, year);
            //  Serial.printf("Number of satelites: %d\n", numOfSat);
        );

        return 1;
    }
    return -1;
}
#endif

/**
 * @fn void smartDelay(unsigned long ms)
 * @brief This custom version of delay() ensures that the gps object is being "fed"
 * @param unsigned long ms time to wait for gps data received successfully
 */
void smartDelay(unsigned long ms)
{
    char ch;
    unsigned long start = millis();
    gpsDebug(Serial.println("GPS raw data: "););
    do
    {
        while (gpsSerial.available())
        {
            ch = gpsSerial.read();
            gpsDebug(Serial.print(ch););
            gps.encode(ch);
        }
    } while (millis() - start < ms);
    gpsDebug(Serial.println(););
}

/**
 * @fn void GpsLoop(void)
 * @brief This function gets gps data from gps.
 * @remark This function shall call as frequently as possible and outside of ISR.
 */
void GpsLoop(void)
{

#ifndef ENABLE_4G

    static uint32_t _generalTick = 0, _gpsResetMillis = 0;

    switch (_gpsState)
    {
    case GPS_SM_IDLE:

        //  wait for gps data to recieve
        //  stinterval is used to send heartbeat data. check this with vishal sir.

        if (millis() - _millisToGetGpsData > GPS_READ_TIMEOUT) // get gps data 10sec before the stinterval
        {
            _millisToGetGpsData = millis();
            _gpsState = GPS_SM_GET_GPS_DATA;
            break;
            // }
        }
        else
        {
        }
        break;

    case GPS_SM_GET_GPS_DATA: // process various logger related events.
        smartDelay(1000);
        _gpsState = GPS_SM_CHECK_FOR_ERRORS;
        break;

    case GPS_SM_CHECK_FOR_ERRORS:       // check for gps related error events.
        if (150 < gpsFailedToReadCount) // gps fix not found for longtime then reset gps. (for 5 minutes)
        {
            // systemFlags.gpsDateTimeFix = false;
            gpsFailedToReadCount = 0;
            hbParam.gps = 0;
            hbParam.gpslock = 0;
            _gpsState = GPS_SM_RESET_GPS;
            break;
        }
        // if (20 < gps.failedChecksum()) //if faildchecksum for sentences is continuously increamenting, then there is a problem
        // {
        //     debugPrintln("[GPS] Too many checksum failed in GPS data!");
        //     hbParam.gps = 0;
        //     hbParam.gpslock = 0;
        //     _gpsState = GPS_SM_RESET_GPS;
        //     break;
        // }
        // Testing overflow in SoftwareSerial is sometimes useful too.
        if (gpsSerial.overflow())
        {
            gpsSerial.flush();
        }
        if (millis() > 30000 && gps.charsProcessed() < 10)
        {
            // debugPrintln(F("\n[GPS] No GPS data received: check wiring"));
            // systemFlags.gpsDateTimeFix = false;
            hbParam.gps = 0;
            hbParam.gpslock = 0;
            _gpsState = GPS_SM_RESET_GPS;
            break;
        }
        hbParam.gps = 1;
        _gpsState = GPS_SM_GET_LATLONG;
        break;

    case GPS_SM_GET_LATLONG: // check status of http process
        // debugPrint(F("Location: "));
        if (gps.location.isValid() && gps.location.isUpdated())
        {
            // systemFlags.gpsDateTimeFix = true;
            hbParam.gpslock = 1;
            hbParam.latitute = gps.location.lat();
            hbParam.longitude = gps.location.lng();
            if (false == systemFlags.firstTimeGpsLockSetDefaultLoc)
            {
                debugPrintln("[Debug] Gps locked, Default location is updated!");
                systemFlags.firstTimeGpsLockSetDefaultLoc = true;

                Ftoa(hbParam.latitute, gSysParam.defaultLat, 6);
                Ftoa(hbParam.longitude, gSysParam.defaultLong, 6);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                debugPrint("[Debug] Gps Location: ");
                debugSmt(Serial.print(hbParam.latitute, 6););
                debugPrint(F(","));
                debugSmt(Serial.println(hbParam.longitude, 6););
            }

            // jump to GPS_SM_GET_DATETIME only after valid location received
            _gpsState = GPS_SM_GET_DATETIME;
            break;
        }
        else
        {
            hbParam.gpslock = 0;
            // set systemFlags.gpsDateTimeFix flag false because gps is not locked
            // systemFlags.gpsDateTimeFix = false;
            gpsFailedToReadCount++;
            // debugPrint(F("INVALID LOCATION"));
            // jump to GPS_SM_IDLE if location received is not valid
            _gpsState = GPS_SM_IDLE;
            break;
        }

        // jump to GPS_SM_IDLE if location received is not valid
        _gpsState = GPS_SM_IDLE;
        break;

    case GPS_SM_GET_DATETIME: // check status of http process
        // debugPrint(F("  Date/Time: "));
        if (gps.location.isValid() && gps.date.isValid() && gps.date.isUpdated())
        {
            gpsDateTime.day = gps.date.day();
            gpsDateTime.month = gps.date.month();
            gpsDateTime.year = gps.date.year();

            if ((0 == gpsDateTime.day) || (0 == gpsDateTime.month))
            {
                debugPrintln("[Debug] Gps date is not valid!");
                // jump to GPS_SM_IDLE if date received is not valid
                _gpsState = GPS_SM_IDLE;
                break;
            }
            // debugPrint(dayFromGps);
            // debugPrint(F("/"));
            // debugPrint(monthFromGps);
            // debugPrint(F("/"));
            // debugPrint(yearFromGps);
        }
        else
        {
            // debugPrint(F("INVALID DATE"));
            // jump to GPS_SM_IDLE if date received is not valid
            _gpsState = GPS_SM_IDLE;
            break;
        }

        // debugPrint(F(" "));
        if (gps.location.isValid() && gps.time.isValid() && gps.time.isUpdated())
        {
            // systemFlags.gpsDateTimeFix = true;
            gpsDateTime.hour = gps.time.hour();
            gpsDateTime.min = gps.time.minute();
            gpsDateTime.sec = gps.time.second();

            // if (hourFromGps < 10)
            //     debugPrint(F("0"));
            // debugPrint(hourFromGps);
            // debugPrint(F(":"));
            // if (minFromGps < 10)
            //     debugPrint(F("0"));
            // debugPrint(minFromGps);
            // debugPrint(F(":"));
            // if (secFromGps < 10)
            //     debugPrint(F("0"));
            // debugPrint(secFromGps);
        }
        else
        {
            // systemFlags.gpsDateTimeFix = false;
            // debugPrint(F("INVALID TIME"));
            // jump to GPS_SM_IDLE if time received is not valid
            _gpsState = GPS_SM_IDLE;
        }

        // debugPrintln();
        _gpsState = GPS_SM_IDLE;
        break;

    case GPS_SM_RESET_GPS:
        // TODO: write logic to reset gps;
        debugPrintln("[GPS] Signal not received for long time, check antenna of GPS");
        // debugPrintln("[" + String(millis()) + " GPS] Resetting GPS...");
        digitalWrite(GPS_RESET_PIN, HIGH);
        _gpsResetMillis = millis();
        _gpsState = GPS_SM_WAIT_FOR_SOMETIME;
        break;

    case GPS_SM_WAIT_FOR_SOMETIME:
        if (millis() - _gpsResetMillis > 1000)
        {
            _gpsResetMillis = millis();
            digitalWrite(GPS_RESET_PIN, LOW);
            // debugPrintln("[" + String(millis()) + " GPS] GPS reset done! going to rest for few sec\n");
            _generalTick = millis();
            _gpsState = GPS_SM_TAKE_REST;
        }
        break;

    case GPS_SM_TAKE_REST:
        if (millis() - _generalTick > 5000)
        {
            // debugPrintln("[" + String(millis()) + " GPS] Rest done.");
            _gpsState = GPS_SM_IDLE;
        }
        break;

    default:
        _gpsState = GPS_SM_IDLE;
        break;
    }
#else

#endif
}
#ifdef ENABLE_4G

#include "QuecGps.h"
#include "Quectel.h"

static GpsData_t _gpsData;

bool GpsInitialization(void)
{
    sendAT(("+QGPS=1"));
    if (1 == waitResponse3("OK"))
    {
        // GPS init successful
        DBG("[GPS] GPS init successful!");
        return true;
    }
    else
    {
        DBG("[GPS] GPS init failed!");
        return false;
    }
}

int GpsGetData(void)
{
    char *token;
    char *delim = ", ";
    double latitude, longitude, lat_deg, lon_deg;

    /* Response
    +QGPSLOC: 130912.000,1835.0376N,07344.2415E,1.1,503.9,3,000.00,0.2,0.1,150523,17
    */

    sendAT("+QGPSLOC=0");

    int8_t status = waitResponse2(3000, "+QGPSLOC: ", GSM_ERROR);
    if (status != 1)
    {
        DBG("Get GPS data failed!");
        // GPS is not lock
        _gpsData.gpsLock = 0;
        return status;
    }

    // received +QMTOPEN: <connectID>,<result>
    char gpsDataBuff[100];
    String gpsStr = streamReadStringUntil('\r', gpsDataBuff);
    // Serial.print("GPSDATA: ");
    // Serial.println(gpsDataBuff);

    // Parse the string
    token = strtok(gpsDataBuff, delim);
    // Extract the time from the first token
    sscanf(token, "%2d%2d%2d.", &_gpsData.hour, &_gpsData.minute, &_gpsData.second);
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
    sscanf(token, "%2d%2d%2d.", &_gpsData.day, &_gpsData.month, &_gpsData.year);

    token = strtok(NULL, delim);
    _gpsData.numOfSat = atoi(token);

    // Convert degrees to decimal degrees
    lat_deg = floor(latitude / 100);
    lat_deg += (latitude - lat_deg * 100) / 60;

    lon_deg = floor(longitude / 100);
    lon_deg += (longitude - lon_deg * 100) / 60;
    _gpsData.latitude = lat_deg;
    _gpsData.longitude = lon_deg;

    // gps is locked
    _gpsData.gpsLock = 1;
    printf("Time (HH:MM:SS): %02d:%02d:%02d\n", _gpsData.hour, _gpsData.minute, _gpsData.second);
    printf("Latitude (Decimal Degrees): %f\n", _gpsData.latitude);
    printf("Longitude (Decimal Degrees): %f\n", _gpsData.longitude);
    printf("Date (DD:MM:YY): %02d:%02d:%02d\n", _gpsData.day, _gpsData.month, _gpsData.year);
    printf("Number of satelites: %d\n", _gpsData.numOfSat);

    return 1;
}

bool GpsIsLock(void)
{
    return _gpsData.gpsLock;
}

double GpsGetLatitude(void)
{
    return _gpsData.latitude;
}

double GpsGetLongitude(void)
{
    return _gpsData.longitude;
}

void GpsGetDate(int32_t *day, int32_t *month, int32_t *year)
{
    *day = _gpsData.day;
    *month = _gpsData.month;
    *year = _gpsData.year;
}

void GpsGetTime(int32_t *hour, int32_t *minute, int32_t *second)
{
    *hour = _gpsData.hour;
    *minute = _gpsData.minute;
    *second = _gpsData.second;
}

int32_t GpsGetNumberOfSatelites(void)
{
    return _gpsData.numOfSat;
}

#endif
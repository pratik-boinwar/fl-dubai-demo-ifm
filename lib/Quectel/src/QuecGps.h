/**
 * @file       QuecGps.h
 * @author     Nivrutti Mahajan
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2023 Nivrutti Mahajan, All Rights Reserved to FountLab
 * @date       May 2023
 */

#ifdef ENABLE_4G

#ifndef QUECGPS_H
#define QUECGPS_H

#include <Arduino.h>

/**
 * GpsData_t, Gps lock data
 */
typedef struct
{
    bool gpsLock;
    double latitude;
    double longitude;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t day;
    int32_t month;
    int32_t year;
    int32_t numOfSat;

} GpsData_t;

bool GpsInitialization(void);
int GpsGetData(void);

bool GpsIsLock(void);
double GpsGetLatitude(void);
double GpsGetLongitude(void);
void GpsGetDate(int32_t *day, int32_t *month, int32_t *year);
void GpsGetTime(int32_t *hour, int32_t *minute, int32_t *second);
int32_t GpsGetNumberOfSatelites(void);

#endif // QUECGPS_H
#endif // ENABLE_4G
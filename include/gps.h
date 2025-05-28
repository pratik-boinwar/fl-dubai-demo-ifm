#ifndef MY_GPS_H
#define MY_GPS_H

#include "Arduino.h"

#define GPS_RX_PIN 26 /// GPS modules Tx is connected to this Rx pin of microcontroller
#define GPS_TX_PIN 25 /// GPS modules Rx is connected to this Tx pin of microcontroller
#define GPS_BAUD 9600 /// BaudRate for GPS serial initialization

#ifdef ENABLE_4G
#define GPS_PPS_LED_PIN 13
#else
#define GPS_RESET_PIN 33
#endif

#ifndef GPS_DEBUG
#define GPS_DEBUG systemFlags.gpsDebugEnableFlag
#endif

#ifndef gpsDebug
#define gpsDebug(funcall) \
    {                     \
        if (GPS_DEBUG)    \
        {                 \
            funcall       \
        }                 \
    }
#endif

/**
 * @fn void GpsInit(void)
 * @brief This function is for gps initialization.
 * @remark This function shall call in setup.
 */
void GpsInit(void);

/**
 * @fn void smartDelay(unsigned long ms)
 * @brief This custom version of delay() ensures that the gps object is being "fed"
 * @param unsigned long ms time to wait for gps data received successfully
 */
void smartDelay(unsigned long ms);

/**
 * @fn void GpsLoop(void)
 * @brief This function gets gps data from gps.
 * @remark This function shall call as frequently as possible and outside of ISR.
 */
void GpsLoop(void);

int GpsGetLocation(void);
#endif
#ifndef RTC_H
#define RTC_H

#include "MCP7940.h" /// Include the MCP7940 RTC library

#define SPRINTF_BUFFER_SIZE 32 /// Buffer size for sprintf()

extern char inputBuffer[SPRINTF_BUFFER_SIZE]; /// buffer to store RTC timestamp

/**
 * @fn bool RtcInit(void)
 * @brief This function Initializes RTC
 * @return false: RTC not initialized, true: RTC initialized.
 * @remark info need to call this function once on boot.
 * @example
 * bool status = RtcInit();
 */
bool RtcInit(void);

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
void GetCurDateTime(char *inputBuffer, char *date, char *time, uint8_t *day, unsigned long *secondsFrom2000, uint32_t *unixTimestamp);

/**
 * @fn bool CalibrateOrAdjustRTCTime(void)
 * @brief This function will update current date time of RTC.
 * @return false: date-time not updated, true: date-time updated
 * @remark info need to call this function after every 1 hour.
 * @example
 * bool status = CalibrateOrAdjustRTCTime();
 */
bool CalibrateOrAdjustRTCTime(void);
uint32_t GetUnixTimeFromNetwork(void);
bool CheckRtcCommunicationIsFailed(void);

extern MCP7940_Class MCP7940; /// making MCP7940_Class global
#endif
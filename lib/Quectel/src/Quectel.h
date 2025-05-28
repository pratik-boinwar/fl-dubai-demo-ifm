/**
 * @file       Quectel.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECTEL_H
#define QUECTEL_H

#include <Arduino.h>

#include "QuecConfig.h"

#define QUECTEL_SERIAL Serial2
#define QUECTEL_SERIAL_TX_PIN 13 // GPIO17
#define QUECTEL_SERIAL_RX_PIN 14 // GPIO16

#define QUECTEL_SERIAL_RX_SIZE (5 * 1024U) //(20 * 1024U) // in bytes
#define QUECTEL_ONOFF_PIN 33
#define QUECTEL_STATUS_PIN 34
#define QUECTEL_STATUS_LEVEL 1 // 0:module off, 1:module off

// #define QUECTEL_SIM_SELECT_PIN 32 // 0:sim_1_selected, 1:sim_2_selected

// #define QUECTEL_SIM_1_DETECT_PIN 36
// #define QUECTEL_SIM_2_DETECT_PIN 39
// #define QUECTEL_SIM_DETECT_LEVEL 0 // 0:sim_present, 1:sim_not_present

#ifdef QUECTEL_DEBUG_PORT
#define QUECTEL_DEBUG
#else
#undef QUECTEL_DEBUG
#endif

#ifdef QUECTEL_DEBUG
#define DBG(...) // QUECTEL_DEBUG_PORT.printf(__VA_ARGS__)
#else
#define DBG(...)
#endif

#ifndef GSM_YIELD_MS
#define GSM_YIELD_MS 0
#endif

#ifndef GSM_YIELD
#define GSM_YIELD()          \
    {                        \
        delay(GSM_YIELD_MS); \
    }
#endif

#define GSM_NL "\r\n"
extern const char GSM_OK[];
extern const char GSM_ERROR[];
extern const char GSM_CONNECT[];
extern const char GSM_CME_ERROR[];
extern const char GSM_CMS_ERROR[];

/**
 * @fn void sendAT(String cmd)
 * @brief This function sending AT commands to modem
 * @remark this function is can be access when IsATavailable() return true.
 */
void sendAT(String cmd);

/**
 * @fn int8_t waitResponse(uint32_t timeout_ms, String &data,
 *                         const char *r1, const char *r2,
 *                         const char *r3, const char *r4,
 *                         const char *r5)
 * @brief This function read out serial till expected strings not matched or else timeout
 * @param uint32_t timeout_ms, timeout in miliseconds
 * @param String &data, string to holds extracted serial data to match with expected string
 * @param const char *r1, pointer to first string to match.
 * @param const char *r2, pointer to first string to match.
 * @param const char *r3, pointer to first string to match.
 * @param const char *r4, pointer to first string to match.
 * @param const char *r5, pointer to first string to match.
 * @return 0: timeout, 1/2/3/4/5: index number of matching string
 * @remark
 */
int8_t waitResponse(uint32_t timeout_ms, String &data,
                    const char *r1 = (GSM_OK),
                    const char *r2 = (GSM_ERROR),
                    const char *r3 = NULL, const char *r4 = NULL,
                    const char *r5 = NULL);

/**
 * @fn int8_t waitResponse2(uint32_t timeout_ms,
 *                          const char *r1, const char *r2,
 *                          const char *r3, const char *r4,
 *                          const char *r5)
 * @brief This function read out serial till expected strings not matched or else timeout
 * @param uint32_t timeout_ms, timeout in miliseconds
 * @param const char *r1, pointer to first string to match. default is GSM_OK
 * @param const char *r2, pointer to first string to match. default is GSM_ERROR
 * @param const char *r3, pointer to first string to match. default is GSM_CME_ERROR
 * @param const char *r4, pointer to first string to match. default is GSM_CMS_ERROR
 * @param const char *r5, pointer to first string to match. NULL
 * @return 0: timeout, 1/2/3/4/5: index number of matching string
 * @remark this function calling to waitResponse() with passing 'data' string argument
 */
int8_t waitResponse2(uint32_t timeout_ms, const char *r1 = (GSM_OK),
                     const char *r2 = (GSM_ERROR),
                     const char *r3 = (GSM_CME_ERROR),
                     const char *r4 = (GSM_CMS_ERROR),
                     const char *r5 = NULL);

/**
 * @fn int8_t waitResponse3(const char *r1, const char *r2,
 *                          const char *r3, const char *r4,
 *                          const char *r5)
 * @brief This function read out serial till expected strings not matched or else timeout
 * @param const char *r1, pointer to first string to match. default is GSM_OK
 * @param const char *r2, pointer to first string to match. default is GSM_ERROR
 * @param const char *r3, pointer to first string to match. default is GSM_CME_ERROR
 * @param const char *r4, pointer to first string to match. default is GSM_CMS_ERROR
 * @param const char *r5, pointer to first string to match. NULL
 * @return 0: timeout, 1/2/3/4/5: index number of matching string
 * @remark this function calling to waitResponse2() with predefined 1000 miliscods timeout
 */
int8_t waitResponse3(const char *r1 = (GSM_OK),
                     const char *r2 = (GSM_ERROR),
                     const char *r3 = (GSM_CME_ERROR),
                     const char *r4 = (GSM_CMS_ERROR),
                     const char *r5 = NULL);

/**
 * @fn bool streamSkipUntil(const char c, const uint32_t timeout_ms)
 * @brief This function read out serial till expected character meet for specific timeout
 * @remark
 */
bool streamSkipUntil(const char c, const uint32_t timeout_ms = 1000L);

/**
 * @fn int16_t streamGetIntBefore(char lastChar)
 * @brief This function read out serial till expected character and converts result into integer value
 * @remark
 */
int32_t streamGetIntBefore(char lastChar);

/**
 * @fn uint32_t streamGetStrBefore(char lastChar)
 * @brief This function read out serial till expected character
 * @remark
 */
uint32_t streamGetStrBefore(char lastChar);

/**
 * @fn long streamGetLongIntBefore(char lastChar)
 * @brief This function read out serial till expected character and converts result into long integer value
 * @remark
 */
long streamGetLongIntBefore(char lastChar);

/**
 * @fn Stream &streamAT()
 * @brief This function gives access to stream or serial port directly
 * @remark use this wisely.
 */
Stream &streamAT();

String streamReadStringUntil(const char lastChar, char *gpsData);

int32_t getCmeError(void);
void clearCmeError(void);
bool isCmeError(void);

/**
 * CurrentOperation_t module current process state
 */
typedef enum _CurrentOperation
{
    MDM_IDLE = 0,
    MDM_INITIALIZING,
    MDM_RUNNING_FTP,
    MDM_RUNNING_SMTP,
    MDM_RUNNING_HTTP,
    MDM_SENDING_SMS,
    MDM_PUBLISHING_MQTT_MSG,
    MDM_CONNECTING_MQTT,
} CurrentOperation_t;

/**
 * SimStatus_t sim card status
 */
typedef enum _SimStatus
{
    SIM_ERROR = 0,
    SIM_READY = 1,
    SIM_LOCKED = 2,
    SIM_ANTITHEFT_LOCKED = 3,
} SimStatus_t;

/**
 * @fn bool IsModemReady(void)
 * @brief This function returns the modem availability
 * @return 0: not available, 1: available
 * @remark if returns false, do not try to access modem
 */
bool IsATavailable(void);

/**
 * @fn bool IsModemReady(void)
 * @brief This function returns the modem availability
 * @return 0: not available, 1: available
 * @remark if returns false, do not try to access modem
 */
bool IsModemReady(void);

/**
 * @fn bool GetModemAccess(CurrentOperation_t op)
 * @brief This function gives access to other processes
 * @param CurrentOperation_t op, operation which trying to get access of the modem
 * @return false: access not grated, true: access granted.
 * @remark always use this function to access modem. internal operations like http, ftp, etc are using this
 *         DO NOT FORGOT TO RELEASE THE ACCESS IF GRANTED
 */
bool GetModemAccess(CurrentOperation_t op);

/**
 * @fn bool ReleaseModemAccess(CurrentOperation_t op)
 * @brief This function releases the access which was grated previously
 * @param CurrentOperation_t op, operation which releasing the access of the modem
 * @return false: access not released because someone else tryig to release it, true: access released.
 * @remark always use this function to release modem. internal operations like http, ftp, etc are using this
 *         DO NOT FORGOT TO RELEASE THE ACCESS IF GRANTED
 */
bool ReleaseModemAccess(CurrentOperation_t op);

/**
 * @fn CurrentOperation_t GetModemCurrentOp(void)
 * @brief This function returns the modem current running operation id
 * @return CurrentOperation_t, 0: Idle, 1: some operation is carrying out
 * @remark
 */
CurrentOperation_t GetModemCurrentOp(void);

/**
 * @fn void QuectelInit(void)
 * @brief This function is entry point of all other processes initialization.
 * @remark This function shall call first than any other related processes.
 */

void QuectelInit(void);

/**
 * @fn void QuectelLoop(void)
 * @brief This function calling other processes.
 * @remark This function shall call as frequently as possible and outside of ISR.
 */
void QuectelLoop(void);

/**
 * @fn void QuectelDebug(bool enable)
 * @brief This function allows enable and disable of serial debugging of module.
 * @param bool enable, True: enable, false: disable serial dump.
 * @remark
 */
void QuectelDebug(bool enable);

/**
 * @fn bool GetModemState(void)
 * @brief This function returns the modem current state
 * @return CurrentOperation_t states which can be 0,1,2,3,4,5,6,7
 * @remark if returns current running state of modem
 */
CurrentOperation_t GetModemState(void);
#endif // QUECTEL_H

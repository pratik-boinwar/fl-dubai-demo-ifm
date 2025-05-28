/**
 * @file       Quectel.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecHttp.h"
#include "QuecMqtt.h"
#include "QuecSms.h"

#ifdef QUECTEL_DEBUG
#include <SerialDebugger.h>
SerialDebugger mSerial(QUECTEL_SERIAL, QUECTEL_DEBUG_PORT);
#else
#define mSerial QUECTEL_SERIAL
#endif

// Stream& mSerial = QUECTEL_SERIAL;

// #define GSM_NL "\r\n"
const char GSM_OK[] = "OK" GSM_NL;
const char GSM_ERROR[] = "ERROR" GSM_NL;
const char GSM_CONNECT[] = "CONNECT" GSM_NL;
const char GSM_CME_ERROR[] = GSM_NL "+CME ERROR:";
const char GSM_CMS_ERROR[] = GSM_NL "+CMS ERROR:";

static bool _accessAT = true;
// static uint8_t _noResponseCount = 0;
static uint32_t _noResponseTick = 0;

extern bool isWifiNetClientSelected;

static int32_t _cmeError = 0;

extern void QuectelGsmGprsSm(void);
extern void QuectelSslSm(void);
extern void QuectelHttpClientSm(void);
extern void QuectelMqttClienSm(void);
extern void mqttDataReceived(void);
extern void QuectelFtpClientSm(void);
extern void QuectelSmsClientSm(void);
extern void SmsReceived(SmsUrcType_t smsType);

/**
 * @fn void freezeAT(void)
 * @brief This function holds AT commands operation
 * @remark this function is used by internal operations. not for user access
 */
void freezeAT(void)
{
    _accessAT = false;
}

/**
 * @fn void unfreezeAT(void)
 * @brief This function release AT commands operation
 * @remark this function is used by internal operations. not for user access
 */
void unfreezeAT(void)
{
    _accessAT = true;
}

/**
 * @fn bool IsATavailable(void)
 * @brief This function returns the modem AT commands availability
 * @return 0: not available, 1: available
 * @remark if returns false, do not try to send AT commands.
 */
bool IsATavailable(void)
{
    return _accessAT;
}

/**
 * @fn void sendAT(String cmd)
 * @brief This function sending AT commands to modem
 * @remark this function is can be access when IsATavailable() return true.
 */
void sendAT(String cmd)
{
    if (!IsATavailable())
        return;
    if (cmd.startsWith("AT"))
    {
        ; // do nothing
    }
    else
    {
        mSerial.print("AT");
    }

    mSerial.print(cmd);
    mSerial.print("\r\n");
    mSerial.flush();

    // DBG("AT");
    // DBG("%s", cmd.c_str());
    // DBG("\r\n");
}

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
 * @return 0: timeout, 1/2/3/4/5: index number of matching string, 10: HttpGetPostCmdResp success
 * @remark
 */
int8_t waitResponse(uint32_t timeout_ms, String &data,
                    const char *r1, const char *r2,
                    const char *r3, const char *r4,
                    const char *r5)
{
    if (!IsATavailable())
        return -1;

    data.reserve(64);
    uint8_t index = 0;
    uint32_t startMillis = millis();
    bool noByteReceived = true;

    do
    {
        GSM_YIELD();
        while (mSerial.available() > 0)
        {
            GSM_YIELD();
            int8_t a = mSerial.read();
            if (a <= 0)
                continue; // Skip 0x00 bytes, just in case
            // DBG("%c", static_cast<char>(a));
            noByteReceived = false;
            data += static_cast<char>(a);
            if (r1 && data.endsWith(r1))
            {
                index = 1;
                goto finish;
            }
            else if (r2 && data.endsWith(r2))
            {
                index = 2;
                goto finish;
            }
            else if (r3 && data.endsWith(r3))
            {
                if (r3 == (GSM_CME_ERROR))
                {
                    _cmeError = streamGetIntBefore('\n'); // Read out the error
                }
                index = 3;
                goto finish;
            }
            else if (r4 && data.endsWith(r4))
            {
                if (r4 == (GSM_CMS_ERROR))
                {
                    streamSkipUntil('\n'); // Read out the error
                }
                index = 4;
                goto finish;
            }
            else if (r5 && data.endsWith(r5))
            {
                index = 5;
                goto finish;
            }
            else if (data.endsWith((GSM_NL "+QMTRECV: ")))
            {
                mqttDataReceived();
                data = "";
            }
#ifdef ENABLE_4G
            else if (data.endsWith((GSM_NL "+QHTTPGET: ")))
            {
                HttpGetPostCmdResRxd();
                data = "";
                return 10;
            }
            else if (data.endsWith((GSM_NL "+QHTTPPOST: ")))
            {
                HttpGetPostCmdResRxd();
                data = "";
                return 10;
            }
#endif
            else if (data.endsWith((GSM_NL "+CMTI: ")))
            {
                // sms received at inbox. when AT+CNMI=2,1
                SmsReceived(SMS_URC_TYPE_CMTI);
                data = "";
            }
            else if (data.endsWith((GSM_NL "+CMT: ")))
            {
                // sms received and outputing data. when AT+CNMI=2,2
                SmsReceived(SMS_URC_TYPE_CMT);
                data = "";
            }
            // we can check UNR flags here as well
        }
    } while (millis() - startMillis < timeout_ms);

finish:
    if (!index)
    {
        data.trim();
        if (data.length())
        {
            DBG("### Unhandled: %s\n", data.c_str());
        }
        data = "";
    }
    // data.replace(GSM_NL, "/");
    // DBG('<', index, '>', data);

    if (noByteReceived)
    {
        // timeout occurred. not a single byte received. no OK, no ERROR..
        // modem either in some unhandled/untracked "CONNECT" mode or stuck somewhere.
        // need to restart it. either use "count" method or "time out" method.
#if 1
        if (millis() - _noResponseTick > 300000)
        {
            ResetModem();
        }
#else
        _noResponseCount++;
        if (_noResponseCount > 5)
        {
            _noResponseCount = 0;
            ResetModem();
        }
#endif
    }
    else
    {
        // _noResponseCount = 0;
        _noResponseTick = millis();
    }

    return index;
}

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
int8_t waitResponse2(uint32_t timeout_ms, const char *r1, const char *r2,
                     const char *r3,
                     const char *r4,
                     const char *r5)
{
    String data;
    return waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
}

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
int8_t waitResponse3(const char *r1, const char *r2,
                     const char *r3, const char *r4,
                     const char *r5)
{
    return waitResponse2(1000, r1, r2, r3, r4, r5);
}

int32_t getCmeError(void)
{
    int32_t cmeError;
    cmeError = _cmeError;
    _cmeError = 0;
    return cmeError;
}

void clearCmeError(void)
{
    _cmeError = 0;
}

bool isCmeError(void)
{
    if (_cmeError)
    {
        return true;
    }
    return false;
}

/**
 * @fn int32_t streamGetIntBefore(char lastChar)
 * @brief This function read out serial till expected character and converts result into integer value
 * @remark
 */
int32_t streamGetIntBefore(char lastChar)
{
    char buf[7];
    size_t bytesRead = mSerial.readBytesUntil(
        lastChar, buf, static_cast<size_t>(7));
    // if we read 7 or more bytes, it's an overflow
    if (bytesRead && bytesRead < 7)
    {
        buf[bytesRead] = '\0';
        // DBG("%s", buf);
        int32_t res = atoi(buf);
        return res;
    }

    return -9999;
}

long HexStrtoIntNum(char *hex)
{
    long decimal, place;
    int i = 0, val, len;

    decimal = 0;
    place = 1;

    /* Find the length of total number of hex digit */
    len = strlen(hex);
    len--;

    /*
     * Iterate over each hex digit
     */
    for (i = 0; hex[i] != '\0'; i++)
    {

        /* Find the decimal representation of hex[i] */
        if (hex[i] >= '0' && hex[i] <= '9')
        {
            val = hex[i] - 48;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f')
        {
            val = hex[i] - 97 + 10;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F')
        {
            val = hex[i] - 65 + 10;
        }

        decimal += val * pow(16, len);
        len--;
    }

    return decimal;
}

/**
 * @fn uint32_t streamGetStrBefore(char lastChar)
 * @brief This function read out serial till expected character
 * @remark
 */
uint32_t streamGetStrBefore(char lastChar)
{
    char buf[7];
    size_t bytesRead = mSerial.readBytesUntil(
        lastChar, buf, static_cast<size_t>(7));
    // if we read 7 or more bytes, it's an overflow
    if (bytesRead && bytesRead < 7)
    {
        buf[bytesRead] = '\0';

        // DBG("%s", buf);
        uint32_t res = HexStrtoIntNum(buf);
        return res;
    }

    return -9999;
}

/**
 * @fn long streamGetLongIntBefore(char lastChar)
 * @brief This function read out serial till expected character and converts result into long integer value
 * @remark
 */
long streamGetLongIntBefore(char lastChar)
{
    char buf[12];
    size_t bytesRead = mSerial.readBytesUntil(
        lastChar, buf, static_cast<size_t>(12));
    // if we read 12 or more bytes, it's an overflow
    if (bytesRead && bytesRead < 12)
    {
        buf[bytesRead] = '\0';
        // DBG("%s", buf);
        long res = atol(buf);
        return res;
    }

    return -9999;
}

/**
 * @fn bool streamSkipUntil(const char c, const uint32_t timeout_ms)
 * @brief This function read out serial till expected character meet for specific timeout
 * @remark
 */
bool streamSkipUntil(const char c, const uint32_t timeout_ms)
{
    char ch;
    uint32_t startMillis = millis();
    while (millis() - startMillis < timeout_ms)
    {
        while (millis() - startMillis < timeout_ms &&
               !mSerial.available())
        {
            GSM_YIELD();
        }
        ch = mSerial.read();
        // DBG("%c", ch);
        if (ch == c)
        {
            return true;
        }
    }
    return false;
}

// String streamReadUntil(const char c)
// {
//     String res = mSerial.readStringUntil(c);

//     return res;
// }

String streamReadStringUntil(const char lastChar, char *gpsData)
{
    char buf[100];
    size_t bytesRead = mSerial.readBytesUntil(
        lastChar, buf, static_cast<size_t>(100));
    // if we read 12 or more bytes, it's an overflow
    if (bytesRead && bytesRead < 100)
    {
        buf[bytesRead] = '\0';
        // DBG("%s", buf);
        String str = String(buf);
        strncpy(gpsData, buf, sizeof(buf));
        return str;
    }

    return "";
}

/**
 * @fn Stream &streamAT()
 * @brief This function gives access to stream or serial port directly
 * @remark use this wisely.
 */
Stream &streamAT()
{
    return mSerial;
}

static CurrentOperation_t mdmCurrentOperaton = MDM_IDLE;
/**
 * @fn bool GetModemAccess(CurrentOperation_t op)
 * @brief This function gives access to other processes
 * @param CurrentOperation_t op, operation which trying to get access of the modem
 * @return false: access not grated, true: access granted.
 * @remark always use this function to access modem. internal operations like http, ftp, etc are using this
 *         DO NOT FORGOT TO RELEASE THE ACCESS IF GRANTED
 */
bool GetModemAccess(CurrentOperation_t op)
{
    if ((mdmCurrentOperaton == MDM_IDLE) || (op == MDM_INITIALIZING))
    {
        mdmCurrentOperaton = op;
        return true;
    }

    return false;
}

/**
 * @fn bool ReleaseModemAccess(CurrentOperation_t op)
 * @brief This function releases the access which was grated previously
 * @param CurrentOperation_t op, operation which releasing the access of the modem
 * @return false: access not released because someone else tryig to release it, true: access released.
 * @remark always use this function to release modem. internal operations like http, ftp, etc are using this
 *         DO NOT FORGOT TO RELEASE THE ACCESS IF GRANTED
 */
bool ReleaseModemAccess(CurrentOperation_t op)
{
    if (mdmCurrentOperaton != op)
        return false;

    mdmCurrentOperaton = MDM_IDLE;
    return true;
}

/**
 * @fn CurrentOperation_t GetModemCurrentOp(void)
 * @brief This function returns the modem current running operation id
 * @return CurrentOperation_t, 0: Idle, 1: some operation is carrying out
 * @remark
 */
CurrentOperation_t GetModemCurrentOp(void)
{
    return mdmCurrentOperaton;
}

/**
 * @fn bool IsModemReady(void)
 * @brief This function returns the modem availability
 * @return 0: not available, 1: available
 * @remark if returns false, do not try to access modem
 */
bool IsModemReady(void)
{
    return mdmCurrentOperaton == MDM_IDLE;
}

void SerialEvent()
{
}

/**
 * @fn void QuectelInit(void)
 * @brief This function is entry point of all other processes initialization.
 * @remark This function shall call first than any other related processes.
 */
void QuectelInit(void)
{
    QUECTEL_SERIAL.setRxBufferSize(QUECTEL_SERIAL_RX_SIZE);
    QUECTEL_SERIAL.begin(115200, SERIAL_8N1, QUECTEL_SERIAL_RX_PIN, QUECTEL_SERIAL_TX_PIN);

#ifdef QUECTEL_ONOFF_PIN
    pinMode(QUECTEL_ONOFF_PIN, OUTPUT);
#endif

#ifdef QUECTEL_STATUS_PIN
    pinMode(QUECTEL_STATUS_PIN, INPUT);
#ifdef QUECTEL_ONOFF_PIN
    delay(50);
    if (digitalRead(QUECTEL_STATUS_PIN) == QUECTEL_STATUS_LEVEL)
    {
        // module is ON, switch OFF it.
        digitalWrite(QUECTEL_ONOFF_PIN, HIGH);
#ifdef ENABLE_4G
        delay(3300);
#else
        delay(1200);
#endif
        digitalWrite(QUECTEL_ONOFF_PIN, LOW);
        delay(500);
    }
#endif
#endif

#ifdef QUECTEL_SIM_1_DETECT_PIN
    pinMode(QUECTEL_SIM_1_DETECT_PIN, INPUT);
#endif

#ifdef QUECTEL_SIM_2_DETECT_PIN
    pinMode(QUECTEL_SIM_2_DETECT_PIN, INPUT);
#endif

#ifdef QUECTEL_SIM_SELECT_PIN
    // pinMode(QUECTEL_SIM_SELECT_PIN, OUTPUT);
    // digitalWrite(QUECTEL_SIM_SELECT_PIN, HIGH);
#endif
}

/**
 * @fn void QuectelLoop(void)
 * @brief This function calling other processes.
 * @remark This function shall call as frequently as possible and outside of ISR.
 */
void QuectelLoop(void)
{
    if (mSerial.available())
    {
        SerialEvent();
    }

    // gsm/gprs init loop
    QuectelGsmGprsSm();

    if (false == isWifiNetClientSelected)
    {
        // ssl loop
        QuectelSslSm();

        // http loop
        QuectelHttpClientSm();

        // ftp loop
        QuectelFtpClientSm();

        // mqtt loop
        QuectelMqttClienSm();

        // tcp loop

        // sms loop
        QuectelSmsClientSm();
    }
}

/**
 * @fn void QuectelDebug(bool enable)
 * @brief This function allows enable and disable of serial debugging of module.
 * @param bool enable, True: enable, false: disable serial dump.
 * @remark
 */
void QuectelDebug(bool enable)
{
#ifdef QUECTEL_DEBUG
    mSerial.enable(enable);
#endif
}

/**
 * @fn CurrentOperation_t GetModemState(void)
 * @brief This function returns the modem current state
 * @return CurrentOperation_t states which can be 0,1,2,3,4,5,6,7
 * @remark if returns current running state of modem
 */
CurrentOperation_t GetModemState(void)
{
    return mdmCurrentOperaton;
}
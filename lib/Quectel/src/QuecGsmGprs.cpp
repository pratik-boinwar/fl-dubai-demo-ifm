/**
 * @file       QuecGsmGprs.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#ifdef ENABLE_4G
#include "QuecGps.h"
#endif

#define MAX_EVENT_CB 10
static pfnGsmGprsEventCb eventCB[MAX_EVENT_CB] = {0};
static uint8_t _gsmNwReady = false, _gprsNwready = false;
static uint8_t _simNotPresent = true, _modemNotResponding = true;
static uint8_t _signalQualityNum = 99;
static char _imei[16] = {0};
static char _simIccd[31] = {0};
static char _operatorName[17] = {0};
static char _modemInfo[50] = {0};
// static GsmCellLocation_t _cellLoc;
static CellInfo_t _cellInfo;
static GsmDateTime_t _datetime;

#ifdef QUECTEL_SIM_SELECT_PIN
#define SIM_1_SELECT 0
#define SIM_2_SELECT (!SIM_1_SELECT)
static uint8_t _sim1Present = false, _sim2Present = false;
const char *apn_name_sim1 = NULL, *apn_username_sim1 = NULL, *apn_password_sim1 = NULL;
const char *apn_name_sim2 = NULL, *apn_username_sim2 = NULL, *apn_password_sim2 = NULL;
#endif

static SimSelect_t _currentSimSelected = SIM_SLOT_2;
static SimSelect_t _userSelectedSIM = SIM_SLOT_AUTO;

extern int apnMode;

#define _autoAPN apnMode

// static bool _autoAPN = true;
const char *apn_name = NULL, *apn_username = NULL, *apn_password = NULL;

// Selecting different APN and checking internet availability
const char apnAirtel[] = GPRS_APN_AIRTEL;
const char apnAirtelWhiteList[] = GPRS_APN_AIRTEL_WHITELIST;
const char apnVodafone[] = GPRS_APN_VODAFONE;
const char apnIdea[] = GPRS_APN_IDEA;
const char apnViWhilteList[] = GPRS_APN_VI_WHITELIST;
const char apnIdeaWhiteList[] = GPRS_APN_IDEA_WHITELIST;
const char apnBsnl[] = GPRS_APN_BSNL;
const char apnTata[] = GPRS_APN_TATA;
const char apnJio[] = GPRS_APN_JIO;

const char *apnArr[] = {apnAirtel, apnAirtelWhiteList, apnVodafone,
                        apnIdea, apnViWhilteList, apnIdeaWhiteList,
                        apnBsnl, apnTata, apnJio};

static int _apnIndex = 0;
#define nextAPN() _apnIndex++;    // advance to next APN to check internet
static bool _firstTimeAPN = true; // reset flag when SIM first time used.

char *Ql_StrToUpper(char *str)
{
    char *pCh = str;
    if (!str)
    {
        return NULL;
    }
    for (; *pCh != '\0'; pCh++)
    {
        if (((*pCh) >= 'a') && ((*pCh) <= 'z'))
        {
            *pCh = toUpperCase(*pCh);
        }
    }
    return str;
}

static void _selectAPN(void)
{
    // static bool _once = true;

    // reset index once reach to max apnArr
    if (_apnIndex > 8)
        _apnIndex = 0;

    Ql_StrToUpper(_operatorName);
    // DEBUG_MSG(PSTR("[NC] GSM/GPRS Operator: %s"), _operatorName);

    if (_firstTimeAPN)
    {
        _firstTimeAPN = false;
        _apnIndex = 0;

        if (0 != strstr(_operatorName, "IDEA"))
            _apnIndex = 3;
        else if (0 != strstr(_operatorName, "AIRTEL"))
            _apnIndex = 0;
        else if (0 != strstr(_operatorName, "VI INDIA"))
            _apnIndex = 2;
        else if (0 != strstr(_operatorName, "TATA DOCOMO"))
            _apnIndex = 7;
        else if (0 != strstr(_operatorName, "BSNL"))
            _apnIndex = 6;
        // else if (0 != strstr(_operatorName, "AIRCEL"))
        //     _apnIndex = 0;
        // else if (0 != strstr(_operatorName, "RELIANCE"))
        //     _apnIndex = 0;
        else if (0 != strstr(_operatorName, "JIO"))
            _apnIndex = 8;
        else
            _apnIndex = 0;
    }

    apn_name = apnArr[_apnIndex];
    apn_username = NULL;
    apn_password = NULL;
}

static void _publishEvents(GsmGprsEvents_t event, void *data, uint8_t dataLen)
{
    for (uint8_t idx = 0; idx < MAX_EVENT_CB; idx++)
    {
        if (eventCB[idx] != NULL)
            eventCB[idx](event, data, dataLen);
    }
}

static int8_t _getIMEI(void)
{
    sendAT(("+CGSN"));
    streamSkipUntil('\n'); // skip first newline
    String res = streamAT().readStringUntil('\n');
    if (1 == waitResponse3(GSM_OK))
    {
        // DBG("%s", res.c_str());
        res.trim();
        strncpy(_imei, res.c_str(), sizeof(_imei) - 1);
        _publishEvents(QUEC_EVENT_IMEI, (char *)_imei, strlen(_imei));
        return 1;
    }

    return 0;
}

static bool _enableDualSimSingleStandby(bool dsss)
{
    char strAT[100];
    snprintf_P(strAT, sizeof(strAT) - 1, "+QDSIMCFG=\"dsss\",%d", dsss);

    sendAT(strAT);
    if (waitResponse2(5000, (GSM_OK)) != 1)
    {
        return false;
    }

    return true;
}

static SimStatus_t _getSimStatus(uint32_t timeout_ms)
{
    for (uint32_t start = millis(); millis() - start < timeout_ms;)
    {
        sendAT(("+CPIN?"));
        if (waitResponse3(("+CPIN:")) != 1)
        {
            delay(1000);
            continue;
        }
        int8_t status =
            waitResponse3(("READY"), ("SIM PIN"), ("SIM PUK"),
                          ("NOT INSERTED"), ("NOT READY"));
        waitResponse3(GSM_OK); // readout remaining "OK" string
        switch (status)
        {
        case 2:
        case 3:
            return SIM_LOCKED;
        case 1:
            return SIM_READY;
        default:
            return SIM_ERROR;
        }
    }
    return SIM_ERROR;
}

#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)
static bool _checkSimPresenceByPin(uint8_t pinNum)
{
    if (!digitalPinIsValid(pinNum))
    {
        DBG("Digital Pin# %d invalid\n", pinNum);
        return false;
    }

    return digitalRead(pinNum) == QUECTEL_SIM_DETECT_LEVEL;
}
#else
#ifndef ENABLE_4G
static bool _checkSimPresence(void)
{
    sendAT(("+QSIMSTAT?"));
    if (waitResponse3(("+QSIMSTAT:")) != 1)
    {
        return false;
    }
    if (!streamSkipUntil(',', 50))
        return false;

    int8_t res = streamGetIntBefore('\n');
    waitResponse3(GSM_OK); // readout remaining "OK" string

    // if (res != 1) { return false; }

    return (res == 0);
}
#else
static bool _selectSim(bool simslot)
{
    if (0 == simslot)
    {
        sendAT(("+QDSIM=0"));
        if (waitResponse2(5000, (GSM_OK)) != 1)
        {
            return false;
        }
    }
    else
    {
        sendAT(("+QDSIM=1"));
        if (waitResponse2(5000, (GSM_OK)) != 1)
        {
            // reset device if CME ERROR: 4 occured
            // This is because we have enabled dss (dual sim select) and we should restart it for once after setting it.
            if (4 == getCmeError())
            {
                DBG("+CME ERROR: 4 occured resetting device...\n", );
                ESP.restart();
                return false;
            }
            return false;
        }
    }
    return true;
}

static bool _checkSimPresence(bool simSlot)
{
    if (false == _selectSim(simSlot))
    {
        return false;
    }

    sendAT(("+QSIMSTAT?"));
    if (waitResponse2(5000, ("+QSIMSTAT:")) != 1)
    {
        return false;
    }
    if (!streamSkipUntil(',', 50))
    {
        return false;
    }

    int8_t res = streamGetIntBefore('\n');
    waitResponse3(GSM_OK); // readout remaining "OK" string

    if (res != 1)
    {
        return false;
    }
    return true;
}
#endif // ENABLE_4G
#endif

static int8_t _getsimICCD(void)
{
#ifndef ENABLE_4G
    sendAT(("+QCCID"));
    streamSkipUntil('\n'); // skip first newline
    String res = streamAT().readStringUntil('\n');
    if (1 == waitResponse3(GSM_OK))
    {
        // DBG("%s", res.c_str());
        res.trim();
        strncpy(_simIccd, res.c_str(), sizeof(_simIccd) - 1);
        _publishEvents(QUEC_EVENT_SIM_ID, (char *)_simIccd, strlen(_simIccd));
        return 1;
    }
#else
    sendAT(("+QCCID"));
    int8_t ret = waitResponse3("+QCCID: ", GSM_ERROR);
    if (1 == ret)
    {
        String res = streamAT().readStringUntil('\n');
        if (1 == waitResponse3(GSM_OK))
        {
            // DBG("%s", res.c_str());
            res.trim();
            strncpy(_simIccd, res.c_str(), sizeof(_simIccd) - 1);
            _publishEvents(QUEC_EVENT_SIM_ID, (char *)_simIccd, strlen(_simIccd));
            return 1;
        }
    }
#endif

    return 0;
}

static int8_t _getSignalQuality(void)
{
    // Reply is:
    // +CSQ: 14,2
    //
    // OK

    sendAT(("+CSQ"));
    if (waitResponse3(("+CSQ:")) != 1)
    {
        _signalQualityNum = 99;
        return 99;
    }
    int8_t res = streamGetIntBefore(',');
    waitResponse3(GSM_OK); // readout remaining "OK" string
    _signalQualityNum = res;
    _publishEvents(QUEC_EVENT_NW_QUALITY, (uint8_t *)&_signalQualityNum, 1);
    return res;
}

// Gets the modem's registration status via CREG/CGREG/CEREG
// CREG = Generic network registration
// CGREG = GPRS service registration
// CEREG = EPS registration for LTE modules
static int8_t _getRegistrationStatusXREG(const char *regCommand)
{
    // sendAT('+', regCommand, '?');
    sendAT("+CGREG?");

    // check for any of the three for simplicity
    int8_t resp = waitResponse3(("+CREG:"), ("+CGREG:"),
                                ("+CEREG:"));
    if (resp != 1 && resp != 2 && resp != 3)
    {
        return -1;
    }
    streamSkipUntil(','); /* Skip format (0) */
    int status = streamAT().parseInt();
    waitResponse3(GSM_OK); // readout remaining "OK" string
    return status;
}

// read operator name
static int8_t _getOperatorName(void)
{
    /*
    AT+COPS?
    +COPS: 0,0,"IDEA"

    OK
    */

    sendAT("+COPS?");

    int8_t ret = waitResponse3("+COPS: ", GSM_ERROR);
    if (1 == ret)
    {
        streamSkipUntil('\"');
        size_t size = streamAT().readBytesUntil('\"', _operatorName, sizeof(_operatorName) - 1);
        _operatorName[size] = 0;
        waitResponse3(GSM_OK); // read out remaining
        _publishEvents(QUEC_EVENT_NW_OPERATOR, (char *)_operatorName, strlen(_operatorName));
        return 1;
    }

    return 0;
}

static int8_t _getCellLocation(void)
{
    /* Response
    +QENG: 1,0

    +QENG: 0,404,20,3e85,6aaf,14,41,-85,80,80,5,5,x,x,x,x,x,x,x

    OK
    */
    sendAT("+QENG?");
    int8_t resp = waitResponse3(("+QENG: 0,"), (GSM_OK), (GSM_ERROR));
    if (resp != 1)
    {
        // if (resp != 2) // if not error received, read out "OK"
        //     waitResponse3(GSM_OK);
        return 0;
    }

    _cellInfo.mcc = (uint16_t)streamGetIntBefore(',');
    _cellInfo.mnc = (uint16_t)streamGetIntBefore(',');
    // _cellInfo.lac = (uint32_t)streamGetIntBefore(',');
    _cellInfo.lac = streamGetStrBefore(',');
    // _cellInfo.cellId = streamGetIntBefore(',');
    _cellInfo.cellId = streamGetStrBefore(',');

    waitResponse3(GSM_OK); // readout remaining "OK" string
    _publishEvents(QUEC_EVENT_CELL_INFO, (CellInfo_t *)&_cellInfo, sizeof(CellInfo_t));
    return 1;
}

static int8_t _getGsmNwTime(void)
{
    /* Response
    +CCLK: "15/12/25,11:42:47+22"

    OK
    */

    sendAT("+CCLK?");
    int8_t resp = waitResponse3(("+CCLK:"), (GSM_OK), (GSM_ERROR));
    if (resp != 1)
    {
        // if (resp != 2) // if not error received, read out "OK"
        //     waitResponse3(GSM_OK);
        return 0;
    }
    streamGetIntBefore('"');
    _datetime.year = streamGetIntBefore('/');
    _datetime.month = streamGetIntBefore('/');
    _datetime.day = streamGetIntBefore(',');
    _datetime.hour = streamGetIntBefore(':');
    _datetime.minute = streamGetIntBefore(':');
    _datetime.second = streamGetIntBefore('+');
    _datetime.timezone = streamGetIntBefore('"');

    // char str[5] = {0, 0, 0, 0, 0};
    // int i = 0;
    // do
    // {
    //     if (streamAT().available())
    //         str[i] = streamAT().read();
    //     i++;
    // } while (i < 4);

    // _datetime.second = (str[0] - '0') * 10 + (str[1] - '0');
    // _datetime.timezone = (str[3] - '0') * 10 + (str[4] - '0');
    // if (str[2] == '-')
    // {
    //     _datetime.timezone *= -1; // time zone sign is '-'
    // }

    waitResponse3(GSM_OK); // readout remaining "OK" string
    _publishEvents(QUEC_EVENT_NW_TIME, (GsmDateTime_t *)&_datetime, sizeof(GsmDateTime_t));
    return 1;
}

static String _getLocalIP(void)
{
    /**
    AT+QILOCIP

    xxx.xxx.xxx.xxx
    */
#ifdef ENABLE_4G
    char localIP[16] = {0};
    sendAT(("+CGPADDR"));
    int8_t ret = waitResponse3("+CGPADDR: ", GSM_ERROR);
    if (1 == ret)
    {
        streamSkipUntil('\"');
        size_t size = streamAT().readBytesUntil('\"', localIP, sizeof(localIP) - 1);
        localIP[size] = 0;
        waitResponse3(GSM_OK); // read out remaining
        // _publishEvents(QUEC_EVENT_NW_OPERATOR, (char *)_operatorName, strlen(_operatorName));
        return String(localIP);
    }
    return String("0.0.0.0");
#else
    sendAT(("+QILOCIP"));
    streamSkipUntil('\n');
    String res = streamAT().readStringUntil('\n');
    // DBG("%s", res.c_str());
    res.trim();
    return res;
#endif
}

static inline IPAddress _GsmIpFromString(const String &strIP)
{
    int Parts[4] = {
        0,
    };
    int Part = 0;
    for (uint8_t i = 0; i < strIP.length(); i++)
    {
        char c = strIP[i];
        if (c == '.')
        {
            Part++;
            if (Part > 3)
            {
                return IPAddress(0, 0, 0, 0);
            }
            continue;
        }
        else if (c >= '0' && c <= '9')
        {
            Parts[Part] *= 10;
            Parts[Part] += c - '0';
        }
        else
        {
            if (Part == 3)
                break;
        }
    }
    return IPAddress(Parts[0], Parts[1], Parts[2], Parts[3]);
}

static IPAddress _localIP()
{
    return _GsmIpFromString(_getLocalIP());
}

static bool _gprsDisconnect(void)
{
#ifdef ENABLE_4G
    sendAT(("+CGACT=0,1")); // Deactivate the bearer context
    return waitResponse2(60000L, (GSM_OK), (GSM_ERROR)) == 1;
#else
    sendAT(("+QIDEACT")); // Deactivate the bearer context
    return waitResponse2(60000L, ("DEACT OK"), (GSM_ERROR)) == 1;
#endif
}

static bool _gprsConnect(const char *apn, const char *user,
                         const char *pwd)
{
    if (_gprsDisconnect())
    {
        _gprsNwready = false;
        _publishEvents(QUEC_EVENT_GPRS_NW_REGISTERATION, (char *)&_gprsNwready, 1);
    }

// select foreground context 0 = VIRTUAL_UART_1
#ifndef ENABLE_4G
    sendAT(("+QIFGCNT=0"));
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }
#endif

    // Select GPRS (=1) as the Bearer
    char strAT[100];
// sendAT(("+QICSGP=1,\""), apn, ("\",\""), user, ("\",\""), pwd,
//        ("\""));
#ifndef ENABLE_4G
    if ((user != NULL) && (pwd != NULL))
        snprintf_P(strAT, sizeof(strAT) - 1, "+QICSGP=1,\"%s\",\"%s\",\"%s\"", apn, user, pwd);
    else
        snprintf_P(strAT, sizeof(strAT) - 1, "+QICSGP=1,\"%s\"", apn);
#else
    if ((user != NULL) && (pwd != NULL))
        snprintf_P(strAT, sizeof(strAT) - 1, "+QICSGP=1,1,\"%s\",\"%s\",\"%s\"", apn, user, pwd);
    else
        snprintf_P(strAT, sizeof(strAT) - 1, "+QICSGP=1,1,\"%s\"", apn);
#endif
    sendAT(strAT);
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }

#ifndef ENABLE_4G
    // Select TCP/IP transfer mode - NOT transparent mode
    sendAT(("+QIMODE=0"));
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }

    // Enable multiple TCP/IP connections
    sendAT(("+QIMUX=1"));
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }

    // Start TCPIP Task and Set APN, User Name and Password
    // sendAT("+QIREGAPP=\"", apn, "\",\"", user, "\",\"", pwd, "\"");
    // if ((user != NULL) && (pwd != NULL))
    //     snprintf_P(strAT, sizeof(strAT), "+QIREGAPP=1,\"%s\",\"%s\",\"%s\"", apn, user, pwd);
    // else
    //     snprintf_P(strAT, sizeof(strAT), "+QIREGAPP=1,\"%s\"", apn);
    // sendAT(strAT);
    sendAT(("+QIREGAPP"));
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }
#endif

#ifdef ENABLE_4G
    sendAT(("+CGACT=1,1"));
    if (waitResponse3((GSM_OK), (GSM_ERROR)) != 1)
    {
        return false;
    }

    IPAddress localIP = _localIP();
    if (localIP == IPAddress(0, 0, 0, 0))
    {
        return false;
    }
    else
    {
        char ipaddress[16];
        strncpy(ipaddress, localIP.toString().c_str(), 16);
        _publishEvents(QUEC_EVENT_IP_ADDRESS, (char *)&ipaddress, 15);
    }
    return true;
#else
    // Activate GPRS/CSD Context
    sendAT(("+QIACT"));
    if (waitResponse2(60000L) != 1)
    {
        return false;
    }

    // Check that we have a local IP address
    IPAddress localIP = _localIP();
    if (localIP == IPAddress(0, 0, 0, 0))
    {
        return false;
    }
    else
    {
        char ipaddress[16];
        strncpy(ipaddress, localIP.toString().c_str(), 16);
        _publishEvents(QUEC_EVENT_IP_ADDRESS, (char *)&ipaddress, 15);
    }
#endif

#ifndef ENABLE_4G
    // Set Method to Handle Received TCP/IP Data
    // Mode = 1 - Output a notification when data is received
    // +QIRDI: <id>,<sc>,<sid>
    sendAT(("+QINDI=1"));
    if (waitResponse3(GSM_OK) != 1)
    {
        return false;
    }
#endif

    // // Request an IP header for received data
    // // "IPD(data length):"
    // sendAT(("+QIHEAD=1"));
    // if (waitResponse() != 1) {
    //   return false;
    // }
    //
    // // Do NOT show the IP address of the sender when receiving data
    // // The format to show the address is: RECV FROM: <IP ADDRESS>:<PORT>
    // sendAT(("+QISHOWRA=0"));
    // if (waitResponse() != 1) {
    //   return false;
    // }
    //
    // // Do NOT show the protocol type at the end of the header for received
    // data
    // // IPD(data length)(TCP/UDP):
    // sendAT(("+QISHOWPT=0"));
    // if (waitResponse() != 1) {
    //   return false;
    // }
    //
    // // Do NOT show the destination address before receiving data
    // // The format to show the address is: TO:<IP ADDRESS>
    // sendAT(("+QISHOWLA=0"));
    // if (waitResponse() != 1) {
    //   return false;
    // }

    return true;
}

// Checks if current attached to GPRS/EPS service
static bool _getGprsStaus(void)
{
    sendAT(("+CGATT?"));
    if (waitResponse3(("+CGATT: ")) != 1)
    {
        return false;
    }
    int8_t res = streamGetIntBefore('\r');
    waitResponse3(GSM_OK); // readout remaining "OK" string
    if (res != 1)
    {
        return false;
    }

    // commented by neev

    // #ifdef ENABLE_4G
    // sendAT(("+CGACT=1,1"));
    // waitResponse3((GSM_OK), (GSM_ERROR));

    // IPAddress localIP = _localIP();
    // if (localIP == IPAddress(0, 0, 0, 0))
    // {
    //     return false;
    // }
    // else
    // {
    //     char ipaddress[16];
    //     strncpy(ipaddress, localIP.toString().c_str(), 16);
    //     _publishEvents(QUEC_EVENT_IP_ADDRESS, (char *)&ipaddress, 15);
    // }
    // return true;
    // #endif

    return _localIP() != IPAddress(0, 0, 0, 0);
}

static int8_t _getModemType(void)
{
    sendAT(("I"));
    streamSkipUntil('\n'); // skip first newline
    streamSkipUntil('\n'); // skip second newline
    String res = streamAT().readStringUntil('\n');
    if (1 == waitResponse3(GSM_OK))
    {
        DBG("%s", res.c_str());
        res.trim();
        strncpy(_modemInfo, res.c_str(), sizeof(_modemInfo) - 1);
        return 1;
    }
    return 0;
}

/**
 * GSMGPRS_SM states for /BG96 handling process
 */
typedef enum _GSMGPRS_SM
{
    GSMGPRS_NULL = 0,
    GSMGPRS_GENERIC_DELAY,

    /******** gsm/gprs initialization states ******/
    GSMGPRS_INIT,
    GSMGPRS_TURN_OFF,
    GSMGPRS_WAIT_FOR_INP_STATUS_TO_HIGH,
    GSMGPRS_WAIT_BEFORE_TURN_ON,
    GSMGPRS_TURN_ON,
    GSMGPRS_WAIT_FOR_INP_STATUS_TO_LOW,
    //    GSMGPRS_CHECK_AT,
    GSMGPRS_GET_IMEI,

    GSMGPRS_CHECK_CPIN_READY,
    GSMGPRS_CONFIGURE,
    GSMGPRS_CHECK_NETWORK_REG,
    GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT,
    GSMGPRS_GET_SIM_ICCID,
    //    GSMGPRS_GET_CELL_ID_LOCATION,
    GSMGPRS_GET_GPRS_SERVICE_STATE,
    GSMGPRS_ATTACH_GPRS_SERVICE,
    GSMGPRS_GET_OPERATOR_NAME,
    GSMGPRS_SET_APN,
    //    GSMGPRS_GET_GPRS_CONTEXT,
    GSMGPRS_OPEN_GPRS_CONTEXT,
    GSMGPRS_CLOSE_GPRS_CONTEXT,

//    GSMGPRS_SET_MUX_MODE,
//    GSMGPRS_SET_DNS_MODE,
#if defined(USE_QUECTEL_M95) || defined(USE_QUECTEL_M66)
    GSMGPRS_REGITER_TCPIP_STACK,
#endif
    //    GSMGPRS_ACTIVATE_PDP,
    //    GSMGPRS_DEACTIVATE_PDP,
    //    GSMGPRS_CHECK_PDP_STATUS,
    //    GSMGPRS_SET_TCP_DATA_RECEIVED_URC_MODE,
    //    GSMGPRS_GET_CELL_LOCATION,
    //    GSMGPRS_WAIT_FOR_CELL_LOCATION,
    GSMGPRS_INIT_DONE,
    GSMGPRS_IDLE,
    //    GSMGPRS_UNKNOWN_COMMAND

} GSMGPRS_SM;

/// holds the current and previous operating state
static GSMGPRS_SM _eState = GSMGPRS_INIT, _ePrevState = GSMGPRS_INIT;
/// general uint32_t used in processing
static uint32_t _localGenericTick = 0, _genericDelayTick = 0;

// #if defined(USE_SMTP_STACK)
//  static GSMGPRS_SM _enSmSmtp = GSMGPRS_CLEAR_EMAIL_SETTINGS;
// #endif

/**
 * @fn void SmChangeState(GSMGPRS_SM curState)
 * @brief This function changes the process state
 * @param GSMGPRS_SM curState the state to process
 * @remark
 */
static void SmChangeState(GSMGPRS_SM curState)
{
    _ePrevState = _eState;
    _eState = curState;
}

/**
 * @fn void SmDelay(uint32_t delayTicks, GSMGPRS_SM nextState)
 * @brief This function suspends current task for given uint32_t
 *        move to another state after given uint32_t
 * @param uint32_t delayTicks uint32_t to suspend
 * @param GSMGPRS_SM nextState state to process after delay
 * @remark
 */
static void SmDelay(uint32_t delayTicks, GSMGPRS_SM nextState)
{
    _genericDelayTick = delayTicks;
    _localGenericTick = millis();
    SmChangeState(GSMGPRS_GENERIC_DELAY);

    if (GSMGPRS_NULL != nextState)
        _ePrevState = nextState;
}

/**
 * @fn void QuectelGsmGprsSm(void)
 * @brief This function is handling modem initialization and makes sure it has gprs connected.
 * @remark this function is required to be called frequently.
 *         Do not make direct call. This is called within QuectelLoop() function
 */
void QuectelGsmGprsSm(void)
{
    // static uint8_t state = 0;
    static uint32_t _generalTimeoutTick = 0;

    switch (_eState)
    {
    case GSMGPRS_GENERIC_DELAY:
        if ((millis() - _localGenericTick) > _genericDelayTick)
        {
            SmChangeState(_ePrevState);
            break;
        }
        break;

    case GSMGPRS_INIT: // prepare to init

        if (!GetModemAccess(MDM_INITIALIZING))
            SmDelay(1000, GSMGPRS_NULL);

        _gsmNwReady = false;
        _gprsNwready = false;
        _simNotPresent = true;

// select SIM slot
#ifdef ENABLE_4G
#else
#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)

        _sim1Present = _checkSimPresenceByPin(QUECTEL_SIM_1_DETECT_PIN);
        _sim2Present = _checkSimPresenceByPin(QUECTEL_SIM_2_DETECT_PIN);

        if (!(_sim1Present || _sim2Present))
        {
            DBG("No SIM card detected\n");
            // SmDelay(100, GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
            // break;
        }

        if (_userSelectedSIM == SIM_SLOT_AUTO)
        {
            // device can select between SIM slots

            if ((_sim1Present && _sim2Present))
            {
                DBG("Both SIM cards detected\n");
                // Note:
                // here we are alternatively selecting SIM cards
                // on each reset.
                // starting with SIM1 on power cycle.
                if (_currentSimSelected == SIM_1_SELECT)
                {
                    // giving chance to another
                    _currentSimSelected = SIM_SLOT_2;
                    digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_2_SELECT);
                    _firstTimeAPN = true; // reset flag to check all APN for _autoAPN mode
                }
                else
                {
                    _currentSimSelected = SIM_SLOT_1;
                    digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_1_SELECT);
                    _firstTimeAPN = true; // reset flag to check all APN for _autoAPN mode
                }
            }
            else
            {
                // select detected SIM card

                if (_sim1Present)
                {
                    DBG("SIM 1 Selcted\n");
                    _currentSimSelected = SIM_SLOT_1;
                    digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_1_SELECT);
                }
                else if (_sim2Present)
                {
                    DBG("SIM 2 Selcted\n");
                    _currentSimSelected = SIM_SLOT_2;
                    digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_2_SELECT);
                }
                else
                {
                    // shall not happen
                    // SmDelay(10000, GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
                    // break;
                }
            }
        }
        else
        {
            // user has selected SIM slot.

            if (_userSelectedSIM == SIM_SLOT_1) // if SIM 1 slot selected
            {
                DBG("\nUSER Selected SIM 1");

                _currentSimSelected = SIM_SLOT_1;
                digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_1_SELECT);

                if (!_sim1Present)
                {
                    // sim 1 not found.
                    if (_sim2Present)
                    {
                        DBG(" not detected, but SIM 2 is detected\n");
                    }
                    // SmDelay(10000, GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
                    // break;
                }
                DBG("\n");
            }
            else if (_userSelectedSIM == SIM_SLOT_2) // if SIM 2 slot selected
            {
                DBG("\nUSER Selected SIM 2");

                _currentSimSelected = SIM_SLOT_2;
                digitalWrite(QUECTEL_SIM_SELECT_PIN, SIM_2_SELECT);

                if (!_sim2Present)
                {
                    // sim 2 not found
                    if (_sim1Present)
                    {
                        DBG(" not detected, but SIM 1 is detected\n");
                    }
                    // SmDelay(10000, GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
                    // break;
                }
                DBG("\n");
            }
            else
            {
                // shall not happen
                // SmDelay(100, GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
                // break;
            }
        }

        // if auto APN mode is not selected, then use given apn details for choosen SIM.
        // else APN will be decided while it is being used.
        if (!_autoAPN)
        {
            if (_currentSimSelected == SIM_SLOT_1)
            {
                apn_name = apn_name_sim1;
                apn_username = apn_username_sim1;
                apn_password = apn_password_sim1;
            }
            else
            {
                apn_name = apn_name_sim2;
                apn_username = apn_username_sim2;
                apn_password = apn_password_sim2;
            }
        }
#endif
#endif

        if (digitalRead(QUECTEL_STATUS_PIN) == HIGH)
        {
            DBG("Modem turning OFF\n");
            digitalWrite(QUECTEL_ONOFF_PIN, HIGH);
        }
        else
        {
            DBG("Modem already OFF\n");
            SmDelay(1500, GSMGPRS_WAIT_BEFORE_TURN_ON);
            break;
        }

#ifdef ENABLE_4G
        SmDelay(3300, GSMGPRS_TURN_OFF);
#else
        SmDelay(800, GSMGPRS_TURN_OFF);
#endif

        // _eState = GSMGPRS_GET_IMEI;
        break;

    case GSMGPRS_TURN_OFF:
        digitalWrite(QUECTEL_ONOFF_PIN, LOW);
        DBG("Modem turned OFF\n");
        SmDelay(8000, GSMGPRS_WAIT_FOR_INP_STATUS_TO_HIGH);
        break;

    case GSMGPRS_WAIT_FOR_INP_STATUS_TO_HIGH:
        if ((HIGH == digitalRead(QUECTEL_STATUS_PIN)))
        {
            DBG("Modem didn't turn OFF\n");
#if defined(USE_QUECTEL_BG96)
            /*
             It is recommended to execute AT+QPOWD command to turn off the module, as it is the safest and best
way. This procedure is realized by letting the module log off from the network and allowing the software to
enter into a secure and safe data state before disconnecting the power supply.
After sending AT+QPOWD, do not enter any other AT commands. The module outputs POWERED
DOWN and sets the STATUS pin as low to enter into the shutdown state. In order to avoid data loss, it is
suggested to wait for 1s to switch off the VBAT after the STATUS pin is set as low and the URC
POWERED DOWN is outputted. If POWERED DOWN cannot be received within 65s, the VBAT shall be
switched off compulsorily.
             */
            sendAT("+QPOWD");
            SmDelay(1500, GSMGPRS_INIT);
#else
            SmChangeState(GSMGPRS_INIT);
#endif

            break;
        }
        SmDelay(1500, GSMGPRS_WAIT_BEFORE_TURN_ON);
        break;

    case GSMGPRS_WAIT_BEFORE_TURN_ON:
        if (LOW == digitalRead(QUECTEL_STATUS_PIN)) // means it is off
        {
            DBG("Modem turning ON\n");
            digitalWrite(QUECTEL_ONOFF_PIN, HIGH);
        }
#ifdef ENABLE_4G
        SmDelay(2200, GSMGPRS_TURN_ON);
#else
        SmDelay(1000, GSMGPRS_TURN_ON);
#endif

        break;

    case GSMGPRS_TURN_ON: // restart modem
        digitalWrite(QUECTEL_ONOFF_PIN, LOW);
        SmDelay(5500, GSMGPRS_WAIT_FOR_INP_STATUS_TO_LOW);
        // state = GSMGPRS_GET_IMEI;
        break;

    case GSMGPRS_WAIT_FOR_INP_STATUS_TO_LOW: // check modem status pin
        if (LOW == digitalRead(QUECTEL_STATUS_PIN))
        {
            DBG("Modem didn't turn ON\n");
            SmChangeState(GSMGPRS_INIT);
            break;
        }
        DBG("Modem turned ON\n");
        SmDelay(5000, GSMGPRS_GET_IMEI);
        _generalTimeoutTick = millis();
        // state = GSMGPRS_GET_IMEI;
        _publishEvents(QUEC_EVENT_POWER_ON, 0, 0);
        break;

    case GSMGPRS_GET_IMEI: // send AT command to test
        sendAT("E0&W");    // Echo off
        if ((waitResponse3((GSM_OK), (GSM_ERROR)) == 1))
        {
            SmDelay(5000, GSMGPRS_CHECK_CPIN_READY); // module required some time after restart to inititalize
            _modemNotResponding = false;
            delay(200);
            waitResponse3(); // read out remaining
            _getIMEI();
            _getIMEI();
            _getIMEI();

#ifdef ENABLE_4G
            // // enable dual sim single standby by default
            // _enableDualSimSingleStandby(1);

            if (false == GpsInitialization())
            {
                DBG("Gps not Activated, retry\n");
                GpsInitialization();
            }
#endif
        }
        else
        {
            SmDelay(15000, GSMGPRS_INIT); // restart module again and check
            _modemNotResponding = true;
        }
        break;

    case GSMGPRS_CHECK_CPIN_READY: // check SIM
    {
#ifdef ENABLE_4G
        if (false == _selectSim(0))
        {
            break;
        }
#endif
        SimStatus_t ret = _getSimStatus(2000);
        if (ret == SIM_READY)
        {
            _getsimICCD();

            SmChangeState(GSMGPRS_CONFIGURE);
            _simNotPresent = false;
        }
        else
        {
            SmChangeState(GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT);
            _simNotPresent = true;
        }
    }
    break;

    case GSMGPRS_CONFIGURE:
    {
        // Get modem type, 2G or 4G
        _getModemType();
        // sendAT("I"); // module identification information
        // waitResponse3((GSM_OK), (GSM_ERROR));

        // sendAT("+CGMI"); //module identification information
        // waitResponse3((GSM_OK), (GSM_ERROR));
        // sendAT("+CGMM"); //module identification information
        // waitResponse3((GSM_OK), (GSM_ERROR));
        // sendAT("+CGMR"); //module identification information
        // waitResponse3((GSM_OK), (GSM_ERROR));
        // sendAT("+GOI"); //module identification information
        // waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+CMGF=1"); // sms mode set to text
        waitResponse3((GSM_OK), (GSM_ERROR));

        // sendAT("+CPMS=\"MT\",\"MT\",\"MT\""); //sms sms memory to mdoule and SIM
        // waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+CNMI=2,1"); // the auto-indication for new short message is enalbed
        waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+CSMP=17,161,0,241"); // set the character encoding for SMS
        waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+CTZU=3"); // auto sync module rtc with network local time
        waitResponse3((GSM_OK), (GSM_ERROR));

#ifndef ENABLE_4G
        sendAT("+QIFGCNT=0"); // brearer profile type
        waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+QIDNSIP=0"); // 0:DNS mode, 1:IP mode
        waitResponse3((GSM_OK), (GSM_ERROR));

        sendAT("+QENG=1"); // Switch on engineering mode, Cell Id location
        waitResponse3((GSM_OK), (GSM_ERROR));
#endif
        _generalTimeoutTick = millis();
        SmChangeState(GSMGPRS_CHECK_NETWORK_REG);
        // _publishEvents(QUEC_EVENT_SIM_SELECTED, (SimSelect_t *)&_currentSimSelected, 1);
    }
    break;

    case GSMGPRS_GET_SIM_CARD_INSERT_STATUS_REPORT: // keep checking SIM presence
    {
#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)

        _sim1Present = _checkSimPresenceByPin(QUECTEL_SIM_1_DETECT_PIN);
        _sim2Present = _checkSimPresenceByPin(QUECTEL_SIM_2_DETECT_PIN);

        if ((_sim1Present || _sim2Present))
        {
            DBG("SIM card detected\n");
            SmDelay(100, GSMGPRS_INIT);
            break;
        }

#else
#ifndef ENABLE_4G
        if (_checkSimPresence())
        {
            SmDelay(100, GSMGPRS_INIT); // restart module
            break;
        }
#else
        // check for sim1slot and sim2slot both

        if ((_checkSimPresence(0)))
        {
            DBG("SIM card detected\n");
            SmDelay(100, GSMGPRS_INIT);
            break;
        }
#endif
#endif
        SmDelay(5000, GSMGPRS_NULL);
    }
    break;

    case GSMGPRS_CHECK_NETWORK_REG:
    {
        _getSignalQuality();

        int8_t stat = _getRegistrationStatusXREG("CREG");
        if ((5 == stat) || (1 == stat))
        {
            // network registered
            _gsmNwReady = true;
            _publishEvents(QUEC_EVENT_GSM_NW_REGISTERATION, (char *)&_gsmNwReady, 1);

#ifndef ENABLE_4G
            _getCellLocation();
#endif
            _getOperatorName();

            _generalTimeoutTick = millis();
            SmChangeState(GSMGPRS_GET_GPRS_SERVICE_STATE);
            break;
        }

        // if gsm network not registered within this period, restart module
        if ((millis() - _generalTimeoutTick) > 60000) // 180000 // 300000
        {
            _publishEvents(QUEC_EVENT_GSM_NW_REGISTERATION, (char *)&_gsmNwReady, 1);
            SmChangeState(GSMGPRS_INIT);
            break;
        }
        else
        {
            SmDelay(5000, GSMGPRS_NULL);
        }
    }
    break;

    case GSMGPRS_GET_GPRS_SERVICE_STATE:
    {
        if (!IsATavailable())
            break;

        // if doing some other operation than "initialization" and not in "idle" , come back again
        if (!((GetModemCurrentOp() == MDM_INITIALIZING) || (GetModemCurrentOp() == MDM_IDLE)))
            break;

        SmDelay(30000, GSMGPRS_NULL);

        if (_getGprsStaus())
        {
            // wow!! we are into cloud now
            _gprsNwready = true;
#ifdef ENABLE_4G
            _publishEvents(QUEC_EVENT_GPRS_NW_REGISTERATION, (uint8_t *)&_gprsNwready, 1);
#endif
            _generalTimeoutTick = millis();
            ReleaseModemAccess(MDM_INITIALIZING);
            SmChangeState(GSMGPRS_INIT_DONE);
            break;
        }
        else
        {
            // though we knew gprs is already disconnected but it is good to call closing procedure
            // SmChangeState(GSMGPRS_CLOSE_GPRS_CONTEXT);

            // we are here in two scenario
            // 1. very first time, we have already gain modem access.
            // 2. periodic gprs connection check. if this is the case, then we have to regain access.
            //    so, that we can pass the message to all other internet depedent module to re-init there process.
            GetModemAccess(MDM_INITIALIZING);
            _gprsNwready = false;

            if (_autoAPN)
            {
                _selectAPN();
                DBG("Auto APN '%s' Selected\n", apn_name);
            }

            // connect to gprs and let this state check if actually it gets connected.
            if (_gprsConnect(apn_name, apn_username, apn_password))
            {
                _gprsNwready = true;
                _publishEvents(QUEC_EVENT_GPRS_NW_REGISTERATION, (uint8_t *)&_gprsNwready, 1);
                SmDelay(5000, GSMGPRS_GET_GPRS_SERVICE_STATE);
                break;
            }
            else
            {
                // if Auto APN mode selected, try gprs connection with other APN
                if (_autoAPN)
                    nextAPN(); // select next APN
            }
            // ToDo:
            // if modem not able to connect to gprs, take some action

            if (millis() - _generalTimeoutTick > 60000) // 180000 // 300000
            {
                // its been quite long we are trying to connect gprs.
                // lets restart the module and try again.
                SmDelay(1000, GSMGPRS_INIT);
                break;
            }
        }
    }
    break;

    case GSMGPRS_CLOSE_GPRS_CONTEXT: // deactivate gprs, it will take some time to close
    {
        _gprsDisconnect();

        // reconnect
        SmDelay(10000, GSMGPRS_GET_GPRS_SERVICE_STATE);
    }
    break;

    case GSMGPRS_INIT_DONE: // gprs init done, keep checking status
    {
        // ToDo:
        // if we first time reach here after power cycle,
        // do some necessary task if required.

        // delete read sms

        SmDelay(5000, GSMGPRS_IDLE);
    }
    break;

    case GSMGPRS_IDLE: // modem is idle
    {
        if (IsATavailable() && IsModemReady())
        {
            _getSignalQuality();

#ifdef ENABLE_4G
            // GpsGetData();
#else
            _getCellLocation();
#endif
            _getGsmNwTime();
            SmDelay((random(60, 120) * 1000), GSMGPRS_NULL);
        }
        else
        {
            SmDelay(1000, GSMGPRS_NULL);
        }

        if (millis() - _generalTimeoutTick > 60000) // 120000
        {
            _generalTimeoutTick = millis();
            SmDelay(1000, GSMGPRS_GET_GPRS_SERVICE_STATE);
        }
    }
    break;

    default:
        SmChangeState(GSMGPRS_GET_IMEI);
        break;
    }
}

/**
 * @fn bool QuectelSubscribeEvents(pfnGsmGprsEventCb cb)
 * @brief This function allows to subscribe various gsm/gprs events.
 * @return false: fail. subscription full , true: success. subscription accepted
 * @remark useful when user need to know various operation stages, events etc.
 */
bool QuectelSubscribeEvents(pfnGsmGprsEventCb cb)
{
    if (cb == NULL)
        return false;

    uint8_t idx = 0;
    // check for empty slot
    for (idx = 0; idx < MAX_EVENT_CB; idx++)
    {
        if (eventCB[idx] == NULL)
        {
            // subscription slot available
            eventCB[idx] = cb;

            // making next slot empty for new subscription
            // assigning slots in cylical
            if (++idx < MAX_EVENT_CB)
                eventCB[idx] = NULL;

            return true;
        }
    }

    // no subscription slot available
    return false;
}

/**
 * @fn bool ResetModem(void)
 * @brief This function restarting modem forcefully and starts re-initiate procedure.
 * @return false: fail, true: success
 * @remark useful when there is no exptected response since long time.
 */
bool ResetModem(void)
{
    _gsmNwReady = false;
    _gprsNwready = false;
    _modemNotResponding = true;
    SmChangeState(GSMGPRS_INIT);
    DBG("restarting modem\n");
    return true;
}

// bool IsModemBusy(void)
// {
//     if (_eState != GSMGPRS_IDLE)
//         return false;

//     return true;
// }

/**
 * @fn bool IsGsmNwConnected(void)
 * @brief This function returns the status of gsm network registration.
 * @return false: gsm nw not registered, true: gsm nw registered
 * @remark check this function always before calling any SMS, Calling related operations.
 */
bool IsGsmNwConnected(void)
{
    return _gsmNwReady;
}

/**
 * @fn bool IsGprsConnected(void)
 * @brief This function returns the status of gprs connection.
 * @return false: gprs not connected, true: gprs connected
 * @remark check this function always before calling any internet related operations.
 */
bool IsGprsConnected(void)
{
    return _gprsNwready;
}

/**
 * @fn bool GetSignalQuality(uint8_t *num)
 * @brief This function gives the gsm signal quality number.
 * @param uint8_t *num, interger pointer to return signal quality data.
 * @return false: signal quality not found, true: signal quality found
 * @remark signal quality measured after gsm network registered.
 * @example
 * uint8_t quality = 99;
 * bool status = GetSignalQuality(&quality);
 */
bool GetSignalQuality(uint8_t *num)
{
    if (!_gsmNwReady)
    {
        *num = _signalQualityNum;
        return false;
    }

    *num = _signalQualityNum;
    return true;
}

/**
 * @fn bool GetIMEI(char *imei)
 * @brief This function returns the IMEI number of modem.
 * @param char *imei, pointer to buffer to hold IMEI number.
 * @return false: imei not found. means modem not responding, true: imei found
 * @remark imei buffer length must be greater than 15.
 * @example
 * char imei[16];
 * bool status = GetIMEI(imei);
 */
bool GetIMEI(char *imei)
{
    if (_modemNotResponding)
        return false;

    strcpy(imei, _imei);
    return true;
}

/**
 * @fn bool GetSimICCD(char *iccd)
 * @brief This function returns the ICCD number of SIM Card in operation.
 * @param char *iccd, pointer to buffer to hold ICCD number.
 * @return false: iccd not found. means SIM not detected, true: iccd found
 * @remark iccd buffer length must be greater than 21.
 * @example
 * char iccd[22];
 * bool status = GetSimICCD(iccd);
 */
bool GetSimICCD(char *iccd)
{
    if (_simNotPresent)
        return false;

    strcpy(iccd, _simIccd);
    return true;
}

/**
 * @fn bool GetCellLocation(CellInfo_t *cellInfo)
 * @brief This function returns the serving cell information.
 * @param CellInfo_t *cellInfo, pointer to structure to copy cell info data.
 * @return false: cell info not found, true: cell info found
 * @remark info will available after gsm network registration.
 * @example
 * CellInfo_t cellInfo;
 * bool status = GetCellLocation(&cellInfo);
 */
bool GetCellLocation(CellInfo_t *cellInfo)
{
    if (!_gsmNwReady)
        return false;

    cellInfo->mcc = _cellInfo.mcc;
    cellInfo->mnc = _cellInfo.mnc;
    cellInfo->lac = _cellInfo.lac;
    cellInfo->cellId = _cellInfo.cellId;

    return true;
}

/**
 * @fn bool GetGsmTime(GsmDateTime_t *datetime)
 * @brief This function returns the ICCD number of SIM Card in operation.
 * @param GsmDateTime_t *datetime, pointer to structure to copy date-time data.
 * @return false: date-time not available, true: date-time available
 * @remark info will available after gsm network registration.
 * @example
 * GsmDateTime_t datetime;
 * bool status = GetGsmTime(&datetime);
 */
bool GetGsmTime(GsmDateTime_t *datetime)
{
    if (!_gsmNwReady)
        return false;

    // if AT available and modem is not busy then get latest time
    if (IsATavailable() && IsModemReady())
    {
        _getGsmNwTime();

        datetime->year = _datetime.year;
        datetime->month = _datetime.month;
        datetime->day = _datetime.day;
        datetime->hour = _datetime.hour;
        datetime->minute = _datetime.minute;
        datetime->second = _datetime.second;
        datetime->timezone = _datetime.timezone;

        return true;
    }
    return false;
}

/**
 * @fn bool SetAPN(const char *apn, const char *username, const char *password, SimApnSelect_t sim)
 * @brief This function receives the bearer APN information.
 * @param const char *apn, pointer to buffer holding beare APN name.
 * @param const char *username, pointer to buffer holding APN username.
 * @param const char *password, pointer to buffer holding APN password.
 * @param  SimApnSelect_t sim, sim number to be used for this set of apn details or Auto APN select.
 * @return false: fail, true: success
 * @remark user need to hold the apn, username and password in buffer throughtout the program.
 *         In Auto APN mode, predefined APN will be selected according to operators name for the current using SIM.
 * @example no username and paasword for APN and information for SIM_1.
 * const char apn_name[]="www";
 * bool status = SetAPN(apn_name, NULL, NULL, SIM_1_APN);
 *
 * select Auto APN
 * bool status = SetAPN(NULL, NULL, NULL, SIM_AUTO_APN);
 */
bool SetAPN(const char *apn, const char *username, const char *password, SimApnSelect_t sim)
{
    // if apn is not given then enable auto APN mode.
    // if (apn == NULL)
    //     _autoAPN = true;

    // if apn is not given and not auto apn mode as well
    if ((apn == NULL) && (sim != SIM_AUTO_APN))
        return false;

#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)
    if (sim == SIM_1_APN)
    {
        apn_name_sim1 = apn;
        apn_username_sim1 = username;
        apn_password_sim1 = apn_password;
        _autoAPN = false;
    }
    else if (sim == SIM_2_APN)
    {
        apn_name_sim2 = apn;
        apn_username_sim2 = username;
        apn_password_sim2 = apn_password;
        _autoAPN = false;
    }
    else
    {
        _autoAPN = true;
    }
#else
    if (sim == SIM_AUTO_APN)
    {
        _autoAPN = true;
    }
    else
    {
        apn_name = apn;
        apn_username = username;
        apn_password = apn_password;
    }
#endif
    return true;
}

/**
 * @fn bool GetAPN(char *name, uint8_t len)
 * @brief This function returns the currently using apn name.
 * @param char *name, pointer to buffer to copy apn name.
 * @param uint8_t len, name buffer size.
 * @return false: error, true: success
 * @remark useful when Auto APN mode is selected and if user wants to know what APN is using.
 * @example
 * char apnName[18];
 * bool status = GetAPN(apnName, sizeof(apnName));
 */
bool GetApn(char *name, uint8_t len)
{
    if (name != NULL)
    {
        if (apn_name != NULL)
        {
            strncpy(name, apn_name, len);
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

/**
 * @fn bool GetOperatorName(char *name, uint8_t len)
 * @brief This function returns the operator name.
 * @param char *name, pointer to buffer to copy operator name.
 * @param uint8_t len, name buffer size.
 * @return false: operator not found. true: operator name found
 * @remark
 * @example
 * char operatorName[17];
 * bool status = GetSimICCD(operatorName, sizeof(operatorName));
 */
bool GetOperatorName(char *name, uint8_t len)
{
    if (_operatorName[0] == 0)
    {
        // may be previous attempt to read _operatorName was not succeded.
        // or network is not registered yet.
        // try again here.

        if (!_gsmNwReady)
            return false;

        // if AT available and modem is not busy then try to get operator name.
        if (IsATavailable() && IsModemReady())
            _getOperatorName();
    }

    if (_operatorName[0] == 0)
        return false;

    strncpy(name, _operatorName, len);

    return true;
}

/**
 * @fn bool SelectSIM(SimSelect_t simSlot)
 * @brief This function allows user to select SIM slot.
 * @param SimSelect_t simSlot, sim slot number.
 * @return false: no dual SIM support. true: SIM slot selected
 * @remark
 * SIM_SLOT_AUTO: SIM 1 & 2 are alternatiely gets selected when gprs signal dropped.
 * SIM_SLOT_1 : Only SIM 1 will be selected all the time.
 * SIM_SLOT_2 : Only SIM 2 will be selected all the time.
 * Note: operations are carried out on user selected SIM,
 *       if GSM/GPRS fail to initialize then also device will not switch to another SIM.
 *       User have to take control of SIM selection.
 * @example
 * bool status = SelectSIM(SIM_SLOT_AUTO);
 */
bool SelectSIM(SimSelect_t simSlot)
{
#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)
    _userSelectedSIM = simSlot;

    _sim1Present = _checkSimPresenceByPin(QUECTEL_SIM_1_DETECT_PIN);
    _sim2Present = _checkSimPresenceByPin(QUECTEL_SIM_2_DETECT_PIN);

    // if user is trying to select SIM 1 and if it is not present
    if ((!_sim1Present) && (simSlot == SIM_SLOT_1))
    {
        DBG("\nSIM not detected in selected slot. Please insert it\n");
    }

    // if user is trying to select SIM 2 and if it is not present
    if ((!_sim2Present) && (simSlot == SIM_SLOT_2))
    {
        DBG("\nSIM not detected in selected slot. Please insert it\n");
    }

    return true;

#endif
#ifdef ENABLE_4G
    // _userSelectedSIM = simSlot;

    // if (simSlot == SIM_SLOT_1)
    // {
    //     _sim1Present = _checkSimPresence(0);
    //     // if user is trying to select SIM 1 and if it is not present
    //     if ((!_sim1Present))
    //     {
    //         DBG("\nSIM not detected in selected slot. Please insert it\n");
    //     }
    // }
    // if (simSlot == SIM_SLOT_2)
    // {
    //     _sim2Present = _checkSimPresence(1);
    //     // if user is trying to select SIM 2 and if it is not present
    //     if ((!_sim2Present) && (simSlot == SIM_SLOT_2))
    //     {
    //         DBG("\nSIM not detected in selected slot. Please insert it\n");
    //     }
    // }
    return true;
#endif
    return false;
}

/**
 * @fn SimSelect_t GetSelectedSIM()
 * @brief This function returns currently selected sim slot.
 * @return SimSelect_t: identification sim slot
 * @remark returns SIM_SLOT_1 always when there is no dual sim support.
 * @example
 * SimSelect_t simSlot = GetSelectedSIM();
 */
SimSelect_t GetSelectedSIM()
{
#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)
    return _currentSimSelected;
#endif
#ifdef ENABLE_4G
    return _currentSimSelected;
#endif
    return SIM_SLOT_1;
}

/**
 * @fn bool CheckSIM_Presence(SimSelect_t simSlot)
 * @brief This function returns status of SIM Card detection/presence.
 *        When simSlot is SIM_SLOT_AUTO then both sim card detection will be checked.
 * @param SimSelect_t: identification sim slot
 * @remark returns, false: not present/detected, true: present/detected
 * @example
 * bool simDetect = CheckSIM_Presence(SIM_SLOT_1);
 */
bool CheckSIM_Presence(SimSelect_t simSlot)
{
#if defined(QUECTEL_SIM_1_DETECT_PIN) || defined(QUECTEL_SIM_2_DETECT_PIN)
    _sim1Present = _checkSimPresenceByPin(QUECTEL_SIM_1_DETECT_PIN);
    _sim2Present = _checkSimPresenceByPin(QUECTEL_SIM_2_DETECT_PIN);
    if (simSlot == SIM_SLOT_1)
        return _sim1Present;
    else if (simSlot == SIM_SLOT_2)
        return _sim2Present;
    else
        return (_sim1Present && _sim2Present);
#else
#ifdef ENABLE_4G
    // if (SIM_SLOT_1 == simSlot)
    // {
    //     _sim1Present = _checkSimPresence(0);
    //     return _sim1Present;
    // }
    // else if (SIM_SLOT_2 == simSlot)
    // {
    //     _sim2Present = _checkSimPresence(1);
    //     return _sim2Present;
    // }
    // else
    // {
    //     return (_sim1Present && _sim2Present);
    // }
    if (_simNotPresent)
        return false;
#else
    return _checkSimPresence();
#endif
#endif

    return true;
}

/**
 * @fn bool CheckSIM_PresenceByPrevStatusAtRestart(SimSelect_t simSlot)
 * @brief This function returns status of SIM Card detection/presence.
 *        When simSlot is SIM_SLOT_AUTO then both sim card detection will be checked.
 * @param SimSelect_t: identification sim slot
 * @remark returns, false: not present/detected, true: present/detected
 * @example
 * bool simDetect = CheckSIM_PresenceByPrevStatusAtRestart(SIM_SLOT_1);
 */
bool CheckSIM_PresenceByPrevStatusAtRestart(SimSelect_t simSlot)
{
#ifdef ENABLE_4G
    // if (SIM_SLOT_1 == simSlot)
    // {
    //     return _sim1Present;
    // }
    // else if (SIM_SLOT_2 == simSlot)
    // {
    //     return _sim2Present;
    // }
    // else
    // {
    //     return (_sim1Present && _sim2Present);
    // }
    if (_simNotPresent)
        return false;
#endif

    return true;
}

/**
 * @fn bool GetGPRSStatus(void)
 * @brief This function returns status of GPRS connection.
 * @remark returns, false: if gprs not connected/ local IP is not valid, true: valid local IP
 * @example
 * bool gprsConnectValidIP = GetGPRSStatus();
 */
bool GetGPRSStatus(void)
{
    return _getGprsStaus();
}

/**
 * @fn bool GetModemInfo(char *modemType)
 * @brief This function gived modem information.
 * @remark returns, false: modem not responding, true: valid modem information
 * @example
 * char modemInfo[100];
 * bool isValidModemInfo = GetModemInfo(modemInfo);
 */
bool GetModemInfo(char *modemType)
{
    if (_modemNotResponding)
        return false;

    strcpy(modemType, _modemInfo);
    return true;
}
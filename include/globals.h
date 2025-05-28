#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "FLToolbox.h"
#include "CmdInterface.h"
#include "helper.h"

// #include "logger_espefs.h"

#include "config.h"
#include "global-vars.h"

#define TRUE_RESPONSE "{\"sts\":true}"
#define FALSE_RESPONSE "{\"sts\":false}"

#define HTTP_SOCKET_ERROR "3815"
#define HTTP_CONNECTION_TIMEOUT "3801"

#define SERIAL_HEART_BEAT
#define GSM_AT_COMMAND_CONSOLE // comment this to disable AT commands used from serial cmdline

#define DEFAULT_LATITUDE def_lat
#define DEFAULT_LONGITUDE def_long

#ifndef MOD_RTU_DEBUG
#define MOD_RTU_DEBUG systemFlags.modbusRtuDebugEnableFlag
#endif

#ifndef modRtuDebug
#define modRtuDebug(funcall) \
    {                        \
        if (MOD_RTU_DEBUG)   \
        {                    \
            funcall          \
        }                    \
    }
#endif

#ifndef MOD_TCP_DEBUG
#define MOD_TCP_DEBUG systemFlags.modbusTcpDebugEnableFlag
#endif

#ifndef modTcpDebug
#define modTcpDebug(funcall) \
    {                        \
        if (MOD_TCP_DEBUG)   \
        {                    \
            funcall          \
        }                    \
    }
#endif

// if http and mqtt fails to connect for 20 times, it will reset modem
#define MAX_NO_OF_TIMES_FAILED_TO_CONNECT_INTERNET conectToInternetFailedCount

typedef union _SYS_FLGS_
{
    uint64_t all;
    struct
    {
        unsigned debugEnableFlag : 1;
        unsigned gsmDebugEnableFlag : 1;
        unsigned modbusRtuDebugEnableFlag : 1;
        unsigned modbusTcpDebugEnableFlag : 1;
        unsigned logDebugEnableFlag : 1;
        unsigned gpsDebugEnableFlag : 1;

        unsigned isSensorConnected : 1;
        unsigned gotSensorDataCbCalled : 1;
        unsigned modbusResponseRx : 1;

        unsigned httpDatasend : 1;
        unsigned searchfirmware : 1;
        unsigned smsCmdRx : 1;
        unsigned resetRmu : 1;
        unsigned configJsonReplyDone : 1;
        unsigned ftpDownCerticates : 1;
        unsigned setApnErrorFlag : 1;
        unsigned formCurrPumpDataHttpLog : 1;
        unsigned formCurrPumpDataMqttLog : 1;
        unsigned sendCurrentLogOverHttp : 1;
        unsigned netDateTimeFix : 1;
        unsigned makeBluetoothOn : 1;
        unsigned setDTFirstTime : 1;
        unsigned bluetoothStatus : 1;
        unsigned httpSocketConnectionError : 1;
        unsigned httpTimeout : 1;
        unsigned flashError : 1;
        unsigned readCompleteDataCmdSend : 1;
        unsigned sendNwStrengthLcd : 1;
        unsigned AntennaError : 1;
        unsigned downloadSSlRootCaFile : 1;
        // Download SSL client certificate flag
        unsigned downloadSSlClientFile : 1;
        unsigned downloadSSlkeyFile : 1;
        unsigned sendRestartTimeToMqttTopic : 1;
        unsigned sendCommandRespOverMqttFLBroker : 1;
        unsigned isRtcSuccess : 1;

        unsigned modbusDataUpdatingSemaphore : 1;
        unsigned sendAttributesonce : 1;
        unsigned otaUpdateFailedFlag : 1;
        unsigned firstTimeGpsLockSetDefaultLoc : 1;
    };

} uSYSFLGS;

typedef union _SENSOR_FLAGS_
{
    uint32_t sensorFailedFlags;
    struct
    {

        unsigned isGetFM1_Data1ValueFailed : 1;
        // unsigned isGetFM1_Data2ValueFailed : 1;
        unsigned isGetFM2_Data1ValueFailed : 1;
        // unsigned isGetFM2_Data2ValueFailed : 1;
    };

} uFM_FLAGS;

typedef struct _SYS_PARAMS_
{
    uint64_t macId;
    uint tcpServerPort;
    uint mqttPort;
    int32_t signalStregth;
    uint8_t signalStregthLowest;
    uint32_t signalStregthLowOccurence;
} tSYSPARAMS;

typedef struct _DEV_ID_
{
    uint8_t id[6];
} tDEV_ID;

typedef struct _HB_PARAMS_
{
    uint8_t gsmNw;
    uint8_t sim;
    uint8_t gprsNw;
    uint8_t rssi;
    uint8_t sd;
    uint8_t online;
    uint8_t gps;
    uint8_t gpslock;
    double latitute;
    double longitude;
    uint8_t simSlot;
    uint16_t simSlotChangeCount;

} tHB_PARAMS;

typedef struct _RTC_DATE_TIME_
{
    char day[3];
    char month[3];
    char year[5];
    char hour[3];
    char min[3];
    char sec[3];
} tDT;

typedef struct _NET_DATE_TIME_
{
    uint16_t year;
    uint8_t day;
    uint8_t month;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} tNETWORKDT;

typedef struct _FM_DATA_  //FM = Flow Meter
{
    float flowRate;
    float totalizer;
    // int lowAlarm;
    // int highAlarm;

} tFM_DATA;

extern uint32_t connectionDelay;

extern bool isWifiNetClientSelected;

extern uint32_t conectToInternetFailedCount;
extern uint32_t modemRestartCount;

extern uint32_t httpConnectionErrorCount;
extern uint32_t flashErrorCount;

extern char timeStamp[20];
extern char rtcDate[15];
extern char rtcTime[15];
extern uint8_t currday;
extern char devStartTime[12];
extern char lastPacketSentSuccessTime[12];

extern String apSSID;
extern String apPASS;

extern String otaURL;

// extern efsdbLoggerHandle efsdbMqttLogHandle;
// extern efsdbLoggerHandle efsdbHttpLogHandle;

extern IPAddress local_IP, gateway_IP, subnet_Mask, p_dns, s_dns;
extern IPAddress modTcpClientIp;
// extern WiFiClient RemoteClient;

extern int apnMode;
extern String apnSim1, apnSim2;

// extern String uID;
// extern String uPass;
// extern String mID;
// extern String mPass;

extern String regionState;

extern unsigned long secondsSince2000, prevSecondsSince2000;
extern uint32_t unixTimeStamp;

extern String simID;

extern String deviceID;

extern bool downloadCertificateBySerialCmdFlag;
extern bool addServiceFlag;
extern bool wifiEnableFlag;
extern bool wifiSettingsResetFlag;
extern bool serverConFlg;
extern bool dnsStsFlg;
extern bool otaSetupFlg;

extern AsyncWebServer asyncWebServer;

extern tSYSPARAMS systemPara;
extern uSYSFLGS systemFlags;
extern tDEV_ID deviceAddress;
extern tHB_PARAMS hbParam;
extern tDT rtcDateTime;
extern tNETWORKDT netDateTime;
extern uFM_FLAGS fmFailedFlagStatus;

extern tFM_DATA fmData[2];

bool CheckForSerquenceNum(int *argc, char *argv[]);
void SpiffsInit();
void ButtonCallback(void);
void WebServerHandler();
void WebServerStart(void);
void WebServerStop(void);

void dnsInit(const char *MyName);
int32_t TwosCompliment(char *buffer, uint8_t sizeOfvalue);

#endif
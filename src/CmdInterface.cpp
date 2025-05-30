#include <CmdInterface.h>
#include <helper.h>
#include <IPAddress.h>
#include <globals.h>
#include "MCP7940.h"
#include "rtc.h"
#include <stdio.h>
#include "app.h"
#include "flWifiHandler.h"
#include "ota.h"
#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecSms.h"
#include <inttypes.h>

#include "ssl_cert.h"
#include "global-vars.h"
#include "user.h"

#include "app-mqtt-publish.h"
#include "esp32ModbusTCP.h"
#include "sensordata.h"
// #include <SoftwareSerial.h>
#include "pid-cntrl.h"
#include "user.h"
// extern SoftwareSerial softSerial;

enum _AutoApn
{
    SIM_MANUAL_APN_MODE = 0,
    SIM_AUTO_APN_MODE /* choose auto APN */
};

bool state = false; // need to set to false for actual working, for testing it is true.

extern char *msgData;

extern uint32_t acOutVoltage, acOutCurrent, outYphaseCurrent, ledFaults, rpm, dcInputVoltage, dcInputCurrent, totalDischargeWater, unknown, temperature;
extern uint32_t analog4, analog5, firmwareVer, firmwareRev, firmwareDate;

const char text_msg[] = "hello from device!!";

tParamEntry GetParamTable[] = {
    {"MAC", GetMac},

    {"WSSID", GetWiFiSSID},
    {"WPASS", GetWiFiPass},
    {"APSSID", GetAPSSID},
    {"APPASS", GetAPPass},
    {"WIFIMODE", GetWiFiMode},
    {"DHCP", GetDhcp},
    {"IPADDR", GetIpAddr},
    {"SUBNET", GetSubnet},
    {"GATEWAY", GetGateway},
    {"PDNS", GetPDns},
    {"SDNS", GetSDns},
    {"DNS", GetDns},

    {"MODTCPCLIENTIP", GetModTcpClientIpAddr},
    {"MODTCPCLIENTPORT", GetModTcpClientPort},

    {"UID", GetUID},
    {"UPASS", GetUPass},

    {"MODSLAVE1ADDR", GetModbusSlave1Addr},
    {"MODSLAVE2ADDR", GetModbusSlave2Addr},
    {"MODSLAVE3ADDR", GetModbusSlave3Addr},
    {"MODSLAVE4ADDR", GetModbusSlave4Addr},
    {"MODSLAVE5ADDR", GetModbusSlave5Addr},
    {"DO3MODADDR", GetDo3ModbusAddr},
    {"PH4MODADDR", GetPh4ModbusAddr},
    {"DO4MODADDR", GetDo4ModbusAddr},
    {"PH5MODADDR", GetPh5ModbusAddr},
    {"DO5MODADDR", GetDo5ModbusAddr},

    {"SLAVE1", GetModbusSlave1EnableStatus},
    {"SLAVE2", GetModbusSlave2EnableStatus},
    {"SLAVE3", GetModbusSlave3EnableStatus},
    {"SLAVE4", GetModbusSlave4EnableStatus},
    {"SLAVE5", GetModbusSlave5EnableStatus},

    {"FARM1NAME", GetFarm1Name},
    {"FARM2NAME", GetFarm2Name},
    {"FARM3NAME", GetFarm3Name},
    {"FARM4NAME", GetFarm4Name},
    {"FARM5NAME", GetFarm5Name},

    {"URATE", GetUpdateRate},
    {"HBINTERVAL", GetHBInterval},
    {"GPSLOCK", GetGpsLockDet},
    {"LOC", GetLocation},

    {"COMPORT", GetComPort},

    {"LOGSEND", GetLogSend},
    {"LOGSAVE", GetLogSave},
    {"LOGCNT", GetLogCount},
    {"LOG", GetLog},

    {"APNMODE", GetAPNMode},
    {"SIM1APN", GetAPNSim1},
    {"SIM2APN", GetAPNSim2},

    {"IMEI", GetIMEI},

    {"TIMEINT", GetLogTime},
    // {"STINTERVAL", GetStInterval},

    {"MQTTHOST", GetMqttHost},
    {"MQTTPORT", GetMqttPort},
    {"MQTTUSER", GetMQTTUser},
    {"MQTTPASS", GetMQTTPass},
    {"CLIENTID", GetMqttClientID},
    {"MQTTSSL", GetMqttSSL},
    {"MQTTCERT", GetMqttCertEnable},

    {"HBPUB", GetHBPubTopic},
    {"DATAPUB", GetDataPubTopic},
    {"OTPSUB", GetOtpSubTopic},
    {"INFOSUB", GetInfoSubTopic},
    {"CONFIGPUB", GetConfigPubTopic},
    {"ONDEMANDPUB", GetOndemandPubTopic},
    {"CONFIGSUB", GetConfigSubTopic},
    {"ONDEMANDSUB", GetOndemandSubTopic},

    {"FLHOST", GetFLMqttHost},
    {"FLMQTTPORT", GetFLMqttPort},
    {"FLMQTTUSER", GetFLMqttUser},
    {"FLMQTTPASS", GetFLMqttPass},
    {"FLCLIENTID", GetFLMqttClientID},
    {"FLPUBTOPIC", GetFLPubtopic},
    {"FLSUBTOPIC", GetFLSubtopic},
    {"FLHTTPPORT", GetFLHttpPort},
    {"FLHTTPURL", GetFLHttpUrl},
    {"FLHTTPTOKEN", GetFLHttpToken},

    {"PROTOCOL", GetDataTransferProtocol},
    {"IP", GetServerIP},
    {"PT", GetServPort},
    {"URL", GetUrl},
    {"TOKEN", GetDataServToken},

    {"DT", GetDateTime},

    {"DEVINFO", GetDevInfo},

    {"FLASHDET", GetFlashDet},
    {"SDDET", GetSDCardDetetct},
    {"RTCSTAT", GetRTCStatus},

    {"MAXFAILCOUNT", GetMaxFailedToConnectCount},
    {"MODEMRESETCOUNT", GetModemRestartCount},

    {"MODEMRSTCNT", GetModemResetCnt},
    {"PREVDAY", GetPrevDay},
    {"PREVTOTALIZER", GetPrevDayTotalizer},

    {"PENDPUB", GetAppPendingPubCnt},
    {"LORAMODE", GetDataSendMode},

    {0, 0}};

tParamEntry SetParamTable[] = {
    {"WSSID", SetWiFiSSID},
    {"WPASS", SetWiFiPass},
    {"APSSID", SetAPSSID},
    {"APPASS", SetAPPass},
    {"WIFIMODE", SetWiFiMode},
    {"DHCP", SetDhcp},
    {"IPADDR", SetIpAddr},
    {"SUBNET", SetSubnet},
    {"GATEWAY", SetGateway},
    {"PDNS", SetPDns},
    {"SDNS", SetSDns},
    {"DNS", SetDns},

    {"MODTCPCLIENTIP", SetModTcpClientIpAddr},
    {"MODTCPCLIENTPORT", SetModTcpClientPort},

    {"UID", SetUID},
    {"UPASS", SetUPass},

    {"MODSLAVE1ADDR", SetModbusSlave1Addr},
    {"MODSLAVE2ADDR", SetModbusSlave2Addr},
    {"MODSLAVE3ADDR", SetModbusSlave3Addr},
    {"MODSLAVE4ADDR", SetModbusSlave4Addr},
    {"MODSLAVE5ADDR", SetModbusSlave5Addr},
    {"DO3MODADDR", SetDo3ModbusAddr},
    {"PH4MODADDR", SetPh4ModbusAddr},
    {"DO4MODADDR", SetDo4ModbusAddr},
    {"PH5MODADDR", SetPh5ModbusAddr},
    {"DO5MODADDR", SetDo5ModbusAddr},

    {"SLAVE1", SetModbusSlave1EnableStatus},
    {"SLAVE2", SetModbusSlave2EnableStatus},
    {"SLAVE3", SetModbusSlave3EnableStatus},
    {"SLAVE4", SetModbusSlave4EnableStatus},
    {"SLAVE5", SetModbusSlave5EnableStatus},

    {"FARM1NAME", SetFarm1Name},
    {"FARM2NAME", SetFarm2Name},
    {"FARM3NAME", SetFarm3Name},
    {"FARM4NAME", SetFarm4Name},
    {"FARM5NAME", SetFarm5Name},

    {"URATE", SetUpdateRate},
    {"HBINTERVAL", SetHBInterval},
    {"LOC", SetLocation},

    {"COMPORT", SetComPort},

    {"LOGSEND", SetLogSend},
    {"LOGSAVE", SetLogSave},

    {"APNMODE", SetAPNMode},
    {"SIM1APN", SetAPNSim1},
    {"SIM2APN", SetAPNSim2},

    {"IMEI", SetIMEI},

    {"TIMEINT", SetLogTime},
    // {"STINTERVAL", SetStInterval},

    {"MQTTHOST", SetMqttHost},
    {"MQTTPORT", SetMqttPort},
    {"MQTTUSER", SetMQTTUser},
    {"MQTTPASS", SetMQTTPass},
    {"CLIENTID", SetMqttClientID},
    {"MQTTSSL", SetMqttSSL},
    {"MQTTCERT", SetMqttCertEnable},

    {"HBPUB", SetHBPubTopic},
    {"DATAPUB", SetDataPubTopic},
    {"OTPSUB", SetOtpSubTopic},
    {"INFOSUB", SetInfoSubTopic},
    {"CONFIGPUB", SetConfigPubTopic},
    {"ONDEMANDPUB", SetOndemandPubTopic},
    {"CONFIGSUB", SetConfigSubTopic},
    {"ONDEMANDSUB", SetOndemandSubTopic},

    {"FLHOST", SetFLMqttHost},
    {"FLMQTTPORT", SetFLMqttPort},
    {"FLMQTTUSER", SetFLMqttUser},
    {"FLMQTTPASS", SetFLMqttPass},
    {"FLCLIENTID", SetFLMqttClientID},
    {"FLPUBTOPIC", SetFLPubtopic},
    {"FLSUBTOPIC", SetFLSubtopic},
    {"FLHTTPPORT", SetFLHttpPort},
    {"FLHTTPURL", SetFLHttpUrl},
    {"FLHTTPTOKEN", SetFLHttpToken},

    {"PROTOCOL", SetDataTransferProtocol},
    {"IP", SetServerIP},
    {"PT", SetSerPort},
    {"URL", SetUrl},
    {"TOKEN", SetDataServToken},
    {"IMEITOKEN", SetIMEIasToken},

    {"DT", SetDateTime},

    {"HTTPLOG", SetSendCurrentHttpLog},
    {"MQTTLOG", SetSendCurrentMqttLog},
    {"TCPZER0", SetTcpZero},

    {"MAXFAILCOUNT", SetMaxFailedToConnectCount},
    {"MODEMRESETCOUNT", SetModemRestartCount},

    {"CONNECTDELAY", SetConnectionDelay},

    {"MODEMRSTCNT", SetModemResetCnt},
    {"PREVDAY", SetPrevDay},
    {"PREVTOTALIZER", SetPrevDayTotalizer},
    {"LORAMODE", SetDataSendMode},

    {0, 0}};

tCmdEntry cmdTable[] = {
    // Commands
    {"SET", NULL, SetParamTable},
    {"GET", NULL, GetParamTable},
    {"RESET", ResetDevice, NULL},
    {"DELLOG", DeleteLog, NULL},
    {"DEFLOG", ClearAllLogs, NULL},
    {"DEFSYS", RestoreFactorySettings, NULL},
    {"FORMAT", FormatPartition, NULL},

    {"DBG", DeepDebugDevice, NULL},
    {"APPDBG", DebugDevice, NULL},
    {"GSMDBG", GsmDebug, NULL},
    {"MODRTUDBG", ModbusRtuDebug, NULL},
    {"MODTCPDBG", ModbusTcpDebug, NULL},
    {"LOGDBG", LogDebug, NULL},

    // {"FLDSC", FlDeviceDiscovery, NULL},
    {"OTAURL", OtaUrlProcess, NULL},
    {"CACERTURL", DownloadSSLRootCa, NULL},
    {"KEYCERTURL", DownloadSSLKey, NULL},
    {"CLIENTCERTURL", DownloadSSLClient, NULL},
    {"TESTSRV", SetTestServer, NULL},
    {"FILEINFO", GetFileInfo, NULL},
    {"SENDSMS", SendTestMsg, NULL},
    {"BTON", BluetoothOn, NULL},

    {"DEFSYS2", RestoreFactorySettings2, NULL},

    {"FILECREATE", CreateFile, NULL},
    {"FILEDEL", DeleteFile, NULL},

    {0, 0, 0}};

static IPAddress TempIPAddress;

CMD_STATUS GetMac(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        CbPushS(respBuf, deviceID.c_str());

        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

/**
 * Command callback for assigning properties
 * */

CMD_STATUS SetWiFiSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], STSSID, 1);

    strcpy(gSysParam.staSsid, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetWiFiPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], STPASS, 1);

    strcpy(gSysParam.staPass, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    wifiSettingsResetFlag = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetAPSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], APSSID, 1);
    strcpy(gSysParam.apSsid, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetAPPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], APPASS, 1);
    strcpy(gSysParam.apPass, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // wifiSettingsResetFlag = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetWiFiMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (0 == strcmp("STA", argv[2]))
    {
        debugPrintln("WiFi mode enabled");
        gSysParam.ap4gOrWifiEn = true;
        wifiApDoneFlag = false;
        if (false == AppSetConfigSysParams(&gSysParam))
        {
        }
    }
    else if (0 == strcmp("AP", argv[2]))
    {
        debugPrintln("Hotspot mode enabled");
        gSysParam.ap4gOrWifiEn = false;
        wifiApDoneFlag = false;
        if (false == AppSetConfigSysParams(&gSysParam))
        {
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    wifiSettingsResetFlag = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetUID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.uID, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetUPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.uPass, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetDhcp(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if ((0 > atoi(argv[2])) && (1 < atoi(argv[2])))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], DHCP, 1);
    gSysParam.dhcp = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetSubnet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], SUBNET, 1);
    strcpy(gSysParam.subnet, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{

    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], IPADDR, 1);
    strcpy(gSysParam.ipAddr, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetGateway(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], GATEWAY, 1);
    strcpy(gSysParam.gateway, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetPDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], PDNS, 1);
    strcpy(gSysParam.pDns, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetSDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], SDNS, 1);
    strcpy(gSysParam.sDns, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (!String(argv[2]).c_str())
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], DNS, 1);
    strcpy(gSysParam.dns, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModTcpClientIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{

    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (!TempIPAddress.fromString(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // spiffsFile.ChangeToken(argv[2], IPADDR, 1);
    strcpy(gSysParam.modTcpClientIpAddr, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModTcpClientPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.modTcpClientPort = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetDateTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char date[7];
    char time[7];
    int j = 0;
    uint8_t day, month, year, hour, min, sec;
    uint32_t date1, time1;
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if (12 < strlen(argv[2]) || 12 > strlen(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_FAIL;
    }
    //  Set the RTC Time to 5:10:30 Nov 3 2020
    // example:
    // RTC.adjust(DateTime(2020,11,3,5,10,30));
    // HexStrToByteArr(argv[2],dateTime.dateTimeStr);
    if (12 < strlen(argv[2]) || 12 > strlen(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_FAIL;
    }

    for (int i = 0; i < 12; i++)
    {
        if (i < 6)
        {
            date[i] = argv[2][i];
        }
        else
        {
            time[j] = argv[2][i];
            j++;
        }
        date[6] = '\0';
        time[6] = '\0';
    }
    date1 = atoi(date);
    time1 = atoi(time);

    year = date1 % 100;
    date1 = date1 / 100;
    month = date1 % 100;
    date1 = date1 / 100;
    day = date1 % 100;
    date1 = date1 / 100;

    sec = time1 % 100;
    time1 = time1 / 100;
    min = time1 % 100;
    time1 = time1 / 100;
    hour = time1 % 100;
    time1 = time1 / 100;

    MCP7940.adjust(DateTime(2000 + year, month, day, hour, min, sec));
    MCP7940.setBattery(true); // enable battery backup mode
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetConnectionDelay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    String logTime;
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    if (5 < strlen(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 60) || (atoi(argv[2]) > 1800))
    {
        debugPrintln("Interval Range is 60 to 1800 seconds");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    connectionDelay = atoi(argv[2]);
    debugPrint("[DEBUG] Connection delay received: ");
    debugPrintln(connectionDelay);

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

// CMD_STATUS SetStInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
// {
//     //Check if no. of arguments is as desired.
//     if (3 != argSize)
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }

//     //Check If its a valid String.
//     if (NULL == argv[2])
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }

//     //Validating for a Numeric Vlaue.
//     if (0 == IsNum(argv[2]))
//     {
//         CbPushS(respBuf, BAD_PARAM_RESP);
//         return CMDLINE_BAD_PARAM;
//     }

//     //Validate Edge Condition i.e. positive value.
//     if ((atoi(argv[2]) < 0))
//     {
//         CbPushS(respBuf, ERROR_RESP);
//         return CMDLINE_FAIL;
//     }

//     spiffsFile.ChangeToken(argv[2], STINTERVAL, 1);
//     maxIndex = (60 / logTime) * 24;
//     spiffsFile.ChangeToken(String(maxIndex).c_str(), MAXINDEX, 1);
//     CbPushS(respBuf, OK_RESP);
//     return CMDLINE_SUCCESS;
// }

/**
 * Command callback for fetching properties
 * */

CMD_STATUS GetWiFiSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.staSsid);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetWiFiPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.staPass);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetAPSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.apSsid);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetAPPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.apPass);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetWiFiMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    const char *wifimode = "";
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (gSysParam.ap4gOrWifiEn)
    {
        wifimode = "STA";
    }
    else
    {
        wifimode = "AP";
    }
    CbPushS(respBuf, wifimode);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetUID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.uID);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetUPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.uPass);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave1Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor1ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModbusSlave1Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor1ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave2Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor2ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModbusSlave2Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor2ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave3Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor3ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModbusSlave3Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor3ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave4Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor4ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModbusSlave4Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor4ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave5Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor5ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModbusSlave5Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor5ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDo3ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor6ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetDo3ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor6ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetPh4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor7ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetPh4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor7ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDo4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor8ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetDo4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor8ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetPh5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor9ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetPh5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor9ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDo5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusAddr[10]; // modbusAddr size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.sensor10ModAddress, modbusAddr);
    CbPushS(respBuf, modbusAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetDo5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.sensor10ModAddress = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave1EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusslave1enableStat[10]; // modbusslave1enableStat size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modbusSlave1Enable, modbusslave1enableStat);
    CbPushS(respBuf, modbusslave1enableStat);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetModbusSlave1EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.modbusSlave1Enable = atoi(argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave2EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusslave2enableStat[10]; // modbusslave2enableStat size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modbusSlave2Enable, modbusslave2enableStat);
    CbPushS(respBuf, modbusslave2enableStat);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetModbusSlave2EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.modbusSlave2Enable = atoi(argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave3EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusslave3enableStat[10]; // modbusslave3enableStat size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modbusSlave3Enable, modbusslave3enableStat);
    CbPushS(respBuf, modbusslave3enableStat);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetModbusSlave3EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.modbusSlave3Enable = atoi(argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave4EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusslave4enableStat[10]; // modbusslave4enableStat size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modbusSlave4Enable, modbusslave4enableStat);
    CbPushS(respBuf, modbusslave4enableStat);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetModbusSlave4EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.modbusSlave4Enable = atoi(argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModbusSlave5EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modbusslave5enableStat[10]; // modbusslave5enableStat size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modbusSlave5Enable, modbusslave5enableStat);
    CbPushS(respBuf, modbusslave5enableStat);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetModbusSlave5EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.modbusSlave5Enable = atoi(argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetUpdateRate(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char updateRate[10]; // updateRate size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.uRate, updateRate);
    CbPushS(respBuf, updateRate);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetUpdateRate(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (!IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 >= atoi(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    gSysParam.uRate = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetHBInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char heartBInterval[10]; // heartBInterval size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.hbInterval, heartBInterval);
    CbPushS(respBuf, heartBInterval);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetHBInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if ((atoi(argv[2]) < 60) || (atoi(argv[2]) > 1800))
    {
        debugPrintln("Interval Range is 60 to 1800 seconds");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.hbInterval = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetComPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    char baudRate[10], startbit[10], stopbit[10], par[10]; // comport settings size is int
    uint32_t dummyLen;

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.baud485, baudRate);
    IntToAscii(gSysParam.dataBits, startbit);
    IntToAscii(gSysParam.stopBits, stopbit);
    IntToAscii(gSysParam.parit, par);

    CbPushS(respBuf, baudRate);
    CbPush(respBuf, ' ');
    CbPushS(respBuf, startbit);
    CbPush(respBuf, ' ');
    CbPushS(respBuf, stopbit);
    CbPush(respBuf, ' ');
    CbPushS(respBuf, par);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetComPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (6 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]) || (NULL == argv[4]) || (NULL == argv[5]))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if ((0 == IsNum(argv[2])) || (0 == IsNum(argv[3])) || (0 == IsNum(argv[4])) || (0 == IsNum(argv[5])))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0) || (atoi(argv[3]) < 5) || (atoi(argv[4]) < 1) || (atoi(argv[5]) < 0))
    {
        debugPrintln("Please enter valid baudrate like 9600, 57600, 115200 etc");
        debugPrintln("Startbits should be 5,6,7 & 8");
        debugPrintln("Stopbits should be 1 & 2");
        debugPrintln("Parity should be 0, 1 & 2");
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if ((atoi(argv[2]) == 1200) || (atoi(argv[2]) == 2400) || (atoi(argv[2]) == 4800) || (atoi(argv[2]) == 9600) ||
        (atoi(argv[2]) == 14400) || (atoi(argv[2]) == 19200) || (atoi(argv[2]) == 38400) || (atoi(argv[2]) == 56000) ||
        (atoi(argv[2]) == 57600) || (atoi(argv[2]) == 115200))
    {
    }
    else
    {
        debugPrintln("Please enter valid baudrate like 9600, 57600, 115200 etc");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    if (8 < atoi(argv[3]))
    {
        debugPrintln("Startbits should be 5,6,7 & 8");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (2 < atoi(argv[4]))
    {
        debugPrintln("Stopbits should be 1 & 2");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (2 < atoi(argv[5]))
    {
        debugPrintln("Parity should be 0, 1 & 2");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.baud485 = atoi(argv[2]);
    gSysParam.dataBits = atoi(argv[3]);
    gSysParam.stopBits = atoi(argv[4]);
    gSysParam.parit = atoi(argv[5]);

    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    Serial1.end();
    SetRs485UartConfigSettings(gSysParam.baud485, gSysParam.dataBits, gSysParam.stopBits, gSysParam.parit);
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetLogSend(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char logsend[10];
    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    uint32_t dummyLen;

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (0 == atoi(argv[2]))
    {
        IntToAscii(gSysParam.logSendMqtt, logsend);
    }
    else if (1 == atoi(argv[2]))
    {
        IntToAscii(gSysParam.logSendHttp, logsend);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    CbPushS(respBuf, logsend);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetLogSend(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    uint8_t arg3, arg4;
    // Check if no. of arguments is as desired.
    if (4 != argSize) // cmd ex. set logsend 0 1   (logsend for mqtt enabled)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[3]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    arg3 = atoi(argv[2]);
    arg4 = atoi(argv[3]);

    if ((arg3 == 0) && (arg4 == 0))
    {
        gSysParam.logSendMqtt = atoi(argv[3]);
        debugPrintln("mqtt log send disabled");
    }
    else if ((arg3 == 0) && (arg4 == 1))
    {
        gSysParam.logSendMqtt = atoi(argv[3]);
        debugPrintln("mqtt log send enabled");
    }
    else if ((arg3 == 1) && (arg4 == 0))
    {
        gSysParam.logSendHttp = atoi(argv[3]);
        debugPrintln("http log send disabled");
    }
    else if ((arg3 == 1) && (arg4 == 1))
    {
        gSysParam.logSendHttp = atoi(argv[3]);
        debugPrintln("http log send enabled");
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetLogSave(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char logsave[10];
    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    uint32_t dummyLen;

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (0 == atoi(argv[2]))
    {
        IntToAscii(gSysParam.logSaveMqtt, logsave);
    }
    else if (1 == atoi(argv[2]))
    {
        IntToAscii(gSysParam.logSaveHttp, logsave);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    CbPushS(respBuf, logsave);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetLogSave(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    uint8_t arg3, arg4;
    // Check if no. of arguments is as desired.
    if (4 != argSize) // cmd ex. set logsend 0 1   (logsend for mqtt enabled)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[3]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    arg3 = atoi(argv[2]);
    arg4 = atoi(argv[3]);

    if ((arg3 == 0) && (arg4 == 0))
    {
        gSysParam.logSaveMqtt = atoi(argv[3]);
        debugPrintln("mqtt log save disabled");
    }
    else if ((arg3 == 0) && (arg4 == 1))
    {
        gSysParam.logSaveMqtt = atoi(argv[3]);
        debugPrintln("mqtt log save enabled");
    }
    else if ((arg3 == 1) && (arg4 == 0))
    {
        gSysParam.logSaveHttp = atoi(argv[3]);
        debugPrintln("http log save disabled");
    }
    else if ((arg3 == 1) && (arg4 == 1))
    {
        gSysParam.logSaveHttp = atoi(argv[3]);
        debugPrintln("http log save enabled");
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetLogCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // char logcount[7];
    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // if (0 == atoi(argv[2]))
    // {
    //     uint32_t unsendLogCount = 0;
    //     if (efsdbLoggerGetCount(efsdbMqttLogHandle, &unsendLogCount))
    //     {
    //         // IntToAscii(unsendLogCount, logcount);
    //         // CbPushS(respBuf, logcount);
    //         CbPushS(respBuf, String(unsendLogCount).c_str());
    //     }
    //     else
    //     {
    //         debugPrintln("Log not available");
    //         CbPushS(respBuf, ERROR_RESP);
    //         return CMDLINE_FAIL;
    //     }
    // }
    // else
    if (1 == atoi(argv[2]))
    {
        uint32_t unsendLogCount = 0;
        if (LoggerGetCount(&unsendLogCount))
        {
            // IntToAscii(unsendLogCount, logcount);
            // CbPushS(respBuf, logcount);
            CbPushS(respBuf, String(unsendLogCount).c_str());
        }
        else
        {
            debugPrintln("Log not available");
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // MQTT_TRANSACTION _mqttSendTrans;
    TRANSACTION _httpSendTrans;
    String logPckt = "";

    if (3 == argSize || 4 == argSize)
    {
        // Validating for a Numeric Vlaue.
        if (0 == IsNum(argv[2]))
        {
            CbPushS(respBuf, BAD_PARAM_RESP);
            return CMDLINE_BAD_PARAM;
        }
        if (4 == argSize)
        {
            if (0 == IsNum(argv[3]))
            {
                CbPushS(respBuf, BAD_PARAM_RESP);
                return CMDLINE_BAD_PARAM;
            }
        }
        // if (0 == atoi(argv[2]))
        // {
        //     if (4 == argSize)
        //     {
        //         if (efsdbLoggerGetPacketByLogNumber(efsdbMqttLogHandle, _mqttSendTrans.array, atoi(argv[3])))
        //         {
        //             // CreateMqttJsonPacketToSend(&_mqttSendTrans, logPckt); // this will format the JSON packet
        //             CbPushS(respBuf, logPckt.c_str());
        //         }
        //         else
        //         {
        //             debugPrintln("No log available at this psition.");
        //             CbPushS(respBuf, ERROR_RESP);
        //             return CMDLINE_FAIL;
        //         }
        //     }
        //     else
        //     {
        //         if (efsdbLoggerGetPacket(efsdbMqttLogHandle, _mqttSendTrans.array))
        //         {
        //             // CreateMqttJsonPacketToSend(&_mqttSendTrans, logPckt); // this will format the JSON packet
        //             CbPushS(respBuf, logPckt.c_str());
        //         }
        //         else
        //         {
        //             debugPrintln("No logs available.");
        //             CbPushS(respBuf, ERROR_RESP);
        //             return CMDLINE_FAIL;
        //         }
        //     }
        // }
        // else
        if (1 == atoi(argv[2]))
        {
            if (4 == argSize)
            {
                if (LoggerGetPacketByLogNumber(&_httpSendTrans, atoi(argv[3])))
                {
                    CreateHttpPacketToSend(&_httpSendTrans, logPckt); // this will format the JSON packet
                    CbPushS(respBuf, logPckt.c_str());
                }
                else
                {
                    debugPrintln("No log available at this psition.");
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                if (LoggerGetPacketByLogNumber(&_httpSendTrans))
                {
                    CreateHttpPacketToSend(&_httpSendTrans, logPckt); // this will format the JSON packet
                    CbPushS(respBuf, logPckt.c_str());
                }
                else
                {
                    debugPrintln("No logs available.");
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
        }
        else
        {
            CbPushS(respBuf, BAD_ARG_RESP);
            return CMDLINE_BAD_ARGS;
        }

        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
}

CMD_STATUS GetAPNMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char apnmode[10]; // apnmode size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.apnMode, apnmode);
    CbPushS(respBuf, apnmode);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetAPNMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) == 0) || (atoi(argv[2]) == 1))
    {
        gSysParam.apnMode = atoi(argv[2]);
        apnMode = gSysParam.apnMode; // read apnmode into a globle variable
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetAPNSim1(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char apnName[18];

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (SIM_AUTO_APN_MODE == gSysParam.apnMode)
    {
        if (GetApn(apnName, sizeof(apnName)))
        {
            CbPushS(respBuf, String(apnName).c_str());
        }
        else
        {
            debugPrintln("APN is not set yet, wait for sometime.");
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, gSysParam.apn1);
    }
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetAPNSim1(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((String(argv[2]).isEmpty()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Check If String contains only numbers.
    if ((IsNum(argv[2])))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (SIM_AUTO_APN_MODE == gSysParam.apnMode)
    {
        debugPrintln("Auto Apn mode is selected, can't set APN.");
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    else
    {
        strcpy(gSysParam.apn1, argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (!SetAPN(gSysParam.apn1, NULL, NULL, SIM_1_APN))
        {
            systemFlags.setApnErrorFlag = true;
        }
        else
        {
            systemFlags.setApnErrorFlag = false;
        }
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetAPNSim2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char apnName[18];
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (SIM_AUTO_APN_MODE == gSysParam.apnMode)
    {
        if (GetApn(apnName, sizeof(apnName)))
        {
            CbPushS(respBuf, String(apnName).c_str());
        }
        else
        {
            debugPrintln("APN is not set yet, wait for sometime.");
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, gSysParam.apn2);
    }
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetAPNSim2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((String(argv[2]).isEmpty()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Check If String contains only numbers.
    if ((IsNum(argv[2])))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (SIM_AUTO_APN_MODE == gSysParam.apnMode)
    {
        debugPrintln("Auto Apn mode is selected, can't set APN.");
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    else
    {
        strcpy(gSysParam.apn2, argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (!SetAPN(gSysParam.apn2, NULL, NULL, SIM_2_APN))
        {
            systemFlags.setApnErrorFlag = true;
        }
        else
        {
            systemFlags.setApnErrorFlag = false;
        }
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetIMEI(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.imei);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetIMEI(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // Check If its a valid String.
    if (15 == strlen(argv[2]))
    {
        strcpy(gSysParam.imei, argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, OK_RESP);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetLogTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    char stIntervalStr[10];
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii((gSysParam.logTime), stIntervalStr);
    CbPushS(respBuf, stIntervalStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetLogTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    String logTime;
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    if (5 < strlen(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 60) || (atoi(argv[2]) > 1800))
    {
        debugPrintln("Interval Range is 60 to 1800 seconds");
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    gSysParam.logTime = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1MqttHostname);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1MqttHostname, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char mqttport[10];
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.serv1MqttPort, mqttport);
    CbPushS(respBuf, mqttport);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.serv1MqttPort = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMQTTUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1MqttUname);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMQTTUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1MqttUname, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMQTTPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1MqttPass);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMQTTPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1MqttPass, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1MqttClientId);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // check if value inputed is of desired lenght.
    //  if (36 != strlen(argv[2]))
    //  {
    //      CbPushS(respBuf, ERROR_RESP);
    //      return CMDLINE_FAIL;
    //  }

    // Validating for '-' at a specific location. eg
    // 8f9c6bf7-66cd-477f-aefa-d12b39716c9d

    // if (('-' != *(argv[2] + 8)) || ('-' != *(argv[2] + 13)) || ('-' != *(argv[2] + 18)) || ('-' != *(argv[2] + 23)))
    // {
    //     CbPushS(respBuf, ERROR_RESP);
    //     return CMDLINE_FAIL;
    // }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1MqttClientId, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMqttSSL(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    char serv1SslEnableStr[10];
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.serv1SslEnable, serv1SslEnableStr);
    CbPushS(respBuf, serv1SslEnableStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMqttSSL(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) > 2) || (atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.serv1SslEnable = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMqttCertEnable(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    char serv1CertEnableStr[10];
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.serv1CertEnable, serv1CertEnableStr);
    CbPushS(respBuf, serv1CertEnableStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetMqttCertEnable(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) > 2) || (atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.serv1CertEnable = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDataPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1DataPubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetDataPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1DataPubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetHBPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1HBPubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetHBPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1HBPubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetOtpSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1OtpSubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetOtpSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1OtpSubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetInfoSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1InfoSubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetInfoSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1InfoSubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetConfigSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1ConfigSubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetConfigSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1ConfigSubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetConfigPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1ConfigPubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetConfigPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1ConfigPubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetOndemandSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1OnDemandSubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetOndemandSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1OnDemandSubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetOndemandPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.serv1OnDemandPubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetOndemandPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.serv1OnDemandPubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerHost);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerHost, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    char mqttportStr[10];
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.flServerMqttPort, mqttportStr);
    CbPushS(respBuf, mqttportStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.flServerMqttPort = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLMqttUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerMqttUname);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLMqttUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerMqttUname, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLMqttPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerMqttPass);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLMqttPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerMqttPass, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerMqttClientId);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // check if value inputed is of desired lenght.
    //  if (36 != strlen(argv[2]))
    //  {
    //      CbPushS(respBuf, ERROR_RESP);
    //      return CMDLINE_FAIL;
    //  }

    // Validating for '-' at a specific location. eg
    // 8f9c6bf7-66cd-477f-aefa-d12b39716c9d

    // if (('-' != *(argv[2] + 8)) || ('-' != *(argv[2] + 13)) || ('-' != *(argv[2] + 18)) || ('-' != *(argv[2] + 23)))
    // {
    //     CbPushS(respBuf, ERROR_RESP);
    //     return CMDLINE_FAIL;
    // }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerMqttClientId, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLPubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerPubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLPubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerPubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLSubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerSubTopic);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLSubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerSubTopic, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLHttpPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    char httpportStr[10];
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.flServerHttpPort, httpportStr);
    CbPushS(respBuf, httpportStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLHttpPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    gSysParam.flServerHttpPort = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLHttpUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerHttpUrl);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLHttpUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (50 >= strlen(argv[2]))
    {
        strcpy(gSysParam.flServerHttpUrl, argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFLHttpToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.flServerHttpToken);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFLHttpToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.flServerHttpToken, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDataTransferProtocol(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char proto[10];
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.protocol, proto);
    CbPushS(respBuf, proto);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetDataTransferProtocol(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0) && atoi(argv[2]) > 3)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    gSysParam.protocol = atoi(argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetServerIP(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (1 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, gSysParam.dataServerIP);
    }
    else if (2 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, gSysParam.backupServerIP);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetServerIP(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (4 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if ((1 == atoi(argv[2])) || (2 == atoi(argv[2])))
    {
        if ((1 == atoi(argv[2])))
        {
            if (32 >= strlen(argv[3]))
            {
                strcpy(gSysParam.dataServerIP, argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        if ((2 == atoi(argv[2])))
        {
            if (32 >= strlen(argv[3]))
            {
                strcpy(gSysParam.backupServerIP, argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetServPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char tcpServerPort[10];

    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (1 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        IntToAscii(gSysParam.dataServerPort, tcpServerPort);
        CbPushS(respBuf, tcpServerPort);
    }
    else if (2 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        IntToAscii(gSysParam.backupServerPort, tcpServerPort);
        CbPushS(respBuf, tcpServerPort);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetSerPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (4 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if ((0 == IsNum(argv[2])) || (0 == IsNum(argv[3])))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if ((1 == atoi(argv[2])) || (2 == atoi(argv[2])))
    {
        if ((1 == atoi(argv[2])))
        {
            if (5 >= strlen(argv[3]))
            {
                // Validate Edge Condition i.e. positive value.
                if ((atoi(argv[3]) <= 0))
                {
                    CbPushS(respBuf, BAD_ARG_RESP);
                    return CMDLINE_BAD_ARGS;
                }
                gSysParam.dataServerPort = atoi(argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        if ((2 == atoi(argv[2])))
        {
            if (5 >= strlen(argv[3]))
            {
                // Validate Edge Condition i.e. positive value.
                if ((atoi(argv[3]) <= 0))
                {
                    CbPushS(respBuf, BAD_ARG_RESP);
                    return CMDLINE_BAD_ARGS;
                }
                gSysParam.backupServerPort = atoi(argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (1 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, gSysParam.dataServerUrl);
    }
    else if (2 == atoi(argv[2]))
    {
        uint32_t dummyLen;
        if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        if (dummyLen != sizeof(gSysParam.array))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
        CbPushS(respBuf, gSysParam.backupServerUrl);
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (4 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if ((1 == atoi(argv[2])) || (2 == atoi(argv[2])))
    {
        if ((1 == atoi(argv[2])))
        {
            if (50 >= strlen(argv[3]))
            {
                strcpy(gSysParam.dataServerUrl, argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        if ((2 == atoi(argv[2])))
        {
            if (50 >= strlen(argv[3]))
            {
                strcpy(gSysParam.backupServerUrl, argv[3]);
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                    CbPushS(respBuf, ERROR_RESP);
                    return CMDLINE_FAIL;
                }
            }
            else
            {
                CbPushS(respBuf, BAD_ARG_RESP);
                return CMDLINE_BAD_ARGS;
            }
        }
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS SetDataServToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (50 >= strlen(argv[2]))
    {
        strcpy(gSysParam.dataServerToken, argv[2]);
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetIMEIasToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    strcpy(gSysParam.dataServerToken, gSysParam.imei);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDataServToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.dataServerToken);

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDhcp(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char dhcpFlag[10];
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.dhcp, dhcpFlag);
    CbPushS(respBuf, dhcpFlag);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetSubnet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.subnet);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.ipAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetGateway(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.gateway);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetPDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.pDns);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetSDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.sDns);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS GetDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.dns);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModTcpClientIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.modTcpClientIpAddr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModTcpClientPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char modTcpPort[10];
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam.modTcpClientPort, modTcpPort);
    CbPushS(respBuf, modTcpPort);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDateTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    GetCurDateTime(timeStamp, rtcDate, rtcTime, &currday, &secondsSince2000, &unixTimeStamp);
    CbPushS(respBuf, timeStamp);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetDevInfo(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    String deviceID = "";

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, "Model: ");
    CbPushS(respBuf, DEVICE_MODEL);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "HW Version: ");
    CbPushS(respBuf, HW_VER);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "FW Version: ");
    CbPushS(respBuf, FW_VER);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "FW Build: ");
    CbPushS(respBuf, FW_BUILD);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "HARDWARE: ");
    CbPushS(respBuf, HARDWARE);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "IMEI: ");
    CbPushS(respBuf, gSysParam.imei);
    CbPushS(respBuf, "\n");
    CbPushS(respBuf, "Serial Number: ");

    for (uint8_t i = 0; i < sizeof(deviceAddress); i++)
    {
        if (10 > deviceAddress.id[i])
        {
            deviceID += "0";
            deviceID += String(deviceAddress.id[i]);
        }
        else
        {
            deviceID += String(deviceAddress.id[i], HEX);
        }
    }
    deviceID.toUpperCase();
    CbPushS(respBuf, deviceID.c_str());

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

// CMD_STATUS GetStInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
// {
//     char interval;
//     if (2 != argSize)
//     {
//         CbPushS(respBuf, ERROR_RESP);
//         return CMDLINE_FAIL;
//     }
//     IntToAscii(logTime, &interval);
//     CbPushS(respBuf, &interval);
//     CbPushS(respBuf, "\r\n");
//     CbPushS(respBuf, OK_RESP);
//     return CMDLINE_SUCCESS;
// }

/**
 * Command callback to soft Reset device.
 * */

CMD_STATUS ResetDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    ESP.restart();
    return CMDLINE_SUCCESS;
}

CMD_STATUS DeleteLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((NULL == argv[1]) || (NULL == argv[2]))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    uint16_t numOfLogsToDelete = atoi(argv[2]);
    if ((numOfLogsToDelete == 0) || (numOfLogsToDelete > 50000))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // if (0 == atoi(argv[1]))
    // {
    //     uint32_t unsendLogCount = 0;

    //     efsdbLoggerGetCount(efsdbMqttLogHandle, &unsendLogCount);

    //     if (numOfLogsToDelete > unsendLogCount)
    //     {
    //         debugPrintln("Sufficient logs are not available");
    //         CbPushS(respBuf, BAD_ARG_RESP);
    //         return CMDLINE_BAD_ARGS;
    //     }
    //     else
    //     {
    //         if (efsdbLoggerDeletePacket(efsdbMqttLogHandle, numOfLogsToDelete))

    //         {
    //             debugPrintln("Deleted Logs!");
    //         }
    //         else
    //         {
    //             debugPrintln("Fail to delete!");
    //             CbPushS(respBuf, ERROR_RESP);
    //             return CMDLINE_FAIL;
    //         }
    //     }
    // }
    // else
    if (1 == atoi(argv[1]))
    {
        uint32_t unsendLogCount = 0;
        LoggerGetCount(&unsendLogCount);
        if (numOfLogsToDelete > unsendLogCount)
        {
            debugPrintln("Sufficient logs are not available");
            CbPushS(respBuf, BAD_ARG_RESP);
            return CMDLINE_BAD_ARGS;
        }
        else
        {
            if (LoggerDeletePacket(numOfLogsToDelete))
            {
                debugPrintln("Deleted Logs!");
                // LoggerGetCount(&unsendLogCount);
                // softSerial.print("logcnt.txt=");
                // softSerial.print("\"");
                // softSerial.print(String(unsendLogCount));
                // softSerial.print("\"");
                // softSerial.write(0xFF);
                // softSerial.write(0xFF);
                // softSerial.write(0xFF);
            }
            else
            {
                debugPrintln("Fail to delete!");
                CbPushS(respBuf, ERROR_RESP);
                return CMDLINE_FAIL;
            }
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS ClearAllLogs(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // if (0 == atoi(argv[1]))
    // {
    //     efsdbLoggerFlush(efsdbMqttLogHandle);
    // }
    // else
    if (1 == atoi(argv[1]))
    {
        LoggerFlush();
    }
    else if (2 == atoi(argv[1]))
    {
        LoggerFlush();
        LoggerFlush();
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // uint32_t unsendLogCount = 0;
    // efsdbLoggerGetCount(efsdbHttpLogHandle, &unsendLogCount);
    // softSerial.print("logcnt.txt=");
    // softSerial.print("\"");
    // softSerial.print(String(unsendLogCount));
    // softSerial.print("\"");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS RestoreFactorySettings(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    AppMakeDefaultConfigSystemParams();
    ESP.restart();
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS FormatPartition(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (1 == atoi(argv[1]))
    {
        AppMakeDefaultConfigSystemParams();
        ESP.restart();
    }
    else if (2 == atoi(argv[1]))
    {
        // efsdbLoggerFlush(efsdbMqttLogHandle);
        LoggerFlush();
    }
    // This is causing ESP32 to be in hang after this command so commented this part
    //  else if (3 == atoi(argv[1]))
    //  {
    //      bool formatted = SPIFFS.format();

    //     if (formatted)
    //     {
    //         CbPushS(respBuf, "Success formatting!");
    //         CbPushS(respBuf, OK_RESP);
    //         return CMDLINE_SUCCESS;
    //     }
    //     else
    //     {
    //         CbPushS(respBuf, "Error formatting!");
    //         CbPushS(respBuf, ERROR_RESP);
    //         return CMDLINE_FAIL;
    //     }
    // }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS DeepDebugDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    state = !state;

    systemFlags.debugEnableFlag = state;
    systemFlags.modbusRtuDebugEnableFlag = state;
    systemFlags.modbusTcpDebugEnableFlag = state;
    systemFlags.gsmDebugEnableFlag = state;
    QuectelDebug(systemFlags.gsmDebugEnableFlag);
    systemFlags.gpsDebugEnableFlag = state;
    // ModRtuDebug1Enable(systemFlags.modbusRtuDebugEnableFlag);
    ModTcpDebug1Enable(systemFlags.modbusTcpDebugEnableFlag);
    if (true == state)
    {
        CbPushS(respBuf, "Deep debug enabled!\r\n");
        CbPushS(respBuf, "Reset device to disable\r\n");
    }
    else
    {
        CbPushS(respBuf, "Deep debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS DebugDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    systemFlags.debugEnableFlag = !systemFlags.debugEnableFlag;
    if (true == systemFlags.debugEnableFlag)
    {
        CbPushS(respBuf, "App debug enabled!\r\n");
    }
    else
    {
        CbPushS(respBuf, "App debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS ModbusRtuDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    systemFlags.modbusRtuDebugEnableFlag = !systemFlags.modbusRtuDebugEnableFlag;
    // ModRtuDebug1Enable(systemFlags.modbusRtuDebugEnableFlag);
    if (true == systemFlags.modbusRtuDebugEnableFlag)
    {
        CbPushS(respBuf, "Modbus RTU debug enabled!\r\n");
    }
    else
    {
        CbPushS(respBuf, "Modbus RTU debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GsmDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    systemFlags.gsmDebugEnableFlag = !systemFlags.gsmDebugEnableFlag;
    QuectelDebug(systemFlags.gsmDebugEnableFlag);
    if (true == systemFlags.gsmDebugEnableFlag)
    {
        CbPushS(respBuf, "GSM debug enabled!\r\n");
    }
    else
    {
        CbPushS(respBuf, "GSM debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS ModbusTcpDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    systemFlags.modbusTcpDebugEnableFlag = !systemFlags.modbusTcpDebugEnableFlag;
    ModTcpDebug1Enable(systemFlags.modbusTcpDebugEnableFlag);
    if (true == systemFlags.modbusTcpDebugEnableFlag)
    {
        CbPushS(respBuf, "Modbus TCP debug enabled!\r\n");
    }
    else
    {
        CbPushS(respBuf, "Modbus TCP debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS LogDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    systemFlags.logDebugEnableFlag = !systemFlags.logDebugEnableFlag;
    // efsdbLoggerDebug(efsdbMqttLogHandle, systemFlags.logDebugEnableFlag);
    // efsdbLoggerDebug(efsdbHttpLogHandle, systemFlags.logDebugEnableFlag);
    if (true == systemFlags.logDebugEnableFlag)
    {
        CbPushS(respBuf, "log debug enabled!\r\n");
    }
    else
    {
        CbPushS(respBuf, "log debug disabled!\r\n");
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

// CMD_STATUS FlDeviceDiscovery(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
// {
//     if (1 != argSize)
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }

//     String deviceID = "";

//     for (uint8_t i = 0; i < sizeof(deviceAddress); i++)
//     {
//         deviceID += String(deviceAddress.id[i], HEX);
//         if (i != 5)
//         {
//             deviceID += ":";
//         }
//     }
//     deviceID.toUpperCase();

//     CbPushS(respBuf, "FountLab Device\r\n");
//     CbPushS(respBuf, deviceID.c_str());
//     CbPushS(respBuf, "\r\n");
//     CbPushS(respBuf, dns.c_str());
//     CbPushS(respBuf, "\r\n");
//     return CMDLINE_SUCCESS;
// }

CMD_STATUS OtaUrlProcess(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Empty string.
    if (String(argv[1]).isEmpty())
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    if (true == gSysParam.ap4gOrWifiEn)
    {
        HTTPDownloadFirmware(argv[1]);
    }
    else
    {
        OtaFromUrl(argv[1]);
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS DownloadSSLRootCa(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Empty string.
    if (String(argv[1]).isEmpty())
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    debugPrintln("[DEBUG] Downloading SSL root ca file...");
    systemFlags.downloadSSlRootCaFile = true;
    downloadCertificateBySerialCmdFlag = true;
    if (SSLCertFromUrl(argv[1]))
    {
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
}

CMD_STATUS DownloadSSLKey(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Empty string.
    if (String(argv[1]).isEmpty())
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    debugPrintln("[DEBUG] Downloading SSL private key file...");
    systemFlags.downloadSSlkeyFile = true;
    downloadCertificateBySerialCmdFlag = true;
    if (SSLCertFromUrl(argv[1]))
    {
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
}

CMD_STATUS DownloadSSLClient(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Empty string.
    if (String(argv[1]).isEmpty())
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if (IsNum(argv[1]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    debugPrintln("[DEBUG] Downloading SSL client certificate file...");
    systemFlags.downloadSSlClientFile = true;
    downloadCertificateBySerialCmdFlag = true;
    if (SSLCertFromUrl(argv[1]))
    {
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
}

CMD_STATUS GetFlashDet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        if (systemFlags.flashError == true)
        {
            CbPush(respBuf, '0');
        }
        else
        {
            CbPush(respBuf, '1');
        }
        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetRTCStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (true == systemFlags.isRtcSuccess)
    {
        CbPush(respBuf, '1');
    }
    else
    {
        CbPush(respBuf, '0');
    }

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetSDCardDetetct(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        if (!hbParam.sd)
        {
            CbPush(respBuf, '0');
        }
        else
        {
            CbPush(respBuf, '1');
        }
        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS SetSIMSelect(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    bool sim1Present, sim2Present;
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    sim1Present = CheckSIM_Presence(SIM_SLOT_1);
    sim2Present = CheckSIM_Presence(SIM_SLOT_2);

    if (0 == (atoi(argv[2])))
    {
        if (sim1Present || sim2Present)
        {
            SelectSIM(SIM_SLOT_AUTO);
        }
        else
        {
            debugPrintln("SIM not detected in any slot");
            CbPushS(respBuf, BAD_PARAM_RESP);
            return CMDLINE_BAD_PARAM;
        }
    }
    else if (1 == (atoi(argv[2])))
    {
        if (sim1Present)
        {
            SelectSIM(SIM_SLOT_1);
        }
        else
        {
            debugPrintln("SIM not detected in selected slot");
            CbPushS(respBuf, BAD_PARAM_RESP);
            return CMDLINE_BAD_PARAM;
        }
    }
    else if (2 == (atoi(argv[2])))
    {
        if (sim2Present)
        {
            SelectSIM(SIM_SLOT_2);
        }
        else
        {
            debugPrintln("SIM not detected in selected slot");
            CbPushS(respBuf, BAD_PARAM_RESP);
            return CMDLINE_BAD_PARAM;
        }
    }
    else
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetSendCurrentHttpLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        systemFlags.formCurrPumpDataHttpLog = true;
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS SetSendCurrentMqttLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        systemFlags.formCurrPumpDataMqttLog = true;
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS SetTestServer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 == argSize)
    {
        systemFlags.formCurrPumpDataHttpLog = true;
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetFileInfo(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char files[50];

    if (1 == argSize)
    {
        File root = SPIFFS.open("/");
        File file = root.openNextFile();

        while (file)
        {
            snprintf(files, sizeof(files), "%s:%8u bytes", file.name(), file.size());
            CbPushS(respBuf, files);
            CbPushS(respBuf, "\r\n");
            file = root.openNextFile();
        }
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS SetTcpZero(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    uint32_t unsendLogCount = 0;
    if (2 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[1])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    LoggerGetCount(&unsendLogCount);
    LoggerFlush();

    CbPushS(respBuf, "Total logs:");
    CbPushS(respBuf, String(unsendLogCount).c_str());
    CbPush(respBuf, ',');
    CbPushS(respBuf, "deleted:");
    CbPushS(respBuf, String(unsendLogCount).c_str());
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SendTestMsg(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    bool status;

    // Check If its a valid String.
    // if (NULL == argv[1])
    // {
    //     CbPushS(respBuf, BAD_ARG_RESP);
    //     return CMDLINE_BAD_ARGS;
    // }

    debugPrintln(argSize);
    if (3 == argSize) // cmd ex. set logsend 0 1   (logsend for mqtt enabled)
    {
        status = SmsSendNow(argv[1], argv[2]);
        if (status)
        {
            CbPushS(respBuf, "SMS Sent\r\n");
            CbPushS(respBuf, OK_RESP);
        }
        else
        {
            CbPushS(respBuf, "SMS Send FAIL!\r\n");
            CbPushS(respBuf, OK_RESP);
        }
        return CMDLINE_SUCCESS;
    }
    else if (2 == argSize)
    {
        status = SmsSendNow(argv[1], text_msg);
        if (status)
        {
            CbPushS(respBuf, "SMS Sent\r\n");
            CbPushS(respBuf, OK_RESP);
        }
        else
        {
            CbPushS(respBuf, "SMS Send FAIL!\r\n");
            CbPushS(respBuf, OK_RESP);
        }
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS BluetoothOn(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    systemFlags.makeBluetoothOn = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetMaxFailedToConnectCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, String(conectToInternetFailedCount).c_str());
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModemRestartCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char mdeomRstCntStr[10]; // mdeomRstCnt size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams2(&gSysParam2, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam2.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam2.modemRstCnt, mdeomRstCntStr);
    CbPushS(respBuf, mdeomRstCntStr);

    // CbPushS(respBuf, String(modemRestartCount).c_str());
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetMaxFailedToConnectCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    conectToInternetFailedCount = atoi(argv[2]);
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModemRestartCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    gSysParam2.modemRstCnt = atoi(argv[2]);
    if (false == AppSetConfigSysParams2(&gSysParam2))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // spiffsFile.ChangeToken(argv[2], MODEM_RST_CNT, 1);
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetModemResetCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char mdeomRstCntStr[10]; // mdeomRstCnt size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams2(&gSysParam2, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam2.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam2.modemRstCnt, mdeomRstCntStr);
    CbPushS(respBuf, mdeomRstCntStr);

    // CbPushS(respBuf, String(modemRestartCount).c_str());
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetModemResetCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    gSysParam2.modemRstCnt = atoi(argv[2]);
    if (false == AppSetConfigSysParams2(&gSysParam2))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    // spiffsFile.ChangeToken(argv[2], MODEM_RST_CNT, 1);

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS RestoreFactorySettings2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (1 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    // restore default pump calculations
    uint32_t dummyReadSize = 0;
    AppMakeDefaultSysParams2();
    // AppGetConfigSysParams2(&gSysParam2, &dummyReadSize);
    ESP.restart();
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS DeleteFile(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        if (false == SPIFFS.remove(argv[1]))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }

        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS CreateFile(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize || 3 == argSize)
    {
        if (2 == argSize)
        {
            File file = SPIFFS.open(argv[1], FILE_WRITE);
            if (!file)
            {
                CbPushS(respBuf, "Failed to create file!");
                CbPushS(respBuf, ERROR_RESP);
                return CMDLINE_FAIL;
            }

            file.close();
        }
        else if (3 == argSize)
        {
            File file = SPIFFS.open(argv[1], FILE_WRITE);
            if (!file)
            {
                CbPushS(respBuf, "Failed to create file!");
                CbPushS(respBuf, ERROR_RESP);
                return CMDLINE_FAIL;
            }

            file.write((uint8_t *)argv[2], strlen(argv[2]));

            file.close();
        }
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetAppPendingPubCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char appPendPubStr[10]; // appMqttPendingPubCnt size is uint32_t
    uint32_t pendingPubCnt;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    pendingPubCnt = AppMqttPendingPub();
    IntToAscii(pendingPubCnt, appPendPubStr);
    CbPushS(respBuf, appPendPubStr);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetGpsLockDet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        if (!hbParam.gpslock)
        {
            CbPush(respBuf, '0');
        }
        else
        {
            CbPush(respBuf, '1');
        }
        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}

CMD_STATUS GetLocation(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 == argSize)
    {
        if (hbParam.gpslock)
        {
            CbPushS(respBuf, String(hbParam.latitute, 6).c_str());
            CbPush(respBuf, ',');
            CbPushS(respBuf, String(hbParam.longitude, 6).c_str());
        }
        else
        {
            CbPushS(respBuf, gSysParam.defaultLat);
            CbPush(respBuf, ',');
            CbPushS(respBuf, gSysParam.defaultLong);
        }
        CbPushS(respBuf, "\r\n");
        CbPushS(respBuf, OK_RESP);
        return CMDLINE_SUCCESS;
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
}
CMD_STATUS SetLocation(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char _lati[10];
    char _longi[10];

    // Check if no. of arguments is as desired.
    if (4 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.

    if ((0 == atof(argv[2])) || (0 == atof(argv[3])))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validating for a Numeric Vlaue.
    if ((9 != strlen(argv[2])) || (9 != strlen(argv[3])))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    strcpy(gSysParam.defaultLat, argv[2]);
    strcpy(gSysParam.defaultLong, argv[3]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFarm1Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.farm1Name);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFarm1Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.farm1Name, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // softSerial.print("farm1.val=");
    // softSerial.print("POND 1");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // send attributes for farm name change
    systemFlags.sendAttributesonce = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFarm2Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.farm2Name);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFarm2Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);

        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.farm2Name, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // softSerial.print("farm2.val=");
    // softSerial.print("POND 2");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // send attributes for farm name change
    systemFlags.sendAttributesonce = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFarm3Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.farm3Name);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFarm3Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.farm3Name, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // softSerial.print("farm3.val=");
    // softSerial.print("POND 3");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // send attributes for farm name change
    systemFlags.sendAttributesonce = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFarm4Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.farm4Name);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFarm4Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.farm4Name, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // softSerial.print("farm4.val=");
    // softSerial.print("POND 4");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // send attributes for farm name change
    systemFlags.sendAttributesonce = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetFarm5Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    uint32_t dummyLen;
    if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    CbPushS(respBuf, gSysParam.farm5Name);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetFarm5Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((!String(argv[2]).c_str()))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    strcpy(gSysParam.farm5Name, argv[2]);
    if (false == AppSetConfigSysParams(&gSysParam))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    // softSerial.print("farm5.val=");
    // softSerial.print("POND 5");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // send attributes for farm name change
    systemFlags.sendAttributesonce = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetPrevDay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char prevday[5]; // mdeomRstCnt size is uint32_t
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams2(&gSysParam2, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam2.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    IntToAscii(gSysParam2.prevDay, prevday);
    CbPushS(respBuf, prevday);

    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS SetPrevDay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Validating for a Numeric Vlaue.
    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    // Validate Edge Condition i.e. positive value.
    if ((atoi(argv[2]) < 0))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    gSysParam2.prevDay = atoi(argv[2]);
    if (false == AppSetConfigSysParams2(&gSysParam2))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

CMD_STATUS GetPrevDayTotalizer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    char fl1PrevTotalizer[12];
    char fl2PrevTotalizer[12];
    uint32_t dummyLen;

    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    if (false == AppGetConfigSysParams2(&gSysParam2, &dummyLen))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (dummyLen != sizeof(gSysParam2.array))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    Ftoa(gSysParam2.fl1_prevDayTotalizer, fl1PrevTotalizer, 4);
    Ftoa(gSysParam2.fl2_prevDayTotalizer, fl2PrevTotalizer, 4);
    CbPushS(respBuf, fl1PrevTotalizer);
    CbPushS(respBuf, " ");
    CbPushS(respBuf, fl2PrevTotalizer);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
CMD_STATUS SetPrevDayTotalizer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    // Check if no. of arguments is as desired.
    if (4 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if ((NULL == argv[2]) || (NULL == argv[3]))
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    gSysParam2.fl1_prevDayTotalizer = atof(argv[2]);
    gSysParam2.fl2_prevDayTotalizer = atof(argv[3]);
    if (false == AppSetConfigSysParams2(&gSysParam2))
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }

    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}

// CMD_STATUS SetDataSendMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
// {

//     // Check if no. of arguments is as desired.
//     if (3 != argSize)
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }

//     // Check If its a valid String.
//     if (NULL == argv[2])
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }

//     // Check If its a valid String.
//     if (!String(argv[2]).c_str())
//     {
//         CbPushS(respBuf, ERROR_RESP);
//         return CMDLINE_FAIL;
//     }

//     if (0 == strcmp("lora", argv[2]))
//     {
//         debugPrintln("Lora mode enabled");
//         gSysParam.loraOr4gEn = true;
//         //  wifiApDoneFlag = false;
//         if (false == AppSetConfigSysParams(&gSysParam))
//         {
//         }
//     }
//     else if (0 == strcmp("4g", argv[2]))
//     {
//         debugPrintln("4G mode enabled");
//         gSysParam.loraOr4gEn = false;
//         // wifiApDoneFlag = false;
//         if (false == AppSetConfigSysParams(&gSysParam))
//         {
//         }
//     }
//     else
//     {
//         CbPushS(respBuf, BAD_ARG_RESP);
//         return CMDLINE_BAD_ARGS;
//     }
//     // wifiSettingsResetFlag = true;
//     CbPushS(respBuf, OK_RESP);
//     return CMDLINE_SUCCESS;
// }

CMD_STATUS GetDataSendMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    const char *loramode = "";
    if (2 != argSize)
    {
        CbPushS(respBuf, ERROR_RESP);
        return CMDLINE_FAIL;
    }
    if (gSysParam.loraOr4gEn)
    {
        loramode = "LORA";
    }
    else
    {
        loramode = "4G";
    }
    CbPushS(respBuf, loramode);
    CbPushS(respBuf, "\r\n");
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}



CMD_STATUS SetDataSendMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf)
{
    uint8_t arg2;
    // Check if no. of arguments is as desired.
    if (3 != argSize)
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    // Check If its a valid String.
    if (NULL == argv[2])
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }

    if (0 == IsNum(argv[2]))
    {
        CbPushS(respBuf, BAD_PARAM_RESP);
        return CMDLINE_BAD_PARAM;
    }

    arg2 = atoi(argv[2]);

    if (1 == arg2)
    {
        debugPrintln("Lora mode enabled");
        gSysParam.loraOr4gEn = true;
        //  wifiApDoneFlag = false;
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else if (0 == arg2)
    {
        debugPrintln("4G mode enabled");
        gSysParam.loraOr4gEn = false;
        // wifiApDoneFlag = false;
        if (false == AppSetConfigSysParams(&gSysParam))
        {
            CbPushS(respBuf, ERROR_RESP);
            return CMDLINE_FAIL;
        }
    }
    else
    {
        CbPushS(respBuf, BAD_ARG_RESP);
        return CMDLINE_BAD_ARGS;
    }
    // wifiSettingsResetFlag = true;
    CbPushS(respBuf, OK_RESP);
    return CMDLINE_SUCCESS;
}
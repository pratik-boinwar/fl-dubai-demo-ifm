#include "netclient.h"
#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecHttp.h"
#include "QuecMqtt.h"
#include "QuecSsl.h"
#include "QuecFile.h"
#include "QuecFtp.h"
#include "QuecSms.h"

#include "FLToolbox.h"
#include "secrets.h"
#include "globals.h"
// #include "logger_espefs.h"
#include "app.h"
#include "ota.h"
#include "user.h"
#include "global-vars.h"
#include "config.h"
#include "app-mqtt-publish.h"
#include "sensordata.h"
#include "hmi-disp-nextion.h"
#include "fl-ethernet.h"

// variables used in process
String logToBeSend;
uint32_t mqttDataSendFailCount = 0, httpDataSendFailCount = 0;
char rxDataBuffer[512] = {0};
char httpCompleteUrl[200] = {0}; // http://serverip:port/url

char devBootTime[20];

char updatedTopic[512];

char cmdRxFromSms[MAX_SIZEOF_SMS_COMMAND];
char smsCh;
char mqttSubCh;
int smsCmdlineStat;
// String replyStrOverSms;
String replyStrOverFLMqttBroker;

CIRCULAR_BUFFER smsRespBuf;

extern CIRCULAR_BUFFER respBuf;

static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;

GsmDateTime_t *datetime;

// mqtt broker handle
MqttBrokerHandler _dataBroker = -1;
MqttBrokerHandler _flBroker = -1;

// ssl channel handle
SslContextHandle _sslContext;

char mqttCommandRespStr[MAX_SIZEOF_COMMAND];
uint16_t mqttCmdRespIndex = 0;
uint16_t mqttCmdRespStrLen = 0;
static StaticJsonDocument<200> _commandDoc;
static char _commandRxfromMqtt[MAX_SIZEOF_COMMAND];
static uAPP_MQTT_PUB _appMqttPub_t;

static String httpAttributePacket;

void MqttCallBack(MqttEvent_t mqttEvent, void *data);
void MqttCallBackForFLServer(MqttEvent_t mqttEvent, void *data);
int HttpCallBack(const char *httpRxData, uint32_t httpRxDataLen);
void FileDownloadCallBack(FileDownloadEvent_t fileEvent, const char *data, uint16_t dataLen);
uint16_t FileUploadCallBack(FileUploadEvent_t fileEvent, char *fileData, uint16_t fileDataLen);
void SmsCallback(const char *number, const char *sms, uint8_t smsLen);
void modemEventsCallBack(GsmGprsEvents_t event, void *data, uint8_t dataLen);
bool _findDirHasCertificateFile(const char *fileExtension, String *filename);

// mqtt call back
void MqttCallBack(MqttEvent_t mqttEvent, void *data)
{
    switch (mqttEvent)
    {
    case MQTT_BROKER_CONNECTED:
        break;

    case MQTT_BROKER_DISCONNECTED:
        break;

    case MQTT_BROKER_MSG_RECEIVED:
    {
        MqttRxMsg_t *mqttMsg;
        StaticJsonDocument<100> respDocument;

        mqttMsg = (MqttRxMsg_t *)data;
        debugPrintf("[MQTT] handle:%d\n", mqttMsg->brokerHandle);
        debugPrintf("[MQTT] msgid:%d\n", mqttMsg->msgId);
        debugPrintf("[MQTT] topic:%s\n", mqttMsg->topic);
        debugPrintf("[MQTT] payload:%s\n", mqttMsg->payload);

        Serial.println("received command from server1");

#ifdef ENABLE_4G
        RemoveQuotes((char *)mqttMsg->topic);
        RemoveQuotes((char *)mqttMsg->payload);
#endif

        if (0 == strcmp(gSysParam.serv1InfoSubTopic, mqttMsg->topic))
        {
            DeserializationError error = deserializeJson(_commandDoc, mqttMsg->payload);

            if (error)
            {
                debugPrint(F("deserializeJson() mqtt command json failed: "));
                debugPrintln(error.c_str());
                return;
            }
            if (_commandDoc.containsKey("cmd"))
            {
                if (false == _commandDoc["cmd"].isNull())
                {

                    const char *commandRx = _commandDoc["cmd"];
                    memset(_commandRxfromMqtt, 0, sizeof(_commandRxfromMqtt));
                    strncpy(_commandRxfromMqtt, commandRx, sizeof(_commandRxfromMqtt));
                    debugPrint("[MQTT] CMD received: ");
                    debugPrintln(_commandRxfromMqtt);
                    smsCmdlineStat = CmdLineProcess(_commandRxfromMqtt, &respBuf, (void *)&testType, CheckForSerquenceNum);

                    memset(mqttCommandRespStr, 0, sizeof(mqttCommandRespStr));
                    while (0 == CbPop(&respBuf, &mqttSubCh))
                    {
                        mqttCommandRespStr[mqttCmdRespIndex] = mqttSubCh;
                        mqttCmdRespIndex++;
                    }
                    mqttCmdRespStrLen = mqttCmdRespIndex;
                    mqttCmdRespIndex = 0;
                    mqttCommandRespStr[mqttCmdRespStrLen] = 0;
                    debugPrint("[MQTT] CMD response: ");
                    debugPrintln(mqttCommandRespStr);
                    CbClear(&respBuf);
#ifndef APP_MQTT_PUB_FUNCTIONALITY
                    systemFlags.sendCommandRespOverMqttFLBroker = true;
#else
                    _appMqttPub_t.handle = mqttMsg->brokerHandle;
                    _appMqttPub_t.qos = MQTT_QOS_LEAST_ONCE;
                    _appMqttPub_t.retain = true;
                    strcpy(_appMqttPub_t.topic, (const char *)gSysParam.serv1DataPubTopic);
                    strcpy(_appMqttPub_t.data, (const char *)mqttCommandRespStr);
                    _appMqttPub_t.dataLen = mqttCmdRespStrLen;
                    strcpy(_appMqttPub_t.infoStr, commandRx);
                    if (!AppMqttPublish(_appMqttPub_t))
                    {
                        debugPrintln("[MQTT] Adding cmd response to to queue failed!");
                    }
#endif
                }
            }
            else
            {
                debugPrintln("***********************");
                debugPrintln("Keyy not found..");
            }
        }
    }
    break;

    default:
        debugPrintf("Unhandled MQTT Event# %d\n", mqttEvent);
        break;
    }
}

void MqttCallBackForFLServer(MqttEvent_t mqttEvent, void *data)
{
    MqttRxMsg_t *mqttMsg;

    debugPrintln("[DEBUG] Mqtt callback executed for FL broker!");
    switch (mqttEvent)
    {
    case MQTT_BROKER_CONNECTED:
        debugPrintln("[DEBUG] Connected to FL broker!");
        break;

    case MQTT_BROKER_DISCONNECTED:
        debugPrintln("[DEBUG] Disconnected from FL broker!");
        break;

    case MQTT_BROKER_MSG_RECEIVED:
    {
        mqttMsg = (MqttRxMsg_t *)data;
        debugPrintf("[MQTT] handle:%d\n", mqttMsg->brokerHandle);
        debugPrintf("[MQTT] msgid:%d\n", mqttMsg->msgId);
        debugPrintf("[MQTT] topic:%s\n", mqttMsg->topic);
        debugPrintf("[MQTT] payload:%s\n", mqttMsg->payload);
#ifdef ENABLE_4G
        RemoveQuotes((char *)mqttMsg->topic);
        RemoveQuotes((char *)mqttMsg->payload);
#endif
        if (0 == strcmp(gSysParam.flServerSubTopic, mqttMsg->topic))
        {
            DeserializationError error = deserializeJson(_commandDoc, mqttMsg->payload);

            if (error)
            {
                debugPrint(F("deserializeJson() mqtt command json failed: "));
                debugPrintln(error.c_str());
                return;
            }
            const char *commandRx = _commandDoc["cmd"];
            memset(_commandRxfromMqtt, 0, sizeof(_commandRxfromMqtt));
            strncpy(_commandRxfromMqtt, commandRx, sizeof(_commandRxfromMqtt));
            debugPrint("[MQTT] CMD received: ");
            debugPrintln(_commandRxfromMqtt);
            smsCmdlineStat = CmdLineProcess(_commandRxfromMqtt, &respBuf, (void *)&testType, CheckForSerquenceNum);

            memset(mqttCommandRespStr, 0, sizeof(mqttCommandRespStr));
            while (0 == CbPop(&respBuf, &mqttSubCh))
            {
                mqttCommandRespStr[mqttCmdRespIndex] = mqttSubCh;
                mqttCmdRespIndex++;
            }
            mqttCmdRespStrLen = mqttCmdRespIndex;
            mqttCmdRespIndex = 0;
            mqttCommandRespStr[mqttCmdRespStrLen] = 0;
            debugPrint("[MQTT] CMD response: ");
            debugPrintln(mqttCommandRespStr);
            CbClear(&respBuf);
#ifndef APP_MQTT_PUB_FUNCTIONALITY
            systemFlags.sendCommandRespOverMqttFLBroker = true;
#else
            _appMqttPub_t.handle = mqttMsg->brokerHandle;
            _appMqttPub_t.qos = MQTT_QOS_LEAST_ONCE;
            _appMqttPub_t.retain = true;
            strcpy(_appMqttPub_t.topic, (const char *)gSysParam.flServerPubTopic);
            strcpy(_appMqttPub_t.data, (const char *)mqttCommandRespStr);
            _appMqttPub_t.dataLen = mqttCmdRespStrLen;
            strcpy(_appMqttPub_t.infoStr, commandRx);
            if (!AppMqttPublish(_appMqttPub_t))
            {
                debugPrintln("[MQTT] Adding cmd response to to queue failed!");
            }
#endif
        }
    }
    break;

    default:
        debugPrintf("Unhandled MQTT Event# %d\n", mqttEvent);
        break;
    }
}

// http call back
int HttpCallBack(HttpEvent_t httpEvent, const char *httpRxData, uint32_t httpRxDataLen)
{
    if (httpEvent == HTTP_EVENT_DWNLD_NEW_DATA)
    {
        debugPrintf("HTTP received length %d\n", httpRxDataLen);
        debugPrintf("HTTP Data -> %s\n", httpRxData);
    }

    return 1;
}

// sms call back
void SmsCallback(const char *number, const char *sms, uint8_t smsLen)
{
    char smsCommandRespStr[256];
    uint8_t smsRespIndex = 0;

    debugPrintf("[SMS] SMS received from %s\n", number);
    debugPrintf("[SMS] SMS -> %s\n", sms);
    // sprintf(cmdRxFromSms, "%s", sms);
    if (smsLen >= MAX_SIZEOF_SMS_COMMAND)
    {
        debugPrintln("[SMS] SMS data length is not valid");
    }

    strcpy(cmdRxFromSms, sms);
    debugPrint("[SMS] SMS RX CMD: ");
    debugPrintln(cmdRxFromSms);
    systemFlags.smsCmdRx = false;
    smsCmdlineStat = CmdLineProcess(cmdRxFromSms, &smsRespBuf, (void *)&testType, CheckForSerquenceNum);
    memset(smsCommandRespStr, 0, sizeof(smsCommandRespStr));

    while (0 == CbPop(&smsRespBuf, &smsCh))
    {
        smsCommandRespStr[smsRespIndex] = smsCh;
        smsRespIndex++;
    }
    smsCommandRespStr[smsRespIndex] = 0;
    memset(cmdRxFromSms, 0, sizeof(cmdRxFromSms));
    CbClear(&smsRespBuf);
    // replyStrOverSms.remove(replyStrOverSms.indexOf('\r'));
    // take action on sms. this sms will be no longer available here after.
    // beacuse it will be deleted.
    // Note, use SmsSendToQueue if it happens to send sms here.

    if (true == systemFlags.smsCmdRx) // send reply only when command received properly, systemFlags.smsCmdRx is set  from cmd.cpp.
    {
        debugPrint("[SMS] Cmd response over SMS: ");
        debugPrintln(smsCommandRespStr);
        if (!SmsSendToQueue(number, (const char *)smsCommandRespStr))
        {
            debugPrintln("[SMS] SMS que failed");
        }
    }
    else
    {
        debugPrintln("[SMS] Cmd not found in database");
    }
}

// file upload call back
uint16_t FileUploadCallBack(FileUploadEvent_t fileEvent, char *fileData, uint16_t fileDataLen)
{

    switch (fileEvent)
    {
    case FILE_UPLD_START:
        debugPrintln("file upload start");
        break;

    case FILE_UPLD_NEW_DATA:
        // here collect data and copy to 'data' and return size of 'data'
        return 0;
        break;

    case FILE_UPLD_END:
        debugPrintln("file upload end\n");
        break;

    default:
        break;
    }

    return 0;
}

// file download call back
void FileDownloadCallBack(FileDownloadEvent_t fileEvent, const char *data, uint16_t dataLen)
{
    static uint32_t _readBytes = 0;
    switch (fileEvent)
    {
    case FILE_DWNLD_START:
        _readBytes = 0;
        debugPrintln("file download start");
        break;

    case FILE_DWNLD_NEW_DATA:
        _readBytes += dataLen;
        debugPrintf("file data received length #%d\n", dataLen);
        break;

    case FILE_DWNLD_END:
        debugPrintf("total length read #%d\n", _readBytes);
        debugPrintln("file download end");
        break;

    default:
        break;
    }
}

void modemEventsCallBack(GsmGprsEvents_t event, void *data, uint8_t dataLen)
{
    String subtopic;
    String pubtopic;

    switch (event)
    {
    case QUEC_EVENT_POWER_ON:
        debugPrintln("[NC] modem powered ON");
        break;

    case QUEC_EVENT_CELL_INFO: // received latest cell info
    {
        CellInfo_t *cellInfo = (CellInfo_t *)data;
        debugPrintf("[NC] MCC= %d\n", cellInfo->mcc);
        debugPrintf("[NC] MNC= %d\n", cellInfo->mnc);
        debugPrintf("[NC] LAC= %d\n", cellInfo->lac);
        debugPrintf("[NC] CELLID= %d\n", cellInfo->cellId);
    }
    break;

    case QUEC_EVENT_CELL_LOCATION: // received latest cell location
        break;

    case QUEC_EVENT_NW_QUALITY: // received latest nw quality
    {
        hbParam.rssi = *((uint8_t *)data);
        debugPrintf("[NC] Nw Quality= %d\n", *((uint8_t *)data));
        // softSerial.print("nwkStat.txt=");
        // softSerial.print("\"");
        // softSerial.print((String)(hbParam.rssi));
        // softSerial.print("\"");
        // softSerial.write(0xFF);
        // softSerial.write(0xFF);
        // softSerial.write(0xFF);
    }
    break;

    case QUEC_EVENT_GSM_NW_REGISTERATION: // gsm nw status
        // DEBUG_MSG("[NC] GSM status = %d\n", *((uint8_t *)data));
        hbParam.gsmNw = *((uint8_t *)data);
        debugPrintf("[NC] GSM Nw %s\n", *((uint8_t *)data) ? "Registered" : "Registration Fail");
        break;

    case QUEC_EVENT_GPRS_NW_REGISTERATION: // gprs nw status
        // DEBUG_MSG("[NC] GPRS status = %d\n", *((uint8_t *)data));
        hbParam.gprsNw = *((uint8_t *)data);
        debugPrintf("[NC] GPRS %s\n", *((uint8_t *)data) ? "Connected" : "Disconnected");
        break;

    case QUEC_EVENT_NW_TIME: // received latest date time
    {
        datetime = (GsmDateTime_t *)data;
        debugPrintf("[NC] Date= %02d/", datetime->day);
        debugPrintf("%02d/", datetime->month);
        debugPrintf("%02d\n", datetime->year);
        debugPrintf("[NC] Time= %02d:", datetime->hour);
        debugPrintf("%02d:", datetime->minute);
        debugPrintf("%02d\n", datetime->second);
    }
    break;

    case QUEC_EVENT_IP_ADDRESS: // internet connect/dis-connect
        debugPrintf("[NC] Local IP= %s\n", (char *)data);
        break;

    case QUEC_EVENT_SIM_ID: // received sim id
        simID = String((char *)data);
        debugPrintf("[NC] SIM ID= %s\n", (char *)data);
        break;

    case QUEC_EVENT_SIM_SELECTED: // received sim selection
        hbParam.simSlot = *((uint8_t *)data);
        debugPrintf("[NC] SIM %d selected\n", *((uint8_t *)data));
        break;

    case QUEC_EVENT_NW_OPERATOR: // received NW operator name
        debugPrintf("[NC] Operator= %s\n", (char *)data);
        // select APN according to operator name
        break;

    case QUEC_EVENT_IMEI:        // received imei
        if (IsNum((char *)data)) // just to verify imei received
        {

#ifndef HARDCODED_IMEI
            strcpy(gSysParam.imei, (const char *)data);
#else
            strcpy(gSysParam.imei, HARD_IMEI);
#endif

            if (false == AppSetConfigSysParams(&gSysParam))
            {
            }

            debugPrintf("[NC] IMEI= %s\n", gSysParam.imei);
            strcpy(gSysParam.flServerHttpToken, (const char *)gSysParam.imei);
            strcpy(gSysParam.flServerMqttClientId, (const char *)gSysParam.imei);
            if (NULL != strstr(gSysParam.dataServerToken, "token"))
            {
                strcpy(gSysParam.dataServerToken, (const char *)gSysParam.imei);
            }

            subtopic = String(gSysParam.imei);
            subtopic += "/";
            subtopic += "commands";
            strcpy(gSysParam.flServerSubTopic, subtopic.c_str());

            pubtopic = String(gSysParam.imei);
            pubtopic += "/";
            pubtopic += "commandsResp";
            strcpy(gSysParam.flServerPubTopic, pubtopic.c_str());

            // update default credentials and pub/sub topics of server1 for first time
            if (NULL != strstr(gSysParam.serv1MqttUname, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1MqttUname, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("mqtt username updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1MqttUname, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1MqttClientId, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1MqttClientId, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("mqtt clientid updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1MqttClientId, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1InfoSubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1InfoSubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("info sub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1InfoSubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1OtpSubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1OtpSubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("otp sub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1OtpSubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1OnDemandSubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1OnDemandSubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("ondemand sub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1OnDemandSubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1ConfigSubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1ConfigSubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("config sub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1ConfigSubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1DataPubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1DataPubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("data pub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1DataPubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1HBPubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1HBPubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("hb pub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1HBPubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1OnDemandPubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1OnDemandPubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("ondemand pub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1OnDemandPubTopic, updatedTopic);
            }
            if (NULL != strstr(gSysParam.serv1ConfigPubTopic, "imei"))
            {
                memset(updatedTopic, 0, sizeof(updatedTopic));
                ReplaceWordInString(gSysParam.serv1ConfigPubTopic, "imei", gSysParam.imei, updatedTopic);
                debugPrintf("config pub topic updated: %s\r\n", updatedTopic);
                strcpy(gSysParam.serv1ConfigPubTopic, updatedTopic);
            }

            if (false == AppSetConfigSysParams(&gSysParam))
            {
            }

            if (false == gSystemFlag.connectToFlbrokerOnlyAfterResetFlag)
            {
                // make this flag true to not call mqtt connect function again untill device gets restart
                gSystemFlag.connectToFlbrokerOnlyAfterResetFlag = true;
                _flBroker = MqttCreateConnetion((const char *)gSysParam.flServerHost, gSysParam.flServerMqttPort, (const char *)gSysParam.flServerMqttClientId,
                                                (const char *)gSysParam.flServerMqttUname, (const char *)gSysParam.flServerMqttPass, true, 300, &MqttCallBackForFLServer);
                if (_flBroker < 0)
                {
                    debugPrintf("MQTT FL broker handler not created, error# %d\n", _flBroker);
                }
                else
                {
                    // MqttSubscribeTopic(_flBroker, flServerSubTopic.c_str(), MQTT_QOS_LEAST_ONCE);
                }

                if (0 > MqttSubscribeTopic(_flBroker, (const char *)gSysParam.flServerSubTopic, MQTT_QOS_LEAST_ONCE))
                {
                    debugPrintln("[DEBUG] Mqtt subcription to commands FL subtopic fail.");
                }
            }
        }
        break;

    default:
        break;
    }
}

enum netClientState
{
    NC_CHECK_EVENTS = 0,
    NC_HTTP_WAIT_FOR_LOG_SEND,
    NC_HTTP_WAIT_FOR_FIRMWARE_INFO,
    NC_HTTP_WAIT_FOR_ATTRIBUTE_SEND,
    NC_FTP_WAIT_FOR_UPLOAD_COMPLETE,
    NC_FTP_WAIT_FOR_DOWNLOAD_COMPLETE,
    NC_TAKE_REST,
    NC_TAKE_POWER_NAP
};

void netclientSetup()
{
    // Initializes Quectel library, function should be called before any other operations.
    QuectelInit();

    /// apply "GSMDBG" to enable/disable
    QuectelDebug(false);

    // modem initialization
    if (false == gSysParam.apnMode)
    {
        SetAPN((const char *)gSysParam.apn1, NULL, NULL, SIM_1_APN);
        // SetAPN((const char *)gSysParam.apn2, NULL, NULL, SIM_2_APN);
    }
    systemFlags.setApnErrorFlag = true; // flag that shows apn is set correct or not.

    // subscribe modem events
    QuectelSubscribeEvents(modemEventsCallBack);

    // subscribed to SMS callback
    SmsSubscribeCallback(SmsCallback);

#ifdef APP_MQTT_PUB_FUNCTIONALITY
    AppMqttPubInit();
#endif
}

void NetclientLoop()
{
    static uint8_t _checkEvent = 0;
    static netClientState _ncState = NC_CHECK_EVENTS;
    static uint32_t _generalTimeoutTick = 0, _powerNapTimeoutTick = 0;
    HttpStatus_t httpStatus;
    StaticJsonDocument<96> restartInfoDoc;
    String restartInfoOverMqtt;
    String restartInfoTopic;
    int status;
    TRANSACTION _httpSendTrans;
    StaticJsonDocument<512> httpRespdoc;
    char commandFromHttp[300];
    StaticJsonDocument<800> attributeDoc;

    // This function should be called as frequently as possible
    QuectelLoop();

    // Reset gsm modem if http fails for long time
    if (MAX_NO_OF_TIMES_FAILED_TO_CONNECT_INTERNET < httpDataSendFailCount)
    {
        debugPrintln("\n*********************************************************");
        debugPrintln("[NET] Modem fails to connect to Http, restarting modem...");
        debugPrintln("*********************************************************\n");
        // reset both protocol fails counts
        httpDataSendFailCount = 0;
        mqttDataSendFailCount = 0;

        gSysParam2.modemRstCnt++;
        if (false == AppSetConfigSysParams2(&gSysParam2))
        {
        }
        ResetModem();
    }

    // Reset gsm modem if mqtt fails for long time
    if (MAX_NO_OF_TIMES_FAILED_TO_CONNECT_INTERNET < mqttDataSendFailCount)
    {
        debugPrintln("\n*********************************************************");
        debugPrintln("[NET] Modem fails to connect to Mqtt, restarting modem...");
        debugPrintln("*********************************************************\n");
        // reset both protocol fails counts
        httpDataSendFailCount = 0;
        mqttDataSendFailCount = 0;

        gSysParam2.modemRstCnt++;
        if (false == AppSetConfigSysParams2(&gSysParam2))
        {
        }
        ResetModem();
    }

    switch (_ncState)
    {
    case NC_CHECK_EVENTS: // check various internet related events to process.
        switch (_checkEvent)
        {
        case 0: // send restart info to FL broker
            _checkEvent++;

            if (!(IsMqttConnected(_flBroker) && IsModemReady()))
            {
                break;
            }

            if (true == systemFlags.sendRestartTimeToMqttTopic)
            {
                debugPrintln("[NET] Sending Restart info over mqtt topic...");
                restartInfoDoc["TIMESTAMP"] = devBootTime;
                restartInfoDoc["IMEI"] = gSysParam.imei;
                restartInfoDoc["VER"] = FW_VER;
                restartInfoDoc["VER_UPDATE_DATE"] = FW_BUILD;
                restartInfoOverMqtt = "";
                serializeJson(restartInfoDoc, restartInfoOverMqtt);
                debugPrintln(restartInfoOverMqtt);

                restartInfoTopic = DEVICE_MODEL;
                restartInfoTopic += "/";
                restartInfoTopic += "restartinfo";

                status = MqttPublishTopic(_flBroker, restartInfoTopic.c_str(), MQTT_QOS_LEAST_ONCE, true,
                                          restartInfoOverMqtt.c_str(), restartInfoOverMqtt.length());
                if (1 == status)
                {
                    systemFlags.sendRestartTimeToMqttTopic = false;
                    debugPrintln("[NET] Restart packet Sent to fl broker.");
                    mqttDataSendFailCount = 0;
                }
                else
                {
                    debugPrintf("[NET] Restart packet sending failed! error# %d\n", status);
                    mqttDataSendFailCount++;
                }
            }
            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;
            break;

        case 1: // send log data packet over mqtt
            _checkEvent++;

            // if (!gSysParam.logSendMqtt) // check added for data send over mqtt
            //     break;

            // if (!(IsMqttConnected(_dataBroker) && IsModemReady()))
            // {
            //     hbParam.online = 0; // device is not online
            //     break;
            // }

            // hbParam.online = 1; // device is online

            // if (!efsdbLoggerIsPacketAvailable(efsdbMqttLogHandle))
            //     break;

            // logToBeSend = ""; // make clear

            // debugPrintln();
            // debugPrintln("[NET] Retriving Data json from mqttlog...");

            //             if (PrepareMqttJsonPacket(logToBeSend))
            //             {
            //                 debugPrintln("[NET] Data packet found! Sending it over MQTT");
            //                 debugPrintln(logToBeSend);
            //                 status = MqttPublishTopic(_dataBroker, (const char *)gSysParam.serv1DataPubTopic, MQTT_QOS_LEAST_ONCE, true,
            //                                           logToBeSend.c_str(), logToBeSend.length());
            //                 if (1 == status)
            //                 {
            //                     debugPrintln("[NET] data packet Sent");
            //                     efsdbLoggerDeletePacket(efsdbMqttLogHandle);
            //                     mqttDataSendFailCount = 0;
            //                 }
            //                 else
            //                 {
            //                     debugPrintf("[NET] data sending failed! error# %d\n", status);
            //                     mqttDataSendFailCount++;
            //                 }
            //                 debugPrintln();
            //                 _generalTimeoutTick = millis();
            //                 _ncState = NC_TAKE_REST;
            //                 break;
            //             }

            // debugPrintln();
            // _generalTimeoutTick = millis();
            // _ncState = NC_TAKE_REST;
            break;

        case 2: // send data over http
            _checkEvent++;

            if (!gSysParam.logSendHttp) // check added for data send over http
                break;

            // check if http client avaiaibilty

            if (!(IsHttpClientAvailable() && IsModemReady()))
            {
                break;
            }

            if (!LoggerIsPacketAvailable())
                break;

            logToBeSend = ""; // make clear

            debugPrintln();
            debugPrintln("[DEBUG] Retriving Data json from httplog file...");

            if (LoggerGetPacketByLogNumber(&_httpSendTrans))
            {
                CreateHttpPacketToSend(&_httpSendTrans, logToBeSend); // this will format the JSON packet
                debugPrint("[DEBUG] Data packet found! Sending it over HTTP... ");
                debugPrintln(logToBeSend);
                strcpy(httpCompleteUrl, "http://");
                strcat(httpCompleteUrl, gSysParam.dataServerIP);
                // strcat(httpCompleteUrl, ":");
                // char portStr[6];
                // IntToAscii(port, portStr);
                // strcat(httpCompleteUrl, portStr);
                if (*gSysParam.dataServerUrl != '/')
                    strcat(httpCompleteUrl, "/");
                strcat(httpCompleteUrl, gSysParam.dataServerUrl);
                if (*gSysParam.dataServerToken != '/')
                    strcat(httpCompleteUrl, "/");
                strcat(httpCompleteUrl, gSysParam.dataServerToken);
                strcat(httpCompleteUrl, "/telemetry");

                if (!HttpSendData(httpCompleteUrl, logToBeSend.c_str(), logToBeSend.length(), rxDataBuffer, sizeof(rxDataBuffer),
                                  HTTP_METHOD_POST, HTTP_CONTENT_APPLICATION_JSON, NULL, 0, 0))
                {
                    debugPrintln("[DEBUG] Http data send failed!");
                    debugPrintln();
                    break;
                }
                memset(rxDataBuffer, 0, sizeof(rxDataBuffer));
                _generalTimeoutTick = millis();
                _ncState = NC_HTTP_WAIT_FOR_LOG_SEND;
            }
            else
            {
                _generalTimeoutTick = millis();
                _ncState = NC_TAKE_REST;
            }
            break;

        case 3:
            _checkEvent++;

            // check if http client avaiaibilty
            if (!(IsHttpClientAvailable() && IsModemReady()))
            {
                break;
            }

            if (true == systemFlags.sendAttributesonce)
            {
                attributeDoc["RESTART_TIME"] = devStartTime;
                attributeDoc["VER"] = FW_VER;
                attributeDoc["VER_UPDATE_DATE"] = FW_BUILD;
                attributeDoc["SIMID"] = simID.c_str();
                if (true == hbParam.gpslock)
                {
                    attributeDoc["LATITUDE"] = hbParam.latitute;
                    attributeDoc["LONGITUDE"] = hbParam.longitude;
                }
                else
                {
                    attributeDoc["LATITUDE"] = gSysParam.defaultLat;
                    attributeDoc["LONGITUDE"] = gSysParam.defaultLong;
                }
                if (1 == gSysParam.modbusSlave1Enable)
                {
                    attributeDoc["DN25_FLOW_METER_DATA1_ERROR"] = fmFailedFlagStatus.isGetFM1_Data1ValueFailed;
                    //  attributeDoc["DN25_FLOW_METER_DATA2_ERROR"] = fmFailedFlagStatus.isGetFM1_Data2ValueFailed;
                }
                if (1 == gSysParam.modbusSlave2Enable)
                {
                    attributeDoc["DN40_FLOW_METER_DATA1_ERROR"] = fmFailedFlagStatus.isGetFM2_Data1ValueFailed;
                    // attributeDoc["DN40_FLOW_METER_DATA2_ERROR"] = fmFailedFlagStatus.isGetFM2_Data2ValueFailed;
                }

                httpAttributePacket = "";
                serializeJson(attributeDoc, httpAttributePacket);
                debugPrintln("\n[DEBUG] Sending Attribute packet..");
                debugPrintln(httpAttributePacket.c_str());

                strcpy(httpCompleteUrl, "http://");
                strcat(httpCompleteUrl, gSysParam.dataServerIP);
                // strcat(httpCompleteUrl, ":");
                // char portStr[6];
                // IntToAscii(port, portStr);
                // strcat(httpCompleteUrl, portStr);
                if (*gSysParam.dataServerUrl != '/')
                    strcat(httpCompleteUrl, "/");
                strcat(httpCompleteUrl, gSysParam.dataServerUrl);
                if (*gSysParam.dataServerToken != '/')
                    strcat(httpCompleteUrl, "/");
                strcat(httpCompleteUrl, gSysParam.dataServerToken);
                strcat(httpCompleteUrl, "/attributes");

                if (!HttpSendData(httpCompleteUrl, httpAttributePacket.c_str(), httpAttributePacket.length(), rxDataBuffer, sizeof(rxDataBuffer),
                                  HTTP_METHOD_POST, HTTP_CONTENT_APPLICATION_JSON, NULL, 0, 0))
                {
                    debugPrintln("[DEBUG] Http Attribute send failed!");
                    debugPrintln();
                    break;
                }
                memset(rxDataBuffer, 0, sizeof(rxDataBuffer));
                _generalTimeoutTick = millis();
                _ncState = NC_HTTP_WAIT_FOR_ATTRIBUTE_SEND;
            }
            else
            {
                _generalTimeoutTick = millis();
                _ncState = NC_TAKE_REST;
            }
            break;

        case 4: // check for new firmware
            _checkEvent++;
            // break;

            // check if http client avaiaibilty
            if (!(IsHttpClientAvailable() && IsModemReady()))
            {
                break;
            }

            if (systemFlags.searchfirmware)
            {
                /* create JSON */
                StaticJsonDocument<512> root;

                root["APPNAME"] = APP_NAME;
                root["DEVICE"] = DEVICE_MODEL;
                root["HWID"] = HW_ID;
                root["HWV"] = HW_VER;
                root["FWV"] = FW_VER;
                root["IMEI"] = gSysParam.imei;

                logToBeSend = "";
                serializeJson(root, logToBeSend);
                // debugPrintln(logToBeSend);

                strcpy(httpCompleteUrl, "http://");
                strcat(httpCompleteUrl, (const char *)gSysParam.flServerHost);
                strcat(httpCompleteUrl, ":");
                char portStr[6];
                IntToAscii(gSysParam.flServerHttpPort, portStr);
                strcat(httpCompleteUrl, portStr);
                strcat(httpCompleteUrl, "/");
                strcat(httpCompleteUrl, "getUpdates");

                if (!HttpSendData(httpCompleteUrl, logToBeSend.c_str(), logToBeSend.length(), rxDataBuffer, sizeof(rxDataBuffer),
                                  HTTP_METHOD_POST, HTTP_CONTENT_APPLICATION_JSON, NULL, 0, 0))
                {
                    debugPrintln("[NET] Firmware info failed!");
                    break;
                }

                /**
                 * Note that we are clearing flag here. we are not caring if this http session succeeds or fail.
                 * We do not want to keep process busy doing this searching task.
                 * Anyhow we are going to be check it on peridoically, may be on hourly basis.
                 */
                systemFlags.searchfirmware = 0;

                memset(rxDataBuffer, 0, sizeof(rxDataBuffer));
                debugPrintln("[NET] Searching Firmware...");
                _generalTimeoutTick = millis();
                _ncState = NC_HTTP_WAIT_FOR_FIRMWARE_INFO;
            }
            break;

        case 5: // publish command response if not sent yet
            _checkEvent++;

#ifndef APP_MQTT_PUB_FUNCTIONALITY
            if (!(IsMqttConnected(_flBroker) && IsModemReady()))
            {
                break;
            }

            if (true == systemFlags.sendCommandRespOverMqttFLBroker)
            {
                status = MqttPublishTopic(_flBroker, (const char *)gSysParam.flServerPubTopic, MQTT_QOS_LEAST_ONCE, true,
                                          (const char *)mqttCommandRespStr, mqttCmdRespStrLen);
                if (1 == status)
                {
                    systemFlags.sendCommandRespOverMqttFLBroker = false;
                    debugPrintln("[NET] CMD response packet Sent");
                    mqttDataSendFailCount = 0;
                }
                else
                {
                    debugPrintf("[NET] CMD response sending failed! error# %d\n", status);
                    mqttDataSendFailCount++;
                }
                debugPrintln();
            }

#else
            // AppMqttPubLoop();
#endif
            break;

        default:
            _checkEvent = 0;
            _ncState = NC_TAKE_REST;
            break;
        }

        break;

    case NC_HTTP_WAIT_FOR_LOG_SEND: // waiting send log over http
        httpStatus = HttpGetLastStatus();
        if (httpStatus == HTTP_STATUS_SUCCESS)
        {
            snprintf(lastPacketSentSuccessTime, sizeof(lastPacketSentSuccessTime), "%s%s%s-%s%s", rtcDateTime.day, rtcDateTime.month, rtcDateTime.year,
                     rtcDateTime.hour, rtcDateTime.min);
            debugPrintln("[NET] Data log sent");

            LoggerDeletePacket();

            // // update logcnt on screen
            // uint32_t unsendLogCount = 0;
            // efsdbLoggerGetCount(efsdbHttpLogHandle, &unsendLogCount);
            // softSerial.print("logcnt.txt=");
            // softSerial.print("\"");
            // softSerial.print(String(unsendLogCount));
            // softSerial.print("\"");
            // softSerial.write(0xFF);
            // softSerial.write(0xFF);
            // softSerial.write(0xFF);

            systemFlags.httpSocketConnectionError = false;
            systemFlags.httpTimeout = false;
            httpDataSendFailCount = 0;

            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;

            // take action rx buffer
            if (rxDataBuffer[0] != 0)
            {
                debugPrint("\nReceived: ");
                debugPrintf("%s\n", rxDataBuffer);
            }
            debugPrintln();
        }
        else if (httpStatus == HTTP_STATUS_FAILED)
        {
            httpDataSendFailCount++;
            httpConnectionErrorCount++;
            systemFlags.httpSocketConnectionError = true;
            debugPrintln("[NET] Data log send failed!");
            debugPrintln();

            _powerNapTimeoutTick = millis();
            _ncState = NC_TAKE_POWER_NAP;
        }
        if ((millis() - _generalTimeoutTick) > 130000)
        {
            httpDataSendFailCount++;
            httpConnectionErrorCount++;
            systemFlags.httpTimeout = true;
            debugPrintln("[NET] Timeout. no response from HTTP Client");
            debugPrintln();
            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;
            break;
        }
        break;

    case NC_HTTP_WAIT_FOR_FIRMWARE_INFO:
        httpStatus = HttpGetLastStatus();
        if (httpStatus == HTTP_STATUS_SUCCESS)
        {
            debugPrintln("[NET] Firmware Info success!");
            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;
            systemFlags.httpSocketConnectionError = false;
            systemFlags.httpTimeout = false;
            // clear the flag
            systemFlags.searchfirmware = false;
            httpDataSendFailCount = 0;

            // take action rx buffer
            if (rxDataBuffer[0] != 0)
            {
                debugPrint("\nReceived: ");
                debugPrintf("%s\n", rxDataBuffer);
                DeserializationError error = deserializeJson(httpRespdoc, rxDataBuffer, strlen(rxDataBuffer));

                if (error)
                {
                    debugSmt(Serial.print(F("[DEBUG] deserializeJson() failed of data received from HTTP: ")););
                    debugPrintln(error.c_str());
                }
                else
                {

                    uint32_t status = httpRespdoc["statusCode"];

                    if (200 != status)
                    {
                        debugPrintln("[DEBUG] Get updates request failed, status coderx is not 200");
                    }

                    if (httpRespdoc.containsKey("otaurl"))
                    {
                        if (false == httpRespdoc["otaurl"].isNull())
                        {
                            debugPrintln("[DEBUG] OTA URL recieved from http response.");
                            otaURL = httpRespdoc["otaurl"].as<String>(); // "http://165.22.221.43:3030/api/getFile/firmware_v1.1.bin"
                            OtaFromUrl(otaURL.c_str());
                        }
                    }
                    if (httpRespdoc.containsKey("command"))
                    {
                        if (false == httpRespdoc["command"].isNull())
                        {
                            debugPrintln("[DEBUG] Command recieved from http response.");
                            String command = httpRespdoc["command"].as<String>(); // "get wssid"
                            command.toCharArray(commandFromHttp, sizeof(commandFromHttp));
                            SerialCommandFromHttp(commandFromHttp); // call cmdline function
                        }
                    }
                }
            }
        }
        else if (httpStatus == HTTP_STATUS_FAILED)
        {
            // httpConnectionErrorCount++;
            systemFlags.httpSocketConnectionError = true;
            debugPrintln("[NET] Firmware info failed!");

            _powerNapTimeoutTick = millis();
            _ncState = NC_TAKE_POWER_NAP;
        }
        if ((millis() - _generalTimeoutTick) > 130000)
        {
            httpConnectionErrorCount++;
            debugPrintln("[NET] Timeout. no response from HTTP Client");
            systemFlags.httpTimeout = true;
            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;
            break;
        }
        break;

    case NC_HTTP_WAIT_FOR_ATTRIBUTE_SEND: // waiting send log over http
        httpStatus = HttpGetLastStatus();
        if (httpStatus == HTTP_STATUS_SUCCESS)
        {
            snprintf(lastPacketSentSuccessTime, sizeof(lastPacketSentSuccessTime), "%s%s%s-%s%s", rtcDateTime.day, rtcDateTime.month, rtcDateTime.year,
                     rtcDateTime.hour, rtcDateTime.min);
            debugPrintln("[HTTP] Attributes log sent");

            systemFlags.sendAttributesonce = false;

            systemFlags.httpSocketConnectionError = false;
            systemFlags.httpTimeout = false;
            httpDataSendFailCount = 0;

            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;

            // take action rx buffer
            if (rxDataBuffer[0] != 0)
            {
                debugPrint("\n[HTTP] Received: ");
                debugPrintf("%s\n", rxDataBuffer);
            }
            debugPrintln();
        }
        else if (httpStatus == HTTP_STATUS_FAILED)
        {
            httpDataSendFailCount++;
            httpConnectionErrorCount++;
            systemFlags.httpSocketConnectionError = true;
            debugPrintln("[HTTP] Attributes log send failed!");
            debugPrintln();

            _powerNapTimeoutTick = millis();
            _ncState = NC_TAKE_POWER_NAP;
        }
        if ((millis() - _generalTimeoutTick) > 130000)
        {
            httpDataSendFailCount++;
            httpConnectionErrorCount++;
            systemFlags.httpTimeout = true;
            debugPrintln("[HTTP] Timeout. no response from HTTP Client");
            debugPrintln();
            _generalTimeoutTick = millis();
            _ncState = NC_TAKE_REST;
            break;
        }
        break;

    case NC_TAKE_REST: // take some rest
        if ((millis() - _generalTimeoutTick) > 1000)
        {
            _ncState = NC_CHECK_EVENTS;
        }
        break;

    case NC_TAKE_POWER_NAP: // take some rest
        if ((millis() - _powerNapTimeoutTick) > 10000)
        {
            _ncState = NC_CHECK_EVENTS;
        }
        break;

    default:
        _ncState = NC_CHECK_EVENTS;
        break;
    }
}
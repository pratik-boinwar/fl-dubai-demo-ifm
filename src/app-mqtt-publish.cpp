#include "app-mqtt-publish.h"
#include "globals.h"

static uAPP_MQTT_PUB _appMqttPubArr[APP_MQTT_PUB_QUEUE_SIZE];

void AppMqttPubLoop(void)
{
    static uint32_t _pubIndex = 0;
    int32_t _status = 0;

    if (true == _appMqttPubArr[_pubIndex].pubStatus)
    {
        if ((IsMqttConnected(_appMqttPubArr[_pubIndex].handle) && IsModemReady()))
        {
            if (strlen(_appMqttPubArr[_pubIndex].topic) < APP_MQTT_PUB_TOPIC_SIZE)
            {
                _status = MqttPublishTopic(_appMqttPubArr[_pubIndex].handle, (const char *)_appMqttPubArr[_pubIndex].topic, _appMqttPubArr[_pubIndex].qos, _appMqttPubArr[_pubIndex].retain,
                                           (const char *)_appMqttPubArr[_pubIndex].data, _appMqttPubArr[_pubIndex].dataLen);
                if (1 == _status)
                {
                    _appMqttPubArr[_pubIndex].pubStatus = false;
                    _appMqttPubArr[_pubIndex].pubFailCount = 0;
                    debugPrintf("[APP_MQTT_PUB] Data publish on server %d success", _appMqttPubArr[_pubIndex].handle);
                    debugPrintf(" for cmd: %s\n\n", _appMqttPubArr[_pubIndex].infoStr);
                }
                else
                {
                    _appMqttPubArr[_pubIndex].pubFailCount++;
                    debugPrintf("[APP_MQTT_PUB] Data publish on server %d failed", _appMqttPubArr[_pubIndex].handle);
                    debugPrintf(" for cmd: %s\n\n", _appMqttPubArr[_pubIndex].infoStr);
                    debugPrintf("[APP_MQTT_PUB] error status: %d\n", _status);
                    debugPrintf("[APP_MQTT_PUB] Failed count: %d\n", _appMqttPubArr[_pubIndex].pubFailCount);
                    if (_appMqttPubArr[_pubIndex].pubFailCount > APP_MQTT_MODEM_FAILS_TO_SEND_DATA_MAX_CNT)
                    {
                        if (true == ResetModem())
                        {
                            _appMqttPubArr[_pubIndex].pubStatus = 0;
                            debugPrintln("\n[APP_MQTT_PUB] Modem restarted!");
                            _appMqttPubArr[_pubIndex].pubFailCount = 0;
                        }
                    }
                }
            }
            else
            {
                debugPrintln("[APP_MQTT_PUB] Topic size is more than APP_MQTT_PUB_TOPIC_SIZE");
                _appMqttPubArr[_pubIndex].pubStatus = 0;
            }
        }
    }

    _pubIndex++;
    if (_pubIndex > APP_MQTT_PUB_QUEUE_SIZE)
    {
        _pubIndex = 0;
    }
}

void AppMqttPubInit(void)
{
    uint32_t i;

    for (i = 0; i < APP_MQTT_PUB_QUEUE_SIZE; i++)
    {
        _appMqttPubArr[i].pubStatus = 0;
        _appMqttPubArr[i].pubFailCount = 0;
    }
}

bool AppMqttPublish(uAPP_MQTT_PUB appMqttPub)
{
    uint32_t i;

    for (i = 0; i < APP_MQTT_PUB_QUEUE_SIZE; i++)
    {
        if (false == _appMqttPubArr[i].pubStatus)
        {
            // _appMqttPubArr[i] = appMqttPub;
            memcpy(_appMqttPubArr[i].array, appMqttPub.array, sizeof(_appMqttPubArr[i].array));
            _appMqttPubArr[i].pubStatus = 1;
            return true;
        }
    }

    return false;
}

uint32_t AppMqttPendingPub(void)
{
    uint32_t i = 0, pendingPubCnt = 0;

    for (i = 0; i < APP_MQTT_PUB_QUEUE_SIZE; i++)
    {
        if (1 == _appMqttPubArr[i].pubStatus)
        {
            pendingPubCnt++;
        }
    }
    return pendingPubCnt;
}

void AppMqttPubPrint(void)
{
    uint32_t i = 0;

    for (i = 0; i < APP_MQTT_PUB_QUEUE_SIZE; i++)
    {
        if (1 == _appMqttPubArr[i].pubStatus)
        {
            debugPrintln("*****************************************************************");
            debugPrintf("[APP_MQTT_PUB] Server: %d\n", _appMqttPubArr[i].handle);
            debugPrintf("[APP_MQTT_PUB] Topic: %s\n", _appMqttPubArr[i].topic);
            debugPrintf("[APP_MQTT_PUB] QoS: %d\n", _appMqttPubArr[i].qos);
            debugPrintf("[APP_MQTT_PUB] Retain: %d\n", _appMqttPubArr[i].retain);
            debugPrintf("[APP_MQTT_PUB] Data: %s\n", _appMqttPubArr[i].data);
            debugPrintf("[APP_MQTT_PUB] DataLen: %d\n", _appMqttPubArr[i].dataLen);
            debugPrintf("[APP_MQTT_PUB] InfoStr: %s\n", _appMqttPubArr[i].infoStr);
            debugPrintf("[APP_MQTT_PUB] PubStatus: %d\n", _appMqttPubArr[i].pubStatus);
            debugPrintf("[APP_MQTT_PUB] PubFailCount: %d\n", _appMqttPubArr[i].pubFailCount);
            debugPrintln("*****************************************************************");
            debugPrintln();
        }
    }
}
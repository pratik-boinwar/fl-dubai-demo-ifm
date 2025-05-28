#include "app-mqtt.h"
#include "app.h"
#include "global-vars.h"
#include "globals.h"
#include <PubSubClient.h>
#include "rtc.h"

extern CIRCULAR_BUFFER respBuf;

long MqttLastReconnectMillis = 0;

WiFiClient WiFiMqttClient;
PubSubClient client(WiFiMqttClient);

void MqttCallback(char *topic, byte *payload, unsigned int length);
bool MqttReconnect(void);

void MqttCallback(char *topic, byte *payload, unsigned int length)
{
    PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;
    char mqttCommandRespStr[512];
    uint16_t mqttCmdRespIndex = 0;
    uint16_t mqttCmdRespStrLen = 0;
    static StaticJsonDocument<200> _commandDoc;
    static char _commandRxfromMqtt[MAX_SIZEOF_COMMAND];
    char mqttSubCh;
    int mqttCmdlineStat;

    // handle message arrived
    Serial.print("[MQTT] Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (0 == strcmp(topic, gSysParam.flServerSubTopic))
    {
        DeserializationError error = deserializeJson(_commandDoc, (char *)payload, length);

        if (error)
        {
            debugPrint(F("[MQTT] deserializeJson() mqtt command json failed: "));
            debugPrintln(error.c_str());
            return;
        }

        const char *commandRx = _commandDoc["cmd"];
        memset(_commandRxfromMqtt, 0, sizeof(_commandRxfromMqtt));
        strncpy(_commandRxfromMqtt, commandRx, sizeof(_commandRxfromMqtt));
        debugPrint("[MQTT] CMD received: ");
        debugPrintln(_commandRxfromMqtt);
        mqttCmdlineStat = CmdLineProcess(_commandRxfromMqtt, &respBuf, (void *)&testType, CheckForSerquenceNum);

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

        client.publish(gSysParam.flServerPubTopic, mqttCommandRespStr);
        CbClear(&respBuf);
    }

    else
    {
        debugPrintln("[MQTT] Unknown topic");
    }
}

bool MqttReconnect(void)
{
    debugPrint("\n[MQTT] Attempting connection to Mqtt broker: ");
    debugPrint(gSysParam.flServerHost);
    debugPrint(":");
    debugPrintln(gSysParam.flServerMqttPort);
    if (client.connect(gSysParam.flServerMqttClientId, gSysParam.flServerMqttUname, gSysParam.flServerMqttPass))
    {
        debugPrintln("[MQTT] Connection success!");
        debugPrint("[MQTT] Subscribing to flServerSubTopic: ");
        debugPrintln(gSysParam.flServerSubTopic);
        if (!client.subscribe(gSysParam.flServerSubTopic))
        {
            debugPrintln("[MQTT] Subscribe failed!");
        }
    }
    return client.connected();
}

void AppMqttInit(void)
{
    client.setServer(gSysParam.flServerHost, gSysParam.flServerMqttPort);
    client.setCallback(MqttCallback);
}

void AppMqttLoop(void)
{
    if (!client.connected())
    {
        // retry every 5s if connection failed
        if (millis() - MqttLastReconnectMillis > 5000)
        {
            MqttLastReconnectMillis = millis();
            // Attempt to MqttReconnect
            if (!MqttReconnect())
            {
                debugPrint("failed, rc=");
                debugPrintln(client.state());
                debugPrintln("retry again in 5 seconds");
            }
        }
    }
    else
    {
        // Client connected
        client.loop();
    }
}

bool AppMqttIsConnected(void)
{
    return client.connected();
}

bool AppMqttGetMqttStatus(void)
{
    return client.state();
}

bool AppMqttSubscribe(const char *topic)
{
    return client.subscribe(topic);
}

bool AppMqttPublish(const char *topic, const char *payload)
{
    return client.publish(topic, payload);
}

#ifndef APP_MQTT_PUBLISH_H
#define APP_MQTT_PUBLISH_H

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecMqtt.h"

#define APP_MQTT_PUB_TOPIC_SIZE 256
#define APP_MQTT_PUB_DATA_SIZE 1024
#define APP_MQTT_PUB_INFOSTR_SIZE 256
// Queue size should not exceed above value 10, It will affect Certificates files read write(SPIFFS operation)
#define APP_MQTT_PUB_QUEUE_SIZE 10
#define APP_MQTT_MODEM_FAILS_TO_SEND_DATA_MAX_CNT 20

typedef union _APP_MQTT_PUB_
{
    unsigned char array[sizeof(MqttBrokerHandler) + (sizeof(char) * APP_MQTT_PUB_TOPIC_SIZE) + sizeof(MqttQos_t) + sizeof(bool) +
                        (sizeof(char) * APP_MQTT_PUB_DATA_SIZE) + sizeof(uint32_t) + sizeof(bool) + sizeof(uint32_t) +
                        (sizeof(char) * APP_MQTT_PUB_INFOSTR_SIZE)];

    struct
    {
        MqttBrokerHandler handle;
        char topic[APP_MQTT_PUB_TOPIC_SIZE];
        MqttQos_t qos;
        bool retain;
        char data[APP_MQTT_PUB_DATA_SIZE]; // if we change size to 512 code works fine
        uint32_t dataLen;
        bool pubStatus;
        uint32_t pubFailCount;
        char infoStr[APP_MQTT_PUB_INFOSTR_SIZE];
    };
} uAPP_MQTT_PUB;

void AppMqttPubInit(void);
bool AppMqttPublish(uAPP_MQTT_PUB appMqttPub);
void AppMqttPubLoop(void);
uint32_t AppMqttPendingPub(void);
void AppMqttPubPrint(void);

#endif
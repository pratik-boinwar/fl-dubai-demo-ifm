/**
 * @file       QuecMqtt.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECMQTT_H
#define QUECMQTT_H

#ifdef ENABLE_4G
#define MQTT_VERSION_3_1 3
#define MQTT_VERSION_3_1_1 4
#else
#define MQTT_VERSION_3_1 0
#define MQTT_VERSION_3_1_1 1
#endif

/**
 * MqttQos_t, various Mqtt Quality Of Service flags
 */
typedef enum _MqttQos
{
    MQTT_QOS_MOST_ONCE = 0,
    MQTT_QOS_LEAST_ONCE = 1,
    MQTT_QOS_EXACT_ONCE = 2
} MqttQos_t;

/**
 * MqttEvent_t, various Mqtt callback events
 */
typedef enum _MqttEvent
{
    MQTT_BROKER_CONNECTED,
    MQTT_BROKER_DISCONNECTED,
    MQTT_BROKER_MSG_RECEIVED,
} MqttEvent_t;

/**
 * MqttRxMsg_t, structure for the Mqtt received message
 */
typedef struct _MqttRxMsg
{
    uint8_t brokerHandle;
    uint16_t msgId;
    const char *topic;
    const char *payload;
} MqttRxMsg_t;

/**
 * MqttBrokerCallBack, function pointer to callback function to receive mqtt events
 */
typedef void (*MqttBrokerCallBack)(MqttEvent_t mqttEvent, void *data);

/**
 * MqttBrokerHandler, represents mqtt broker connection handle
 */
typedef int8_t MqttBrokerHandler;

/**
 * @fn MqttBrokerHandler MqttCreateConnetion(const char *pHost, uint16_t port, const char *pClientID,
 *                                          const char *pUsername, const char *pPassword,
                                            bool cleanSession, uint16_t keepAliveSeconds, MqttBrokerCallBack cb)
 * @brief This function creates new broker connection using given parameters
 * @param const char *pHost, mqtt broker ip or dns
 * @param uint16_t port, mqtt broker port
 * @param const char *pClientID, mqtt connection id
 * @param const char *pUsername, mqtt connectiion user name
 * @param const char *pPassword, mqtt connection password
 * @param bool cleanSession, mqtt clean session flag
 * @param uint16_t keepAliveSeconds, mqtt keep alive session timeout in seconds
 * @param MqttBrokerCallBack cb, pointer to call back functions for various mqtt activity messages
 * @return MqttBrokerHandler handle, -ve number: validation fail, mqtt connection identifier: success
 * @remark user need to hold Host, ClientId, Username, Password in buffer all the time.
 *         process automatically re-connecting to broker in case of disconnection.
 */
MqttBrokerHandler MqttCreateConnetion(const char *pHost, uint16_t port, const char *pClientID, const char *pUsername, const char *pPassword,
                                      bool cleanSession, uint16_t keepAliveSeconds, MqttBrokerCallBack cb);

/**
 * @fn int8_t MqttSslConfig(MqttBrokerHandler handle, bool sslEnable, uint8_t contextId)
 * @brief This function takes ssl parameters for broker connection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param bool sslEnable, enable/disable ssl session
 * @param uint8_t contextId, ssl context id for broker connection
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttSslConfig(MqttBrokerHandler handle, bool sslEnable, uint8_t contextId);

/**
 * @fn int8_t MqttSubscribeTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos)
 * @brief This function adds subscripted mqtt topic to the queue
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param const char *pTopic, pointer to the topic to unsubscribe
 * @param MqttQos_t qos, Quality of service of the topic
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection.  also required to name of topic in buffer all time.
 */
int8_t MqttSubscribeTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos);

/**
 * @fn int8_t MqttUnSubscribeTopic(MqttBrokerHandler handle, const char *pTopic)
 * @brief This function unsubscribes mqtt topic
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param const char *pTopic, pointer to the topic to unsubscribe
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection and subscribed to same topic
 */
int8_t MqttUnSubscribeTopic(MqttBrokerHandler handle, const char *pTopic);

/**
 * @fn int8_t MqttPublishTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos,
 *                          bool retain, const char *data, uint16_t dataLen)
 * @brief This function publishes mqtt data to provided topic
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param const char *pTopic, pointer to the topic to send data
 * @param MqttQos_t qos, Quality Of Service of transaction
 * @param bool retain, enable/disable message retaining
 * @param const char *data, pointer to data to be send
 * @param uint16_t dataLen, data length
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection and check mqtt connection before call this
 */
int8_t MqttPublishTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos, bool retain, const char *data, uint16_t dataLen);

/**
 * @fn int8_t MqttClose(MqttBrokerHandler handle)
 * @brief This function start the procedure to close mqtt cnnection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @return -ve number: validation fail, 0: fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttClose(MqttBrokerHandler handle);

/**
 * @fn bool IsMqttConnected(MqttBrokerHandler handle)
 * @brief This function returns the connection status of the broker
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @return false: not connected, true: connected
 * @remark user need to first create mqtt connection
 */
bool IsMqttConnected(MqttBrokerHandler handle);

/**
 * @fn int8_t MqttVersionConfig(MqttBrokerHandler handle, uint8_t version)
 * @brief This function takes ssl parameters for broker connection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param uint8_t version Integer type. MQTT version number. 0 MQTT Version 3.1, 1 MQTT Version 3.1.1
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttVersionConfig(MqttBrokerHandler handle, uint8_t version);

#endif // QUECMQTT_H

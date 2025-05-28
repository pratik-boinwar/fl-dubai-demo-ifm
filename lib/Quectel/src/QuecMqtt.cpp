/**
 * @file       QuecMqtt.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecMqtt.h"
#include "QuecConfig.h"

extern void freezeAT(void);
extern void unfreezeAT(void);

/**
 * @fn static uint16_t _mqttGetMsgId(void)
 * @brief This function creates incremental message id required in mqtt publish/subscribe topics
 * @return message id
 * @remark range is 1 - 65535
 */
static uint16_t _mqttGetMsgId(void)
{
    static uint16_t _msgId = 1;

    _msgId++;
    if (_msgId > 65535)
        _msgId = 0;

    return _msgId;
}

/**
 * @fn static int _mqttSslConfig(uint8_t connectID, bool sslEnable, uint8_t sslContxtID)
 * @brief This function configures ssl context for mqtt broker connection
 * @param uint8_t connectID, broker handle
 * @param bool sslEnable, enable/disable ssl flag
 * @param uint8_t sslContxtID, ssl context id to associated with this broker handle
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark range is 1 - 65535
 */
static int _mqttSslConfig(uint8_t connectID, bool sslEnable, uint8_t sslContxtID)
{
    // range is 0 to 5
    if (connectID > 5)
        return -1;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTCFG=\"SSL\",%d,%d,%d", connectID, sslEnable, sslContxtID);
    sendAT(strAt);

    if (waitResponse2(2000, GSM_OK, GSM_ERROR) == 1)
        return 1;

    return 0;
}

/**
 * @fn static int _mqttVersionConfig(uint8_t connectID, uint8_t version)
 * @brief This function configures mqtt version for mqtt broker connection
 * @param uint8_t connectID, broker handle
 * @param uint8_t version Integer type. MQTT version number.
 * 0 MQTT Version 3.1
 * 1 MQTT Version 3.1.1
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark range is 1 - 65535
 */
static int _mqttVersionConfig(uint8_t connectID, uint8_t version)
{
    // range is 0 to 5
    if (connectID > 5)
        return -1;

    char strAt[200];
#ifdef ENABLE_4G
    snprintf(strAt, sizeof(strAt), "+QMTCFG=\"version\",%d,%d", connectID, version);
#else
    snprintf(strAt, sizeof(strAt), "+QMTCFG=\"VERSION\",%d,%d", connectID, version);
#endif
    sendAT(strAt);

    if (waitResponse2(2000, GSM_OK, GSM_ERROR) == 1)
        return 1;

    return 0;
}

/**
 * @fn static int _mqttOpen(uint8_t connectID, const char *host, uint16_t port)
 * @brief This function opens connection with the broker server
 * @param uint8_t connectID, broker handle
 * @param const char *host, broker srever ip or dns
 * @param uint16_t port, broker server port
 * @return -ve number: validation fail, 0: timeout, 1: success, 2: ERROR
 * @remark
 */
static int _mqttOpen(uint8_t connectID, const char *host, uint16_t port)
{
    /** response
    * OK

    * +QMTOPEN: 0,0
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    // host length shall < 100 bytes

    char strAt[512];
    snprintf(strAt, sizeof(strAt), "+QMTOPEN=%d,\"%s\",%d", connectID, host, port);
    sendAT(strAt);

    int8_t status = waitResponse2(1, "+QMTOPEN: ", GSM_ERROR);
    if (status != 1)
        return status;

    // received +QMTOPEN: <connectID>,<result>
    streamSkipUntil(',', 50); // skip connectID

    int8_t res = streamGetIntBefore('\r');
    streamAT().readStringUntil('\n'); // readout left over

    if ((0 == res) || (2 == res))
        return 1;

    return -1;
}

/**
 * @fn static int _mqttClose(uint8_t connectID)
 * @brief This function closed connection with the broker server
 * @param uint8_t connectID, broker handle
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
static int _mqttClose(uint8_t connectID)
{
    /** response
    * OK

    * +QMTCLOSE: 0,0
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTCLOSE=%d", connectID);
    sendAT(strAt);

    if (waitResponse2(10000, "+QMTCLOSE: ", GSM_ERROR) != 1)
        return 0;

    // received +QMTCLOSE: <TCP_connectID>,<result>
    streamSkipUntil(',', 50); // skip connectID

    int8_t res = streamGetIntBefore('\r');
    streamAT().readStringUntil('\n'); // readout left over

    if (0 == res)
        return 1;

    return 0;
}

/**
 * @fn static int _mqttConnect(uint8_t connectID, const char *clientID, const char *username, const char *password)
 * @brief This function creates mqtt connection with the broker
 * @param uint8_t connectID, broker handle
 * @param const char *clientID, mqtt connection client id
 * @param const char *username, mqtt connection username
 * @param const char *password, mqtt connection password
 * @return -ve number: validation fail, 0: timeout, 1: success, 2: ERROR
 * @remark
 */
static int _mqttConnect(uint8_t connectID, const char *clientID, const char *username, const char *password)
{
    /** response
    * OK

    * +QMTCONN: 0,0,0
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    char strAt[512];

    if (username != 0 && password != 0)
        snprintf(strAt, sizeof(strAt), "+QMTCONN=%d,\"%s\",\"%s\",\"%s\"", connectID, clientID, username, password);
    else
        snprintf(strAt, sizeof(strAt), "+QMTCONN=%d,\"%s\"", connectID, clientID);
    sendAT(strAt);

    int8_t status = waitResponse2(1, "+QMTCONN: ", GSM_ERROR);
    if (status != 1)
    {
        return status;
    }

    // received +QMTCONN: <TCP_connectID>,<result>[,<retcode>]
    streamSkipUntil(',', 50); // skip connectID
    // streamSkipUntil(',', 50); // skip result
    int8_t result = streamGetIntBefore(',');
    if (0 != result)
    {
        return 2;
    }

    int8_t res = streamGetIntBefore('\r');
    streamAT().readStringUntil('\n'); // readout left over

    if (0 == res)
    {
        return 1;
    }

    return -1;
}

/**
 * @fn static int _mqttDisconnect(uint8_t connectID)
 * @brief This function disconnect mqtt connection with the broker
 * @param uint8_t connectID, broker handle
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
static int _mqttDisconnect(uint8_t connectID)
{
    /** response
    * OK

    * +QMTDISC: 0,0
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTDISC=%d", connectID);
    sendAT(strAt);

    if (waitResponse2(10000, "+QMTDISC: ", GSM_ERROR) != 1)
        return 0;

    // received +QMTDISC: <TCP_connectID>,<result>
    streamSkipUntil(',', 50); // skip connectID

    int8_t res = streamGetIntBefore('\r');
    streamAT().readStringUntil('\n'); // readout left over

    if (0 == res)
        return 1;

    return 0;
}

/**
 * @fn static int _mqttSubscribeTopic(uint8_t connectID, uint16_t msgID, const char *topic, MqttQos_t qos)
 * @brief This function subscribing mqtt topic to the broker
 * @param uint8_t connectID, broker handle
 * @param uint16_t msgID, mqtt topics message id
 * @param const char *topic, mqtt subscribing topic name
 * @param MqttQos_t qos, mqtt topic quality of service
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
static int _mqttSubscribeTopic(uint8_t connectID, uint16_t msgID, const char *topic, MqttQos_t qos)
{
    /** response
    * OK

    * +QMTSUB: 0,2,0,1
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    if (topic == NULL)
        return -2;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTSUB=%d,%d,\"%s\",%d", connectID, msgID, topic, qos);
    sendAT(strAt);

    if (waitResponse2(10000, "+QMTSUB: ", GSM_ERROR) != 1)
        return 0;

    // received +QMTSUB: <TCP_connectID>,<msgID>,<result>[,<value>]
    streamSkipUntil(',', 50); // connectID
    streamSkipUntil(',', 50); // skip msgID

    int8_t res = streamGetIntBefore(',');
    if (0 == res)
    {
        // ACK from server.
        res = streamGetIntBefore('\r');
        streamAT().readStringUntil('\n'); // readout left over
        if (128 == res)
        {
            // subscription is rejected by server.
            return 0;
        }
        else
        {
            // any other number is vector of granted QOS level
            return 1;
        }
    }

    streamAT().readStringUntil('\n'); // readout left over
    return 0;
}

/**
 * @fn static int _mqttUnSubscribeTopic(uint8_t connectID, uint16_t msgID, const char *topic)
 * @brief This function un-subscribing mqtt topic to the broker
 * @param uint8_t connectID, broker handle
 * @param uint16_t msgID, mqtt topics message id
 * @param const char *topic, mqtt subscribing topic name
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
static int _mqttUnSubscribeTopic(uint8_t connectID, uint16_t msgID, const char *topic)
{
    /** response
    * OK

    * +QMTUNS: 0,2,0,1
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    if (topic == NULL)
        return -2;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTUNS=%d,%d,\"%s\"", connectID, msgID, topic);
    sendAT(strAt);

    if (waitResponse2(10000, "+QMTUNS: ", GSM_ERROR) != 1)
        return 0;

    // received +QMTUNS: <TCP_connectID>,<msgID>,<result>
    streamSkipUntil(',', 50); // connectID
    streamSkipUntil(',', 50); // skip msgID

    int8_t res = streamGetIntBefore('\r');
    streamAT().readStringUntil('\n'); // readout left over
    if (0 == res)
    {
        // ACK from server.
        return 1;
    }

    return 0;
}

/**
 * @fn static int _mqttPublishTopic(uint8_t connectID, uint8_t msgID, const char *topic, MqttQos_t qos,
 *                                  bool retain, const char *data, uint16_t dataLen)
 * @brief This function publishes data to mqtt topic
 * @param uint8_t connectID, broker handle
 * @param uint16_t msgID, mqtt topics message id
 * @param const char *topic, mqtt publishing topic name
 * @param MqttQos_t qos, mqtt topic quality of service
 * @param bool retain, enable/disable message retaining
 * @param const char *data, pointer to topic data to be publish
 * @param uint16_t dataLen, length of publishing topic data
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark
 */
static int _mqttPublishTopic(uint8_t connectID, uint8_t msgID, const char *topic, MqttQos_t qos, bool retain, const char *data, uint16_t dataLen)
{
    /** response
    * OK

    * +QMTPUB: 0,0,0
    */

    // range is 0 to 5
    if (connectID > 5)
        return -1;

    if (topic == NULL)
        return -2;

    char strAt[200];
    snprintf(strAt, sizeof(strAt), "+QMTPUB=%d,%d,%d,%d,\"%s\"", connectID, msgID, qos, (retain == true ? 1 : 0), topic);
    sendAT(strAt);

    if (waitResponse2(15000, ">", GSM_ERROR) != 1)
    {
        // safe side sending ctl+1A, ESCAPE
        streamAT().write(0x1A);
        streamAT().flush();
        return 0;
    }

    streamAT().write(data, dataLen);
    delay(5);
    streamAT().write(0x1A); // CTRL+Z
    streamAT().flush();

    if (waitResponse2(15000, "+QMTPUB: ", GSM_ERROR) != 1)
        return 0;

    // received +QMTPUB: <TCP_connectID>,<msgID>,<result>[,<value>]
    streamSkipUntil(',', 50); // skip connectID
    streamSkipUntil(',', 50); // skip msgID

    int8_t res = streamGetIntBefore(',');
    streamAT().readStringUntil('\n'); // readout left over
    if (0 == res)
    {
        // ACK from server. if QOS=0, ACK is not required
        return 1;
    }

    return 0;
}

#define MAX_MQTT_BORKERS 5     // max 5 brokers only
#define MAX_MQTT_SUB_TOPICS 10 // subscription topics per broker

/**
 * MqttSubTopic_t, structure for the subscription topic data
 */
typedef struct _MqttSubTopic
{
    char topic[256];
    MqttQos_t qos;
    uint16_t msgID;
    bool subscribed;
} MqttSubTopic_t;

/**
 * BrokerCnxEvents_t, various broker connection processing events
 */
typedef enum _BrokerCnxEvents
{
    BRKR_EVT_OPEN_NETWORK = 0,
    BRKR_EVT_CONNECT_RQST,
    BRKR_EVT_SUBSCRIBE_RQST,
    // BRKR_EVT_PUBLISH_RQST
    BRKR_EVT_DISCONNECT_RQST,
    BRKR_EVT_CLOSE_RQST,
    BRKR_EVT_BUSY,
    BRKR_EVT_IDLE
} BrokerCnxEvents_t;

/**
 * BrokerCnxEvents_t, various broker connection status
 * DO NOT CHANGE
 */
typedef enum _BrokerCnxSatus
{
    BRKR_CNX_STAT_NULL = 0,
    BRKR_CNX_STAT_INTIIATING,
    BRKR_CNX_STAT_CONECTTING,
    BRKR_CNX_STAT_CONNECTED,
    BRKR_CNX_STAT_DISCONNECTING
} BrokerCnxSatus_t;

/**
 *
 * MqttBroker_t, structure for broker data
 */
typedef struct _MqttBroker
{
    char host[100];
    uint16_t port;
    char clientID[50];
    char username[256];
    char password[256];
    bool cleanSession;
    uint16_t keepAliveSeconds;
    MqttBrokerCallBack callback;
    bool isValid;
    bool isConnected;
    bool isAllTopicSubscribed;
    uint8_t mqttVersionNum;
    bool isSecurityEnanble;
    uint8_t sslContextID;
    uint32_t keppAliveTick;
    MqttSubTopic_t subTopic[MAX_MQTT_SUB_TOPICS];
    BrokerCnxEvents_t brokerCnxEvent;
    BrokerCnxSatus_t brokerCnxStatus;
} MqttBroker_t;

// holds data of the mqtt broker handle
static MqttBroker_t mqttBroker[MAX_MQTT_BORKERS];

// holds total number of created mqtt brokers
static uint8_t _totalActiveBrokers = 0;

/**
 * @fn static void _mqttManualDisconnection(void)
 * @brief This function is being called to set/clear associated flags to re-initiated process
 * @remark
 */
static void _mqttManualDisconnection(void)
{
    DBG("[MQTT] manual disconnection\n");

    for (uint8_t handle = 0; handle < MAX_MQTT_BORKERS; handle++)
    {
        mqttBroker[handle].isConnected = false;
        mqttBroker[handle].isAllTopicSubscribed = false;

        for (uint8_t idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
        {
            mqttBroker[handle].subTopic[idx].subscribed = false;
        }
    }
}

/**
 * @fn static int _mqttCheckConnection()
 * @brief This function checks connection status of all broker channels
 * @param uint8_t connectID, broker handle
 * @param const char *clientID, mqtt connection client id
 * @param const char *username, mqtt connection username
 * @param const char *password, mqtt connection password
 * @return 0: finish with fail, 1: success
 * @remark
 */
static int _mqttCheckConnection()
{
    /** response
     *
     *
     *
     *
     *
     * +QMTCONN: 0,<state>
     * +QMTCONN: 1,<state>
     * +QMTCONN: 2,<state>
     * +QMTCONN: 3,<state>
     * +QMTCONN: 4,<state>
     * +QMTCONN: 5,<state>
     * OK
     *
     *
     *
     *
     *
     */

    // REQUIRED MORE TESTING

    // trying to read out any previous remaining
    waitResponse3(GSM_OK);

    sendAT("+QMTCONN?");

    int16_t state = 0, cnxId = 0;
    int8_t res;
    uint8_t totalCnxIdReceived = 0xFF; // unclear bit w.r.t. connectId received

    for (uint8_t connectID = 0; ((connectID <= 5) && (connectID < MAX_MQTT_BORKERS)); connectID++)
    {
        res = waitResponse2(1000, "+QMTCONN: ", GSM_OK, GSM_ERROR);
        if (res != 1)
        {
            // all connectID read out or there are no ConnectID
            // if (connectID == 0)
            // {
            //     // if the first line of response is "OK". that means no cnxId connection is attempted.
            //     for (uint8_t handle = 0; handle < MAX_MQTT_BORKERS; handle++)
            //     {
            //         mqttBroker[handle].isConnected = false; // this connection is not connected for sure.
            //         if (mqttBroker[handle].isValid)
            //         {
            //             // if this is valid one, then start re-connection.
            //             // we might have lost the disconnection URC
            //             mqttBroker[handle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
            //         }
            //     }
            // }
            break;
        }

        // received +QMTCONN: <TCP_connectID>,<state>
        cnxId = streamGetIntBefore(','); // get connectID
        if (cnxId < MAX_MQTT_BORKERS)
        {
            // clear bit whose connectID received.
            totalCnxIdReceived &= ~(1 << cnxId);

            // it will be always true
            state = streamGetIntBefore('\r'); // get state
            mqttBroker[cnxId].brokerCnxStatus = (BrokerCnxSatus_t)state;

            switch ((mqttBroker[cnxId].brokerCnxStatus))
            {
            case BRKR_CNX_STAT_CONNECTED: // newly connected or already connected
                if (mqttBroker[cnxId].isAllTopicSubscribed == true)
                    mqttBroker[cnxId].brokerCnxEvent = BRKR_EVT_IDLE;
                else
                    mqttBroker[cnxId].brokerCnxEvent = BRKR_EVT_SUBSCRIBE_RQST;
                break;

            case BRKR_CNX_STAT_INTIIATING: // wait till initiating and status chnaged
                mqttBroker[cnxId].brokerCnxEvent = BRKR_EVT_CONNECT_RQST;
                break;

            case BRKR_CNX_STAT_CONECTTING:    // wait till connecting and status changed
            case BRKR_CNX_STAT_DISCONNECTING: // wait till disconnecting and status changed
                mqttBroker[cnxId].brokerCnxEvent = BRKR_EVT_BUSY;
                mqttBroker[cnxId].isAllTopicSubscribed = false; // safe side clear the flag
                mqttBroker[cnxId].isConnected = false;          // safe side clear the flag
                break;

            default:                                            // disconnected. not decided if re-connect or not
                mqttBroker[cnxId].isAllTopicSubscribed = false; // safe side clear the flag
                mqttBroker[cnxId].isConnected = false;          // safe side clear the flag
                break;
            }

            // DBG("[QMTCON] cnxid#%d, state=%d\n", cnxId, mqttBroker[cnxId].brokerCnxStatus);
        }
        else
        {
            DBG("QMTCON: cnxid#%d, expected=%d\n", cnxId, connectID);
        }

        streamAT().readStringUntil('\n'); // readout left over
    }

    waitResponse3(GSM_OK);

    for (uint8_t handle = 0; handle < MAX_MQTT_BORKERS; handle++)
    {
        if ((totalCnxIdReceived >> handle) & 0x1)
        {
            // this cnxId not received.
            if (mqttBroker[handle].isValid)
            {
                // if this is valid one, then start re-connection.
                // we might have lost the disconnection URC
                mqttBroker[handle].isConnected = false; // this connection is not connected for sure.
                mqttBroker[handle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
            }
        }
    }

    if (res == 2)
    {
        // command ERROR
        return 0;
    }

    return 1;
}

/**
 * @fn void mqttDataReceived(void)
 * @brief This function is being called whe mqtt data received from broker.
 * @remark this function is called from waitResponse() function
 */
void mqttDataReceived(void)
{
    //+QMTRECV: 0,0,topic/example,This is the payload related to topic
    MqttRxMsg_t mqttRxMsg;

    mqttRxMsg.brokerHandle = streamGetIntBefore(',');
    mqttRxMsg.msgId = streamGetIntBefore(',');
    String topic = streamAT().readStringUntil(',');
    String payload = streamAT().readStringUntil('\r');
    // streamAT().readString();
    streamAT().readStringUntil('\n'); // read out till line ends only. there might be another URC.

    mqttRxMsg.topic = topic.c_str();
    mqttRxMsg.payload = payload.c_str();

    // DBG("[MQTT] handle:%d\n", mqttRxMsg.brokerHandle);
    // DBG("[MQTT] msgid:%d\n", mqttRxMsg.msgId);
    // DBG("[MQTT] topic:%s\n", mqttRxMsg.topic);
    // DBG("[MQTT] payload:%s\n", mqttRxMsg.payload);

    if (mqttRxMsg.brokerHandle < MAX_MQTT_BORKERS)
    {
        if (mqttBroker[mqttRxMsg.brokerHandle].callback != NULL)
            mqttBroker[mqttRxMsg.brokerHandle].callback(MQTT_BROKER_MSG_RECEIVED, &mqttRxMsg);
    }
}

/**
 * MqttClientState_t various internal processing states
 */
typedef enum _MqttClientState
{
    MQTT_SM_NULL = 0,
    MQTT_SM_INIT,
    MQTT_SM_CHECK_EVENT,
    MQTT_SM_OPEN_NETWORK,
    MQTT_SM_WAIT_FOR_OPEN_RESPONSE,
    MQTT_SM_CONNECT_BROKER,
    MQTT_SM_WAIT_FOR_CONNECT_RESPONSE,
    MQTT_SM_DISCONNECT_BROKER,
    MQTT_SM_CLOSE_BROKER,
    MQTT_SM_SBSCRIBE_TOPIC,
    MQTT_SM_CHECK_BROKER_CONNECTION,
    MQTT_SM_TAKE_REST,
    MQTT_SM_GENERIC_DELAY
} MqttClientState_t;

// holds the current operating state
MqttClientState_t _mqttClientState = MQTT_SM_INIT;

/**
 * @fn void QuectelMqttClienSm(void)
 * @brief This function is handling connections with mqtt broker.
 * @remark this function is required to be called frequently.
 *         Do not make direct call. This is called within QuectelLoop() function
 */
void QuectelMqttClienSm(void)
{
    static int8_t _curBrokerHandle = -1;
    static uint32_t _generalTimeoutTick = 0;

    if (!IsATavailable())
        return;

    // if modem is initializing then wait till finish it
    // and re-init mqtt
    if (GetModemCurrentOp() == MDM_INITIALIZING)
        _mqttClientState = MQTT_SM_INIT;

    if (!IsModemReady())
    {
        if (GetModemCurrentOp() != MDM_CONNECTING_MQTT)
            return;
    }

    switch (_mqttClientState)
    {
    case MQTT_SM_INIT: // wait till gprs connected
        if (!IsGprsConnected())
        {
            break;
        }

        // clear all connected flags.
        _mqttManualDisconnection();

        _curBrokerHandle = -1;
        _generalTimeoutTick = millis();
        _mqttClientState = MQTT_SM_TAKE_REST;
        break;

    case MQTT_SM_CHECK_EVENT: // check what to do with the currentBorkerHandle
        // if (!IsATavailable())
        //     break;

        // if (!IsModemReady())
        //     break;

        if (!IsGprsConnected())
        {
            _mqttManualDisconnection();
            _mqttClientState = MQTT_SM_INIT;
            break;
        }

        if (_totalActiveBrokers == 0)
            break;

        if (++_curBrokerHandle >= _totalActiveBrokers) // MAX_MQTT_BORKERS
        {
            // check all broker connetions
            //  _mqttCheckConnection();
            _curBrokerHandle = -1;
            _mqttClientState = MQTT_SM_CHECK_BROKER_CONNECTION; // MQTT_SM_TAKE_REST;
            _generalTimeoutTick = millis();
            break;
        }

        if (mqttBroker[_curBrokerHandle].isValid)
        {
            /*if (mqttBroker[_curBrokerHandle].isSecurityEnanble)
            {
                // we can wait here till ssl sertificates and key are not uploaded.
                // but for now we have given option to user to override QuceSsl stack.
                // so, we are not checking ssl handle is ready or not

                if (!IsSslChannelAvailable(mqttBroker[_curBrokerHandle].sslContextID))
                    break;
            }*/

            if (mqttBroker[_curBrokerHandle].isConnected)
            {
                // already connected
                if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_SUBSCRIBE_RQST)
                {
                    _mqttClientState = MQTT_SM_SBSCRIBE_TOPIC;
                    _generalTimeoutTick = millis();
                }
                else if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_DISCONNECT_RQST)
                {
                    _mqttClientState = MQTT_SM_DISCONNECT_BROKER;
                    _generalTimeoutTick = millis();
                }
                else if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_BUSY)
                {
                    // previous operation is in process.
                    // wait till status is not changed.
                    _mqttClientState = MQTT_SM_GENERIC_DELAY;
                    _generalTimeoutTick = millis();
                    break;
                }
                else
                {
                    //_mqttClientState = MQTT_SM_CHECK_BROKER_CONNECTION;
                    //_generalTimeoutTick = millis();
                }
            }
            else
            {
                // if not connected, try to connect
                if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_OPEN_NETWORK)
                {
                    _mqttClientState = MQTT_SM_OPEN_NETWORK;
                    _generalTimeoutTick = millis();
                }
                else if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_CONNECT_RQST)
                {
                    _mqttClientState = MQTT_SM_CONNECT_BROKER;
                    _generalTimeoutTick = millis();
                }
                else if (mqttBroker[_curBrokerHandle].brokerCnxEvent == BRKR_EVT_CLOSE_RQST)
                {
                    _mqttClientState = MQTT_SM_CLOSE_BROKER;
                    _generalTimeoutTick = millis();
                }
                else
                {
                    // ideally this condition will not happen
                    // either we have not updated 'isConnected' flag or not updated 'brokerCnxEvent'
                }
            }
        }
        else
        {
            _mqttClientState = MQTT_SM_GENERIC_DELAY;
            _generalTimeoutTick = millis();
        }
        break;

    case MQTT_SM_OPEN_NETWORK: // open connection to the broker
        if (millis() - _generalTimeoutTick > 10000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
            ReleaseModemAccess(MDM_CONNECTING_MQTT);
            // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
            //     Serial.println(" MQTT_SM_OPEN_NETWORK timeout released ");
            // else
            //     Serial.println(" MQTT_SM_OPEN_NETWORK timeout not released ");
            break;
        }
        if (IsATavailable())
        {
            if (!IsModemReady())
                break;

            if (!GetModemAccess(MDM_CONNECTING_MQTT))
                break;

            int ret;

            if (mqttBroker[_curBrokerHandle].isSecurityEnanble)
            {
                ret = _mqttVersionConfig(_curBrokerHandle, mqttBroker[_curBrokerHandle].mqttVersionNum);
                if (ret != 1)
                {
                    _mqttClientState = MQTT_SM_GENERIC_DELAY;
                    break;
                }

                ret = _mqttSslConfig(_curBrokerHandle, mqttBroker[_curBrokerHandle].isSecurityEnanble,
                                     mqttBroker[_curBrokerHandle].sslContextID);

                if (ret != 1)
                {
                    _mqttClientState = MQTT_SM_GENERIC_DELAY;
                    break;
                }
            }

            ret = _mqttOpen(_curBrokerHandle, mqttBroker[_curBrokerHandle].host, mqttBroker[_curBrokerHandle].port);
            if (1 == ret)
            {
                // suceess
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_CONNECT_RQST;
                _mqttClientState = MQTT_SM_CONNECT_BROKER;
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
                // Serial.println(" MQTT_SM_OPEN_NETWORK released ");
                // else
                // Serial.println(" MQTT_SM_OPEN_NETWORK not released ");

                break;
            }
            else if (2 == ret)
            {
                // ERROR received
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                ReleaseModemAccess(MDM_CONNECTING_MQTT);

                break;
            }
            else
            {
                _mqttClientState = MQTT_SM_WAIT_FOR_OPEN_RESPONSE;
                // _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                break;
            }
        }
        break;
#if 1
    case MQTT_SM_WAIT_FOR_OPEN_RESPONSE:
        if (millis() - _generalTimeoutTick > 120000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
            unfreezeAT();
            ReleaseModemAccess(MDM_CONNECTING_MQTT);
            // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
            //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE released ");
            // else
            //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE not released ");
            break;
        }
        else
        {
            int ret = waitResponse2(1, "+QMTOPEN: ", GSM_ERROR);
            if (ret == 1)
            {
                // we have received "QMTOPEN", process data.

                // received +QMTOPEN: <connectID>,<result>
                streamSkipUntil(',', 50); // skip connectID

                int8_t res = streamGetIntBefore('\r');
                streamAT().readStringUntil('\n'); // readout left over

                if ((0 == res) || (2 == res))
                {
                    mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_CONNECT_RQST;
                    _mqttClientState = MQTT_SM_CONNECT_BROKER;
                }
                else
                {
                    _mqttClientState = MQTT_SM_GENERIC_DELAY;
                    ReleaseModemAccess(MDM_CONNECTING_MQTT);
                    // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
                    //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE rcvd released ");
                    // else
                    //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE rcvd not released ");
                }
                _generalTimeoutTick = millis();

                // do not release here.
                // ReleaseModemAccess(MDM_CONNECTING_MQTT);

                break;
            }
            else if (ret == 2)
            {
                // we have receivved error
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
                //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE error released ");
                // else
                //     Serial.println(" MQTT_SM_WAIT_FOR_OPEN_RESPONSE error not released ");
                break;
            }
        }
        break;
#endif

    case MQTT_SM_CONNECT_BROKER: // make mqtt connection
        if (millis() - _generalTimeoutTick > 10000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
            ReleaseModemAccess(MDM_CONNECTING_MQTT);
            // if (ReleaseModemAccess(MDM_CONNECTING_MQTT))
            //     Serial.println(" MQTT_SM_CONNECT_BROKER timeout released ");
            // else
            //     Serial.println(" MQTT_SM_CONNECT_BROKER timeout not released ");
            break;
        }
        if (IsATavailable())
        {
            // we haven't released yet, so commented it
            // if (!IsModemReady())
            //     break;

            // if (!GetModemAccess(MDM_CONNECTING_MQTT))
            //     break;

            int ret = _mqttConnect(_curBrokerHandle, mqttBroker[_curBrokerHandle].clientID,
                                   mqttBroker[_curBrokerHandle].username, mqttBroker[_curBrokerHandle].password);

            if (1 == ret)
            {
                // suceess
                mqttBroker[_curBrokerHandle].isConnected = true;
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_SUBSCRIBE_RQST;
                _mqttClientState = MQTT_SM_SBSCRIBE_TOPIC;

                // re-subscribe
                mqttBroker[_curBrokerHandle].isAllTopicSubscribed = false;
                for (uint8_t idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
                {
                    mqttBroker[_curBrokerHandle].subTopic[idx].subscribed = false;
                }
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                break;
            }
            else if (2 == ret)
            {
                // ERROR received
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                break;
            }
            else
            {
                _mqttClientState = MQTT_SM_WAIT_FOR_CONNECT_RESPONSE;
                // mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
                // _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                break;
            }
        }
        break;

#if 1
    case MQTT_SM_WAIT_FOR_CONNECT_RESPONSE:
        if (millis() - _generalTimeoutTick > 60000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
            ReleaseModemAccess(MDM_CONNECTING_MQTT);
        }
        else
        {
            int ret = waitResponse2(1, "+QMTCONN: ", GSM_ERROR);

            if (ret == 1)
            {
                // we have received "QMTCONN", process data.

                // received +QMTCONN: <TCP_connectID>,<result>[,<retcode>]
                streamSkipUntil(',', 50); // skip connectID
                                          // streamSkipUntil(',', 50); // skip result
                int8_t result = streamGetIntBefore(',');
                if (0 != result)
                {
                    mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
                    _mqttClientState = MQTT_SM_GENERIC_DELAY;
                    _generalTimeoutTick = millis();
                    ReleaseModemAccess(MDM_CONNECTING_MQTT);
                    break;
                }

                int8_t res = streamGetIntBefore('\r');
                streamAT().readStringUntil('\n'); // readout left over

                mqttBroker[_curBrokerHandle].isConnected = true;
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_SUBSCRIBE_RQST;
                _mqttClientState = MQTT_SM_SBSCRIBE_TOPIC;

                // re-subscribe
                mqttBroker[_curBrokerHandle].isAllTopicSubscribed = false;
                for (uint8_t idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
                {
                    mqttBroker[_curBrokerHandle].subTopic[idx].subscribed = false;
                }
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                break;
            }
            else if (ret == 2)
            {
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                ReleaseModemAccess(MDM_CONNECTING_MQTT);
                break;
            }
        }
        break;
#endif

    case MQTT_SM_DISCONNECT_BROKER: // disconnect mqtt connection
        if (IsATavailable())
        {
            int ret = _mqttDisconnect(_curBrokerHandle);

            if (1 == ret)
            {
                // suceess
                mqttBroker[_curBrokerHandle].isConnected = false;
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_CLOSE_RQST;
                _mqttClientState = MQTT_SM_CLOSE_BROKER;
                break;
            }
            else
            {
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                break;
            }
        }

        if (millis() - _generalTimeoutTick > 10000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
        }
        break;

    case MQTT_SM_CLOSE_BROKER: // close connection from broker
        if (IsATavailable())
        {
            int ret = _mqttClose(_curBrokerHandle);

            if (1 == ret)
            {
                // suceess
                _totalActiveBrokers--;
                mqttBroker[_curBrokerHandle].isConnected = false;
                mqttBroker[_curBrokerHandle].isValid = false;
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_IDLE;
                // _mqttClientState = MQTT_SM_CHECK_EVENT;
                // break;
            }
            // else
            // {
            _mqttClientState = MQTT_SM_GENERIC_DELAY;
            _generalTimeoutTick = millis();
            break;
            // }
        }

        if (millis() - _generalTimeoutTick > 10000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
        }
        break;

    case MQTT_SM_SBSCRIBE_TOPIC: // make subscription to topic
        if (IsATavailable())
        {
            // find which topic is need to subscribed.
            int idx = 0;
            for (idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
            {
                if ((mqttBroker[_curBrokerHandle].subTopic[idx].subscribed == false) && mqttBroker[_curBrokerHandle].subTopic[idx].topic[0] != 0)
                {
                    // we got topic to be subscribed
                    break;
                }
            }

            if (idx >= MAX_MQTT_SUB_TOPICS)
            {
                // all topics of this borker is already subscribed.

                // ToDo:
                // 1. if we selected, cleanSession=true for the broker connection, then everytime when client s=idsconnected
                // and connected again, we required to subscribed all topics again.
                // 2. if we selected, cleanSession=false, then broker keeps track. but here clientId must be same.
                //  don't knew about if broker track with IP address of client.

                // I suggest, always subscribe topic if client re-connected

                mqttBroker[_curBrokerHandle].isAllTopicSubscribed = true;
                mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_IDLE;
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                break;
            }

            int ret = _mqttSubscribeTopic(_curBrokerHandle, mqttBroker[_curBrokerHandle].subTopic[idx].msgID,
                                          mqttBroker[_curBrokerHandle].subTopic[idx].topic,
                                          mqttBroker[_curBrokerHandle].subTopic[idx].qos);

            if (1 == ret)
            {
                // suceess
                mqttBroker[_curBrokerHandle].subTopic[idx].subscribed = true;
                // mqttBroker[_curBrokerHandle].brokerCnxEvent = BRKR_EVT_SUBSCRIBE_RQST;
                //_mqttClientState = MQTT_SM_CONNECT_BROKER;
                _mqttClientState = MQTT_SM_GENERIC_DELAY; // take a power nap to receive subscription data if any
                break;
            }
            else
            {
                _mqttClientState = MQTT_SM_GENERIC_DELAY;
                _generalTimeoutTick = millis();
                break;
            }
        }

        if (millis() - _generalTimeoutTick > 10000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
        }
        break;

    case MQTT_SM_TAKE_REST: // take some rest
        if (millis() - _generalTimeoutTick > 20000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
        }
        break;

    case MQTT_SM_GENERIC_DELAY: // power nap
        if (millis() - _generalTimeoutTick > 20000)
        {
            _mqttClientState = MQTT_SM_CHECK_EVENT;
        }
        break;

    case MQTT_SM_CHECK_BROKER_CONNECTION: // check broker connection
        if (IsATavailable())
        {
            // ToDo:
            // set connection status flags according to data received
            // to do that, call AT command process here.

            _mqttCheckConnection();
            _generalTimeoutTick = millis();
            _mqttClientState = MQTT_SM_TAKE_REST;
        }
        if (millis() - _generalTimeoutTick > 60000)
        {
            _mqttClientState = MQTT_SM_INIT;
        }
        break;

    default:
        _generalTimeoutTick = millis();
        _mqttClientState = MQTT_SM_TAKE_REST;
        break;
    }
}

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
                                      bool cleanSession, uint16_t keepAliveSeconds, MqttBrokerCallBack cb)
{
    MqttBrokerHandler handle = 0;

    while (true == mqttBroker[handle].isValid)
    {
        handle++;
        if (handle >= MAX_MQTT_BORKERS)
        {
            DBG("\r\nMQTT_ERROR: No Brokers Left\r\n");
            return -1;
        }
    }

    // validation
    if (strlen(pHost) > sizeof(mqttBroker[handle].host))
    {
        DBG("\r\nMQTT_ERROR: Server URL too Long\r\n");
        return -1;
    }
    else if (strlen(pClientID) > sizeof(mqttBroker[handle].clientID))
    {
        DBG("\r\nMQTT_ERROR: Client ID too Long\r\n");
        return -1;
    }
    else if (strlen(pUsername) > sizeof(mqttBroker[handle].username))
    {
        DBG("\r\nMQTT_ERROR: Username too Long\r\n");
        return -1;
    }
    else if (strlen(pPassword) > sizeof(mqttBroker[handle].password))
    {
        DBG("\r\nMQTT_ERROR: Password too Long\r\n");
        return -1;
    }
    //    else if(strlen(pServerPort) > sizeof(mqttBroker[handle].serverPort))
    //    {
    //        return -1;
    //    }
    // end of validation

    strcpy(mqttBroker[handle].host, pHost);
    mqttBroker[handle].port = port;
    strcpy(mqttBroker[handle].clientID, pClientID);
    strcpy(mqttBroker[handle].username, pUsername);
    strcpy(mqttBroker[handle].password, pPassword);
    mqttBroker[handle].cleanSession = cleanSession;
    mqttBroker[handle].keepAliveSeconds = keepAliveSeconds;
    // mqttBroker[handle].keepAliveCompare = ((keepAliveSeconds >> 1) * 1000);
    mqttBroker[handle].keppAliveTick = millis();
    mqttBroker[handle].callback = cb;

    mqttBroker[handle].isValid = true;
    mqttBroker[handle].isConnected = false;
    mqttBroker[handle].brokerCnxEvent = BRKR_EVT_OPEN_NETWORK;
    mqttBroker[handle].brokerCnxStatus = BRKR_CNX_STAT_NULL;

    _totalActiveBrokers++;
    return handle;
}

/**
 * @fn int8_t MqttSslConfig(MqttBrokerHandler handle, bool sslEnable, uint8_t contextId)
 * @brief This function takes ssl parameters for broker connection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param bool sslEnable, enable/disable ssl session
 * @param uint8_t contextId, ssl context id for broker connection
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttSslConfig(MqttBrokerHandler handle, bool sslEnable, uint8_t contextId)
{
    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    if (mqttBroker[handle].isValid)
    {
        mqttBroker[handle].isSecurityEnanble = sslEnable;
        mqttBroker[handle].sslContextID = contextId;
        return 1;
    }

    return 0;
}

/**
 * @fn int8_t MqttSubscribeTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos)
 * @brief This function adds subscripted mqtt topic to the queue
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param const char *pTopic, pointer to the topic to unsubscribe
 * @param MqttQos_t qos, Quality of service of the topic
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection. also required to name of topic in buffer all time.
 */
int8_t MqttSubscribeTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos)
{
    uint8_t idx = 0;

    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    // check if same topic is already available for same broker
    for (idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
    {
        if (0 == strcmp(mqttBroker[handle].subTopic[idx].topic, pTopic))
        {
            DBG("\r\nMQTT_STATUS: Sub Topic already present, QOS updated\r\n");
            mqttBroker[handle].subTopic[idx].qos = qos;
            mqttBroker[handle].subTopic[idx].subscribed = false;
            mqttBroker[handle].subTopic[idx].msgID = _mqttGetMsgId();
            mqttBroker[handle].isAllTopicSubscribed = false;
            return idx;
        }
    }

    idx = 0;
    while (mqttBroker[handle].subTopic[idx].topic[0] != 0)
    {
        idx++;
        if (idx >= MAX_MQTT_SUB_TOPICS)
        {
            DBG("\r\nMQTT_ERROR: No Sub Topics Left\r\n");
            return -1;
        }
    }

    if (strlen(pTopic) <= sizeof(mqttBroker[handle].subTopic[idx].topic))
    {
        strcpy(mqttBroker[handle].subTopic[idx].topic, pTopic);
        mqttBroker[handle].subTopic[idx].qos = qos;
        mqttBroker[handle].subTopic[idx].msgID = _mqttGetMsgId();
        mqttBroker[handle].subTopic[idx].subscribed = false;

        DBG("\r\nMQTT_STATUS: Subscription Topic Created\r\n");
        return idx;
    }
    else
    {
        DBG("\r\nMQTT_ERROR: Subscription topic too long\r\n");
        return -1;
    }
}

/**
 * @fn int8_t MqttUnSubscribeTopic(MqttBrokerHandler handle, const char *pTopic)
 * @brief This function unsubscribes mqtt topic
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param const char *pTopic, pointer to the topic to unsubscribe
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection and subscribed to same topic
 */
int8_t MqttUnSubscribeTopic(MqttBrokerHandler handle, const char *pTopic)
{
    uint8_t idx = 0;

    if (!IsModemReady())
        return -3;

    if (!IsATavailable())
        return -2;

    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    // check if same topic is already available for same broker
    for (idx = 0; idx < MAX_MQTT_SUB_TOPICS; idx++)
    {
        if (0 == strcmp(mqttBroker[handle].subTopic[idx].topic, pTopic))
        {
            DBG("\r\nMQTT_STATUS: Sub Topic found\r\n");
            if (mqttBroker[handle].subTopic[idx].subscribed == true)
            {
                int ret = _mqttUnSubscribeTopic(handle, mqttBroker[handle].subTopic[idx].msgID, mqttBroker[handle].subTopic[idx].topic);
                if (1 == ret)
                    return 1;
                else
                    return 0;
            }
            else
            {
                // not subscribed yet
                mqttBroker[handle].subTopic[idx].topic[0] = 0; // delete topic
            }
        }
    }

    if (idx >= MAX_MQTT_SUB_TOPICS)
    {
        DBG("\r\nMQTT_ERROR: Unkown Sub Topics\r\n");
    }

    return -1;
}

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
int8_t MqttPublishTopic(MqttBrokerHandler handle, const char *pTopic, MqttQos_t qos, bool retain, const char *data, uint16_t dataLen)
{
    if (!IsModemReady())
        return -3;

    if (!IsATavailable())
        return -2;

    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    if (!mqttBroker[handle].isConnected)
    {
        return 0;
    }
    uint16_t msgid = _mqttGetMsgId();
    int8_t ret = _mqttPublishTopic(handle, msgid, pTopic, qos, retain, data, dataLen);

    if (1 == ret)
        return 1;

    return 0;
}

/**
 * @fn int8_t MqttClose(MqttBrokerHandler handle)
 * @brief This function start the procedure to close mqtt cnnection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @return -ve number: validation fail, 0: fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttClose(MqttBrokerHandler handle)
{
    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    // provided handle is not valid or not created
    if (!mqttBroker[handle].isValid)
    {
        return 0;
    }

    // if not connected to broker then simply clear the flag
    if (!mqttBroker[handle].isConnected)
    {
        mqttBroker[handle].isValid = false;
        return 1;
    }

    // connected to broker, start disconnection process
    mqttBroker[handle].brokerCnxEvent = BRKR_EVT_DISCONNECT_RQST;

    return 1;
}

/**
 * @fn bool IsMqttConnected(MqttBrokerHandler handle)
 * @brief This function returns the connection status of the broker
 * @param MqttBrokerHandler handle, mqtt broker identifier
 * @return false: not connected, true: connected
 * @remark user need to first create mqtt connection
 */
bool IsMqttConnected(MqttBrokerHandler handle)
{
    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return 0;

    return mqttBroker[handle].isConnected;
}

/**
 * @fn int8_t MqttVersionConfig(MqttBrokerHandler handle, uint8_t version)
 * @brief This function takes ssl parameters for broker connection
 * @param MqttBrokerHandler handle, mqtt connection identifier
 * @param uint8_t version Integer type. MQTT version number. 0 MQTT Version 3.1, 1 MQTT Version 3.1.1
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark user need to first create mqtt connection
 */
int8_t MqttVersionConfig(MqttBrokerHandler handle, uint8_t version)
{
    if (handle < 0 || handle >= MAX_MQTT_BORKERS)
        return -1;

    if (mqttBroker[handle].isValid)
    {
        mqttBroker[handle].mqttVersionNum = version;
        return 1;
    }

    return 0;
}

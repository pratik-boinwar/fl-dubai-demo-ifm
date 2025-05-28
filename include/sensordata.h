#ifndef _SENSOR_H
#define _SENSOR_H

#include "Arduino.h"
#include "globals.h"
#include "logger.h"

#define PUMP_RESPONSE_TIMEOUT 3000

typedef enum _GET_FM_DATA_STATE_
{
    GET_FM_DATA_STATE_IDLE,

    GET_FM1_DATA_STATE1,
    // GET_FM1_DATA_STATE2,

    GET_FM2_DATA_STATE1,
    // GET_FM2_DATA_STATE2,

    GET_FM_DATA_STATE_DONE_GO_IDLE,

    GET_SENSOR_DATA_STATE_RECIEVE_RES,
    GET_SENSOR_DATA_STATE_EM_RESPONSE_RX_DONE,
    GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY,
    GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE,
    GET_FM_DATA_STATE_TAKE_REST,
} eGET_FM_DATA;

typedef enum _SEND_FM_DATA_STATE_
{
    SEND_FM_DATA_STATE_IDLE,
    SEND_FM_DATA_STATE_CALCULATION_TO_FORM_FILL_JSON,
    SEND_FM_DATA_SEND_HTTP_PACKET,
    SEND_FM_DATA_RX_HTTP_RESP
} eSEND_FM_DATA;

extern int powerIndex;
extern int flowCalP1, flowCalP2, flowCalF1, flowCalF2;

void GetSensorDataLoop(void);
/**
 * @fn void SendSensorDataLoop(void)
 * @brief State Machine which runs a loop for sending data to cloud.
 */
void SendSensorDataLoop(void);

void SetRs485UartConfigSettings(int baudrate, int databits, int stopbits, int parity);
void ModbusRTU_Init(void);

// bool PrepareHttpJsonPacket(String &finalHttpPacketToSend, String dataPacket = "");
// bool PrepareMqttJsonPacket(String &finalMqttPacketToSend);
// bool HttpSendDataNow(const char *ipaddr, int port, const char *url, String dataPacket, char *response, uint32_t sizeofResponse, uint32_t timeout_ms);
// bool HttpSendDataNow(const char *ipaddr, int port, const char *url, String dataPacket, char *response, uint32_t sizeofResponse);
// uint8_t HttpDataReceive(uint32_t timeout_ms);

/**
 * @fn void CreateHttpPacketToSend(TRANSACTION *mqttTrans, String &LogTobeSend)
 * @brief prepare mqtt data packet to send over mqtt
 */
void CreateHttpPacketToSend(TRANSACTION *mqttTrans, String &LogTobeSend);

/**
 * @fn uint32_t GetSystemAlertStatus()
 * @brief This function returns system generated alerts.
 * @return remote alert number
 */
uint32_t GetSystemAlertStatus(void);

extern HardwareSerial COD_SENSOR_SERIAL;
#endif
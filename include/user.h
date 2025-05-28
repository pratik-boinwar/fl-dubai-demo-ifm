#ifndef USER_H
#define USER_H

#include "Arduino.h"
#include "CircularBuffer.h"
// #include "WiFiServer.h"
// #include "BluetoothSerial.h"
#include "globals.h"

#define UDP_LISTEN_PORT 30303
#define BLUETOOTH_TIMEOUT 30 * MINUTE // bluetooth timeout to disable bluetooth

#define SEND_UNSEND_LOG_TIMEOUT 200 // send unsend logs on serial/bluetooth every second

typedef enum _BLUETOOTH_SM_
{
    BLUETOOTH_SM_IDLE_STATE,
    BLUETOOTH_SM_START_BLUETOOTH_STATE,
    BLUETOOTH_SM_CHECK_FOR_CLIENT,
    BLUETOOTH_SM_WAIT_FOR_SOMETIME,
    BLUETOOTH_SM_CLIENT_CONNECTED,
    BLUETOOTH_SM_IS_CLIENT_DISCONNECTED
} eBLUETOOTH_SM;

typedef enum _UNSEND_LOGS_SM_
{
    UNSEND_LOGS_IDLE_STATE,
    UNSEND_LOGS_SEND_MQTT_LOGS,
    UNSEND_LOGS_SEND_HTTP_LOGS,
    UNSEND_LOGS_WAIT_FOR_SOMETIME
} eUNSEND_LOGS_SM;

extern void GetMacFromEsp(void);
extern void CreateCircularBuffer(void);
extern void CheckForConnections(void);
extern void SerialCmdLine(void);
extern void TelnetSerialInit(void);
extern void TelnetSerialEnd(void);
extern void TelnetSerialCmdLine(void);
extern void BluetoothCmdLine(void);
extern void DeviceDiscoveryLoop(void);
extern bool CheckSignalQuality(void);

bool SerialCommandFromHttp(char *command);

// bool LoggerInit(void);
void LoggerLoop(void);

void SysInit(void);

void Print_reset_reason(int reason);

/**
 * @fn int CreateHttpClubbedLogPacketToSend(String &LogTobeSend)
 * @brief prepare http data packet to send over http
 * @return int return number logs sent to server
 */
int CreateHttpClubbedLogPacketToSend(String &LogTobeSend);
void WiFiEthNetclientLoop(void);
void HttpSendPendingLogsLoop(void);
bool HTTPPostData(const char *url, const char *postData);
// HTTPGetData will send a GET request to the specified url and will return the response in payload
bool HTTPDownloadFirmware(const char *url);
/**
 * @fn bool GetNetworkTime(tNETWORKDT *datetime)
 * @brief This function returns the success true and false on basis of network time availability.
 * @param tNETWORKDT *datetime, pointer to structure to copy date-time data.
 * @return false: date-time not available, true: date-time available
 * @remark info will available after network registration.
 * @example
 * tNETWORKDT datetime;
 * bool status = GetNetworkTime(&datetime);
 */
bool GetNetworkTime(tNETWORKDT *datetime);

// extern WiFiServer tcpServer;
// extern BluetoothSerial serialBT;

#endif

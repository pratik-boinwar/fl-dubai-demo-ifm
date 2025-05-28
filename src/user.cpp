/*!
 * @file user.cpp
 * @brief This file contains all the user defined functions
 * @author Nivrutti Mahajan.
 */

#include "user.h"
#include "globals.h"
#include "AsyncUDP.h"
#include "ota.h"
#include "ssl_cert.h"
#include "netclient.h"
#include "QuecGsmGprs.h"
#include "logger.h"
#include "typedefs.h"
#include "global-vars.h"
#include "app.h"
#include "flWifiHandler.h"
#include "sensordata.h"
#include <HTTPClient.h>
#include "app-mqtt.h"
#include <Update.h> // Required for OTA updates

// #include "flmodbus.h"
#include "buzzer.h"
#include "fl-ethernet.h"

typedef enum DEVCIE_DISCOVERY_STATES
{
    DISCOVERY_STATE_IDLE,
    DISCOVERY_STATE_MONITOR_CONNECTION,
} eDEVCIE_DISCOVERY_STATES;

eDEVCIE_DISCOVERY_STATES deviceState = DISCOVERY_STATE_IDLE;

static eBLUETOOTH_SM _bluetoothSMState = BLUETOOTH_SM_IDLE_STATE;

eUNSEND_LOGS_SM _sendUnsendLogsCurrState = UNSEND_LOGS_IDLE_STATE;
eUNSEND_LOGS_SM _sendUnsendLogsPrevState = UNSEND_LOGS_IDLE_STATE;

HTTPClient http;

// TODO: make this port configurable by command
WiFiServer TelnetServer(23);
WiFiClient Telnet;

// BluetoothSerial serialBT;
CIRCULAR_BUFFER respBuf, udpRepBuff;

String replyStr;

AsyncUDP udp;

byte token[50];

uint8_t rxbuff[255];
uint8_t length;
uint8_t data[20] = {0};
char utRxdCmd[MAX_SIZEOF_COMMAND];
char ch;
int status, udpCmdlineStatus;
byte rxCh;
int rxIndex = 0;
int serAvailableCnt, tcpAvailableCnt, udpAvailabeCnt;

char commandRespStr[MAX_SIZEOF_COMMAND];
uint16_t cmdRespIndex = 0;

bool startAfter5Min = true;

const char *mqtt_db_name = "/mqttlog.db";
const char *http_db_name = "/httplog.db";

String certificateDownloadFileName;

extern char devBootTime[20];

void GetUnsendLogsChangeState(eUNSEND_LOGS_SM state);

/**
 * @fn void GetMacFromEsp(void)
 * @brief This function get the mac address/device address from esp32
 */
void GetMacFromEsp(void)
{
    // Reading mac Address of device.
    systemPara.macId = ESP.getEfuseMac();
    memcpy(deviceAddress.id, &systemPara.macId, sizeof(systemPara.macId));

    for (uint8_t i = 0; i < sizeof(deviceAddress); i++)
    {
        deviceID += String(deviceAddress.id[i], HEX);
        if (i != 5)
        {
            deviceID += ":";
        }
    }

    deviceID.toUpperCase();
}

/**
 * @fn void CreateCircularBuffer(void)
 * @brief This is user function create circular buffer for serial cmdline and udp
 */
void CreateCircularBuffer(void)
{
    // creating size for Circular Buffer.
    if (0 != CbCreate(&respBuf, 2000))
    {
        debugPrintln("Errror in creating Circular Buffer For Serial");
    }

    if (0 != CbCreate(&udpRepBuff, 200))
    {
        debugPrintln("Errror in creating Circular Buffer");
    }

    if (0 != CbCreate(&smsRespBuf, 250))
    {
        debugPrintln("Errror in creating Circular Buffer for sms");
    }
}

void TelnetSerialInit(void)
{
    TelnetServer.begin();
    // there was a lag in reception of data observed. Got fixed using this function.
    TelnetServer.setNoDelay(true);
}
void TelnetSerialEnd(void)
{
    TelnetServer.close();
}

/**
 * @fn void CheckForConnections(void)
 * @brief This function checks is any tcpclient is connected or not
 */
void CheckForConnections(void)
{
    // if (FlWifi.isConnected())
    // {
    //     if (!serverConFlg)
    //     {
    //         TelnetServer.begin();
    //         // there was a lag in reception of data observed. Got fixed using this function.
    //         TelnetServer.setNoDelay(true);

    //         // if (udp.listen(UDP_LISTEN_PORT))
    //         // {
    //         //     debugPrintln("[DEBUG] UDP Listening started");
    //         // }
    //         serverConFlg = true;
    //     }

    //     if (!dnsStsFlg)
    //     {
    //         // dnsInit(gSysParam.dns); // Initializing dns again
    //         // WebServerStart();
    //         dnsStsFlg = true;
    //     }

    //     if (!otaSetupFlg)
    //     {
    //         // OtaInit();
    //         otaSetupFlg = true;
    //     }
    // }
    // else
    // {
    //     if (serverConFlg)
    //     {
    //         // TelnetServer.stop();
    //         // WebServerStop();
    //         // udp.close();
    //         serverConFlg = false;
    //     }

    //     if (dnsStsFlg)
    //     {
    //         dnsStsFlg = false;
    //         // MDNS.end(); // dns closed
    //         addServiceFlag = true;
    //     }

    //     if (otaSetupFlg)
    //     {
    //         otaSetupFlg = false;
    //     }
    // }

    if (TelnetServer.hasClient())
    {
        // If we are already connected to another computer,
        // then reject the new connection. Otherwise accept
        // the connection.
        if (Telnet.connected())
        {
            debugPrintln("[TCP-SERVER] Connection rejected, Already Connected!");
            TelnetServer.stop();
            Telnet.stop();
        }
        else
        {
            debugPrintln("[TCP-SERVER] Connection accepted!");
            Telnet = TelnetServer.available();
        }
    }
}

/**
 * @fn void SerialCmdLine(void)
 * @brief This function recieves serial command through serial and process it
 * print response to serial for the command recieved.
 */
void SerialCmdLine(void)
{
    // uint32_t len;
    static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;

    // Serial Cmd Line
    if (Serial.available() > 0)
    {
        serAvailableCnt = Serial.available();

        for (int i = 0; i < serAvailableCnt; i++)
        {
            rxCh = Serial.read();
            if (MAX_SIZEOF_COMMAND == rxIndex)
            {
                rxIndex = 0;
            }
            if ((0x0D == rxCh) || (0x0A == rxCh))
            {
                // debugPrint("[CMDLINE] Cmd Rxd: ");
                utRxdCmd[rxIndex] = 0;
                rxIndex = 0;
                Serial.println(utRxdCmd);
                status = CmdLineProcess(utRxdCmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
                // debugPrint("[CMDLINE] Cmd Resp: ");
                while (0 == CbPop(&respBuf, &ch))
                {
                    Serial.write(ch);
                }
                memset(utRxdCmd, 0, sizeof(utRxdCmd));
                CbClear(&respBuf);
                rxIndex = 0;
                // Serial.println(); //neev: removed this for akshay's rotomag utility
            }
            else
            {
                if (rxCh == 0x08)
                {
                    rxIndex--;
                }
                else
                {
                    utRxdCmd[rxIndex] = rxCh;
                    rxIndex++;
                }
            }
        }
        serAvailableCnt = 0;

        // len = Serial.readBytesUntil('\r', utRxdCmd, MAX_SIZEOF_COMMAND);
        // utRxdCmd[len] = 0;
        // if (len)
        // {
        //     Serial.println(utRxdCmd);
        //     status = CmdLineProcess(utRxdCmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
        //     // debugPrint("[CMDLINE] Cmd Resp: ");
        //     while (0 == CbPop(&respBuf, &ch))
        //     {
        //         Serial.write(ch);
        //     }
        // }
    }
}

/**
 * @fn void TelnetSerialCmdLine(void)
 * @brief This function recieves serial command through tcp connection and process it
 * print response to serial and tcp client terminal for the command recieved.
 */
void TelnetSerialCmdLine(void)
{
    static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;
    // Serial Cmd Line
    if (Telnet.available() > 0)
    {
        tcpAvailableCnt = Telnet.available();

        for (byte i = 0; i < tcpAvailableCnt; i++)
        {
            rxCh = Telnet.read();
            if ((0x0D == rxCh) || (0x0A == rxCh))
            {
                // debugPrint("[CMDLINE] Cmd Rxd: ");
                utRxdCmd[rxIndex] = 0;
                rxIndex = 0;
                Serial.println(utRxdCmd);
                status = CmdLineProcess(utRxdCmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
                // debugPrint("[CMDLINE] Cmd Resp: ");
                memset(commandRespStr, 0, sizeof(commandRespStr));
                cmdRespIndex = 0;
                while (0 == CbPop(&respBuf, &ch))
                {
                    commandRespStr[cmdRespIndex] = ch;
                    cmdRespIndex++;
                    Serial.write(ch);
                }
                Serial.println();
                commandRespStr[cmdRespIndex] = 0;
                if (Telnet.connected())
                {
                    Telnet.println(commandRespStr);
                }
                CbClear(&respBuf);
            }
            else
            {
                utRxdCmd[rxIndex] = rxCh;
                rxIndex++;
            }
        }
        tcpAvailableCnt = 0;
    }
}

// /**
//  * @fn void BluetoothCmdLine(void)
//  * @brief This function recieves command from bluetooth application and process it
//  * print response to serial and on bluetooth terminal as well for the command recieved.
//  */
// void BluetoothCmdLine(void)
// {
//     static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;
//     String bluetoothName = "SMART-STP-MONITORING";
//     static uint32_t lastBLmillis = millis();
//     static uint32_t lastStartBLmillis = millis();
//     static uint32_t waitForSomeTimeMillis = millis();
//     String sendBtData = "";

//     switch (_bluetoothSMState)
//     {
//     case BLUETOOTH_SM_IDLE_STATE:
//         if (true == systemFlags.makeBluetoothOn)
//         {
//             systemFlags.makeBluetoothOn = false;
//             Serial.println("here2");
//             _bluetoothSMState = BLUETOOTH_SM_START_BLUETOOTH_STATE;
//             break;
//         }
//         if (true == startAfter5Min)
//         {
//             if (millis() - lastStartBLmillis > 1000) // BLUETOOTH_TIMEOUT)
//             {
//                 startAfter5Min = false;
//                 Serial.println("here3");
//                 _bluetoothSMState = BLUETOOTH_SM_START_BLUETOOTH_STATE;
//                 lastStartBLmillis = millis();
//             }
//         }
//         break;

//     case BLUETOOTH_SM_START_BLUETOOTH_STATE:
//         // bluetoothName.concat(deviceID);
//         Serial.println("here");
//         if (serialBT.begin("neev-dev"))
//         {
//             systemFlags.bluetoothStatus = true;
//             debugPrintln("[BT] Bluetooth started!");
//         }
//         else
//         {
//             systemFlags.bluetoothStatus = false;
//             debugPrintln("[BT] Bluetooth failed to start!");
//         }
//         Serial.println("here1");
//         lastBLmillis = millis();
//         _bluetoothSMState = BLUETOOTH_SM_CHECK_FOR_CLIENT;
//         break;

//     case BLUETOOTH_SM_CHECK_FOR_CLIENT:
//         // if (millis() - lastBLmillis > BLUETOOTH_TIMEOUT)
//         // {
//         //     lastBLmillis = millis();
//         //     serialBT.end();
//         //     systemFlags.bluetoothStatus = false;
//         //     debugPrintln("[BT] No device is connecting since long time, Bluetooth disabled!");
//         //     _bluetoothSMState = BLUETOOTH_SM_IDLE_STATE;
//         // }
//         // else
//         if (true == systemFlags.makeBluetoothOn)
//         {
//             systemFlags.makeBluetoothOn = false;
//             debugPrintln("[BT] Bluetooth is already on");
//             serialBT.end();
//             serialBT.begin(bluetoothName);
//             break;
//         }
//         else
//         {
//             if (!serialBT.hasClient())
//             {
//                 waitForSomeTimeMillis = millis();
//                 _bluetoothSMState = BLUETOOTH_SM_WAIT_FOR_SOMETIME;
//                 break;
//             }
//             else
//             {
//                 debugPrintln("[BT] Bluetooth client device connected!");
//                 _bluetoothSMState = BLUETOOTH_SM_CLIENT_CONNECTED;
//                 break;
//             }
//         }
//         break;

//     case BLUETOOTH_SM_WAIT_FOR_SOMETIME:
//         if (millis() - waitForSomeTimeMillis > 50)
//         {
//             _bluetoothSMState = BLUETOOTH_SM_CHECK_FOR_CLIENT;
//         }
//         /* code */
//         break;

//     case BLUETOOTH_SM_CLIENT_CONNECTED:
//         if (serialBT.available() > 0)
//         {
//             serAvailableCnt = serialBT.available();

//             for (int i = 0; i < serAvailableCnt; i++)
//             {
//                 rxCh = serialBT.read();
//                 if (MAX_SIZEOF_COMMAND == rxIndex)
//                 {
//                     rxIndex = 0;
//                 }
//                 if (0x0A == rxCh)
//                 {
//                     utRxdCmd[rxIndex - 1] = 0;
//                     utRxdCmd[rxIndex] = 0;
//                     rxIndex = 0;
//                     // debugPrint("[CMDLINE] Command Rxd from bluetooth: ");
//                     Serial.println(utRxdCmd);
//                     status = CmdLineProcess(utRxdCmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
//                     // debugPrint("[CMDLINE] Response sent over bluetooth: ");
//                     sendBtData = "";
//                     serialBT.flush();
//                     while (0 == CbPop(&respBuf, &ch))
//                     {
//                         Serial.write(ch);
//                         sendBtData += ch;
//                         // serialBT.write(ch);
//                     }
//                     // Serial.println();
//                     serialBT.print(sendBtData.c_str());
//                     serAvailableCnt = 0;
//                     memset(utRxdCmd, 0, sizeof(utRxdCmd));
//                     CbClear(&respBuf);
//                     rxIndex = 0;
//                     return;
//                 }
//                 if (rxCh == 0x08)
//                 {
//                     rxIndex--;
//                 }
//                 else
//                 {
//                     utRxdCmd[rxIndex] = rxCh;
//                     rxIndex++;
//                 }
//             }
//             utRxdCmd[rxIndex] = 0;
//             rxIndex = 0;
//             // debugPrint("[CMDLINE] Command Rxd from bluetooth: ");
//             Serial.println(utRxdCmd);
//             status = CmdLineProcess(utRxdCmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
//             // debugPrint("[CMDLINE] Response sent over bluetooth: ");
//             sendBtData = "";
//             serialBT.flush();
//             while (0 == CbPop(&respBuf, &ch))
//             {
//                 Serial.write(ch);
//                 sendBtData += ch;
//                 // serialBT.write(ch);
//             }
//             // Serial.println();
//             serialBT.print(sendBtData.c_str());
//             serAvailableCnt = 0;
//             memset(utRxdCmd, 0, sizeof(utRxdCmd));
//             CbClear(&respBuf);
//             rxIndex = 0;
//         }
//         else
//         {
//             _bluetoothSMState = BLUETOOTH_SM_IS_CLIENT_DISCONNECTED;
//         }
//         break;

//     case BLUETOOTH_SM_IS_CLIENT_DISCONNECTED:
//         if (!serialBT.hasClient())
//         {
//             debugPrintln("[BT] Bluetooth client device disconnected!");
//             lastBLmillis = millis();
//             _bluetoothSMState = BLUETOOTH_SM_CHECK_FOR_CLIENT;
//         }
//         else if (true == systemFlags.makeBluetoothOn)
//         {
//             systemFlags.makeBluetoothOn = false;

//             debugPrintln("[BT] Bluetooth is already connected to a client!");
//             break;
//         }
//         else
//         {
//             _bluetoothSMState = BLUETOOTH_SM_CLIENT_CONNECTED;
//         }
//         break;

//     default:
//         break;
//     }
// }

/**
 * @fn void DeviceDiscoveryLoop()
 * @brief This function recieves command from udp terminal and it sends response to udp terminal
 */
void DeviceDiscoveryLoop(void)
{
    switch (deviceState)
    {
    case DISCOVERY_STATE_IDLE:
        // if (IsEthernetConnected())
        {
            if (udp.listen(UDP_LISTEN_PORT))
            {
                static PARSE_ID_TYPE modeOfOperation = BYPASS_SEQ_ID;
                static char udpCh;
                debugPrintln("[DEBUG] UDP Listening started");
                udp.onPacket([](AsyncUDPPacket packet)
                             {
                                 debugPrint("UDP Packet Type: ");
                                 debugPrint(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast"
                                                                                                      : "Unicast");
                                 debugPrint(", From: ");
                                 debugPrint(packet.remoteIP());
                                 debugPrint(":");
                                 debugPrint(packet.remotePort());
                                 debugPrint(", To: ");
                                 debugPrint(packet.localIP());
                                 debugPrint(":");
                                 debugPrint(packet.localPort());
                                 debugPrint(", Length: ");
                                 debugPrint(packet.length());
                                 debugPrint(", Data: ");
                                 debugSmt(Serial.write(packet.data(), packet.length());
                                          );
                                 debugPrintln();
                                 memset(token, 0, 50);
                                 memcpy(token, packet.data(), packet.length());
                                 udpCmdlineStatus = CmdLineProcess((char *)token, &udpRepBuff, &modeOfOperation);

                                 replyStr = "";

                                 while (!CbPop(&udpRepBuff, &udpCh))
                                 {
                                     replyStr += udpCh;
                                 }
                                 packet.print(replyStr); });
                deviceState = DISCOVERY_STATE_MONITOR_CONNECTION;
            }
        }
        break;
    case DISCOVERY_STATE_MONITOR_CONNECTION:
        if (!udp.connected())
        {
            deviceState = DISCOVERY_STATE_IDLE;
        }
        break;

    default:
        deviceState = DISCOVERY_STATE_IDLE;
        break;
    }
}

bool SerialCommandFromHttp(char *command)
{
    static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;

    Serial.println(command);
    int status = CmdLineProcess(command, &respBuf, (void *)&testType, CheckForSerquenceNum);
    // debugPrint("[CMDLINE] Response sent over bluetooth: ");
    while (0 == CbPop(&respBuf, &ch))
    {
        Serial.write(ch);
    }
    // Serial.println();
    if (status)
    {
        return true;
    }
    return false;
}

bool CheckSignalQuality(void)
{
    /*
        rssi [0 to 31] : [-113 to -51] dBm
        rssi [ 0 to 9] : Marginal
        rssi [ 10 to 14] : Ok
        rssi [ 15 to 19] : Good
        rssi [ 20 to 31] : Excellent
        rssi [99] : not known or not detectable
    */

    /*  0 --- 113 dBm or less
        1 --- 111 dBm
        2 to 30 --- 109 to 53 dBm
        31 --- 51 dBm or greater
        99 --- not known or not detectable
    */

    uint8_t rssi = 0;
    static uint32_t prevRSSI = 0;

    GetSignalQuality(&rssi);

    if (rssi != prevRSSI)
    {
        if (rssi == 99)
        {
            systemPara.signalStregth = 0xFFFFFFFF;
            systemPara.signalStregthLowest = rssi;
            systemPara.signalStregthLowOccurence++;
            systemFlags.AntennaError = true;
            // registerRAE(RAE_Antenna_error);
        }
        else
        {
            systemPara.signalStregth = (-113) + (2 * rssi);
            // debugPrintf("RSSI: %d\n", rssi);
            // debugPrintf("systemPara.signalStregth: %X\n", systemPara.signalStregth);

            if (rssi < systemPara.signalStregthLowest)
            {
                systemPara.signalStregthLowest = rssi;
            }
            if (rssi < 10)
            {
                systemPara.signalStregthLowOccurence++;
            }
            systemFlags.AntennaError = false;
            // unregisterRAE(RAE_Antenna_error);
        }
        prevRSSI = rssi;
        return true;
    }
    prevRSSI = rssi;
    return false;
    // systemPara.updateSignalStrength = 1;
}

// bool LoggerInit(void)
// {
//     // efsdbMqttLogHandle = efsdbLoggerSubscribe(mqtt_db_name, (3 * 1024 * 1024), sizeof(MQTT_TRANSACTION), SPIFFS);
//     // efsdbHttpLogHandle = efsdbLoggerSubscribe(http_db_name, (3 * 1024 * 1024), sizeof(HTTP_TRANSACTION), SPIFFS);
//     return true;
// }

void LoggerLoop(void)
{
    return;
}

void SysInit(void)
{
    systemFlags.all = false;
}

void Print_reset_reason(int reason)
{
    switch (reason)
    {
    case 1:
        debugPrintln("POWERON_RESET");
        break; /**<1,  Vbat power on reset*/
    case 3:
        debugPrintln("SW_RESET");
        break; /**<3,  Software reset digital core*/
    case 4:
        debugPrintln("OWDT_RESET");
        break; /**<4,  Legacy watch dog reset digital core*/
    case 5:
        debugPrintln("DEEPSLEEP_RESET");
        break; /**<5,  Deep Sleep reset digital core*/
    case 6:
        debugPrintln("SDIO_RESET");
        break; /**<6,  Reset by SLC module, reset digital core*/
    case 7:
        debugPrintln("TG0WDT_SYS_RESET");
        break; /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:
        debugPrintln("TG1WDT_SYS_RESET");
        break; /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9:
        debugPrintln("RTCWDT_SYS_RESET");
        break; /**<9,  RTC Watch dog Reset digital core*/
    case 10:
        debugPrintln("INTRUSION_RESET");
        break; /**<10, Instrusion tested to reset CPU*/
    case 11:
        debugPrintln("TGWDT_CPU_RESET");
        break; /**<11, Time Group reset CPU*/
    case 12:
        debugPrintln("SW_CPU_RESET");
        break; /**<12, Software reset CPU*/
    case 13:
        debugPrintln("RTCWDT_CPU_RESET");
        break; /**<13, RTC Watch dog Reset CPU*/
    case 14:
        debugPrintln("EXT_CPU_RESET");
        break; /**<14, for APP CPU, reseted by PRO CPU*/
    case 15:
        debugPrintln("RTCWDT_BROWN_OUT_RESET");
        break; /**<15, Reset when the vdd voltage is not stable*/
    case 16:
        debugPrintln("RTCWDT_RTC_RESET");
        break; /**<16, RTC Watch dog reset digital core and rtc module*/
    default:
        debugPrintln("NO_MEAN");
    }
}

void HttpSendPendingLogsLoop(void)
{
    static uint32_t _lastSendDataMillis = 0;
    static TRANSACTION _httpSendTrans;
    String logToBeSend;
    char httpCompleteUrl[200] = {0};

#ifdef ENABLE_HTTP_ARRAY_LOG_SENDING
    uint32_t unsendLogCount = 0;
    uint32_t numOfLogsSent = 0;
#endif

    // send logs every 200ms
    if (millis() - _lastSendDataMillis > 1000)
    {
        _lastSendDataMillis = millis();

        if (!gSysParam.logSendHttp) // check added for data send over mqtt
            return;

        if (false == FlWifi.isConnected()) // if mqtt is not connected or busy, then return
            return;

        if (!LoggerIsPacketAvailable())
            return;

        debugPrintln();
        debugPrintln("[USER] Retriving Data from httplog...");

#ifdef ENABLE_HTTP_ARRAY_LOG_SENDING
        LoggerGetCount(&unsendLogCount);
        if (unsendLogCount > 0)
        {
            numOfLogsSent = CreateHttpClubbedLogPacketToSend(httpPacketToSend); // this will format the JSON packet
#else
        if (LoggerGetPacketByLogNumber(&_httpSendTrans))
        {
            CreateHttpPacketToSend(&_httpSendTrans, logToBeSend); // this will format the JSON packet
#endif
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

            debugPrint("Link: ");
            debugPrintln(httpCompleteUrl);

            // send data over http was getting failed, so added delay of 1ms resolved this issue
            delay(1);
            if (http.begin(httpCompleteUrl))
            {
                http.addHeader("Content-Type", "application/json"); // Specify content-type header
                int httpCode = http.POST(logToBeSend);              // Send the request
                String payload = http.getString();                  // Get the response payload
                if (httpCode >= HTTP_CODE_OK)
                {
                    debugPrintln("[DEBUG] HTTP data packet Sent");
                    debugPrintf("[DEBUG] HTTP error code: %d\n", httpCode);
#ifdef ENABLE_HTTP_ARRAY_LOG_SENDING
                    LoggerDeletePacket(numOfLogsSent);
#else
                    LoggerDeletePacket();
#endif
                }
                else
                {
                    debugPrintf("[DEBUG] HTTP data packet Send failed, error code: %d\n", httpCode);
                }
                http.end();

                debugPrint("Received data: ");
                debugPrintln(payload);
            }
            else
            {
                debugPrintln("[DEBUG] HTTP connection failed");
            }
        }
        else
        {
            // control should not come here
            // if control comes here, then there is some problem in LoggerGetPacket
            debugPrintln("[USER] No data packet found!");
        }
        debugPrintln();
    }
}

// HTTPGetData will send a GET request to the specified url and will return the response in payload
bool HTTPGetData(const char *url, String &payload)
{
    // HTTPClient http;

    bool ret = false;
    if (true == FlWifi.isConnected())
    {
        if (http.begin(url))
        {
            int httpCode = http.GET(); // Send the request
            if (httpCode == 200)
            {
                payload = http.getString(); // Get the response payload
                ret = true;
            }
            else
            {
                debugPrintf("[DEBUG] HTTP data packet Send failed, error code: %d\n", httpCode);
            }
            http.end();
        }
        else
        {
            debugPrintln("[DEBUG] HTTP connection failed");
        }
    }
    else
    {
        debugPrintln("[DEBUG] Ethernet/WiFi not connected!");
    }
    return ret;
}

bool HTTPPostData(const char *url, const char *postData)
{
    bool ret = false;

    if (true == FlWifi.isConnected())
    {
        // send data over http was getting failed, so added delay of 1ms resolved this issue
        delay(1);
        http.begin(url);
        http.addHeader("Content-Type", "application/json"); // Specify content-type header
        int httpCode = http.POST(String(postData));         // Send the request
        String payload = http.getString();                  // Get the response payload
        debugPrintf("[DEBUG] Payload received: %s\n", payload.c_str());
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED)
        {
            debugPrintln("[DEBUG] HTTP data packet Sent");
            ret = true;
        }
        else
        {
            debugPrintf("[DEBUG] HTTP data packet Send failed, error code: %d\n", httpCode);
        }
        http.end();
    }

    else
    {
        debugPrintln("[DEBUG] Ethernet/WiFi not connected!");
    }
    return ret;
}

// HTTPGetData will send a GET request to the specified url and will return the response in payload
bool HTTPDownloadFirmware(const char *url)
{
    static uint8_t percentageOfOta = 0; // to show download percentage

    HTTPClient httpOta;

    bool ret = false;

    if (true == FlWifi.isConnected())
    {
        if (httpOta.begin(url))
        {
            int httpCode = httpOta.GET(); // Send the request
                                          // Check if the request was successful and the file is available
            if (httpCode == HTTP_CODE_OK)
            {

                // Start OTA update process
                debugPrintln("Starting OTA update...");
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                {
                    // start with max available size
                    Update.printError(Serial);
                }

                // Stream the firmware update data and write it to flash
                WiFiClient *stream = httpOta.getStreamPtr();
                int written = 0;
                uint8_t buffer[1024];
                int totolSize = httpOta.getSize();
                debugPrintf("[OTA] download start, expecting %d bytes\r\n", totolSize);

                percentageOfOta = 0;
                while (httpOta.connected() && (written < httpOta.getSize() || httpOta.getSize() == -1))
                {
                    size_t len = stream->readBytes(buffer, sizeof(buffer));
                    if (!Update.hasError())
                    {
                        if (Update.write(buffer, len) != len)
                        {
                            Update.printError(Serial);
                        }
                        written += len;
                        debugPrintf("[OTA] received length %d ", len);
                        debugPrintf(", written %d\r\n", len);
                        debugPrintf("[OTA] recieved %d bytes,", written);
                        debugPrintf("outof %d bytes\r\n", totolSize);
                        // map totol bytes recieved and maximum expected bytes with 0 to 100 %
                        percentageOfOta = map(written, 0, totolSize, 0, 100);
                        debugPrintf("\n[OTA] Download percentage: %d%%\r\n\n", percentageOfOta);
                    }
                    else
                    {
                        Update.printError(Serial);
                    }
                }

                // Finish OTA update process
                if (Update.end(true))
                {
                    debugPrintln("OTA update successful!");
                    ret = true;
                    ESP.restart();
                }
                else
                {
                    debugPrintln("OTA update failed!");
                }
            }
            else
            {
                debugPrintln("Firmware file download failed!");
            }

            // Close the HTTP connection
            http.end();
        }
        else
        {
            debugPrintln("[DEBUG] HTTP connection failed");
        }
    }
    else
    {
        debugPrintln("[DEBUG] WiFi not connected!");
    }
    return ret;
}

void WiFiEthNetclientLoop(void)
{
    StaticJsonDocument<800> attributeDoc;
    StaticJsonDocument<512> httpRespdoc;
    char commandFromHttp[300];
    static uint32_t restartInfoSendRetryMillis = 0, firmwareSearchRetryMillis = 0, attributeSendRetryMillis = 0;
    String logToBeSend;
    char httpCompleteUrl[200];
    String httpAttributePacket;
    StaticJsonDocument<96> restartInfoDoc;
    String restartInfoOverMqtt;
    String restartInfoTopic;

    if (millis() - restartInfoSendRetryMillis > 3000)
    {
        restartInfoSendRetryMillis = millis();
        if (AppMqttIsConnected())
        {
            if (true == systemFlags.sendRestartTimeToMqttTopic)
            {
                debugPrintln("[NET] Sending Restart info over mqtt topic...");
                restartInfoDoc["TIMESTAMP"] = devBootTime;
                restartInfoDoc["IMEI"] = gSysParam.imei;
                restartInfoDoc["VER"] = FW_VER;
                restartInfoDoc["VER_UPDATE_DATE"] = FW_BUILD;
                restartInfoOverMqtt = "";
                serializeJson(restartInfoDoc, restartInfoOverMqtt);

                restartInfoTopic = DEVICE_MODEL;
                restartInfoTopic += "/";
                restartInfoTopic += "restartinfo";

                debugPrint("Restart Info packet: ");
                debugPrintln(restartInfoOverMqtt.c_str());
                debugPrint("Restart Info topic: ");
                debugPrintln(restartInfoTopic);

                if (AppMqttIsConnected())
                {
                    if (AppMqttPublish(restartInfoTopic.c_str(), restartInfoOverMqtt.c_str()))
                    {
                        debugPrintln("Restart Info packet sent!");
                        systemFlags.sendRestartTimeToMqttTopic = false;
                    }
                    else
                    {
                        debugPrintln("Restart Info packet send failed!");
                    }
                }
                else
                {
                    debugPrintln("Restart Info packet send failed, client not connected!");
                }
            }
        }
    }

    if (millis() - firmwareSearchRetryMillis > 3000)
    {
        firmwareSearchRetryMillis = millis();

        if (systemFlags.searchfirmware)
        {
            /* create JSON */
            StaticJsonDocument<512> root;

            debugPrintln("[NET] Searching Firmware...");

            root["APPNAME"] = APP_NAME;
            root["DEVICE"] = DEVICE_MODEL;
            root["HWID"] = HW_ID;
            root["HWV"] = HW_VER;
            root["FWV"] = FW_VER;
            root["IMEI"] = gSysParam.imei;

            logToBeSend = "";
            serializeJson(root, logToBeSend);
            debugPrint("Get updates packet: ");
            debugPrintln(logToBeSend);

            strcpy(httpCompleteUrl, "http://");
            strcat(httpCompleteUrl, (const char *)gSysParam.flServerHost);
            strcat(httpCompleteUrl, ":");
            char portStr[6];
            IntToAscii(gSysParam.flServerHttpPort, portStr);
            strcat(httpCompleteUrl, portStr);
            strcat(httpCompleteUrl, "/");
            strcat(httpCompleteUrl, "getUpdates");

            debugPrint("Get updates link: ");
            debugPrintln(httpCompleteUrl);

            // send data over http was getting failed, so added delay of 1ms resolved this issue
            delay(1);
            http.begin(httpCompleteUrl);
            http.addHeader("Content-Type", "application/json"); // Specify content-type header
            int httpCode = http.POST(String(logToBeSend));      // Send the request
            String payload = http.getString();                  // Get the response payload
            debugPrintf("[DEBUG] Payload received: %s\n", payload.c_str());
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED)
            {
                debugPrintln("[DEBUG] Get updates request Sent");
                DeserializationError error = deserializeJson(httpRespdoc, payload.c_str());

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
                            HTTPDownloadFirmware(otaURL.c_str());
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
            else
            {
                debugPrintf("[DEBUG] Get updates request failed, error code: %d\n", httpCode);
            }
            http.end();

            /**
             * Note that we are clearing flag here. we are not caring if this http session succeeds or fail.
             * We do not want to keep process busy doing this searching task.
             * Anyhow we are going to be check it on peridoically, may be on hourly basis.
             */
            systemFlags.searchfirmware = 0;
        }
    }

    if (millis() - attributeSendRetryMillis > 3000)
    {
        attributeSendRetryMillis = millis();
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
                // attributeDoc["DN25_FLOW_METER_DATA2_ERROR"] = fmFailedFlagStatus.isGetFM1_Data2ValueFailed;
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

            debugPrint("Attributes link: ");
            debugPrintln(httpCompleteUrl);

            // send data over http was getting failed, so added delay of 1ms resolved this issue
            delay(1);
            http.begin(httpCompleteUrl);
            http.addHeader("Content-Type", "application/json");    // Specify content-type header
            int httpCode = http.POST(String(httpAttributePacket)); // Send the request
            String payload = http.getString();                     // Get the response payload
            debugPrintf("[DEBUG] Payload received: %s\n", payload.c_str());
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED)
            {
                debugPrintln("[DEBUG] Attributes Sent success");
                systemFlags.sendAttributesonce = false;
            }
            else
            {
                debugPrintf("[DEBUG] Attributes Sent failed, error code: %d\n", httpCode);
            }
            http.end();
        }
    }

    HttpSendPendingLogsLoop();
    AppMqttLoop();
}

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
bool GetNetworkTime(tNETWORKDT *datetime)
{
    StaticJsonDocument<200> dateTimeDoc;
    static tNETWORKDT _datetime;
    String serverLink, payload;
    bool ret;

    // if AT available and modem is not busy then get latest time
    if (true == FlWifi.isConnected())
    {
        debugPrintln("\n[USER] Getting datetime from server...");
        serverLink = "http://";
        serverLink.concat(gSysParam.flServerHost);
        serverLink.concat(":");
        serverLink.concat(3030);
        // serverLink.concat(gSysParam.flServerHttpPort);
        serverLink.concat("/time");

        debugPrint("Link: ");
        debugPrintln(serverLink);

        if (true == HTTPGetData(serverLink.c_str(), payload))
        {
            debugPrintln(payload);

            DeserializationError error = deserializeJson(dateTimeDoc, payload);

            if (error)
            {
                debugSmt(Serial.print(F("[DEBUG] deserializeJson() failed for download credentials: ")););
                debugPrintln(error.c_str());
                ret = false;
            }

            if (dateTimeDoc.containsKey("status"))
            {
                bool status = dateTimeDoc["status"]; // true
                if (false == status)
                {
                    debugPrintf("[NET] Failed to Sync RTC DT, status: %d", status);
                    ret = false;
                }
            }
            JsonObject datetime = dateTimeDoc["datetime"];
            if (datetime.containsKey("day"))
            {
                if (false == datetime["day"].isNull())
                {
                    _datetime.day = datetime["day"];
                    debugPrintf("[NET] Day: %d\n", _datetime.day);
                }
            }
            if (datetime.containsKey("month"))
            {
                if (false == datetime["month"].isNull())
                {
                    _datetime.month = datetime["month"];
                    debugPrintf("[NET] Month: %d\n", _datetime.month);
                }
            }
            if (datetime.containsKey("year"))
            {
                if (false == datetime["year"].isNull())
                {
                    _datetime.year = datetime["year"];
                    debugPrintf("[NET] Year: %d\n", _datetime.year);
                }
            }
            if (datetime.containsKey("hour"))
            {
                if (false == datetime["hour"].isNull())
                {
                    _datetime.hour = datetime["hour"];
                    debugPrintf("[NET] Hour: %d\n", _datetime.hour);
                }
            }
            if (datetime.containsKey("min"))
            {
                if (false == datetime["min"].isNull())
                {
                    _datetime.min = datetime["min"];
                    debugPrintf("[NET] Min: %d\n", _datetime.min);
                }
            }
            if (datetime.containsKey("sec"))
            {
                if (false == datetime["sec"].isNull())
                {
                    _datetime.sec = datetime["sec"];
                    debugPrintf("[NET] Sec: %d\n", _datetime.sec);
                }
            }
            debugPrintln("[NET] Syncing RTC date time success!\n");
            ret = true;
        }
        else
        {
            debugPrintln("[NET] Syncing RTC date time failed!\n");
            ret = false;
        }
    }
    else
    {
        // debugPrintln("[NET] Syncing RTC date time failed, ETH not connected!\n");
        ret = false;
    }
    datetime->year = _datetime.year;
    datetime->month = _datetime.month;
    datetime->day = _datetime.day;
    datetime->hour = _datetime.hour;
    datetime->min = _datetime.min;
    datetime->sec = _datetime.sec;
    return ret;
}
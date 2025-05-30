/*!
 * @file pumpSM.cpp
 * @brief This file contains the API function for Pump controller.
 * It has a state machine which takes data from pump after every interval and do the required calculations.
 * @author Nivrutti Mahajan.
 */

#include "sensordata.h"
#include "rtc.h"
#include "user.h"

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecMqtt.h"
#include "QuecSsl.h"
#include "QuecHttp.h"
#include "SoftwareSerial.h"
#include "flmodbus.h"
#include "app.h"

extern MqttBrokerHandler _dataBroker;
extern MqttBrokerHandler _flBroker;

extern uint32_t mqttDataSendFailCount;

extern SoftwareSerial softSerial;

String jsonTobeSentImmediatelyOverHttp;

extern uint32_t mqttDataSendFailCount, httpDataSendFailCount;
extern char rxDataBuffer[512];
extern char httpCompleteUrl[200]; // http://serverip:port/url

bool fl1DailyTotalizerSendFlag = false;
bool fl2DailyTotalizerSendFlag = false;

eGET_FM_DATA getFMSensDataSMPrevState = GET_FM_DATA_STATE_IDLE;
eGET_FM_DATA getFMSensDataSMCurrState = GET_FM_DATA_STATE_IDLE;
eGET_FM_DATA getFMDataSMPrevPrevState = GET_FM_DATA_STATE_IDLE;
eGET_FM_DATA jumpToTheState;
eGET_FM_DATA jumpToNextState; // for slave enable disable check

eSEND_FM_DATA sendSensDataSMPrevState = SEND_FM_DATA_STATE_IDLE;
eSEND_FM_DATA sendSensDataSMCurrState = SEND_FM_DATA_STATE_IDLE;

byte pumpSerAvailableCnt, pumpRxCh, dummyCh;
char pumpRxdData[128];
char pumpControllerId[20];
byte recieveIndex = 0;

static uint32_t _pumpRxDataWaitMillis = 0, _pumpResponseTimeoutMillis = 0;
static uint32_t afterCmdSuccessTakeRestMillis = 0;
static uint32_t _millisToUpdatedJson = 0, _millisToTransmitDataJson = 0;

uint8_t retryCount = 0;

String jsonTobeSentOverMqtt;
String jsonTobeSentOverHttp;
String mqttJsonTobeSave;

uFM_FLAGS prevSensorFailedFlagStatus;

// NOTE: DCBA float format of Water Quality sensor
/**
 * @fn void SetRs485UartConfigSettings(int baudrate, int databits, int stopbits, int parity)
 * @brief This function Initializes Pump
 * @param int buadrate It is baudrate to be set for Pump
 * @param int databits
 * @param int stopbits
 * @param int parity
 */
void SetRs485UartConfigSettings(int baudrate, int databits, int stopbits, int parity)
{
    if (databits == 5 && stopbits == 1 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_5N1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 1 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_6N1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 1 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_7N1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 1 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
    }
    if (databits == 5 && stopbits == 2 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_5N2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 2 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_6N2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 2 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_7N2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 2 && parity == 0)
    {
        Serial1.begin(baudrate, SERIAL_8N2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 5 && stopbits == 1 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_5E1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 1 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_6E1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 1 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_7E1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 1 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_8E1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 5 && stopbits == 2 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_5E2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 2 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_6E2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 2 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_7E2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 2 && parity == 1)
    {
        Serial1.begin(baudrate, SERIAL_8E2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 5 && stopbits == 1 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_5O1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 1 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_6O1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 1 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_7O1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 1 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_8O1, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 5 && stopbits == 2 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_5O2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 6 && stopbits == 2 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_6O2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 7 && stopbits == 2 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_7O2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
    if (databits == 8 && stopbits == 2 && parity == 2)
    {
        Serial1.begin(baudrate, SERIAL_8O2, RS485_RX_PIN, RS485_TX_PIN); // pump uart settings
    }
}

/**
 * @fn void ModbusRTU_Init(void)
 * @brief This function Initializes Pump
 */
void ModbusRTU_Init(void)
{
    // Serial1.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
    // RTUutils::prepareHardwareSerial(Serial1);
    SetRs485UartConfigSettings(gSysParam.baud485, gSysParam.dataBits, gSysParam.stopBits, gSysParam.parit);
    // UART1.setRxBufferSize(MAX_SIZEOF_PUMP_RX_BUFFER);
    ModbusInit();
    debugPrintln("[Pump Log] Modbus RTU UART Init done!");
}

/**
 * @fn void GetSensDataSMChangeState(eGET_SENSOR_DATA state)
 * @brief This function changes state of pumpdata statemachine.
 * @param eGET_SENSOR_DATA state
 */
void GetSensDataSMChangeState(eGET_FM_DATA state)
{
    getFMSensDataSMPrevState = getFMSensDataSMCurrState;
    getFMSensDataSMCurrState = state;
}

/**
 * @fn void SendSensDataSMChangeState(eSEND_FM_DATA state)
 * @brief This function changes state of pumpdata statemachine.
 * @param eSEND_FM_DATA state
 */
void SendSensDataSMChangeState(eSEND_FM_DATA state)
{
    sendSensDataSMPrevState = sendSensDataSMCurrState;
    sendSensDataSMCurrState = state;
}

/**
 * @fn void GetSensorDataLoop(void)
 * @brief State Machine which runs a loop for getting data from Energy meter, It sends commands to the Energy meter and wait for response.
 */
void GetSensorDataLoop(void)
{
    static uint8_t retryCntForCmd = 0;

    switch (getFMSensDataSMCurrState)
    {
    case GET_FM_DATA_STATE_IDLE:
        if (millis() - _millisToUpdatedJson > (gSysParam.uRate * SECOND))
        {
            _millisToUpdatedJson = millis();

            debugPrintln("\n**************Reading data from FLOW METER**************");
            GetSensDataSMChangeState(GET_FM1_DATA_STATE1);
            break;
        }
        else
        {
            // wait in this state while timeout
        }
        break;

    case GET_FM1_DATA_STATE1:
        if (1 == gSysParam.modbusSlave1Enable)
        {
            if (GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY == getFMSensDataSMPrevState)
            {
                debugPrintln("[DEBUG] Retrying to read Flow Meter1 Data1 values...");
            }
            else
            {
                debugPrintln("[DEBUG] Reading Flow Meter1 Data1 values...");
            }
            if (false == ReadFlowMeteMData1(gSysParam.sensor1ModAddress))
            {
                // break;
            }
            retryCntForCmd++;
            _pumpResponseTimeoutMillis = millis();
            GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE);
            break;
        }
        else
        {
            jumpToNextState = eGET_FM_DATA(getFMSensDataSMCurrState + 1);
            GetSensDataSMChangeState(jumpToNextState);
            break;
        }

        // case GET_FM1_DATA_STATE2:
        //     if (1 == gSysParam.modbusSlave1Enable)
        //     {
        //         if (GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY == getFMSensDataSMPrevState)
        //         {
        //             debugPrintln("[DEBUG] Retrying to read Flow Meter1 Data2 values...");
        //         }
        //         else
        //         {
        //             debugPrintln("[DEBUG] Reading Flow Meter1 Data2 values...");
        //         }
        //         if (false == ReadFlowMeterData2(gSysParam.sensor1ModAddress))
        //         {
        //             // break;
        //         }
        //         retryCntForCmd++;
        //         _pumpResponseTimeoutMillis = millis();
        //         GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE);
        //         break;
        //     }
        //     else
        //     {
        //         jumpToNextState = eGET_FM_DATA(getFMSensDataSMCurrState + 1);
        //         GetSensDataSMChangeState(jumpToNextState);
        //         break;
        //     }

    case GET_FM2_DATA_STATE1:
        if (1 == gSysParam.modbusSlave2Enable)
        {
            if (GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY == getFMSensDataSMPrevState)
            {
                debugPrintln("[DEBUG] Retrying to read Flow Meter2 Data1 values...");
            }
            else
            {
                debugPrintln("[DEBUG] Reading Flow Meter2 Data1 values...");
            }
            if (false == ReadFlowMeteMData1(gSysParam.sensor2ModAddress))
            {
                // break;
            }
            retryCntForCmd++;
            _pumpResponseTimeoutMillis = millis();
            GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE);
            break;
        }
        else
        {
            jumpToNextState = eGET_FM_DATA(getFMSensDataSMCurrState + 1);
            GetSensDataSMChangeState(jumpToNextState);
            break;
        }

        // case GET_FM2_DATA_STATE2:
        //     if (1 == gSysParam.modbusSlave2Enable)
        //     {
        //         if (GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY == getFMSensDataSMPrevState)
        //         {
        //             debugPrintln("[DEBUG] Retrying to read Flow Meter2 Data2 values...");
        //         }
        //         else
        //         {
        //             debugPrintln("[DEBUG] Reading Flow Meter2 Data2 values...");
        //         }
        //         if (false == ReadFlowMeterData2(gSysParam.sensor2ModAddress))
        //         {
        //             // break;
        //         }
        //         retryCntForCmd++;
        //         _pumpResponseTimeoutMillis = millis();
        //         GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE);
        //         break;
        //     }
        //     else
        //     {
        //         jumpToNextState = eGET_FM_DATA(getFMSensDataSMCurrState + 1);
        //         GetSensDataSMChangeState(jumpToNextState);
        //         break;
        //     }

    case GET_FM_DATA_STATE_DONE_GO_IDLE:
        retryCntForCmd = 0;

        // just to send attributes if a sensor gets failed
        if (prevSensorFailedFlagStatus.sensorFailedFlags != fmFailedFlagStatus.sensorFailedFlags)
        {
            systemFlags.sendAttributesonce = true;
        }
        prevSensorFailedFlagStatus.sensorFailedFlags = fmFailedFlagStatus.sensorFailedFlags;

        debugPrintln("[DEBUG] going idle...");
        debugPrintln("**********Reading data from FLOW METER done!**********");
        GetSensDataSMChangeState(GET_FM_DATA_STATE_IDLE);
        break;

    case GET_FM_DATA_STATE_WAIT_FOR_SENSOR_RESPONSE:
        if (true == systemFlags.modbusResponseRx && true == systemFlags.gotSensorDataCbCalled)
        {
            systemFlags.modbusResponseRx = false;
            systemFlags.gotSensorDataCbCalled = false;
            retryCntForCmd = 0;

            if (getFMSensDataSMPrevState == GET_FM1_DATA_STATE1)
            {
            }

            // GetSensDataSMChangeState(eGET_SENSOR_DATA(getSensDataSMPrevState + 1));
            jumpToTheState = eGET_FM_DATA(getFMSensDataSMPrevState + 1);
            afterCmdSuccessTakeRestMillis = millis();
            GetSensDataSMChangeState(GET_FM_DATA_STATE_TAKE_REST);
            break;
        }
        else if (true == systemFlags.modbusResponseRx && false == systemFlags.gotSensorDataCbCalled)
        {
            systemFlags.modbusResponseRx = false;
            debugPrintln("**************Flow meter not responding, Error! check connection**************\n");
            if (getFMSensDataSMPrevState == GET_FM1_DATA_STATE1)
            {
                debugPrintln("Flow meter DN25 Data1 failed to respond!\n");
                fmFailedFlagStatus.isGetFM1_Data1ValueFailed = true;
            }
            // else if (getFMSensDataSMPrevState == GET_FM1_DATA_STATE2)
            // {
            //     debugPrintln("Flow meter DN25 Data2 failed to respond!\n");
            //     fmFailedFlagStatus.isGetFM1_Data2ValueFailed = true;
            // }
            else if (getFMSensDataSMPrevState == GET_FM2_DATA_STATE1)
            {
                debugPrintln("Flow meter DN40 Data1 failed to respond!\n");
                fmFailedFlagStatus.isGetFM2_Data1ValueFailed = true;
            }
            // else if (getFMSensDataSMPrevState == GET_FM2_DATA_STATE2)
            // {
            //     debugPrintln("Flow meter DN40 Data2 failed to respond!\n");
            //     fmFailedFlagStatus.isGetFM2_Data2ValueFailed = true;
            // }

            if (retryCntForCmd >= 2)
            {
                retryCntForCmd = 0;
                GetSensDataSMChangeState(eGET_FM_DATA(getFMSensDataSMPrevState + 1));
                break;
            }
            else
            {
                _pumpRxDataWaitMillis = millis();
                getFMDataSMPrevPrevState = getFMSensDataSMPrevState;
                GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY);
                break;
            }
            break;
        }
        else
        {
            if (millis() - _pumpResponseTimeoutMillis > PUMP_RESPONSE_TIMEOUT)
            {
                systemFlags.modbusResponseRx = false;
                systemFlags.gotSensorDataCbCalled = false;
                debugPrintln("**************Flow meter not responding, Communication timeout! check connection**************\n");

                if (getFMSensDataSMPrevState == GET_FM1_DATA_STATE1)
                {
                    debugPrintln("Flow meter DN25 Data1 failed to respond!\n");
                    fmFailedFlagStatus.isGetFM1_Data1ValueFailed = true;
                }
                // else if (getFMSensDataSMPrevState == GET_FM1_DATA_STATE2)
                // {
                //     debugPrintln("Flow meter DN25 Data2 failed to respond!\n");
                //     fmFailedFlagStatus.isGetFM1_Data2ValueFailed = true;
                // }
                else if (getFMSensDataSMPrevState == GET_FM2_DATA_STATE1)
                {
                    debugPrintln("Flow meter DN40 Data1 failed to respond!\n");
                    fmFailedFlagStatus.isGetFM2_Data1ValueFailed = true;
                }
                // else if (getFMSensDataSMPrevState == GET_FM2_DATA_STATE2)
                // {
                //     debugPrintln("Flow meter DN40 Data2 failed to respond!\n");
                //     fmFailedFlagStatus.isGetFM2_Data2ValueFailed = true;
                // }

                if (retryCntForCmd >= 2)
                {
                    retryCntForCmd = 0;
                    GetSensDataSMChangeState(eGET_FM_DATA(getFMSensDataSMPrevState + 1));
                    break;
                }
                else
                {
                    _pumpRxDataWaitMillis = millis();
                    getFMDataSMPrevPrevState = getFMSensDataSMPrevState;
                    GetSensDataSMChangeState(GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY);
                    break;
                }
                break;
            }
        }
        break;

    case GET_FM_DATA_STATE_WAIT_FOR_SOME_TIME_TO_RETRY:
        if (millis() - _pumpRxDataWaitMillis > 100)
        {
            GetSensDataSMChangeState(getFMDataSMPrevPrevState);
            break;
        }
        break;

    case GET_FM_DATA_STATE_TAKE_REST:
        if (millis() - afterCmdSuccessTakeRestMillis > 200)
        {
            GetSensDataSMChangeState(jumpToTheState);
            break;
        }
        break;

    default:
        GetSensDataSMChangeState(GET_FM_DATA_STATE_IDLE);
        break;
    }
}

/**
 * @fn void SendSensorDataLoop(void)
 * @brief State Machine which runs a loop for sending data to cloud.
 */
void SendSensorDataLoop(void)
{
    uint32_t _unixTimeStamp;
    static TRANSACTION _httpTrans;

    switch (sendSensDataSMCurrState)
    {
    case SEND_FM_DATA_STATE_IDLE:
        if (millis() - _millisToTransmitDataJson > (gSysParam.logTime * SECOND))
        {
            _millisToTransmitDataJson = millis();

            debugPrintln("[DEBUG] Sending data to server...");
            SendSensDataSMChangeState(SEND_FM_DATA_STATE_CALCULATION_TO_FORM_FILL_JSON);
            break;
        }
        else if (true == systemFlags.formCurrPumpDataHttpLog)
        {
            systemFlags.formCurrPumpDataHttpLog = false;
            debugPrintln("[DEBUG] Sending current data packet to server...");
            SendSensDataSMChangeState(SEND_FM_DATA_STATE_CALCULATION_TO_FORM_FILL_JSON);
        }
        break;

    case SEND_FM_DATA_STATE_CALCULATION_TO_FORM_FILL_JSON:

        if (false == systemFlags.modbusDataUpdatingSemaphore)
        {
            systemFlags.modbusDataUpdatingSemaphore = true;

            debugPrintln("\n");
            debugPrintln("[DEBUG] formning sensor data packet..");
            if (false == CheckRtcCommunicationIsFailed())
            {
                GetCurDateTime(timeStamp, rtcDate, rtcTime, &currday, &secondsSince2000, &unixTimeStamp);

                _unixTimeStamp = unixTimeStamp - 19800;
                debugPrint("unix ts from rtc: ");
                debugPrintln(_unixTimeStamp);
                // if (serialBT.hasClient())
                // {
                //     serialBT.print("unix ts from rtc: ");
                //     serialBT.println(_unixTimeStamp);
                // }
            }
            else
            {
                debugPrintln("[DEBUG] RTC failed to read, unix time from network");
                _unixTimeStamp = GetUnixTimeFromNetwork();
                debugPrint("unix ts from network: ");
                debugPrintln(_unixTimeStamp);
                // if (serialBT.hasClient())
                // {
                //     serialBT.print("unix ts from network: ");
                //     serialBT.println(_unixTimeStamp);
                // }
                if (_unixTimeStamp)
                {
                    _unixTimeStamp = _unixTimeStamp - 19800;
                }
                else
                {
                    debugPrintln("[DEBUG] date time from network not received yet!");
                    _unixTimeStamp = unixTimeStamp - 19800;
                }
            }
            if (gSysParam.logSaveHttp)
            {
                memset(&_httpTrans, 0, sizeof(_httpTrans));

                // prepare transaction
                _httpTrans.unixTimestamp = _unixTimeStamp;

                if (1 == gSysParam.modbusSlave1Enable)
                {
                    if (false == fmFailedFlagStatus.isGetFM1_Data1ValueFailed)
                    {
                        _httpTrans.fl1_FlowRate = fmData[0].flowRate;
                        _httpTrans.fl1_totalizer = fmData[0].totalizer;
                        fl1DailyTotalizerSendFlag = true;
                    }
                    else
                    {
                        _httpTrans.fl1_FlowRate = -10000;
                        _httpTrans.fl1_totalizer = -10000;
                        fl1DailyTotalizerSendFlag = false;
                    }
                    // if (false == fmFailedFlagStatus.isGetFM1_Data2ValueFailed)
                    // {
                    //     _httpTrans.fl1_highAlarm = fmData[0].highAlarm;
                    //     _httpTrans.fl1_lowAlarm = fmData[0].lowAlarm;
                    // }
                    // else
                    // {
                    //     _httpTrans.fl1_highAlarm = -10000;
                    //     _httpTrans.fl1_lowAlarm = -10000;
                    // }
                }
                else
                {
                    fl1DailyTotalizerSendFlag = true;
                }

                // if (1 == gSysParam.modbusSlave2Enable)
                // {
                //     if (false == fmFailedFlagStatus.isGetFM2_Data1ValueFailed)
                //     {
                //         _httpTrans.fl2_FlowRate = fmData[1].flowRate;
                //         _httpTrans.fl2_totalizer = fmData[1].totalizer;
                //         fl2DailyTotalizerSendFlag = true;
                //     }
                //     else
                //     {
                //         _httpTrans.fl2_FlowRate = -10000;
                //         _httpTrans.fl2_totalizer = -10000;
                //         fl2DailyTotalizerSendFlag = false;
                //     }
                //     // if (false == fmFailedFlagStatus.isGetFM2_Data2ValueFailed)
                //     // {
                //     //     _httpTrans.fl2_highAlarm = fmData[1].highAlarm;
                //     //     _httpTrans.fl2_lowAlarm = fmData[1].lowAlarm;
                //     // }
                //     // else
                //     // {
                //     //     _httpTrans.fl2_highAlarm = -10000;
                //     //     _httpTrans.fl2_lowAlarm = -10000;
                //     // }
                // }
                // else
                // {
                //     fl2DailyTotalizerSendFlag = true;
                // }

                if (true == fl1DailyTotalizerSendFlag)//&& true == fl2DailyTotalizerSendFlag)
                {

                    if (gSysParam2.prevDay != currday)
                    {
                        _httpTrans.fl1_dalilyFlowRate = _httpTrans.fl1_totalizer - gSysParam2.fl1_prevDayTotalizer;
                        gSysParam2.fl1_prevDayTotalizer = _httpTrans.fl1_totalizer; // update prev totalizer FL1
                        // _httpTrans.fl2_dalilyFlowRate = _httpTrans.fl2_totalizer - gSysParam2.fl2_prevDayTotalizer;
                        // gSysParam2.fl2_prevDayTotalizer = _httpTrans.fl2_totalizer; // update prev totalizer FL2
                        gSysParam2.prevDay = currday;    // update prev day
                        if(false == AppSetConfigSysParams2(&gSysParam2))
                        {
                            debugPrintln("[DEBUG] PrevDailyFlowrate and PrevDay Set to failed!");
                        }
                        else
                        {
                            debugPrintln("[DEBUG] PrevDailyFlowrate and PrevDay Set to done!");
                        }
                    }
                    else
                    {
                        _httpTrans.fl1_dalilyFlowRate = -10000;
                      //  _httpTrans.fl2_dalilyFlowRate = -10000;
                    }
                }

                // format packet for the JSON and then try to send over mqtt. if succeed good otherwise push to databse
                jsonTobeSentOverHttp = "";
                CreateHttpPacketToSend(&_httpTrans, jsonTobeSentOverHttp); // this will format the JSON packet
                debugPrint("Http LogTobeSend: ");
                debugPrintln(jsonTobeSentOverHttp);

                if (!LoggerPush(&_httpTrans))
                {
                    debugPrintln("[DEBUG] Log saving failed!");
                }
                else
                {
                    debugPrintln("[DEBUG] Log saving done!");
                }

                // update logcnt on screen
                // uint32_t unsendLogCount = 0;
                // efsdbLoggerGetCount(efsdbHttpLogHandle, &unsendLogCount);
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
                debugPrintln("[DEBUG] Log saving disabled");
            }

            systemFlags.modbusDataUpdatingSemaphore = false;
        }
        else
        {
            break;
        }
        SendSensDataSMChangeState(SEND_FM_DATA_STATE_IDLE);
        break;

    default:
        SendSensDataSMChangeState(SEND_FM_DATA_STATE_IDLE);
        break;
    }
}

/**
 * @fn void CreateHttpPacketToSend(TRANSACTION *httpTrans, String LogTobeSend)
 * @brief prepare mqtt data packet to send over mqtt
 */
void CreateHttpPacketToSend(TRANSACTION *httpTrans, String &LogTobeSend)
{
    StaticJsonDocument<800> httpDoc;

    JsonObject values = httpDoc.createNestedObject("values");

    httpDoc["ts"] = String(httpTrans->unixTimestamp) + "000";
    if (1 == gSysParam.modbusSlave1Enable)
    {
        if (-10000 != httpTrans->fl1_FlowRate)
        {
            values["1-FlowRate"] = httpTrans->fl1_FlowRate;
        }
        if (-10000 != httpTrans->fl1_totalizer)
        {
            values["1-Totalizer"] = httpTrans->fl1_totalizer;
        }
        if (-10000 != httpTrans->fl1_dalilyFlowRate)
        {
            values["1-DailyFlowRate"] = httpTrans->fl1_dalilyFlowRate;
        }
    }
    // if (1 == gSysParam.modbusSlave2Enable)
    // {
    //     if (-10000 != httpTrans->fl2_FlowRate)
    //     {
    //         values["2-FlowRate"] = httpTrans->fl2_FlowRate;
    //     }
    //     if (-10000 != httpTrans->fl2_totalizer)
    //     {
    //         values["2-Totalizer"] = httpTrans->fl2_totalizer;
    //     }
    //     if (-10000 != httpTrans->fl2_dalilyFlowRate)
    //     {
    //         values["2-DailyFlowRate"] = httpTrans->fl2_dalilyFlowRate;
    //     }
    // }

    serializeJson(httpDoc, LogTobeSend);

    // if (serialBT.hasClient())
    // {
    //     serialBT.print("Http LogTobeSend: ");
    //     serialBT.println(LogTobeSend);
    // }
}

/**
 * @fn uint32_t GetSystemAlertStatus()
 * @brief This function returns system generated alerts.
 * @return remote alert number
 */
uint32_t GetSystemAlertStatus(void)
{
    uint8_t ret = 0;

    if (false == gSysParam.ap4gOrWifiEn)
    {
        CheckSignalQuality();

        if ((0 == CheckSIM_PresenceByPrevStatusAtRestart(SIM_SLOT_1)))
        {
            // Sim_Detection_error
            ret = 1;
            return ret;
        }
        else if (systemFlags.AntennaError == true)
        {
            // Antenna_error
            ret = 2;
            return ret;
        }
        else if (!IsGsmNwConnected())
        {
            // Network_registration_error
            ret = 3;
            return ret;
        }
        else if (!IsGprsConnected())
        {
            // GPRS_Network_registration_error
            ret = 4;
            return ret;
        }
        else if (!IsMqttConnected(_flBroker))
        {
            // MQTT_server_connection_error
            ret = 5;
            return ret;
        }
        else if (!systemFlags.setApnErrorFlag)
        {
            // APN_setting_error
            ret = 6;
            return ret;
        }
        else if (!MCP7940.deviceStatus())
        {
            // RTC_read_error_RAE_RTC_write_error
            ret = 7;
            return ret;
        }
        else if (false == systemFlags.isSensorConnected)
        {
            // Communication_error
            ret = 8;
            return ret;
        }
        else
        {
            // no_error
            ret = 0;
            return ret;
        }
    }
    else
    {
        // else if (!IsMqttConnected(_dataBroker))
        // {
        //     // MQTT_server_connection_error
        //     ret = 5;
        //     return ret;
        // }
        if (!MCP7940.deviceStatus())
        {
            // RTC_read_error_RAE_RTC_write_error
            ret = 7;
            return ret;
        }
        if (false == systemFlags.isSensorConnected)
        {
            // Communication_error
            ret = 8;
            return ret;
        }
        else if (WL_CONNECTED != WiFi.status())
        {
            // WiFi connection error
            ret = 9;
            return ret;
        }
        else if (abs(WiFi.RSSI()) > 80)
        {
            // Low RSSI error
            ret = 10;
            return ret;
        }
        else
        {
            // no_error
            ret = 0;
            return ret;
        }
    }
    return ret;
}
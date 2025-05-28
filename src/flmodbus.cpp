#include "globals.h"
// Include the header for the ModbusClient RTU style
#include "ModbusClientRTU.h"
#include "flmodbus.h"

// Create a ModbusRTU client instance
// The RS485 module has no halfduplex, so the parameter with the DE/RE pin is required!
ModbusClientRTU modbusRTU(RE_DE_PIN);

uint8_t READ_FM_DATA1 = 1;
uint8_t READ_FM_DATA2 = 2;

static unsigned int _start = 0;
static unsigned int _end = 0;
static unsigned int _current = 0;
typedef struct _MODBUS_READ_CB_TYPE_
{
    pfncModbusReadCb fncModbusReadCb;
    uint8_t info;
    // char *infoStr;
} tMODBUS_READ_CB;

uint8_t infoStrArr[MODBUS_READ_CB_FUNCTION_MAX_NUM] = {0};
static tMODBUS_READ_CB _modbusReadCbArr[MODBUS_READ_CB_FUNCTION_MAX_NUM];

static bool _ModbusReadCbPush(pfncModbusReadCb fnc, uint8_t *infoch)
{
    _modbusReadCbArr[_end].fncModbusReadCb = fnc;
    infoStrArr[_end] = *infoch;
    _end = (_end + 1) % MODBUS_READ_CB_FUNCTION_MAX_NUM;

    if (_start >= MODBUS_READ_CB_FUNCTION_MAX_NUM)
    {
        return false;
    }

    if (_current < MODBUS_READ_CB_FUNCTION_MAX_NUM)
    {
        _current++;
    }
    else
    {
        _start = (_start + 1) % MODBUS_READ_CB_FUNCTION_MAX_NUM;
        if (_start >= MODBUS_READ_CB_FUNCTION_MAX_NUM)
        {
            return false;
        }
    }
    return true;
}

static bool _ModbusReadCbPop(pfncModbusReadCb *fnc, uint8_t *infoch)
{
    if (!_current)
    {
        return false;
    }
    if (_start >= MODBUS_READ_CB_FUNCTION_MAX_NUM)
    {
        return false;
    }

    *fnc = _modbusReadCbArr[_start].fncModbusReadCb;

    *infoch = infoStrArr[_start];

    _start = (_start + 1) % MODBUS_READ_CB_FUNCTION_MAX_NUM;

    if (_start >= MODBUS_READ_CB_FUNCTION_MAX_NUM)
    {
        return false;
    }
    _current--;
    return true;
}

// Define an onData handler function to receive the regular responses
// Arguments are Modbus server ID, the function code requested, the message data and length of it,
// plus a user-supplied token to identify the causing request
void handleData(ModbusMessage response, uint32_t token)
{
    static pfncModbusReadCb pCbFunc;
    static uint8_t infoch;
    uint8_t arrayOfDataRx[256];

    // Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
    modRtuDebug(Serial.print("[MODBUS] RX: ");
                for (auto &byte : response) {
                    Serial.printf("%02X ", byte);
                } Serial.println(););

    memcpy(arrayOfDataRx, &response.data()[3], response.size() - 3);

    if (token == READ_FM_DATA1)
    {
        ReadFlowMeterData1Callback(arrayOfDataRx, response.size() - 3, 0, response.getServerID());
    }
    // else if (token == READ_FM_DATA2)
    // {
    //     ReadFlowMeterData2Callback(arrayOfDataRx, response.size() - 3, 0, response.getServerID());
    // }
    else
    {
        Serial.println("[ERROR] Unknown modbus command token");
    }

    systemFlags.modbusResponseRx = true;
    systemFlags.isSensorConnected = true;
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token)
{
    static pfncModbusReadCb pCbFunc;
    static uint8_t infoch;

    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    modRtuDebug(Serial.printf("[MODBUS] RX: %02X - %s\n", (int)me, (const char *)me););

    // if (true == _ModbusReadCbPop(&pCbFunc, &infoch))
    // {
    //     if (pCbFunc != 0)
    //     {
    //         pCbFunc(0, 0, error);
    //     }
    // }

    if (token == READ_FM_DATA1)
    {
        ReadFlowMeterData1Callback(0, 0, error, 0);
    }
    // else if (token == READ_FM_DATA2)
    // {
    //     ReadFlowMeterData2Callback(0, 0, error, 0);
    // }
    else
    {
        Serial.println("[ERROR] Unknown modbus command token");
    }

    systemFlags.modbusResponseRx = true;
    systemFlags.isSensorConnected = false;
}

void ModbusInit(void)
{
    // Set up ModbusRTU client.

    // RTUclient.onResponseHandler(&handleData);

    // - provide onData handler function
    modbusRTU.onDataHandler(&handleData);
    // - provide onError handler function
    modbusRTU.onErrorHandler(&handleError);
    // Set message timeout to 2000ms
    modbusRTU.setTimeout(2000);
    // Start ModbusRTU background task
    modbusRTU.begin(Serial1);
}

bool ReadFlowMeterData1Callback(uint8_t *modbusCallbackRxByteArray, size_t length, bool error, uint8_t slaveId)
{
    debugPrintf("Flow Meter %d data1\n", slaveId);
    uint16_t fmData1[6];
    uint8_t emNo = slaveId - 1;

    if (error)
    {
        debugPrintln("[DEBUG] Error in reading Flow meter Data1");
        systemFlags.gotSensorDataCbCalled = false;
        if (slaveId == 1)
        {
            fmFailedFlagStatus.isGetFM1_Data1ValueFailed = true;
        }
        else if (slaveId == 2)
        {
            fmFailedFlagStatus.isGetFM2_Data1ValueFailed = true;
        }
        return false;
    }
    // Frame format CDAB
    systemFlags.modbusDataUpdatingSemaphore = true;
    float flowRate;
    long int integerTotalizer;
    float fractionalTotalizer;

    fmData1[0] = ((modbusCallbackRxByteArray[0] << 8) | modbusCallbackRxByteArray[1]); // Flow Rate (CD)
    fmData1[1] = ((modbusCallbackRxByteArray[2] << 8) | modbusCallbackRxByteArray[3]); //(AB)

    fmData1[2] = ((modbusCallbackRxByteArray[12] << 8) | modbusCallbackRxByteArray[13]); // Integer Totalizer
    fmData1[3] = ((modbusCallbackRxByteArray[14] << 8) | modbusCallbackRxByteArray[15]);

    fmData1[4] = ((modbusCallbackRxByteArray[16] << 8) | modbusCallbackRxByteArray[17]); // Fractional Totalizer
    fmData1[5] = ((modbusCallbackRxByteArray[18] << 8) | modbusCallbackRxByteArray[19]);

    flowRate = ToFloat(fmData1[1], fmData1[0]);
    integerTotalizer = fmData1[3] << 16 | fmData1[2];
    fractionalTotalizer = ToFloat(fmData1[5], fmData1[4]);

    fmData[emNo].flowRate = flowRate;
    fmData[emNo].totalizer = integerTotalizer + fractionalTotalizer;

    systemFlags.gotSensorDataCbCalled = true;
    if (slaveId == 1)
    {
        fmFailedFlagStatus.isGetFM1_Data1ValueFailed = false;
    }
    else if (slaveId == 2)
    {
        fmFailedFlagStatus.isGetFM2_Data1ValueFailed = false;
    }

    modRtuDebug(Serial.print("Flow Rate: ");
                Serial.println(fmData[emNo].flowRate););
    modRtuDebug(Serial.print("Integer Totalizer: ");
                Serial.println(integerTotalizer););
    modRtuDebug(Serial.print("Fractional Totalizer: ");
                Serial.println(fractionalTotalizer););
    
    //some changes told by Vishal sir shown  only totalizer
    modRtuDebug(Serial.print("Int + Fractonal = Totalizer: ");
                Serial.println(fmData[emNo].totalizer););

    systemFlags.modbusDataUpdatingSemaphore = false;

    debugPrintf("[DEBUG] Reading Flow meter %d Data1 suceess!\n", slaveId);

    return true;
}
bool ReadFlowMeteMData1(uint8_t slaveId)
{
    bool status = false;

    Error err = modbusRTU.addRequest(READ_FM_DATA1, slaveId, READ_HOLD_REGISTER, 2000, 10);
    if (err != SUCCESS)
    {
        ModbusError e(err);
        modRtuDebug(Serial.printf("Error creating request %s: %02X - %s\n", "FLOW METER Data1", (int)e, (const char *)e););
        status = false;
    }
    else
    {
        modRtuDebug(Serial.print("[MODBUS] TX: ");
                    Serial.printf("%02X 03 07 D0 00 0A C5 40\n",slaveId););
        status = true;
    }
    return status;
}

// bool ReadFlowMeterData2Callback(uint8_t *modbusCallbackRxByteArray, size_t length, bool error, uint8_t slaveId)
// {
//     debugPrintf("Flow Meter %d data2\n", slaveId);
//     uint16_t flData2[2];
//     uint8_t fmNo = slaveId - 1;
//     if (error)
//     {
//         debugPrintln("[DEBUG] Error in reading Flow Meter Data2");
//         systemFlags.gotSensorDataCbCalled = false;
//         if (slaveId == 1)
//         {
//             fmFailedFlagStatus.isGetFM1_Data2ValueFailed = true;
//         }
//         else if (slaveId == 2)
//         {
//             fmFailedFlagStatus.isGetFM2_Data2ValueFailed = true;
//         }
//         return false;
//     }

//     systemFlags.modbusDataUpdatingSemaphore = true;

//     flData2[0] = ((modbusCallbackRxByteArray[0] << 8) | modbusCallbackRxByteArray[1]);
//     flData2[1] = ((modbusCallbackRxByteArray[2] << 8) | modbusCallbackRxByteArray[3]);

//     fmData[fmNo].highAlarm = flData2[0];
//     fmData[fmNo].lowAlarm = flData2[1];

//     systemFlags.gotSensorDataCbCalled = true;
//     if (slaveId == 1)
//     {
//         fmFailedFlagStatus.isGetFM1_Data2ValueFailed = false;
//     }
//     else if (slaveId == 2)
//     {
//         fmFailedFlagStatus.isGetFM2_Data2ValueFailed = false;
//     }

//     modRtuDebug(Serial.print("High Alarm: ");
//                 Serial.println(fmData[fmNo].highAlarm););
//     modRtuDebug(Serial.print("Low Alarm: ");
//                 Serial.println(fmData[fmNo].lowAlarm););

//     systemFlags.modbusDataUpdatingSemaphore = false;

//     debugPrintf("[DEBUG] Reading FM %d Data2 suceess!\n", slaveId);

//     return true;
// }


// bool ReadFlowMeterData2(uint8_t slaveId)
// {
//     bool status = false;

//     Error err = modbusRTU.addRequest(READ_FM_DATA2, slaveId, READ_HOLD_REGISTER, 2017, 2);
//     if (err != SUCCESS)
//     {
//         ModbusError e(err);
//         modRtuDebug(Serial.printf("Error creating request %s: %02X - %s\n", "FLOW METER Data2", (int)e, (const char *)e););
//         status = false;
//     }
//     else
//     {
//         modRtuDebug(Serial.print("[MODBUS] TX: ");
//                     Serial.printf("%02X 03 07 E1 00 02 95 49\n",slaveId););
//         status = true;
//     }
//     return status;
// }
/*!
 *  @file fl-modbus-rtu.h
 *
 *  @mainpage fl-modbus-rtu library for ESP32
 *
 *  @section Introduction
 *
 *  This is a library is for any modbus slave device. It is a wrapper for esp32ModbusRTU library.
 *
 *  You must have esp32ModbusRTU library by Bert Melis installed to use this class.
 *
 *  @see https://github.com/bertmelis/esp32ModbusRTU
 *
 *  @section Author
 *
 *  @author Nivrutti Mahajan
 *
 *  @date 07-09-2019 07:27:30 PM
 *
 *        Written by Fountlab Solutions.
 *
 */

#ifndef FLMODBUS_H
#define FLMODBUS_H

#include <string.h>
#include "config.h"
#include "global-vars.h"

// #include <esp32ModbusRTU.h>
// #include <ModbusMessage.h>
#include <string.h>

#define SINGLE_REGISTER 1 /**< single register selected */
/// MODBUS_ADDR is the Address of Modbus Device
// #define MODBUS_ADDR1 gSysParam.sensor1ModAddress
// #define MODBUS_ADDR2 gSysParam.sensor2ModAddress
// #define MODBUS_ADDR3 gSysParam.sensor3ModAddress
// #define MODBUS_ADDR4 gSysParam.sensor4ModAddress
// #define MODBUS_ADDR5 gSysParam.sensor5ModAddress
// #define MODBUS_ADDR6 gSysParam.sensor6ModAddress
// #define MODBUS_ADDR7 gSysParam.sensor7ModAddress
// #define MODBUS_ADDR8 gSysParam.sensor8ModAddress
// #define MODBUS_ADDR9 gSysParam.sensor9ModAddress
// #define MODBUS_ADDR10 gSysParam.sensor10ModAddress

#define MODBUS_READ_CB_FUNCTION_MAX_NUM 50

typedef bool (*pfncModbusReadCb)(uint8_t *data, size_t length, bool modError);

/*!
 *  @brief  Initialization of FL905 Modbus temperature humidity sensor
 *
 */
void ModbusInit(void);

/*!
 *  @brief  Write value to perticular register of modbus sensor
 *
 *  @param slaveId modbus slave address
 *  @param address address where value to be written
 *  @param data value to be written to register
 *
 */
bool WriteRegisterData16(uint8_t slaveId, uint16_t address, uint16_t data);

/*!
 *  @brief  Write value to perticular register of modbus sensor
 *
 *  @param slaveId modbus slave address
 *  @param address address where value to be written
 *  @param data value to be written to register
 *
 */
bool WriteRegister(uint8_t slaveId, uint16_t address, uint16_t data);

/*!
 *  @brief  Write multiple values to multiple registers of modbus sensor
 *
 *  @param slaveId modbus slave address
 *  @param address start register address from value to be written
 *  @param noOfRegisters number of registers
 *  @param data data array which written to multiple register
 */
bool WriteMultipleRegisters(uint8_t slaveId, uint16_t address, uint16_t noOfRegisters, uint8_t *data);

/*!
 *  @brief  read a perticular address of modbus sensor
 *
 *  @param slaveId modbus slave address
 *  @param address address to be read
 *
 */
bool ReadRegister(uint8_t slaveId, uint16_t address);

/*!
 *  @brief  read multiple registers of modbus sensor
 *
 *  @param slaveId modbus slave address
 *  @param address start address from values to be read
 *  @param noOfRegisters number of registers to be read
 *
 */
bool ReadMultipleRegisters(uint8_t slaveId, uint16_t address, uint16_t noOfRegisters);

bool ReadFlowMeterData1Callback(uint8_t *modbusCallbackRxByteArray, size_t length, bool error, uint8_t slaveId);
bool ReadFlowMeteMData1(uint8_t slaveId);
// bool ReadFlowMeterData2Callback(uint8_t *modbusCallbackRxByteArray, size_t length, bool error, uint8_t slaveId);
// bool ReadFlowMeterData2(uint8_t slaveId);

/**
 * Ennum for Modbus register addresses
 */
enum modbusRegSet : uint16_t
{
    // PH_VALUE_REG = 2,
    PH_VALUE_REG = 1536,
    RMU_RID_DETAILS = 5102,
    TCP_CTR = 5106,
    REMOTE_ALERT_STATUS = 5108,
    BLUETOOTH_STATUS = 5110,
    REMOTE_ON_OFF = 5112,
    WATER_DIS_MULT_FACTOR = 5113,
    WATER_DIS_HEAD_FACTOR_SEL = 5121,
};

#endif
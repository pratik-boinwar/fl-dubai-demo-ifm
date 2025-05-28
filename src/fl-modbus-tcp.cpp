#include "fl-modbus-tcp.h"
#include "fl-ethernet.h"
#include <WiFi.h>
#include <esp32ModbusTCP.h>
#include "fl-ethernet.h"

esp32ModbusTCP modbusTcp(1, modTcpClientIp, DEFAULT_MODBUS_TCP_CLEINT_PORT);

uMODBUS_TCP_DATA modbusTcpData;

ModbusTcpRegInfo ioControllerRegisters[] = {
#ifndef GET_ALL_DATA_TOGETHER
    "input status", 0, 1, UFIX0, 0,
    "relay status", 1, 1, UFIX0, 0,
    "clear relay", 2, 1, UFIX0, 0,
    "set relay", 3, 1, UFIX0, 0,
    "get DAC out 1", 4, 1, SFIX0, 0,
    "get DAC out 2", 5, 1, SFIX0, 0,
    "get DAC out 3", 6, 1, SFIX0, 0,
    "get DAC out 4", 7, 1, SFIX0, 0,
    "set DAC out 1", 4, 1, SFIX0, 0,
    "set DAC out 2", 5, 1, SFIX0, 0,
    "set DAC out 3", 6, 1, SFIX0, 0,
    "set DAC out 4", 7, 1, SFIX0, 0
#else
    "device status", 0, 12, DEV_STAT, 0,
    "Read adc input data", 20, 16, ADC_IN, 0
#endif
};

uint8_t numberSmaRegisters = sizeof(ioControllerRegisters) / sizeof(ioControllerRegisters[0]);

void ModbusTCPInit(void)
{
  modbusTcp.onData([](uint16_t packet, uint8_t slave, esp32Modbus::FunctionCode fc, uint8_t *data, uint16_t len)
                   {
      uint16_t value = 0;
  for (uint8_t i = 0; i < numberSmaRegisters; ++i)
  {
    if (ioControllerRegisters[i].packetId == packet)
    {
      ioControllerRegisters[i].packetId = 0;
      switch (ioControllerRegisters[i].type)
      {
      case ENUM:
      case UFIX0:
      {
        value = data[1] | (data[0] << 8);
        modTcpDebug(Serial.printf("%s: %X\n", ioControllerRegisters[i].name, value););
        break;
      }
      case SFIX0:
      {
        value = data[1] | (data[0] << 8);
        modTcpDebug(Serial.printf("%s: %X\n", ioControllerRegisters[i].name, value););
        break;
      }
      case DEV_STAT:
      {
        modbusTcpData.digitalInStat = data[1] | (data[0] << 8);
        modbusTcpData.relayStat = data[3] | (data[2] << 8);
        modbusTcpData.clearRelayStat = data[5] | (data[4] << 8);
        modbusTcpData.setRelayStat = data[7] | (data[6] << 8);
        modbusTcpData.dacOut1Data = data[9] | (data[8] << 8);
        modbusTcpData.dacOut2Data = data[11] | (data[10] << 8);
        modbusTcpData.dacOut3Data = data[13] | (data[12] << 8);
        modbusTcpData.dacOut4Data = data[15] | (data[14] << 8);
        modbusTcpData.setDacOut1Data = data[17] | (data[16] << 8);
        modbusTcpData.setDacOut2Data = data[19] | (data[18] << 8);
        modbusTcpData.setDacOut3Data = data[21] | (data[20] << 8);
        modbusTcpData.setDacOut4Data = data[23] | (data[22] << 8);

        modTcpDebug(Serial.printf("input status: %X\n", modbusTcpData.digitalInStat);
                    Serial.printf("relay status: %X\n", modbusTcpData.relayStat);
                    Serial.printf("clear Relay Status: %X\n", modbusTcpData.clearRelayStat);
                    Serial.printf("set Relay Status: %X\n", modbusTcpData.setRelayStat);
                    Serial.printf("dacOut1Data: %X\n", modbusTcpData.dacOut1Data);
                    Serial.printf("dacOut2Data: %X\n", modbusTcpData.dacOut2Data);
                    Serial.printf("dacOut3Data: %X\n", modbusTcpData.dacOut3Data);
                    Serial.printf("dacOut4Data: %X\n", modbusTcpData.dacOut4Data);
                    Serial.printf("setDacOut1Data: %X\n", modbusTcpData.setDacOut1Data);
                    Serial.printf("setDacOut2Data: %X\n", modbusTcpData.setDacOut2Data);
                    Serial.printf("setDacOut3Data: %X\n", modbusTcpData.setDacOut3Data);
                    Serial.printf("setDacOut4Data: %X\n", modbusTcpData.setDacOut4Data););
        break;
      }
      case ADC_IN:
      {
        modbusTcpData.adc1Val = data[1] | (data[0] << 8);
        modbusTcpData.adc2Val = data[3] | (data[2] << 8);
        modbusTcpData.adc3Val = data[5] | (data[4] << 8);
        modbusTcpData.adc4Val = data[7] | (data[6] << 8);
        modbusTcpData.adc5Val = data[9] | (data[8] << 8);
        modbusTcpData.adc6Val = data[11] | (data[10] << 8);
        modbusTcpData.adc7Val = data[13] | (data[12] << 8);
        modbusTcpData.adc8Val = data[15] | (data[14] << 8);
        modbusTcpData.adc9Val = data[17] | (data[16] << 8);
        modbusTcpData.adc10Val = data[19] | (data[18] << 8);
        modbusTcpData.adc11Val = data[21] | (data[20] << 8);
        modbusTcpData.adc12Val = data[23] | (data[22] << 8);
        modbusTcpData.adc13Val = data[25] | (data[24] << 8);
        modbusTcpData.adc14Val = data[27] | (data[26] << 8);
        modbusTcpData.adc15Val = data[29] | (data[28] << 8);
        modbusTcpData.adc16Val = data[31] | (data[30] << 8);

        modTcpDebug(Serial.printf("adc1Val: %X\n", modbusTcpData.adc1Val);
                    Serial.printf("adc2Val: %X\n", modbusTcpData.adc2Val);
                    Serial.printf("adc3Val: %X\n", modbusTcpData.adc3Val);
                    Serial.printf("adc4Val: %X\n", modbusTcpData.adc4Val);
                    Serial.printf("adc5Val: %X\n", modbusTcpData.adc5Val);
                    Serial.printf("adc6Val: %X\n", modbusTcpData.adc6Val);
                    Serial.printf("adc7Val: %X\n", modbusTcpData.adc7Val);
                    Serial.printf("adc8Val: %X\n", modbusTcpData.adc8Val);
                    Serial.printf("adc9Val: %X\n", modbusTcpData.adc9Val);
                    Serial.printf("adc10Val: %X\n", modbusTcpData.adc10Val);
                    Serial.printf("adc11Val: %X\n", modbusTcpData.adc11Val);
                    Serial.printf("adc12Val: %X\n", modbusTcpData.adc12Val);
                    Serial.printf("adc13Val: %X\n", modbusTcpData.adc13Val);
                    Serial.printf("adc14Val: %X\n", modbusTcpData.adc14Val);
                    Serial.printf("adc15Val: %X\n", modbusTcpData.adc15Val);
                    Serial.printf("adc16Val: %X\n", modbusTcpData.adc16Val););

        break;
      }
      }
      return;
    }
  } });
  modbusTcp.onError([](uint16_t packet, esp32Modbus::Error e)
                    { modTcpDebug(Serial.printf("Error packet %u: %02x\n", packet, e);); });

  delay(1000);
}

void ModbusTCPLoop(void)
{
  static uint32_t lastMillis = 0;
  if ((millis() - lastMillis > 10000 && IsEthernetConnected()))
  {
    lastMillis = millis();
    debugPrintln("Reading modbus TCP registers...");
    for (uint8_t i = 0; i < numberSmaRegisters; ++i)
    {
      uint16_t packetId = modbusTcp.readHoldingRegisters(ioControllerRegisters[i].address, ioControllerRegisters[i].length);
      if (packetId > 0)
      {
        ioControllerRegisters[i].packetId = packetId;
      }
      else
      {
        debugPrintln("Reading modbus TCP registers failed!");
      }
    }

    // // Write holding reg example
    // uint8_t dataToWrite[2] = {0x03, 0xE8};
    // ModbusTCPWriteRegister(0x0B, dataToWrite);
  }
}

bool ModbusTCPWriteRegister(uint16_t address, uint8_t *data)
{
  bool status = false;
  status = modbusTcp.writeMultHoldingRegisters(address, 1, data);
  return status;
}

bool ModbusTCPWriteMultipleRegisters(uint16_t address, uint16_t noOfRegisters, uint8_t *data)
{
  bool status = false;
  status = modbusTcp.writeMultHoldingRegisters(address, noOfRegisters, data);
  return status;
}

bool ModbusTCPReadRegister(uint16_t address)
{
  bool status = false;
  status = modbusTcp.readHoldingRegisters(address, 1);
  return status;
}

bool ModbusTCPReadMultipleRegisters(uint16_t address, uint16_t noOfRegisters)
{
  bool status = false;
  status = modbusTcp.readHoldingRegisters(address, noOfRegisters);
  return status;
}

void ModbusTCPSettings(IPAddress addr, uint16_t port)
{
  modbusTcp.SetIpAddrOfEsp32ModbusTCP(addr, port);
}
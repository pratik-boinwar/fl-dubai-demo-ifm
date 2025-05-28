#ifndef FL_MODBUS_TCP
#define FL_MODBUS_TCP

#include "globals.h"

// uncomment below line to get data one by one separately
#define GET_ALL_DATA_TOGETHER

enum smaType
{
    ENUM,     // enumeration
    UFIX0,    // unsigned, no decimals
    SFIX0,    // signed, no decimals
    DEV_STAT, // read device data
    ADC_IN    // adc input data
};
struct ModbusTcpRegInfo
{
    const char *name;
    uint16_t address;
    uint16_t length;
    smaType type;
    uint16_t packetId;
};

typedef union __MODBUS_TCP_DATA__
{
    uint8_t deviceStatusRegData[24];
    uint8_t adcInputData[32];
    struct
    {
        uint16_t digitalInStat;
        uint16_t relayStat;
        uint16_t clearRelayStat;
        uint16_t setRelayStat;
        uint16_t dacOut1Data;
        uint16_t dacOut2Data;
        uint16_t dacOut3Data;
        uint16_t dacOut4Data;
        uint16_t setDacOut1Data;
        uint16_t setDacOut2Data;
        uint16_t setDacOut3Data;
        uint16_t setDacOut4Data;

        uint16_t adc1Val;
        uint16_t adc2Val;
        uint16_t adc3Val;
        uint16_t adc4Val;
        uint16_t adc5Val;
        uint16_t adc6Val;
        uint16_t adc7Val;
        uint16_t adc8Val;
        uint16_t adc9Val;
        uint16_t adc10Val;
        uint16_t adc11Val;
        uint16_t adc12Val;
        uint16_t adc13Val;
        uint16_t adc14Val;
        uint16_t adc15Val;
        uint16_t adc16Val;
    };
} uMODBUS_TCP_DATA;

void ModbusTCPInit(void);
void ModbusTCPLoop(void);

bool ModbusTCPWriteRegister(uint16_t address, uint8_t *data);
bool ModbusTCPWriteMultipleRegisters(uint16_t address, uint16_t noOfRegisters, uint8_t *data);
bool ModbusTCPReadRegister(uint16_t address);
bool ModbusTCPReadMultipleRegisters(uint16_t address, uint16_t noOfRegisters);
void ModbusTCPSettings(IPAddress addr, uint16_t port = 9760);

#endif
#include "hmi-disp-nextion.h"
#include <SoftwareSerial.h>

/**
 * SoftwareSerial object
 */
SoftwareSerial softSerial;

void NextionHMIDispInit()
{
    uint32_t unsendLogCount = 0;

    // softSerial.begin(SOFT_SERIAL_BAUD, SWSERIAL_8N1, SOFT_SERIAL_RX_PIN, SOFT_SERIAL_TX_PIN);

    // softSerial.print("page StartUp");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // delay(6000);

    // efsdbLoggerGetCount(efsdbHttpLogHandle, &unsendLogCount);
    // softSerial.print("n0.txt=");
    // softSerial.print("\"");
    // softSerial.print((String)(unsendLogCount));
    // softSerial.print("\"");
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // delay(2);
    // softSerial.print("nwkStat.val=");
    // softSerial.print((uint32_t)(hbParam.rssi));
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // delay(2);
    // softSerial.print("ethStat.val=");
    // softSerial.print((uint32_t)0);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // softSerial.write(0xFF);
    // delay(2);
}

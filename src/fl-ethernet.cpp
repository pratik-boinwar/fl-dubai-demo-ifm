// #include "fl-ethernet.h"
// #include "globals.h"
// #include "global-vars.h"
// #include "app.h"
// #include "fl-modbus-tcp.h"
// #include "hmi-disp-nextion.h"
// #include <SPI.h>

// bool isEthernetConnected = false;

// void WiFiEvent(WiFiEvent_t event);

// void WiFiEvent(WiFiEvent_t event)
// {
//     switch (event)
//     {
//     case ARDUINO_EVENT_ETH_START:
//         debugPrintln("ETH Started");
//         // set eth hostname here
//         ETH.setHostname(DEVICE_MODEL);
//         break;
//     case ARDUINO_EVENT_ETH_CONNECTED:
//         debugPrintln("ETH Connected");
//         break;
//     case ARDUINO_EVENT_ETH_GOT_IP:
//         debugPrint("ETH MAC: ");
//         debugPrint(ETH.macAddress());
//         debugPrint(", IPv4: ");
//         debugPrint(ETH.localIP());
//         if (ETH.fullDuplex())
//         {
//             debugPrint(", FULL_DUPLEX");
//         }
//         debugPrint(", ");
//         debugPrint(ETH.linkSpeed());
//         debugPrintln("Mbps");
//         strcpy(gSysParam.subnet, ETH.subnetMask().toString().c_str());
//         strcpy(gSysParam.ipAddr, ETH.localIP().toString().c_str());
//         strcpy(gSysParam.gateway, ETH.gatewayIP().toString().c_str());
//         if (false == AppSetConfigSysParams(&gSysParam))
//         {
//         }
// #ifdef MODBUS_TCP_EANBLE
//         ModbusTCPSettings(modTcpClientIp, gSysParam.modTcpClientPort);
// #endif

//         // softdebugPrint("ethStat.val=");
//         // softdebugPrint((uint32_t)1);
//         // softSerial.write(0xFF);
//         // softSerial.write(0xFF);
//         // softSerial.write(0xFF);

//         isEthernetConnected = true;
//         break;
//     case ARDUINO_EVENT_ETH_DISCONNECTED:
//         debugPrintln("ETH Disconnected");

//         // softdebugPrint("ethStat.val=");
//         // softdebugPrint((uint32_t)0);
//         // softSerial.write(0xFF);
//         // softSerial.write(0xFF);
//         // softSerial.write(0xFF);

//         isEthernetConnected = false;
//         break;
//     case ARDUINO_EVENT_ETH_STOP:
//         debugPrintln("ETH Stopped");
//         isEthernetConnected = false;
//         break;
//     default:
//         break;
//     }
// }

// void EthernetInit(void)
// {
//     // tcpip_adapter_init();
//     WiFi.onEvent(WiFiEvent);
//     ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
//     if (0 == gSysParam.dhcp)
//     {
//         ETH.config(local_IP, gateway_IP, subnet_Mask, p_dns);
//     }
// }

// bool IsEthernetConnected(void)
// {
//     return isEthernetConnected;
// }
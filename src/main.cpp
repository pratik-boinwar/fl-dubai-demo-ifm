/***************************************************************************************************
** Include user defined libraries                                                                  **
***************************************************************************************************/
#include "globals.h"
#include "flWifiHandler.h"
#include "rtc.h"
#include "buzzer.h"
#include "user.h"
// #include "logger_espefs.h"
#include "ota.h"
#include "ssl_cert.h"
#include "netclient.h"
// #include "flmodbus.h"
#ifdef APP_MQTT_PUB_FUNCTIONALITY
#include "app-mqtt-publish.h"
#endif
#include <esp_task_wdt.h>
#include "fl-ethernet.h"
#include "fl-modbus-tcp.h"
#include "app.h"
#include "sensordata.h"
#ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
#if CONFIG_IDF_TARGET_ESP32  // ESP32/PICO-D4
#include "rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/rtc.h"
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif
#else // ESP32 Before IDF 4.0
#include "rom/rtc.h"
#endif
#include "resetButton.h"
#include "hmi-disp-nextion.h"
// #include "oled.h"
#include "logger.h"
#include "app-mqtt.h"

#include "gps.h"
#include "fl-lora.h"
//************************************************************************************
//  SETUP AND LOOP
//************************************************************************************

extern char devBootTime[20]; // boot time to send it to fl mqtt server

void setup()
{
  BuzzerInit();

  startup_tone();

  Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
  Serial.begin(SERIAL_BAUD_RATE);

  SysInit();

  AppDebug(true);

  debugPrint("CPU0 reset reason: ");
  Print_reset_reason(rtc_get_reset_reason(0));
  debugPrint("CPU1 reset reason: ");
  Print_reset_reason(rtc_get_reset_reason(1));

  GetMacFromEsp();

  debugPrintf("[INFO] Device        : %s\n", DEVICE_MODEL);
  debugPrintf("[INFO] Hardware      : %s\n", HARDWARE);
  debugPrintf("[INFO] HW Version:   : %s\n", HW_VER);
  debugPrintf("[INFO] FW Version    : %s\n", FW_VER);
  debugPrintf("[INFO] FW Build      : %s\n", FW_BUILD);
  debugPrintf("[INFO] Serial Number : %s\n", (char *)deviceID.c_str());

  debugPrintln();

  // adding delay to avoid spiffs corruption due to brownout or other power issues.
  // letting the power to settle down before initializing spiffs
  delay(1000);
  SpiffsInit(); // Serial output

  if (!LoggerInit())
  {
    flashErrorCount++;
    systemFlags.flashError = true;
  }
  else
  {
    systemFlags.flashError = false;
  }

  AppInit();

  debugPrintf("\n[INIT] Flash size (SDK)    : %8u bytes\n", ESP.getFlashChipSize());
  debugPrintf("[INIT] Firmware size       : %8u bytes\n", ESP.getSketchSize());
  debugPrintf("[INIT] OTA size            : %8u bytes\n", ESP.getFreeSketchSpace());

  debugPrintf("[MAIN] SPIFFS total size   : %8u bytes\n", SPIFFS.totalBytes());
  debugPrintf("[MAIN] Used size           : %8u bytes\n\n", SPIFFS.usedBytes());

  ModbusRTU_Init();

  // NextionHMIDispInit();

  CreateCircularBuffer();

  FlWifi.WifiInit(gSysParam.staSsid, gSysParam.staPass, gSysParam.dhcp,
                  local_IP, gateway_IP, subnet_Mask, p_dns, s_dns);

  pinMode(HEARTBEAT_LED, OUTPUT);
  digitalWrite(HEARTBEAT_LED, LOW);

  netclientSetup();

  RtcInit();

  // get current datetime from RTC
  GetCurDateTime(timeStamp, rtcDate, rtcTime, &currday, &secondsSince2000, &unixTimeStamp);

  snprintf(devStartTime, sizeof(devStartTime), "%s%s%s-%s%s", rtcDateTime.day, rtcDateTime.month, rtcDateTime.year,
           rtcDateTime.hour, rtcDateTime.min);
  snprintf(devBootTime, sizeof(devBootTime), "%s/%s/%s %s:%s:%s", rtcDateTime.day, rtcDateTime.month, rtcDateTime.year,
           rtcDateTime.hour, rtcDateTime.min, rtcDateTime.sec);
  snprintf(lastPacketSentSuccessTime, sizeof(lastPacketSentSuccessTime), "%s%s%s-%s%s", rtcDateTime.day, rtcDateTime.month, rtcDateTime.year,
           rtcDateTime.hour, rtcDateTime.min);

  debugPrint("[RTC] Time: ");
  debugPrintln(timeStamp);

  // Give size of the log storage transaction
  debugPrintf("Number of maximum logs: %d\n", MAX_TRANSACTIONS);

  // mqtt broker on wifi
  AppMqttInit();

  systemFlags.searchfirmware = true;             // currently this is not in use so made this false
  systemFlags.sendRestartTimeToMqttTopic = true; // this flag is used to publish restart time over mqtt
  systemFlags.sendAttributesonce = true;         // used to send restart time as attributes to the http server

#ifdef MODBUS_TCP_EANBLE
  ModbusTCPInit();
#endif
#ifdef ETHERNET_EANBLE
  //EthernetInit();
#endif

  success_tone();

  WebServerHandler();

  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);
  lorainit();


  debugPrintln("All Init done!\n");
  AppDebug(false); // disable all serial prints.
}

void loop()
{
  static uint32_t _hbLastMillis = 0, _gpsDataLastMillis = 0;
  static uint32_t _checkForNewFWMillis = 0;

#ifdef SERIAL_HEART_BEAT

  if (millis() - _hbLastMillis > HEARTBEAT_RATE)
  {
    _hbLastMillis = millis();
    digitalWrite(HEARTBEAT_LED, !digitalRead(HEARTBEAT_LED));
    // debugPrintln("Heartbeat");
  }
#endif

#ifdef ENABLE_4G
  if (millis() - _gpsDataLastMillis > 10 * SECOND)
  {
    _gpsDataLastMillis = millis();
    debugPrintln("\n[DEBUG] Getting GPS data...");
    GpsGetLocation();
  }
#endif

  // update current RTC date time from GPS or NETWORK
  CalibrateOrAdjustRTCTime();

  // loop to get data from different water quality sensors
  GetSensorDataLoop();

  // loop for to send sensor data to cloud
if (true == gSysParam.loraOr4gEn)
{
  SendLoraData();
}
else
{
  SendSensorDataLoop();
}

  // if any serial cmd received
  SerialCmdLine();

  FlWifi.WiFiMainLoop();

  // if telnet enabled from webserver
  if (1 == gSysParam.telnetEn)
  {
    // check for any tcp connections
    CheckForConnections();

    // if any cmd received from tcp client
    TelnetSerialCmdLine();
  }

  // handling of modem related operations
  NetclientLoop();

  if (true == gSysParam.ap4gOrWifiEn)
  {
    isWifiNetClientSelected = true;
    if (FlWifi.isConnected())
    {
      WiFiEthNetclientLoop();
    }
  }
  else
  {
      isWifiNetClientSelected = false;
    
#ifdef APP_MQTT_PUB_FUNCTIONALITY
    // App mqtt publish loop to send command resp stored in a queue
    AppMqttPubLoop();
#endif

    // OTA handler
    OtaLoop();

    // SSLCertLoop();
  }

  if ((millis() - _checkForNewFWMillis) > 1 * HOUR)
  {
    debugPrintln("[NET] Serching for new Firware after a hour...");
    _checkForNewFWMillis = millis();
    systemFlags.searchfirmware = true; // search for new firmware flag is enabled
  }

#ifdef MODBUS_TCP_EANBLE
  ModbusTCPLoop();
#endif

  esp_task_wdt_reset();
}
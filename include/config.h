#ifndef CONFIG_H
#define CONFIG_H

#define APP_NAME "FL-COCA-COLA-FLOW-METER"
#define DEVICE_MODEL "COCA-COLA-FLOW-METER"

#define FW_BUILD __DATE__ " " __TIME__
#define HARDWARE "MODGATE-0003"

#define HW_ID "210701"
#define HW_VER "1.0"
#define FW_VER "1.01"
#define FW_VERSION "001"
#define FW_REVISION "01"
#define FW_DATE "18072024" //before FW_DATE "06032024"

// Pinout
#define SOFT_SERIAL_RX_PIN 35
#define SOFT_SERIAL_TX_PIN 32
#define RS485_RX_PIN 16
#define RS485_TX_PIN 17
#define RE_DE_PIN 2
#define HEARTBEAT_LED 5 /// GPIO12
#define SDA_PIN 15      /// SDA_pin
#define SCL_PIN 4       /// SCL_pin
#define BUZZER_PIN 12
#define BUTTON_PIN 36
//

#define SECOND (1000)
#define MINUTE (1000 * 60)
#define HOUR (60 * MINUTE)
#define HEARTBEAT_RATE (3 * SECOND)
#define GPS_READ_TIMEOUT (2 * SECOND)

// #define UART1 Serial1

#define SOFT_SERIAL_BAUD 9600 /// BaudRate for GPS serial initialization
#define SERIAL_BAUD_RATE 115200
#define SERIAL_RX_BUFFER_SIZE 1024 // used in Serial.setRxBufferSize()

#define MAX_SIZEOF_COMMAND 256
#define MAX_SIZEOF_SMS_COMMAND 256
#define MAX_SIZEOF_PUMP_RX_BUFFER 1000

#define CERT_FILE_NAME_SIZE 30

// select DO sensor from below list of vendors
//  #define DO_NENGSHI
#define DO_ALICE
// #define DO_BOQU

// 120 seconds WDT timeout, if still 2 mins wdt reset funct is not called it will reset controller
// #define WDT_TIMEOUT 120
#define WDT_TIMEOUT 120

#define TCP_SERVER_PORT 3000

// uncomment below line to enable NEW_PH_SENSOR code
#define NEW_PH_SENSOR

// // uncomment below line to enable testing of large data store restore in file of flash.
// #define TEST_SPIFFS_READ_WRITE_LARGEDATA

// // uncomment below line to enable testing of successful restore of default config file.
// #define TEST_CONFIG_SETTING_RESTORED_SUCCESS_DEBUG

// uncomment below line to enable app mqtt pub loop which will handle commandline from fl mqtt server.
#define APP_MQTT_PUB_FUNCTIONALITY

// uncomment below line to enbale ethernet code
#define ETHERNET_EANBLE

// uncomment below line to enbale send data using ethernet
#define SEND_DATA_USING_ETHERNET

// // uncomment below line to enable continuous hotspot of device
// #define CONTINUOUS_HOTSPOT_MODE

// uncomment below line to enbale modbus tcp code
// #define MODBUS_TCP_EANBLE

// uncoment below line to enable hardcoded IMEI for testing purpose only
// #define HARDCODED_IMEI
#ifdef HARDCODED_IMEI
#define HARD_IMEI "862493053533109"
#endif

// uncomment below line to enable data sending on mqtt broker even if pump is not connected.
#define SEND_DATA_ON_MQTT_EVEN_IF_PUMP_NOT_CONNECTED

// Default Sensor calculations parameters
#define DEFAULT_MODEM_RST_CNT 0
#define DEFAULT_PREV_DAY 1
#define DEFAULT_PREV_DAY_TOTALIZER 0


// Default config system parameters
#define DEFAULT_MODBUS_SLAVE1_ADDR 1
#define DEFAULT_MODBUS_SLAVE2_ADDR 2
#define DEFAULT_MODBUS_SLAVE3_ADDR 3
#define DEFAULT_MODBUS_SLAVE4_ADDR 4
#define DEFAULT_MODBUS_SLAVE5_ADDR 5
#define DEFAULT_MODBUS_SLAVE6_ADDR 6
#define DEFAULT_MODBUS_SLAVE7_ADDR 7
#define DEFAULT_MODBUS_SLAVE8_ADDR 8
#define DEFAULT_MODBUS_SLAVE9_ADDR 9
#define DEFAULT_MODBUS_SLAVE10_ADDR 10

#define DEFAULT_MODBUS_SLAVE1_EN_STAT 1
#define DEFAULT_MODBUS_SLAVE2_EN_STAT 1
#define DEFAULT_MODBUS_SLAVE3_EN_STAT 0
#define DEFAULT_MODBUS_SLAVE4_EN_STAT 0
#define DEFAULT_MODBUS_SLAVE5_EN_STAT 0



#define DEFAULT_FARM1_NAME "POND 1"
#define DEFAULT_FARM2_NAME "POND 2"
#define DEFAULT_FARM3_NAME "POND 3"
#define DEFAULT_FARM4_NAME "POND 4"
#define DEFAULT_FARM5_NAME "POND 5"

#define DEFAULT_UPDATE_RATE 20
#define DEFAULT_HB_INTERVAL 300
#define DEFAULT_LOG_TIME 300
#define DEFAULT_AP_4G_OR_WIFI_MODE 0 // 0=AP-4G mode, 1=WIFI mode
#define DEFAULT_LORA_OR_4G_MODE 0  // 0=4G mode, 1=LORA mode

#define DEFAULT_HOSTNAME DEVICE_MODEL
#define DEFAULT_DEFLAT "22.535430"
#define DEFAULT_DEFLONG "72.925167"
#define DEFAULT_SITENAME ""
#define DEFAULT_CLEINTNAME ""

#define DEFAULT_BAUDRATE_232 57600
#define DEFAULT_BAUDRATE_485 19200
#define DEFAULT_DATABIT 8
#define DEFAULT_STOPBIT 1
#define DEFAULT_PARIT 2

#define DEFAULT_LOGSEND_MQTT 1
#define DEFAULT_LOGSEND_HTTP 1
#define DEFAULT_LOGSAVE_MQTT 1
#define DEFAULT_LOGSAVE_HTTP 1
#define DEFAULT_LOGCOUNT 0

#define DEFAULT_APN_MODE 1
#define DEFAULT_APN_1 "airtelgprs.com"
#define DEFAULT_APN_2 "www"

#define DEFAULT_IMEI_NUM "862818049577812"

#define DEFAULT_SERV1_MQTT_HOST "rms1.kusumiiot.co"
#define DEFAULT_SERV1_MQTT_PORT 8883
#define DEFAULT_SERV1_MQTT_UNAME "imei$standalonesolarpump$20"
#define DEFAULT_SERV1_MQTT_PASS "b3bd01f1"
#define DEFAULT_SERV1_MQTT_CLIENTID "d:imei$standalonesolarpump$20"
#define DEFAULT_SERV1_MQTT_SSL_EN 1
#define DEFAULT_SERV1_MQTT_CERT_EN 0

#define DEFAULT_INFOSUBTOP "iiot-1/standalonesolarpump/imei/info/sub"
#define DEFAULT_OTPSUBTOP "iiot-1/standalonesolarpump/imei/otp/sub"
#define DEFAULT_HBPUBTOP "iiot-1/standalonesolarpump/imei/heartbeat/pub"
#define DEFAULT_DATAPUBTOP "iiot-1/standalonesolarpump/imei/data/pub"
#define DEFAULT_ONDMD_SUBTOP "iiot-1/standalonesolarpump/imei/ondemand/sub"
#define DEFAULT_ONDMD_PUBTOP "iiot-1/standalonesolarpump/imei/ondemand/pub"
#define DEFAULT_CONFIG_SUBTOP "iiot-1/standalonesolarpump/imei/config/sub"
#define DEFAULT_CONFIG_PUBTOP "iiot-1/standalonesolarpump/imei/config/pub"

#define DEFAULT_SERV2_MQTT_HOST "139.59.49.165"
#define DEFAULT_SERV2_MQTT_PORT 1883
#define DEFAULT_SERV2_MQTT_UNAME "tcpserver"
#define DEFAULT_SERV2_MQTT_PASS "tcpserver"
#define DEFAULT_SERV2_MQTT_CLIENTID "hnHkJGW3tGDi222d99FP"
#define DEFAULT_SERV2_MQTT_QOS 0
#define DEFAULT_SERV2_MQTT_RETAIN 0
#define DEFAULT_SERV2_MQTT_KEEPALIVE 300
#define DEFAULT_SERV2_MQTT_AUTH 0

#define DEFAULT_SERV2_MQTT_SUBTOPIC "imei/commands"
#define DEFAULT_SERV2_MQTT_PUBTOPIC "imei/commandsResp"
#define DEFAULT_SERV2_MQTT_HB_PUBTOPIC "imei/hb"
#define DEFAULT_SERV2_MQTT_DATA_PUBTOPIC "imei/data"
#define DEFAULT_FL_SERV_HTTPURL "/credentials"
#define DEFAULT_FL_SERV_HTTPTOKEN "26pFy0SXWu7HkZWZlLZY"
#define DEFAULT_FL_SERV_HTTPPORT 3000

#define DEFAULT_DHEAD_EN 0
#define DEFAULT_PROTOCOL_SEL 0
#define DEFAULT_HTTP_DATA_SERVIP "65.2.96.64"
#define DEFAULT_HTTP_DATA_SERVURL "/api/v1"
#define DEFAULT_HTTP_DATA_SERVTOKEN "token"
#define DEFAULT_HTTP_DATA_SERVPORT 80
#define DEFAULT_HTTP_BACKUP_SERVIP "pumpeye.rotosol.solar"
#define DEFAULT_HTTP_BACKUP_SERVURL "/api/PumpData"
#define DEFAULT_HTTP_BACKUP_SERVPORT 80
#define DEFAULT_HTTP_SSL_EN 1

#define DEFAULT_FTP_URL "862818049577812"
#define DEFAULT_FTP_UNAME "436e558"
#define DEFAULT_FTP_PASS "rms1.kusumiiot.co"
#define DEFAULT_FTP_PORT 22
#define DEFAULT_LOCAL_IPPORT "rms1.kusumiiot.co:8883"
#define DEFAULT_LOCAL_SSID "862818049577812"
#define DEFAULT_LOCAL_SSID_PASS "ec13e30"
#define DEFAULT_CIPHER_KEY "8f59e932239841bba21c1f90ef0dc848"
#define DEFAULT_SERPASS_KEY "5c13a5170ff24e49aa0399e783a62206"

#define DEFAULT_MODBUS_TCP_CLEINT_IP "192.168.0.120"
#define DEFAULT_MODBUS_TCP_CLEINT_PORT 9760

#define DEFAULT_DHCP 1
#define DEFAULT_SUBNET "255.255.255.0"
#define DEFAULT_IP "192.168.0.201"
#define DEFAULT_GATEWAY "192.168.0.1"
#define DEFAULT_PDNS "8.8.8.8"
#define DEFAULT_SDNS "8.8.8.4"
#define DEFAULT_DNS "aquasen"

#define DEFAULT_STASSID "FountLab"
#define DEFAULT_STAPASS "FLEIflei"
#define DEFAULT_APSSID DEVICE_MODEL
#define DEFAULT_APPASS "12345678"

#define DEFAULT_UID "user"
#define DEFAULT_UPASS "user"
#define DEFAULT_MID "admin"
#define DEFAULT_MPASS "admin123"

#define DEFAULT_TELNET_EN 1
#define DEFAULT_MQTT_EN 2
#endif
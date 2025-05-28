#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#include <stdint.h>

typedef union _SYSTEM_FLAG_
{
    uint32_t all;
    struct
    {
        unsigned btOrWifiAPEnableFlag : 1;
        unsigned connectToFlbrokerOnlyAfterResetFlag : 1;
    };

} SYSTEM_FLAG;

typedef union _DISPLAY_SYSTEM_FLAG_
{
    uint32_t all;
    struct
    {
        unsigned loginButPressed : 1;
        unsigned datetimeButPressed : 1;
        unsigned networkButPressed : 1;
        unsigned sensorCalibButPressed : 1;
        unsigned wifiSelected : 1;
        unsigned ap4gSelected : 1;
        unsigned switchAutoApn : 1;
        unsigned saveDateTimeButPressed : 1;
        unsigned saveNetworkButPressed : 1;
        unsigned saveSensorCalibButPressed : 1;
    };

} DISPLAY_SYSTEM_FLAG;

#define IMEI_SIZE 20

#define DEF_LAT_LONG_SIZE 20

#define APN_1_SIZE 20
#define APN_2_SIZE 20

#define MQTT_HOST_SIZE 100
#define MQTT_UNAME_SIZE 256
#define MQTT_PASS_SIZE 256
#define MQTT_CLIENTID_SIZE 50

#define MQTT_TOPICS_SIZE 256

#define FL_MQTT_TOPICS_SIZE 100
#define FL_SERV_HTTPURL_SIZE 50
#define FL_SERV_HTTPTOKEN_SIZE 50

#define HTTP_SERVIP_SIZE 50
#define HTTP_SERVURL_SIZE 50

#define FTP_URL_SIZE 50
#define FTP_UNAME_SIZE 50
#define FTP_PASS_SIZE 50

#define LOCAL_IPPORT_SIZE 50
#define LOCAL_SSID_SIZE 50
#define LOCAL_SSID_PASS_SIZE 50
#define CIPHER_KEY_SIZE 50
#define SERPASS_KEY_SIZE 50

#define IP_SIZE 20

#define SSID_LEN 50
#define PASS_LEN 20

#define FARM_NAME_SIZE 20

#define NAME_SIZE 25

typedef uint32_t SENSOR1_MODBUS_ADDR, SENSOR2_MODBUS_ADDR;
typedef uint32_t SENSOR3_MODBUS_ADDR, SENSOR4_MODBUS_ADDR;
typedef uint32_t SENSOR5_MODBUS_ADDR, SENSOR6_MODBUS_ADDR;
typedef uint32_t SENSOR7_MODBUS_ADDR, SENSOR8_MODBUS_ADDR;
typedef uint32_t SENSOR9_MODBUS_ADDR, SENSOR10_MODBUS_ADDR;

typedef uint32_t SLAVE1_EN, SLAVE2_EN, SLAVE3_EN, SLAVE4_EN, SLAVE5_EN;

typedef char FARM1_NAME[FARM_NAME_SIZE];
typedef char FARM2_NAME[FARM_NAME_SIZE];
typedef char FARM3_NAME[FARM_NAME_SIZE];
typedef char FARM4_NAME[FARM_NAME_SIZE];
typedef char FARM5_NAME[FARM_NAME_SIZE];

typedef uint32_t UPDATE_RATE, HB_INTERVAL, WIFI_OR_4G_AP_EN, LORA_OR_4G_EN;
typedef uint32_t BAUDRATE_485, BAUDRATE_232, DATABIT, STOPBIT, PARIT;
typedef uint32_t LOGSEND_MQTT, LOGSEND_HTTP, LOGSAVE_MQTT, LOGSAVE_HTTP, LOGCOUNT;
typedef uint32_t LOG_TIME;

typedef char HOST_NAME[NAME_SIZE];
typedef char DEFLAT[DEF_LAT_LONG_SIZE];
typedef char DEFLONG[DEF_LAT_LONG_SIZE];
typedef char SITE_NAME[NAME_SIZE];
typedef char CLIENT_NAME[NAME_SIZE];

typedef uint32_t APN_MODE;
typedef char APN_1[APN_1_SIZE];
typedef char APN_2[APN_2_SIZE];

typedef char IMEI_NUM[IMEI_SIZE];

typedef char SERV1_MQTT_HOST[MQTT_HOST_SIZE];
typedef uint32_t SERV1_MQTT_PORT, SERV1_MQTT_SSL_EN, SERV1_MQTT_CERT_EN;
typedef char SERV1_MQTT_UNAME[MQTT_UNAME_SIZE];
typedef char SERV1_MQTT_PASS[MQTT_PASS_SIZE];
typedef char SERV1_MQTT_CLIENTID[MQTT_CLIENTID_SIZE];

typedef char INFOSUBTOP[MQTT_TOPICS_SIZE];
typedef char OTPSUBTOP[MQTT_TOPICS_SIZE];
typedef char HBPUBTOP[MQTT_TOPICS_SIZE];
typedef char DATAPUBTOP[MQTT_TOPICS_SIZE];
typedef char ONDMD_SUBTOP[MQTT_TOPICS_SIZE];
typedef char ONDMD_PUBTOP[MQTT_TOPICS_SIZE];
typedef char CONFIG_SUBTOP[MQTT_TOPICS_SIZE];
typedef char CONFIG_PUBTOP[MQTT_TOPICS_SIZE];

typedef char SERV2_MQTT_HOST[MQTT_HOST_SIZE];
typedef uint32_t SERV2_MQTT_PORT, SERV2_MQTT_QOS, SERV2_MQTT_RETAIN, SERV2_MQTT_KEEPALIVE, SERV2_MQTT_AUTH;
typedef char SERV2_MQTT_UNAME[MQTT_UNAME_SIZE];
typedef char SERV2_MQTT_PASS[MQTT_PASS_SIZE];
typedef char SERV2_MQTT_CLIENTID[MQTT_CLIENTID_SIZE];
typedef char SERV2_MQTT_SUBTOPIC[FL_MQTT_TOPICS_SIZE];
typedef char SERV2_MQTT_PUBTOPIC[FL_MQTT_TOPICS_SIZE];
typedef char SERV2_MQTT_HB_PUBTOPIC[FL_MQTT_TOPICS_SIZE];
typedef char SERV2_MQTT_DATA_PUBTOPIC[FL_MQTT_TOPICS_SIZE];

typedef char FL_SERV_HTTPURL[FL_SERV_HTTPURL_SIZE];
typedef char FL_SERV_HTTPTOKEN[FL_SERV_HTTPTOKEN_SIZE];
typedef uint32_t FL_SERV_HTTPPORT;

typedef uint32_t DHEAD_EN, PROTOCOL_SEL, HTTP_DATA_SERVPORT, HTTP_BACKUP_SERVPORT, HTTP_SSL_EN;
typedef char HTTP_DATA_SERVIP[HTTP_SERVIP_SIZE];
typedef char HTTP_DATA_SERVURL[HTTP_SERVURL_SIZE];
typedef char HTTP_DATA_SERVTOKEN[HTTP_SERVURL_SIZE];
typedef char HTTP_BACKUP_SERVIP[HTTP_SERVIP_SIZE];
typedef char HTTP_BACKUP_SERVURL[HTTP_SERVURL_SIZE];

typedef char FTP_URL[FTP_URL_SIZE];
typedef char FTP_UNAME[FTP_UNAME_SIZE];
typedef char FTP_PASS[FTP_PASS_SIZE];
typedef uint32_t FTP_PORT;
typedef char LOCAL_IPPORT[LOCAL_IPPORT_SIZE];
typedef char LOCAL_SSID[LOCAL_SSID_SIZE];
typedef char LOCAL_SSID_PASS[LOCAL_SSID_PASS_SIZE];
typedef char CIPHER_KEY[CIPHER_KEY_SIZE];
typedef char SERPASS_KEY[SERPASS_KEY_SIZE];

typedef char MODBUS_TCP_CLIENT_IPADDR[IP_SIZE];
typedef uint32_t MODBUS_TCP_CLIENT_PORT;

typedef uint32_t DHCP;
typedef char SUBNET_ADDR[IP_SIZE];
typedef char IP_ADDR[IP_SIZE];
typedef char GATEWAY_ADDR[IP_SIZE];
typedef char PDNS_ADDR[IP_SIZE];
typedef char SDNS_ADDR[IP_SIZE];
typedef char DNS_NAME[IP_SIZE];

typedef char STA_SSID[SSID_LEN];
typedef char STA_PASS[PASS_LEN];
typedef char AP_SSID[SSID_LEN];
typedef char AP_PASS[PASS_LEN];

typedef char UID[20];
typedef char UPASS[20];
typedef char MID[20];
typedef char MPASS[20];

typedef uint32_t TELNET;
typedef uint32_t MQTT_EN;

// typedef uint32_t SLAVE_ID1, SLAVE_ID2, SLAVE_ID3, SLAVE_ID4, SLAVE_ID5;

typedef union _SYSTEM_PARAM_
{
    unsigned char array[sizeof(SENSOR1_MODBUS_ADDR) + sizeof(SENSOR3_MODBUS_ADDR) + sizeof(SENSOR5_MODBUS_ADDR) + sizeof(SENSOR7_MODBUS_ADDR) + sizeof(SENSOR9_MODBUS_ADDR) +
                        sizeof(SENSOR2_MODBUS_ADDR) + sizeof(SENSOR4_MODBUS_ADDR) + sizeof(SENSOR6_MODBUS_ADDR) + sizeof(SENSOR8_MODBUS_ADDR) + sizeof(SENSOR10_MODBUS_ADDR) +
                        sizeof(SLAVE1_EN) + sizeof(SLAVE2_EN) + sizeof(SLAVE3_EN) + sizeof(SLAVE4_EN) + sizeof(SLAVE5_EN) +
                        sizeof(FARM1_NAME) + sizeof(FARM2_NAME) + sizeof(FARM3_NAME) + sizeof(FARM4_NAME) + sizeof(FARM5_NAME) +
                        sizeof(UPDATE_RATE) + sizeof(HB_INTERVAL) + sizeof(WIFI_OR_4G_AP_EN) + sizeof(LORA_OR_4G_EN) + sizeof(TELNET) + sizeof(MQTT_EN) +
                        sizeof(HOST_NAME) + sizeof(DEFLAT) + sizeof(DEFLONG) + sizeof(SITE_NAME) + sizeof(CLIENT_NAME) +
                        sizeof(BAUDRATE_485) + sizeof(BAUDRATE_232) + sizeof(DATABIT) + sizeof(STOPBIT) + sizeof(PARIT) +
                        sizeof(LOGSEND_MQTT) + sizeof(LOGSEND_HTTP) + sizeof(LOGSAVE_MQTT) + sizeof(LOGSAVE_HTTP) +
                        sizeof(LOGCOUNT) + sizeof(LOG_TIME) + sizeof(APN_MODE) + sizeof(APN_1) + sizeof(APN_2) + sizeof(IMEI_NUM) +
                        sizeof(SERV1_MQTT_HOST) + sizeof(SERV1_MQTT_PORT) + sizeof(SERV1_MQTT_UNAME) + sizeof(SERV1_MQTT_PASS) + sizeof(SERV1_MQTT_CLIENTID) + sizeof(SERV1_MQTT_SSL_EN) + sizeof(SERV1_MQTT_CERT_EN) +
                        sizeof(INFOSUBTOP) + sizeof(OTPSUBTOP) + sizeof(HBPUBTOP) + sizeof(DATAPUBTOP) + sizeof(ONDMD_SUBTOP) + sizeof(ONDMD_PUBTOP) + sizeof(CONFIG_SUBTOP) + sizeof(CONFIG_PUBTOP) +
                        sizeof(SERV2_MQTT_HOST) + sizeof(SERV2_MQTT_PORT) + sizeof(SERV2_MQTT_UNAME) + sizeof(SERV2_MQTT_PASS) + sizeof(SERV2_MQTT_CLIENTID) +
                        sizeof(SERV2_MQTT_QOS) + sizeof(SERV2_MQTT_RETAIN) + sizeof(SERV2_MQTT_KEEPALIVE) + sizeof(SERV2_MQTT_AUTH) +
                        sizeof(SERV2_MQTT_SUBTOPIC) + sizeof(SERV2_MQTT_PUBTOPIC) + sizeof(SERV2_MQTT_HB_PUBTOPIC) + sizeof(SERV2_MQTT_DATA_PUBTOPIC) +
                        sizeof(FL_SERV_HTTPURL) + sizeof(FL_SERV_HTTPTOKEN) + sizeof(FL_SERV_HTTPPORT) +
                        sizeof(DHEAD_EN) + sizeof(PROTOCOL_SEL) + sizeof(HTTP_DATA_SERVIP) + sizeof(HTTP_DATA_SERVPORT) + sizeof(HTTP_DATA_SERVURL) + sizeof(HTTP_DATA_SERVTOKEN) +
                        sizeof(HTTP_BACKUP_SERVIP) + sizeof(HTTP_BACKUP_SERVPORT) + sizeof(HTTP_BACKUP_SERVURL) + sizeof(HTTP_SSL_EN) +
                        sizeof(FTP_URL) + sizeof(FTP_UNAME) + sizeof(FTP_PASS) + sizeof(FTP_PORT) + sizeof(LOCAL_IPPORT) + sizeof(LOCAL_SSID) +
                        sizeof(LOCAL_SSID_PASS) + sizeof(CIPHER_KEY) + sizeof(SERPASS_KEY) +
                        sizeof(MODBUS_TCP_CLIENT_IPADDR) + sizeof(MODBUS_TCP_CLIENT_PORT) +
                        sizeof(DHCP) + sizeof(SUBNET_ADDR) + sizeof(IP_ADDR) + sizeof(GATEWAY_ADDR) + sizeof(PDNS_ADDR) + sizeof(SDNS_ADDR) + sizeof(DNS_NAME) +
                        sizeof(STA_SSID) + sizeof(STA_PASS) + sizeof(AP_SSID) + sizeof(AP_PASS) +
                        sizeof(UID) + sizeof(UPASS) + sizeof(MID) + sizeof(MPASS) + 1];
    struct
    {
        SENSOR1_MODBUS_ADDR sensor1ModAddress;
        SENSOR2_MODBUS_ADDR sensor2ModAddress;
        SENSOR3_MODBUS_ADDR sensor3ModAddress;
        SENSOR4_MODBUS_ADDR sensor4ModAddress;
        SENSOR5_MODBUS_ADDR sensor5ModAddress;
        SENSOR6_MODBUS_ADDR sensor6ModAddress;
        SENSOR7_MODBUS_ADDR sensor7ModAddress;
        SENSOR8_MODBUS_ADDR sensor8ModAddress;
        SENSOR9_MODBUS_ADDR sensor9ModAddress;
        SENSOR10_MODBUS_ADDR sensor10ModAddress;

        SLAVE1_EN modbusSlave1Enable;
        SLAVE2_EN modbusSlave2Enable;
        SLAVE3_EN modbusSlave3Enable;
        SLAVE4_EN modbusSlave4Enable;
        SLAVE5_EN modbusSlave5Enable;

        FARM1_NAME farm1Name;
        FARM2_NAME farm2Name;
        FARM3_NAME farm3Name;
        FARM4_NAME farm4Name;
        FARM5_NAME farm5Name;

        UPDATE_RATE uRate;
        HB_INTERVAL hbInterval;
        WIFI_OR_4G_AP_EN ap4gOrWifiEn;
        LORA_OR_4G_EN loraOr4gEn;

        TELNET telnetEn;
        MQTT_EN mqttEn;

        HOST_NAME hostname;
        DEFLAT defaultLat;
        DEFLONG defaultLong;
        SITE_NAME sitename;
        CLIENT_NAME clientname;

        BAUDRATE_485 baud485;
        BAUDRATE_232 baud232;
        DATABIT dataBits;
        STOPBIT stopBits;
        PARIT parit;

        LOGSEND_MQTT logSendMqtt;
        LOGSEND_HTTP logSendHttp;
        LOGSAVE_MQTT logSaveMqtt;
        LOGSAVE_HTTP logSaveHttp;
        LOGCOUNT logCnt;
        LOG_TIME logTime;

        APN_MODE apnMode;
        APN_1 apn1;
        APN_2 apn2;

        IMEI_NUM imei;

        SERV1_MQTT_HOST serv1MqttHostname;
        SERV1_MQTT_PORT serv1MqttPort;
        SERV1_MQTT_UNAME serv1MqttUname;
        SERV1_MQTT_PASS serv1MqttPass;
        SERV1_MQTT_CLIENTID serv1MqttClientId;
        SERV1_MQTT_SSL_EN serv1SslEnable;
        SERV1_MQTT_CERT_EN serv1CertEnable;

        INFOSUBTOP serv1InfoSubTopic;
        OTPSUBTOP serv1OtpSubTopic;
        HBPUBTOP serv1HBPubTopic;
        DATAPUBTOP serv1DataPubTopic;
        ONDMD_SUBTOP serv1OnDemandSubTopic;
        ONDMD_PUBTOP serv1OnDemandPubTopic;
        CONFIG_SUBTOP serv1ConfigSubTopic;
        CONFIG_PUBTOP serv1ConfigPubTopic;

        SERV2_MQTT_HOST flServerHost;
        SERV2_MQTT_PORT flServerMqttPort;
        SERV2_MQTT_UNAME flServerMqttUname;
        SERV2_MQTT_PASS flServerMqttPass;
        SERV2_MQTT_CLIENTID flServerMqttClientId;
        SERV2_MQTT_QOS flServerMqttQos;
        SERV2_MQTT_RETAIN flServerMqttRetain;
        SERV2_MQTT_KEEPALIVE flServerMqttKeepAlive;
        SERV2_MQTT_AUTH flServerMqttAuth;

        SERV2_MQTT_SUBTOPIC flServerSubTopic;
        SERV2_MQTT_PUBTOPIC flServerPubTopic;
        SERV2_MQTT_HB_PUBTOPIC flServerHBPubtopic;
        SERV2_MQTT_DATA_PUBTOPIC flServerDataPubtopic;
        FL_SERV_HTTPURL flServerHttpUrl;
        FL_SERV_HTTPTOKEN flServerHttpToken;
        FL_SERV_HTTPPORT flServerHttpPort;

        DHEAD_EN dheadEnable;
        PROTOCOL_SEL protocol;
        HTTP_DATA_SERVIP dataServerIP;
        HTTP_DATA_SERVURL dataServerUrl;
        HTTP_DATA_SERVTOKEN dataServerToken;
        HTTP_DATA_SERVPORT dataServerPort;
        HTTP_BACKUP_SERVIP backupServerIP;
        HTTP_BACKUP_SERVURL backupServerUrl;
        HTTP_BACKUP_SERVPORT backupServerPort;
        HTTP_SSL_EN httpSslEnable;

        FTP_URL ftpUrl;
        FTP_UNAME ftpUser;
        FTP_PASS ftpPass;
        FTP_PORT ftpPort;
        LOCAL_IPPORT localIpPort;
        LOCAL_SSID localSSID;
        LOCAL_SSID_PASS localSSIDPASS;
        CIPHER_KEY cipher;
        SERPASS_KEY serpass;

        MODBUS_TCP_CLIENT_IPADDR modTcpClientIpAddr;
        MODBUS_TCP_CLIENT_PORT modTcpClientPort;

        DHCP dhcp;
        SUBNET_ADDR subnet;
        IP_ADDR ipAddr;
        GATEWAY_ADDR gateway;
        PDNS_ADDR pDns;
        SDNS_ADDR sDns;
        DNS_NAME dns;

        STA_SSID staSsid;
        STA_PASS staPass;
        AP_SSID apSsid;
        AP_PASS apPass;

        UID uID;
        UPASS uPass;
        MID mID;
        MPASS mPass;

    };

} SYSTEM_PARAM;

typedef uint32_t MODEMRSTCNT;
typedef uint32_t PREV_DAY;
typedef float PREV_DAY_TOTALIZER;
typedef union _SYSTEM_PARAM_PUMP_CALC_
{
    unsigned char array[sizeof(MODEMRSTCNT) + sizeof(PREV_DAY) + sizeof(PREV_DAY_TOTALIZER) + sizeof(PREV_DAY_TOTALIZER)];
    struct
    {
        MODEMRSTCNT modemRstCnt;
        PREV_DAY prevDay;
        PREV_DAY_TOTALIZER fl1_prevDayTotalizer;
        PREV_DAY_TOTALIZER fl2_prevDayTotalizer;
    };

} SYSTEM_PARAM_2;

typedef enum
{
    APP_SM_INIT
} APP_SM;

#endif

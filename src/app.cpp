#include "app.h"
#include "global-vars.h"

#ifdef TEST_SPIFFS_READ_WRITE_LARGEDATA
uint8_t testWriteArray[10240] = {0};
uint8_t testReadArray[10240] = {0};
#endif

void AppInit(void)
{
    uint32_t sysParamFileReadSize, pumpCalcFileReadSize;

#ifdef TEST_SPIFFS_READ_WRITE_LARGEDATA

    for (uint32_t i = 0; i < 10240; i++)
    {
        testWriteArray[i] = i;
    }
    File file = SPIFFS.open("/test.txt", FILE_WRITE);
    if (!file)
    {
        debugPrintln("Failed to open test file for reading");
    }

    uint32_t len = file.write(testWriteArray, 10240);
    debugPrint("Write len: ");
    debugPrintln(len);

    /* write large file in chunks
        // uint32_t size = 0;
        // uint32_t len = file.write(&testWriteArray[size], 1000);
        // debugPrint("Write len: ");
        // debugPrintln(len);
        // size = size + len;
        // file.flush();

        // len = file.write(&testWriteArray[size], 1000);
        // debugPrint("Write len: ");
        // debugPrintln(len);
        // size = size + len;
        // file.flush();

        // len = file.write(&testWriteArray[size], 1000);
        // debugPrint("Write len: ");
        // debugPrintln(len);
        // size = size + len;
        // file.flush();

        // len = file.write(&testWriteArray[size], 1000);
        // debugPrint("Write len: ");
        // debugPrintln(len);
        // size = size + len;
        // file.flush();

        // len = file.write(&testWriteArray[size], 1000);
        // debugPrint("Write len: ");
        // debugPrintln(len);
        // size = size + len;
        // file.flush();
        */

    file.close();

    file = SPIFFS.open("/test.txt", FILE_READ);
    if (!file)
    {
        debugPrintln("Failed to open test file for reading");
    }

    len = file.read(testReadArray, file.size());
    debugPrint("Read len: ");
    debugPrintln(len);

    for (uint32_t i = 0; i < 10240; i++)
    {
        if (testWriteArray[i] != testReadArray[i])
        {
            debugPrint("read write compare failed at: ");
            debugPrintln(i);
            break;
        }
    }
    debugPrintln("[APP DEBUG] Read write large file spiffs file test done!");
    file.close();
#endif

    // Get system parameters saved in flash memory to local RAM
    if (false == AppGetConfigSysParams(&gSysParam, &sysParamFileReadSize))
    {
        debugPrintln("[APP DEBUG] Config system parameter file read failed! restoring default.");
        // restore default system parameters
        flashErrorCount++;
        systemFlags.flashError = true;
        AppMakeDefaultConfigSystemParams();
        debugPrintln("\n[APP DEBUG] Resetting device to restore default config settings!\n");
        ESP.restart();
        // reading default configs into globle structure
        AppGetConfigSysParams(&gSysParam, &sysParamFileReadSize);
    }
    else
    {
        apnMode = gSysParam.apnMode; // read apnmode into a globle variable

        // if memory had corrupted, then stinterval value will be 0.
        if (0 >= gSysParam.uRate)
        {
            gSysParam.uRate = 5;
        }
        if (0 >= gSysParam.hbInterval)
        {
            gSysParam.hbInterval = 300;
        }
        if (0 >= gSysParam.logTime)
        {
            gSysParam.logTime = 300;
        }

        modTcpClientIp.fromString((const char *)gSysParam.modTcpClientIpAddr);

        local_IP.fromString((const char *)gSysParam.ipAddr);
        gateway_IP.fromString((const char *)gSysParam.gateway);
        subnet_Mask.fromString((const char *)gSysParam.subnet);
        p_dns.fromString((const char *)gSysParam.pDns);
        s_dns.fromString((const char *)gSysParam.sDns);

        apSSID = String(gSysParam.apSsid);
        apSSID.concat("-");
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X%02X%02X", deviceAddress.id[3], deviceAddress.id[4], deviceAddress.id[5]);
        apSSID.concat(mac);
        apPASS = String(gSysParam.apPass);
    }

    // Get pump calculation parameters saved in flash memory to local RAM
    if (false == AppGetConfigSysParams2(&gSysParam2, &pumpCalcFileReadSize))
    {
        debugPrintln("[APP DEBUG] Sys parameter 2 file read failed! restoring default.");

        flashErrorCount++;
        systemFlags.flashError = true;
        AppMakeDefaultSysParams2();
        // reading default configs into globle structure
        AppGetConfigSysParams2(&gSysParam2, &pumpCalcFileReadSize);
    }
    else
    {
    }

#ifdef TEST_CONFIG_SETTING_RESTORED_SUCCESS_DEBUG
    if (gSysParam.modbusAddr != DEFAULT_MODBUS_ADDR)
    {
        debugPrintln("failed to get modbus address.");
    }
    if (gSysParam.uRate != DEFAULT_UPDATE_RATE)
    {
        debugPrintln("failed to get uRate.");
    }
    if (gSysParam.hbInterval != DEFAULT_HB_INTERVAL)
    {
        debugPrintln("failed to get hbInterval.");
    }
    if (gSysParam.calibrationFactorForTime != DEFAULT_CALIBFACTOR)
    {
        debugPrintln("failed to get calibrationFactorForTime.");
    }
    if (0 != strcmp(gSysParam.defaultLat, DEFAULT_DEFLAT))
    {
        debugPrintln("failed to get defaultLat.");
    }
    if (0 != strcmp(gSysParam.defaultLong, DEFAULT_DEFLONG))
    {
        debugPrintln("failed to get defaultLong.");
    }

    if (gSysParam.baud232 != DEFAULT_BAUDRATE_232)
    {
        debugPrintln("failed to get baud232.");
    }
    if (gSysParam.baud485 != DEFAULT_BAUDRATE_485)
    {
        debugPrintln("failed to get baud485.");
    }
    if (gSysParam.dataBits != DEFAULT_DATABIT)
    {
        debugPrintln("failed to get dataBits.");
    }
    if (gSysParam.stopBits != DEFAULT_STOPBIT)
    {
        debugPrintln("failed to get stopBits.");
    }
    if (gSysParam.parit != DEFAULT_PARIT)
    {
        debugPrintln("failed to get parit.");
    }
#endif
}

// Neevo: declaring it global as it was causig esp reset when defsys is done from mqtt cmdline
SYSTEM_PARAM defaultSysParam;

bool AppMakeDefaultConfigSystemParams(void)
{
    defaultSysParam.sensor1ModAddress = DEFAULT_MODBUS_SLAVE1_ADDR;
    defaultSysParam.sensor2ModAddress = DEFAULT_MODBUS_SLAVE2_ADDR;
    defaultSysParam.sensor3ModAddress = DEFAULT_MODBUS_SLAVE3_ADDR;
    defaultSysParam.sensor4ModAddress = DEFAULT_MODBUS_SLAVE4_ADDR;
    defaultSysParam.sensor5ModAddress = DEFAULT_MODBUS_SLAVE5_ADDR;
    defaultSysParam.sensor6ModAddress = DEFAULT_MODBUS_SLAVE6_ADDR;
    defaultSysParam.sensor7ModAddress = DEFAULT_MODBUS_SLAVE7_ADDR;
    defaultSysParam.sensor8ModAddress = DEFAULT_MODBUS_SLAVE8_ADDR;
    defaultSysParam.sensor9ModAddress = DEFAULT_MODBUS_SLAVE9_ADDR;
    defaultSysParam.sensor10ModAddress = DEFAULT_MODBUS_SLAVE10_ADDR;

    defaultSysParam.modbusSlave1Enable = DEFAULT_MODBUS_SLAVE1_EN_STAT;
    defaultSysParam.modbusSlave2Enable = DEFAULT_MODBUS_SLAVE2_EN_STAT;
    defaultSysParam.modbusSlave3Enable = DEFAULT_MODBUS_SLAVE3_EN_STAT;
    defaultSysParam.modbusSlave4Enable = DEFAULT_MODBUS_SLAVE4_EN_STAT;
    defaultSysParam.modbusSlave5Enable = DEFAULT_MODBUS_SLAVE5_EN_STAT;

    strcpy(defaultSysParam.farm1Name, DEFAULT_FARM1_NAME);
    strcpy(defaultSysParam.farm2Name, DEFAULT_FARM2_NAME);
    strcpy(defaultSysParam.farm3Name, DEFAULT_FARM3_NAME);
    strcpy(defaultSysParam.farm4Name, DEFAULT_FARM4_NAME);
    strcpy(defaultSysParam.farm5Name, DEFAULT_FARM5_NAME);

    defaultSysParam.uRate = DEFAULT_UPDATE_RATE;
    defaultSysParam.hbInterval = DEFAULT_HB_INTERVAL;
    defaultSysParam.ap4gOrWifiEn = DEFAULT_AP_4G_OR_WIFI_MODE;
    defaultSysParam.loraOr4gEn = DEFAULT_LORA_OR_4G_MODE;

    strcpy(defaultSysParam.hostname, DEFAULT_HOSTNAME);
    strcpy(defaultSysParam.defaultLat, DEFAULT_DEFLAT);
    strcpy(defaultSysParam.defaultLong, DEFAULT_DEFLONG);
    strcpy(defaultSysParam.sitename, DEFAULT_SITENAME);
    strcpy(defaultSysParam.clientname, DEFAULT_CLEINTNAME);

    defaultSysParam.baud232 = DEFAULT_BAUDRATE_232;
    defaultSysParam.baud485 = DEFAULT_BAUDRATE_485;
    defaultSysParam.dataBits = DEFAULT_DATABIT;
    defaultSysParam.stopBits = DEFAULT_STOPBIT;
    defaultSysParam.parit = DEFAULT_PARIT;

    defaultSysParam.logSendMqtt = DEFAULT_LOGSEND_MQTT;
    defaultSysParam.logSendHttp = DEFAULT_LOGSEND_HTTP;
    defaultSysParam.logSaveMqtt = DEFAULT_LOGSAVE_MQTT;
    defaultSysParam.logSaveHttp = DEFAULT_LOGSAVE_HTTP;
    defaultSysParam.logCnt = DEFAULT_LOGCOUNT;

    defaultSysParam.apnMode = DEFAULT_APN_MODE;
    strcpy(defaultSysParam.apn1, DEFAULT_APN_1);
    strcpy(defaultSysParam.apn2, DEFAULT_APN_2);

    strcpy(defaultSysParam.imei, DEFAULT_IMEI_NUM);

    defaultSysParam.logTime = DEFAULT_LOG_TIME;

    strcpy(defaultSysParam.serv1MqttHostname, DEFAULT_SERV1_MQTT_HOST);
    defaultSysParam.serv1MqttPort = DEFAULT_SERV1_MQTT_PORT;
    strcpy(defaultSysParam.serv1MqttUname, DEFAULT_SERV1_MQTT_UNAME);
    strcpy(defaultSysParam.serv1MqttPass, DEFAULT_SERV1_MQTT_PASS);
    strcpy(defaultSysParam.serv1MqttClientId, DEFAULT_SERV1_MQTT_CLIENTID);
    defaultSysParam.serv1SslEnable = DEFAULT_SERV1_MQTT_SSL_EN;
    defaultSysParam.serv1CertEnable = DEFAULT_SERV1_MQTT_CERT_EN;

    strcpy(defaultSysParam.serv1InfoSubTopic, DEFAULT_INFOSUBTOP);
    strcpy(defaultSysParam.serv1OtpSubTopic, DEFAULT_OTPSUBTOP);
    strcpy(defaultSysParam.serv1HBPubTopic, DEFAULT_HBPUBTOP);
    strcpy(defaultSysParam.serv1DataPubTopic, DEFAULT_DATAPUBTOP);
    strcpy(defaultSysParam.serv1OnDemandSubTopic, DEFAULT_ONDMD_SUBTOP);
    strcpy(defaultSysParam.serv1OnDemandPubTopic, DEFAULT_ONDMD_PUBTOP);
    strcpy(defaultSysParam.serv1ConfigSubTopic, DEFAULT_CONFIG_SUBTOP);
    strcpy(defaultSysParam.serv1ConfigPubTopic, DEFAULT_CONFIG_PUBTOP);

    strcpy(defaultSysParam.flServerHost, DEFAULT_SERV2_MQTT_HOST);
    defaultSysParam.flServerMqttPort = DEFAULT_SERV2_MQTT_PORT;
    strcpy(defaultSysParam.flServerMqttUname, DEFAULT_SERV2_MQTT_UNAME);
    strcpy(defaultSysParam.flServerMqttPass, DEFAULT_SERV2_MQTT_PASS);
    strcpy(defaultSysParam.flServerMqttClientId, DEFAULT_SERV2_MQTT_CLIENTID);
    defaultSysParam.flServerMqttQos = DEFAULT_SERV2_MQTT_QOS;
    defaultSysParam.flServerMqttRetain = DEFAULT_SERV2_MQTT_RETAIN;
    defaultSysParam.flServerMqttKeepAlive = DEFAULT_SERV2_MQTT_KEEPALIVE;
    defaultSysParam.flServerMqttAuth = DEFAULT_SERV2_MQTT_AUTH;

    strcpy(defaultSysParam.flServerSubTopic, DEFAULT_SERV2_MQTT_SUBTOPIC);
    strcpy(defaultSysParam.flServerPubTopic, DEFAULT_SERV2_MQTT_PUBTOPIC);
    strcpy(defaultSysParam.flServerHBPubtopic, DEFAULT_SERV2_MQTT_HB_PUBTOPIC);
    strcpy(defaultSysParam.flServerDataPubtopic, DEFAULT_SERV2_MQTT_DATA_PUBTOPIC);
    strcpy(defaultSysParam.flServerHttpUrl, DEFAULT_FL_SERV_HTTPURL);
    strcpy(defaultSysParam.flServerHttpToken, DEFAULT_FL_SERV_HTTPTOKEN);
    defaultSysParam.flServerHttpPort = DEFAULT_FL_SERV_HTTPPORT;

    defaultSysParam.dheadEnable = DEFAULT_DHEAD_EN;
    defaultSysParam.protocol = DEFAULT_PROTOCOL_SEL;
    strcpy(defaultSysParam.dataServerIP, DEFAULT_HTTP_DATA_SERVIP);
    strcpy(defaultSysParam.dataServerUrl, DEFAULT_HTTP_DATA_SERVURL);
    strcpy(defaultSysParam.dataServerToken, DEFAULT_HTTP_DATA_SERVTOKEN);
    defaultSysParam.dataServerPort = DEFAULT_HTTP_DATA_SERVPORT;
    strcpy(defaultSysParam.backupServerIP, DEFAULT_HTTP_BACKUP_SERVIP);
    strcpy(defaultSysParam.backupServerUrl, DEFAULT_HTTP_BACKUP_SERVURL);
    defaultSysParam.backupServerPort = DEFAULT_HTTP_BACKUP_SERVPORT;
    defaultSysParam.httpSslEnable = DEFAULT_HTTP_SSL_EN;

    strcpy(defaultSysParam.ftpUrl, DEFAULT_FTP_URL);
    strcpy(defaultSysParam.ftpUser, DEFAULT_FTP_UNAME);
    strcpy(defaultSysParam.ftpPass, DEFAULT_FTP_PASS);
    defaultSysParam.ftpPort = DEFAULT_FTP_PORT;
    strcpy(defaultSysParam.localIpPort, DEFAULT_LOCAL_IPPORT);
    strcpy(defaultSysParam.localSSID, DEFAULT_LOCAL_SSID);
    strcpy(defaultSysParam.localSSIDPASS, DEFAULT_LOCAL_SSID_PASS);
    strcpy(defaultSysParam.cipher, DEFAULT_CIPHER_KEY);
    strcpy(defaultSysParam.serpass, DEFAULT_SERPASS_KEY);

    strcpy(defaultSysParam.modTcpClientIpAddr, DEFAULT_MODBUS_TCP_CLEINT_IP);
    defaultSysParam.modTcpClientPort = DEFAULT_MODBUS_TCP_CLEINT_PORT;

    defaultSysParam.dhcp = DEFAULT_DHCP;
    strcpy(defaultSysParam.subnet, DEFAULT_SUBNET);
    strcpy(defaultSysParam.ipAddr, DEFAULT_IP);
    strcpy(defaultSysParam.gateway, DEFAULT_GATEWAY);
    strcpy(defaultSysParam.pDns, DEFAULT_PDNS);
    strcpy(defaultSysParam.sDns, DEFAULT_SDNS);
    strcpy(defaultSysParam.dns, DEFAULT_DNS);

    strcpy(defaultSysParam.staSsid, DEFAULT_STASSID);
    strcpy(defaultSysParam.staPass, DEFAULT_STAPASS);
    strcpy(defaultSysParam.apSsid, DEFAULT_APSSID);
    strcpy(defaultSysParam.apPass, DEFAULT_APPASS);

    strcpy(defaultSysParam.uID, DEFAULT_UID);
    strcpy(defaultSysParam.uPass, DEFAULT_UPASS);
    strcpy(defaultSysParam.mID, DEFAULT_MID);
    strcpy(defaultSysParam.mPass, DEFAULT_MPASS);

    defaultSysParam.telnetEn = DEFAULT_TELNET_EN;
    defaultSysParam.mqttEn = DEFAULT_MQTT_EN;
    

    if (false == AppSetConfigSysParams(&defaultSysParam))
    {
        return false;
    }
    return true;
}

bool AppSetConfigSysParams(SYSTEM_PARAM *pSysParam)
{
    size_t len;

    File file = SPIFFS.open(APP_SYS_PARAM_CONFIG_FILE_PATH, FILE_WRITE);
    if (!file)
    {
        debugPrintln("[APP DEBUG] Failed to open sysparam file for writing");
        return false;
    }

    len = file.write(pSysParam->array, sizeof(pSysParam->array));

    file.close();

    if (len != sizeof(pSysParam->array))
    {
        debugPrintln("[APP DEBUG] Sys parameter size and write size mismatched!");
        return false;
    }
    return true;
}

bool AppGetConfigSysParams(SYSTEM_PARAM *pSysParam, uint32_t *pReadLen)
{
    File file = SPIFFS.open(APP_SYS_PARAM_CONFIG_FILE_PATH, FILE_READ);
    if (!file)
    {
        debugPrintln("[APP DEBUG] Failed to open sysparam file for reading");
        return false;
    }

    *pReadLen = file.read(pSysParam->array, file.size());

    file.close();

    if (*pReadLen != sizeof(pSysParam->array))
    {
        debugPrintln("\n[APP DEBUG] Sys parameter size and read size mismatched!");
        return false;
    }
    return true;
}

bool AppMakeDefaultSysParams2(void)
{
    SYSTEM_PARAM_2 defaultSysParams2;

    defaultSysParams2.modemRstCnt = DEFAULT_MODEM_RST_CNT;
    defaultSysParams2.prevDay = DEFAULT_PREV_DAY;
    defaultSysParams2.fl1_prevDayTotalizer = DEFAULT_PREV_DAY_TOTALIZER; 
    defaultSysParams2.fl2_prevDayTotalizer = DEFAULT_PREV_DAY_TOTALIZER;

    if (false == AppSetConfigSysParams2(&defaultSysParams2))
    {
        return false;
    }
    return true;
}

bool AppSetConfigSysParams2(SYSTEM_PARAM_2 *pSysParam2)
{
    size_t len;

    File file = SPIFFS.open(APP_SYS_PARAM_2_CONFIG_FILE_PATH, FILE_WRITE);
    if (!file)
    {
        debugPrintln("[APP DEBUG] Failed to open sysparam 2 file for writing");
        return false;
    }

    len = file.write(pSysParam2->array, sizeof(pSysParam2->array));

    file.close();

    if (len != sizeof(pSysParam2->array))
    {
        debugPrintln("[APP DEBUG] sys param 2 file size and write size mismatched!");
        return false;
    }
    return true;
}

bool AppGetConfigSysParams2(SYSTEM_PARAM_2 *pSysParam2, uint32_t *pReadLen)
{
    File file = SPIFFS.open(APP_SYS_PARAM_2_CONFIG_FILE_PATH, FILE_READ);
    if (!file)
    {
        debugPrintln("[APP DEBUG] Failed to open sysparam 2 file for reading");
        return false;
    }

    *pReadLen = file.read(pSysParam2->array, file.size());

    file.close();

    if (*pReadLen != sizeof(pSysParam2->array))
    {
        debugPrintln("\n[APP DEBUG] sys param 2 file size and read size mismatched!");
        return false;
    }
    return true;
}
#include "globals.h"
#include <helper.h>
#include <FS.h>
#include "ESPmDNS.h"
#include <Update.h>
#include "flWifiHandler.h"
#include "flmodbus.h"
#include "global-vars.h"
#include "app.h"
#include "buzzer.h"

byte gMsgID = 0;

tSYSPARAMS systemPara;
uSYSFLGS systemFlags;
tDEV_ID deviceAddress;
tHB_PARAMS hbParam;
tDT rtcDateTime;
tNETWORKDT netDateTime;
uFM_FLAGS fmFailedFlagStatus;

tFM_DATA fmData[2];

// efsdbLoggerHandle efsdbMqttLogHandle;
// efsdbLoggerHandle efsdbHttpLogHandle;

uint32_t httpConnectionErrorCount = 0;
uint32_t flashErrorCount = 0;

uint32_t conectToInternetFailedCount = 10;
uint32_t modemRestartCount;

String apSSID;
String apPASS;

char timeStamp[20];
char rtcDate[15];
char rtcTime[15];
uint8_t currday = 0;
char devStartTime[12];
char lastPacketSentSuccessTime[12];

String otaURL;

uint32_t connectionDelay = 0;

bool isWifiNetClientSelected = false;

int apnMode;
String apnSim1, apnSim2;

// String uID;
// String uPass;
// String mID;
// String mPass;

String regionState = "";

unsigned long secondsSince2000 = 0, prevSecondsSince2000;
uint32_t unixTimeStamp;

String simID;

String deviceID;

AsyncWebServer asyncWebServer(TCP_SERVER_PORT);

// This flag is used to differentiate download certificate command received from serial/http response
bool downloadCertificateBySerialCmdFlag = false;

bool addServiceFlag = true;
bool wifiEnableFlag = false;
bool wifiSettingsResetFlag = false;
bool serverConFlg = false;
bool dnsStsFlg = false;
bool otaSetupFlg = false;

IPAddress local_IP, gateway_IP, subnet_Mask, p_dns, s_dns;
IPAddress modTcpClientIp;

// WiFiClient RemoteClient;
// Enum which Checks the ongoing Upload Process.
typedef enum UploadStatus
{
    OTA_UPLOAD_START,
    OTA_UPLOAD_WRITE,
    OTA_UPLOAD_END,
    OTA_UPLOAD_ABORTED
} eOTA_UPLOAD_STS;

eOTA_UPLOAD_STS uploadStatus;

extern CIRCULAR_BUFFER respBuf;

StaticJsonDocument<1536> systemParmBackupJsonDoc;
StaticJsonDocument<2048> systemParmRestoreJsonDoc;

void hotspotInit(void);

bool CheckForSerquenceNum(int *argc, char *argv[])
{
    char *seqNumStr = argv[(*argc) - 1];

    if ('^' == seqNumStr[0])
    {
        return true;
    }
    else
    {
        return false;
    }
}

void SpiffsInit()
{
    String tempID;

    if (!SPIFFS.begin(true))
    {
        reTry(
            {
                debugPrint("[SPIFFS] Retrying to mount flash memory\n");
                if (SPIFFS.begin(true))
                {
                    debugPrint("[SPIFFS] Flash memory Mounted\n");
                    break;
                }
                else
                {
                    debugPrint("[SPIFFS] Flash memory mount Error\n");
                    flashErrorCount++;
                    systemFlags.flashError = true;
                    delay(100);
                }
            },
            3);
    }
    else
    {
        debugPrint("[SPIFFS] Flash memory mounted\n");
    }
    systemFlags.flashError = false;
}

void hotspotInit(void)
{
    IPAddress ipAddress(192, 168, 0, 200);
    IPAddress gatewayIp(192, 168, 0, 1);
    IPAddress subnetM(255, 255, 255, 0);

    WiFi.mode(WIFI_AP);
    debugPrintln("Entered AP Mode");
    WiFi.softAP(apSSID.c_str(), apPASS.c_str()); // AP Mode connection
    // delay(1000);
    WiFi.softAPConfig(ipAddress, gatewayIp, subnetM);
    // WiFi.softAPConfig(local_IP, gateway_IP, subnet_Mask);
    debugPrint("IP address: ");
    debugPrintln(WiFi.softAPIP());
}

void dnsInit(const char *MyName)
{
    if (MDNS.begin(MyName))
    {
        debugPrintln(F("mDNS responder started"));
        debugPrint(F("WiFi DNS: "));
        debugPrint(MyName);
        debugPrint(F(":"));
        debugPrintln(TCP_SERVER_PORT);
    }
    else
    {
        debugPrintln(F("Error setting up MDNS responder"));
    }

    // Add service to MDNS-SD for first time only when user button is pressed for 10sec
    if (addServiceFlag == true)
    {
        MDNS.addService("http", "tcp", TCP_SERVER_PORT);
        addServiceFlag = false;
    }

    asyncWebServer.begin();
}

void ButtonCallback(void)
{
    if (gSystemFlag.btOrWifiAPEnableFlag)
    {
        WiFi.mode(WIFI_OFF);
        debugPrintln("Start Hotspot");
        hotspotInit();
        // delay(1000);
        asyncWebServer.begin();
        // dnsInit(gSysParam.dns); // dns pulsecounter.local:3000
        // HotspotStart_tone();
    }
    else
    {
        debugPrintln("Stop Hotspot mode");
        // debugPrintln("dns closed");
        // addServiceFlag = true;
        asyncWebServer.end();
        WiFi.mode(WIFI_OFF);
        // MDNS.end();
        // server.end();
        // HotspotStop_tone();
    }
}

void WebServerStart(void)
{
    asyncWebServer.begin();
}

void WebServerStop(void)
{
    asyncWebServer.end();
}

void WebServerHandler()
{
    // Uncomment when we have web pages.
    asyncWebServer.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

    // Authentication API:
    asyncWebServer.on(
        "/api/login", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }

            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<96> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {
                if (((doc["userid"] == gSysParam.uID) && (doc["password"] == gSysParam.uPass)) || ((doc["userid"] == gSysParam.mID) && (doc["password"] == gSysParam.mPass)))
                {
                    request->send(200, "application/json", TRUE_RESPONSE);
                }
                else
                {
                    request->send(401, "application/json", FALSE_RESPONSE);
                }
            }
        });

    // Change Password API:
    asyncWebServer.on(
        "/api/changeP", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }

            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<96> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrintln(F("deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {
                if (strlen(doc["password1"]) < sizeof(gSysParam.uPass))
                {
                    strcpy(gSysParam.uPass, doc["password1"]);
                    debugPrintln("[WEBPAGE] Password changed successfully");
                    debugPrintln();
                    if (false == AppSetConfigSysParams(&gSysParam))
                    {
                    }
                    request->send(200, "application/json", TRUE_RESPONSE);
                }
                else
                {
                    request->send(500, "application/json", FALSE_RESPONSE);
                }
            }
        });

    asyncWebServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<384> doc;

                    doc["Manufacturer"] = "Fountlab Solutions Pvt. Ltd.";
                    doc["Device"] = gSysParam.hostname;
                    doc["IMEI"] = gSysParam.imei;
                    doc["DeviceID"] = deviceID.c_str();
                    doc["WiFiMac"] = WiFi.macAddress();
                    doc["SDKVersion"] = ESP.getSdkVersion();
                    doc["FirmwareName"] = HARDWARE;
                    char fwVer[10];
                    sprintf(fwVer, "V%s", FW_VER);
                    doc["FirmwareVersion"] = fwVer;
                    doc["FirmwareBuildDate"] = FW_BUILD;
                    doc["Network"] = apSSID.c_str();
                    doc["WiFiIP"] = gSysParam.ipAddr;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on("/api/general", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<192> doc;

                    doc["hostname"] = gSysParam.hostname;
                    if (true == hbParam.gpslock)
                    {
                        doc["lattitude"] = hbParam.latitute;
                        doc["longitude"] = hbParam.longitude;
                    }
                    else
                    {
                        doc["lattitude"] = gSysParam.defaultLat;
                        doc["longitude"] = gSysParam.defaultLong;
                    }
                    doc["sitename"] = gSysParam.sitename;
                    doc["clientname"] = gSysParam.clientname;
                    doc["logstorage"] = gSysParam.logSaveHttp;
                    doc["logsend"] = gSysParam.logSendHttp;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/general", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<256> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("hostname"))
                {
                    if (!doc["hostname"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["hostname"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.hostname, doc["hostname"]);
                        debugPrint("[WEBPAGE] Updating hostname as: ");
                        debugPrintln(gSysParam.hostname);
                    }
                }
                if (doc.containsKey("lattitude"))
                {
                    if (!doc["lattitude"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["lattitude"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.defaultLat, doc["lattitude"]);
                        debugPrint("[WEBPAGE] Updating lattitude as: ");
                        debugPrintln(gSysParam.defaultLat);
                    }
                }
                if (doc.containsKey("longitude"))
                {
                    if (!doc["longitude"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["longitude"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.defaultLong, doc["longitude"]);
                        debugPrint("[WEBPAGE] Updating longitude as: ");
                        debugPrintln(gSysParam.defaultLong);
                    }
                }
                if (doc.containsKey("sitename"))
                {
                    if (!doc["sitename"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["sitename"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.sitename, doc["sitename"]);
                        debugPrint("[WEBPAGE] Updating sitename as: ");
                        debugPrintln(gSysParam.sitename);
                    }
                }
                if (doc.containsKey("clientname"))
                {
                    if (!doc["clientname"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["clientname"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.clientname, doc["clientname"]);
                        debugPrint("[WEBPAGE] Updating clientname as: ");
                        debugPrintln(gSysParam.clientname);
                    }
                }
                if (doc.containsKey("logstorage"))
                {
                    if (!doc["logstorage"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["logstorage"].as<String>().c_str(), URATE);
                        gSysParam.logSaveHttp = (int)doc["logstorage"];
                        debugPrint("[WEBPAGE] Updating logstorage as :");
                        debugPrintln(gSysParam.logSaveHttp);
                    }
                }
                if (doc.containsKey("logsend"))
                {
                    if (!doc["logsend"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["logsend"].as<String>().c_str(), URATE);
                        gSysParam.logSendHttp = (int)doc["logsend"];
                        debugPrint("[WEBPAGE] Updating logsend as :");
                        debugPrintln(gSysParam.logSendHttp);
                    }
                }
                debugPrintln("[WEBPAGE] General settings Succes!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    asyncWebServer.on("/api/getconnectivity", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<400> doc;

                    doc["conn"] = gSysParam.ap4gOrWifiEn;
                    doc["ssid"] = gSysParam.staSsid;
                    doc["password"] = gSysParam.staPass;
                    // gSysParam.ap4gOrWifiEn = 0, dhcp will be static by default
                    if (false == gSysParam.ap4gOrWifiEn)
                    {
                        doc["dhcp"] = 0;
                    }
                    else
                    {
                        doc["dhcp"] = gSysParam.dhcp;
                    }
                    doc["staticIP"] = gSysParam.ipAddr;
                    doc["gatewayIP"] = gSysParam.gateway;
                    doc["subnetMask"] = gSysParam.subnet;
                    doc["pdns"] = gSysParam.pDns;
                    doc["sdns"] = gSysParam.sDns;
                    doc["dns"] = gSysParam.dns;
                    doc["autoApn"] = gSysParam.apnMode;
                    doc["sim1APN"] = gSysParam.apn1;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/setconnectivity", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            IPAddress TempAddress;

            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<400> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("conn"))
                {
                    if (!doc["conn"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["conn"].as<String>().c_str(), URATE);
                        gSysParam.ap4gOrWifiEn = (int)doc["conn"];
                        debugPrint("[WEBPAGE] Updating conn as :");
                        debugPrintln(gSysParam.ap4gOrWifiEn);
                        wifiApDoneFlag = false;
                    }
                }
                if (doc.containsKey("ssid"))
                {
                    if (!doc["ssid"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["ssid"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.staSsid, doc["ssid"]);
                        debugPrint("[WEBPAGE] Updating ssid as: ");
                        debugPrintln(gSysParam.staSsid);
                    }
                }
                if (doc.containsKey("password"))
                {
                    if (!doc["password"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["password"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.staPass, doc["password"]);
                        debugPrint("[WEBPAGE] Updating password as: ");
                        debugPrintln(gSysParam.staPass);
                    }
                }
                if (doc.containsKey("dhcp"))
                {
                    if (!doc["dhcp"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["dhcp"].as<String>().c_str(), URATE);
                        gSysParam.dhcp = (int)doc["dhcp"];
                        debugPrint("[WEBPAGE] Updating dhcp as :");
                        debugPrintln(gSysParam.dhcp);
                    }
                }
                if (doc.containsKey("staticIP"))
                {
                    if (!doc["staticIP"].isNull())
                    {
                        if (!TempAddress.fromString(doc["staticIP"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(doc["staticIP"], IPADDR);
                        strcpy(gSysParam.ipAddr, doc["staticIP"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating ipAddr as :");
                        debugPrintln(gSysParam.ipAddr);
                    }
                }
                if (doc.containsKey("gatewayIP"))
                {
                    if (!doc["gatewayIP"].isNull())
                    {
                        if (!TempAddress.fromString(doc["gatewayIP"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(doc["gatewayIP"], GATEWAY);
                        strcpy(gSysParam.gateway, doc["gatewayIP"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating gateway as :");
                        debugPrintln(gSysParam.gateway);
                    }
                }
                if (doc.containsKey("subnetMask"))
                {
                    if (!doc["subnetMask"].isNull())
                    {
                        if (!TempAddress.fromString(doc["subnetMask"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(doc["subnetMask"], SUBNET);
                        strcpy(gSysParam.subnet, doc["subnetMask"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating subnet as :");
                        debugPrintln(gSysParam.subnet);
                    }
                }
                if (doc.containsKey("dns"))
                {
                    if (!doc["dns"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["dns"], DNS);
                        strcpy(gSysParam.dns, doc["dns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating dns as :");
                        debugPrintln(gSysParam.dns);
                    }
                }
                if (doc.containsKey("pdns"))
                {
                    if (!doc["pdns"].isNull())
                    {
                        if (!TempAddress.fromString(doc["pdns"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(doc["pdns"], PDNS);
                        strcpy(gSysParam.pDns, doc["pdns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating pdns as :");
                        debugPrintln(gSysParam.pDns);
                    }
                }
                if (doc.containsKey("sdns"))
                {
                    if (!doc["sdns"].isNull())
                    {
                        if (!TempAddress.fromString(doc["sdns"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(doc["sdns"], SDNS);
                        strcpy(gSysParam.sDns, doc["sdns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating sdns as :");
                        debugPrintln(gSysParam.sDns);
                    }
                }

                if (doc.containsKey("autoApn"))
                {
                    if (!doc["autoApn"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["autoApn"].as<String>().c_str(), URATE);
                        gSysParam.apnMode = (int)doc["autoApn"];
                        debugPrint("[WEBPAGE] Updating autoApn as :");
                        debugPrintln(gSysParam.apnMode);
                    }
                }
                if (doc.containsKey("sim1APN"))
                {
                    if (!doc["sim1APN"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["sim1APN"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.apn1, doc["sim1APN"]);
                        debugPrint("[WEBPAGE] Updating sim1APN as: ");
                        debugPrintln(gSysParam.apn1);
                    }
                }
                debugPrintln("[WEBPAGE] Connectivity settings Succes!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                wifiSettingsResetFlag = true;
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    // Mac Address API:
    asyncWebServer.on("/api/getMAC", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                      String outstream;
                      StaticJsonDocument<48> doc;

                      doc["mac"] = deviceID.c_str();

                      serializeJsonPretty(doc, outstream);
                      debugPrintln(outstream);
                      request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/serial/cmd", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body, resp, outstream;
            int status;
            char ch;
            static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;

            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }

            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<300> doc;
            StaticJsonDocument<300> res;

            DeserializationError error = deserializeJson(doc, body);

            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {
                const char *cmd = doc["cmd"]; // "SET DT 270721115200$0D"
                // debugPrint("[CMDLINE] Cmd Rxd: ");
                Serial.println(cmd);
                status = CmdLineProcess((char *)cmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
                if (0 > status)
                {
                    // resp = "[CMDLINE] Cmd Resp: ";
                    while (0 == CbPop(&respBuf, &ch))
                    {
                        resp.concat(ch);
                    }
                    res["res"] = resp;
                    res["sts"] = false;
                    serializeJsonPretty(res, outstream);
                    Serial.println(resp);
                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream);
                }
                else
                {
                    // resp = "[CMDLINE] Cmd Resp: ";
                    while (0 == CbPop(&respBuf, &ch))
                    {
                        resp.concat(ch);
                    }
                    res["res"] = resp;
                    res["sts"] = true;
                    serializeJsonPretty(res, outstream);
                    Serial.println(resp);
                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream);
                }
            }
        });

    // // Network Setting API:
    // asyncWebServer.on("/api/getnetset", HTTP_GET, [](AsyncWebServerRequest *request)
    //                   {

    //                 String outstream;
    //                 StaticJsonDocument<192> doc;

    //                 if (false == gSysParam.ap4gOrWifiEn)
    //                 {
    //                     doc["dhcp"] = 0;
    //                 }
    //                 else
    //                 {
    //                     doc["dhcp"] = gSysParam.dhcp;
    //                 }
    //                 doc["staticIP"] = gSysParam.ipAddr;
    //                 doc["gatewayIP"] = gSysParam.gateway;
    //                 doc["subnetMask"] = gSysParam.subnet;
    //                 doc["pdns"] = gSysParam.pDns;
    //                 doc["sdns"] = gSysParam.sDns;
    //                 doc["dns"] = gSysParam.dns;
    //                 serializeJsonPretty(doc, outstream);

    //                 debugPrintln(outstream);
    //                 request->send(200, "application/json", outstream); });

    // asyncWebServer.on(
    //     "/api/savnetset", HTTP_POST, [](AsyncWebServerRequest *request) {},
    //     NULL,
    //     [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    //     {
    //         String body;
    //         IPAddress TempAddress;

    //         for (size_t i = 0; i < len; i++)
    //         {
    //             body += (char)data[i];
    //         }
    //         debugPrint("Data received: ");
    //         debugPrintln(body.c_str());

    //         StaticJsonDocument<256> doc;

    //         DeserializationError error = deserializeJson(doc, body);

    //         if (error)
    //         {
    //             debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
    //             debugPrintln(error.c_str());
    //             request->send(400, "application/json", FALSE_RESPONSE);
    //             return;
    //         }
    //         else
    //         {
    //             if (doc.containsKey("staticIP"))
    //             {
    //                 if (!doc["staticIP"].isNull())
    //                 {
    //                     if (!TempAddress.fromString(doc["staticIP"].as<String>()))
    //                     {
    //                         request->send(500, "application/json", FALSE_RESPONSE);
    //                         return;
    //                     }
    //                     // spiffsFile.ChangeToken(doc["staticIP"], IPADDR);
    //                     strcpy(gSysParam.ipAddr, doc["staticIP"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating ipAddr as :");
    //                     debugPrintln(gSysParam.ipAddr);
    //                 }
    //             }
    //             if (doc.containsKey("gatewayIP"))
    //             {
    //                 if (!doc["gatewayIP"].isNull())
    //                 {
    //                     if (!TempAddress.fromString(doc["gatewayIP"].as<String>()))
    //                     {
    //                         request->send(500, "application/json", FALSE_RESPONSE);
    //                         return;
    //                     }
    //                     // spiffsFile.ChangeToken(doc["gatewayIP"], GATEWAY);
    //                     strcpy(gSysParam.gateway, doc["gatewayIP"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating gateway as :");
    //                     debugPrintln(gSysParam.gateway);
    //                 }
    //             }
    //             if (doc.containsKey("subnetMask"))
    //             {
    //                 if (!doc["subnetMask"].isNull())
    //                 {
    //                     if (!TempAddress.fromString(doc["subnetMask"].as<String>()))
    //                     {
    //                         request->send(500, "application/json", FALSE_RESPONSE);
    //                         return;
    //                     }
    //                     // spiffsFile.ChangeToken(doc["subnetMask"], SUBNET);
    //                     strcpy(gSysParam.subnet, doc["subnetMask"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating subnet as :");
    //                     debugPrintln(gSysParam.subnet);
    //                 }
    //             }
    //             // if (!doc["port"].isNull())
    //             // {
    //             //     debugPrint("Updating port as :");
    //             //     debugPrintln(doc["port"].as<String>());

    //             //     if (!IsNum((CHAR *)doc["port"].as<String>().c_str()))
    //             //     {
    //             //         request->send(500, "application/json", FALSE_RESPONSE);
    //             //         return;
    //             //     }
    //             //     spiffsFile.ChangeToken(doc["port"].as<String>(), TCPCLIENTPORT);
    //             // }
    //             if (doc.containsKey("dhcp"))
    //             {
    //                 // spiffsFile.ChangeToken(doc["dhcp"].as<String>().c_str(), DHCP);
    //                 gSysParam.dhcp = (int)doc["dhcp"];
    //                 debugPrint("[WEBPAGE] Updating dhcp as :");
    //                 debugPrintln(gSysParam.dhcp);
    //             }
    //             if (doc.containsKey("dns"))
    //             {
    //                 if (!doc["dns"].isNull())
    //                 {
    //                     // spiffsFile.ChangeToken(doc["dns"], DNS);
    //                     strcpy(gSysParam.dns, doc["dns"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating dns as :");
    //                     debugPrintln(gSysParam.dns);
    //                 }
    //             }
    //             if (doc.containsKey("pdns"))
    //             {
    //                 if (!doc["pdns"].isNull())
    //                 {
    //                     if (!TempAddress.fromString(doc["pdns"].as<String>()))
    //                     {
    //                         request->send(500, "application/json", FALSE_RESPONSE);
    //                         return;
    //                     }
    //                     // spiffsFile.ChangeToken(doc["pdns"], PDNS);
    //                     strcpy(gSysParam.pDns, doc["pdns"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating pdns as :");
    //                     debugPrintln(gSysParam.pDns);
    //                 }
    //             }
    //             if (doc.containsKey("sdns"))
    //             {
    //                 if (!doc["sdns"].isNull())
    //                 {
    //                     if (!TempAddress.fromString(doc["sdns"].as<String>()))
    //                     {
    //                         request->send(500, "application/json", FALSE_RESPONSE);
    //                         return;
    //                     }
    //                     // spiffsFile.ChangeToken(doc["sdns"], SDNS);
    //                     strcpy(gSysParam.sDns, doc["sdns"].as<String>().c_str());
    //                     debugPrint("[WEBPAGE] Updating sdns as :");
    //                     debugPrintln(gSysParam.sDns);
    //                 }
    //             }
    //             debugPrintln("[WEBPAGE] Network settings updated!");
    //             debugPrintln();
    //             if (false == AppSetConfigSysParams(&gSysParam))
    //             {
    //             }
    //             request->send(200, "application/json", TRUE_RESPONSE);
    //             // delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
    //             ESP.restart();
    //         }
    //     });

    // Cloud Settings API:
    asyncWebServer.on("/api/getserverset", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<128> doc;

                    doc["serverIP"] = gSysParam.dataServerIP;
                    doc["port"] = gSysParam.dataServerPort;
                    doc["serverURL"] = gSysParam.dataServerUrl;
                    doc["token"] = gSysParam.dataServerToken;
                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/setserverset", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<192> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("serverIP"))
                {
                    if (!doc["serverIP"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["serverIP"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.dataServerIP, doc["serverIP"]);
                        debugPrint("[WEBPAGE] Updating serverIP as: ");
                        debugPrintln(gSysParam.dataServerIP);
                    }
                }
                if (doc.containsKey("port"))
                {
                    if (doc["port"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["port"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.dataServerPort = (int)doc["port"];
                        systemPara.mqttPort = (int)doc["port"];
                        debugPrint("[WEBPAGE] Updating port as: ");
                        debugPrintln(gSysParam.dataServerPort);
                    }
                }
                if (doc.containsKey("serverURL"))
                {
                    if (!doc["serverURL"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["serverURL"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.dataServerUrl, doc["serverURL"]);
                        debugPrint("[WEBPAGE] Updating serverURL as: ");
                        debugPrintln(gSysParam.dataServerUrl);
                    }
                }
                if (doc.containsKey("token"))
                {
                    if (!doc["token"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["token"], DATA_SERV_TOKEN);
                        strcpy(gSysParam.dataServerToken, doc["token"]);
                        debugPrint("[WEBPAGE] Updating token as: ");
                        debugPrintln(gSysParam.dataServerToken);
                    }
                }
                debugPrintln("[WEBPAGE] Server settings Succes!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    // device config Settings API:
    asyncWebServer.on("/api/getdevconf", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<48> doc;

                    doc["urate"] = gSysParam.uRate;
                    doc["modaddr"] = gSysParam.sensor1ModAddress;
                    doc["stinterval"] = gSysParam.logTime;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/setdevconf", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<96> doc;

            DeserializationError error = deserializeJson(doc, body);

            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("urate"))
                {
                    if (!doc["urate"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["urate"].as<String>().c_str(), URATE);
                        gSysParam.uRate = (int)doc["urate"];
                        debugPrint("[WEBPAGE] Updating urate as :");
                        debugPrintln(gSysParam.uRate);
                    }
                }
                if (doc.containsKey("modaddr"))
                {
                    if (!doc["modaddr"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["modaddr"].as<String>(), MODBUSADDR);
                        gSysParam.sensor1ModAddress = (int)doc["modaddr"];
                        debugPrint("Updating modaddr as :");
                        debugPrintln(gSysParam.sensor1ModAddress);
                    }
                }
                if (doc.containsKey("stinterval"))
                {
                    if (!doc["stinterval"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["stinterval"].as<String>().c_str(), STINTERVAL);
                        gSysParam.logTime = (int)doc["stinterval"];
                        debugPrint("[WEBPAGE] Updating log interval as :");
                        debugPrintln(gSysParam.logTime);
                    }
                }
                debugPrintln("[WEBPAGE] Device setting Success!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    asyncWebServer.on("/api/getsenscalib", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<96> doc;
                    //add sensor calibration parameter
                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/setsenscalib", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<192> doc;

            DeserializationError error = deserializeJson(doc, body);

            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    asyncWebServer.on("/api/mqtt", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<512> doc;

                    doc["enable_mqtt"] = gSysParam.mqttEn;
                    doc["broker"] = gSysParam.flServerHost;
                    doc["port"] = gSysParam.flServerMqttPort;
                    doc["user"] = gSysParam.flServerMqttUname;
                    doc["pwd"] = gSysParam.flServerMqttPass;
                    doc["cid"] = gSysParam.flServerMqttClientId;
                    doc["qos"] = gSysParam.flServerMqttQos;
                    doc["retain"] = gSysParam.flServerMqttRetain;
                    doc["keep_alive"] = gSysParam.flServerMqttKeepAlive;
                    doc["userSecureConnection"] = gSysParam.flServerMqttAuth;

                    JsonObject certificates = doc.createNestedObject("certificates");
                    certificates["ca"] = "";
                    certificates["crt"] = "";
                    certificates["key"] = "";
                    doc["pubtopic"] = gSysParam.flServerPubTopic;
                    doc["attrpubtopic"] = "hostname/attribute";
                    doc["subtopic"] = gSysParam.flServerSubTopic;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/mqtt", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<768> doc;

            DeserializationError error = deserializeJson(doc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("broker"))
                {
                    if (!doc["broker"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["broker"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.flServerHost, doc["broker"]);
                        debugPrint("[WEBPAGE] Updating broker as: ");
                        debugPrintln(gSysParam.flServerHost);
                    }
                }
                if (doc.containsKey("port"))
                {
                    if (doc["port"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["port"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttPort = (int)doc["port"];
                        debugPrint("[WEBPAGE] Updating port as: ");
                        debugPrintln(gSysParam.flServerMqttPort);
                    }
                }
                if (doc.containsKey("user"))
                {
                    if (!doc["user"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["user"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttUname, doc["user"]);
                        debugPrint("[WEBPAGE] Updating user as: ");
                        debugPrintln(gSysParam.flServerMqttUname);
                    }
                }
                if (doc.containsKey("pwd"))
                {
                    if (!doc["pwd"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["user"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttPass, doc["pwd"]);
                        debugPrint("[WEBPAGE] Updating pwd as: ");
                        debugPrintln(gSysParam.flServerMqttPass);
                    }
                }
                if (doc.containsKey("cid"))
                {
                    if (!doc["cid"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["cid"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttClientId, doc["cid"]);
                        debugPrint("[WEBPAGE] Updating cid as: ");
                        debugPrintln(gSysParam.flServerMqttClientId);
                    }
                }
                if (doc.containsKey("qos"))
                {
                    if (doc["qos"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["qos"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttQos = (int)doc["qos"];
                        debugPrint("[WEBPAGE] Updating qos as: ");
                        debugPrintln(gSysParam.flServerMqttQos);
                    }
                }
                if (doc.containsKey("retain"))
                {
                    if (doc["retain"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["retain"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttRetain = (int)doc["retain"];
                        debugPrint("[WEBPAGE] Updating retain as: ");
                        debugPrintln(gSysParam.flServerMqttRetain);
                    }
                }
                if (doc.containsKey("keep_alive"))
                {
                    if (doc["keep_alive"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["keep_alive"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttKeepAlive = (int)doc["keep_alive"];
                        debugPrint("[WEBPAGE] Updating keep_alive as: ");
                        debugPrintln(gSysParam.flServerMqttKeepAlive);
                    }
                }
                if (doc.containsKey("userSecureConnection"))
                {
                    if (doc["userSecureConnection"] > 0)
                    {
                        // spiffsFile.ChangeToken(doc["userSecureConnection"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttAuth = (int)doc["userSecureConnection"];
                        debugPrint("[WEBPAGE] Updating Auth as: ");
                        debugPrintln(gSysParam.flServerMqttAuth);
                    }
                }

                JsonObject certificates = doc["certificates"];
                const char *certificates_ca = certificates["ca"];   // nullptr
                const char *certificates_crt = certificates["crt"]; // nullptr
                const char *certificates_key = certificates["key"]; // nullptr

                if (doc.containsKey("pubtopic"))
                {
                    if (!doc["pubtopic"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["pubtopic"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerPubTopic, doc["pubtopic"]);
                        debugPrint("[WEBPAGE] Updating pubtopic as: ");
                        debugPrintln(gSysParam.flServerPubTopic);
                    }
                }
                const char *attrpubtopic = doc["attrpubtopic"]; // "hostname/attribute"
                if (doc.containsKey("subtopic"))
                {
                    if (!doc["subtopic"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["subtopic"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerSubTopic, doc["subtopic"]);
                        debugPrint("[WEBPAGE] Updating subtopic as: ");
                        debugPrintln(gSysParam.flServerSubTopic);
                    }
                }

                debugPrintln("[WEBPAGE] MQTT settings Succes!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    // device config Settings API:
    asyncWebServer.on("/api/telnet", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    StaticJsonDocument<48> doc;

                    doc["telnet"] = gSysParam.telnetEn;

                    serializeJsonPretty(doc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/telnet", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            StaticJsonDocument<48> doc;

            DeserializationError error = deserializeJson(doc, body);

            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (doc.containsKey("telnet"))
                {
                    if (!doc["telnet"].isNull())
                    {
                        // spiffsFile.ChangeToken(doc["urate"].as<String>().c_str(), URATE);
                        gSysParam.telnetEn = (int)doc["telnet"];
                        debugPrint("[WEBPAGE] Updating telnet as :");
                        debugPrintln(gSysParam.telnetEn);
                    }
                }

                debugPrintln("[WEBPAGE] Telnet setting Success!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                return;
            }
        });

    asyncWebServer.on("/api/backup", HTTP_GET, [](AsyncWebServerRequest *request)
                      {
                    String outstream;

                    systemParmBackupJsonDoc["hostname"] = gSysParam.hostname;
                    systemParmBackupJsonDoc["lattitude"] = gSysParam.defaultLat;
                    systemParmBackupJsonDoc["longitude"] = gSysParam.defaultLong;
                    systemParmBackupJsonDoc["sitename"] = gSysParam.sitename;
                    systemParmBackupJsonDoc["clientname"] = gSysParam.clientname;
                    systemParmBackupJsonDoc["logstorage"] = gSysParam.logSaveHttp;
                    systemParmBackupJsonDoc["logsend"] = gSysParam.logSendHttp;
                    systemParmBackupJsonDoc["conn"] = gSysParam.ap4gOrWifiEn;
                    systemParmBackupJsonDoc["ssid"] = gSysParam.staSsid;
                    systemParmBackupJsonDoc["password"] = gSysParam.staPass;
                    systemParmBackupJsonDoc["dhcp"] = gSysParam.dhcp;
                    systemParmBackupJsonDoc["staticIP"] = gSysParam.ipAddr;
                    systemParmBackupJsonDoc["gatewayIP"] = gSysParam.gateway;
                    systemParmBackupJsonDoc["subnetMask"] = gSysParam.subnet;
                    systemParmBackupJsonDoc["pdns"] = gSysParam.pDns;
                    systemParmBackupJsonDoc["sdns"] = gSysParam.sDns;
                    systemParmBackupJsonDoc["dns"] = gSysParam.dns;
                    systemParmBackupJsonDoc["autoApn"] = gSysParam.apnMode;
                    systemParmBackupJsonDoc["sim1APN"] = gSysParam.apn1;
                    systemParmBackupJsonDoc["serverIP"] = gSysParam.dataServerIP;
                    systemParmBackupJsonDoc["port"] = gSysParam.dataServerPort;
                    systemParmBackupJsonDoc["serverURL"] = gSysParam.dataServerUrl;
                    systemParmBackupJsonDoc["token"] = gSysParam.dataServerToken;
                    systemParmBackupJsonDoc["urate"] = gSysParam.uRate;
                    systemParmBackupJsonDoc["modaddr"] = gSysParam.sensor1ModAddress;
                    systemParmBackupJsonDoc["stinterval"] = gSysParam.logTime;
                    systemParmBackupJsonDoc["enable_mqtt"] = gSysParam.mqttEn;
                    systemParmBackupJsonDoc["broker"] = gSysParam.flServerHost;
                    systemParmBackupJsonDoc["port"] = gSysParam.flServerMqttPort;
                    systemParmBackupJsonDoc["user"] = gSysParam.flServerMqttUname;
                    systemParmBackupJsonDoc["pwd"] = gSysParam.flServerMqttPass;
                    systemParmBackupJsonDoc["cid"] = gSysParam.flServerMqttClientId;
                    systemParmBackupJsonDoc["qos"] = gSysParam.flServerMqttQos;
                    systemParmBackupJsonDoc["retain"] = gSysParam.flServerMqttRetain;
                    systemParmBackupJsonDoc["keep_alive"] = gSysParam.flServerMqttKeepAlive;
                    systemParmBackupJsonDoc["userSecureConnection"] = gSysParam.flServerMqttAuth;

                    JsonObject certificates = systemParmBackupJsonDoc.createNestedObject("certificates");
                    certificates["ca"] = "";
                    certificates["crt"] = "";
                    certificates["key"] = "";
                    systemParmBackupJsonDoc["pubtopic"] = gSysParam.flServerPubTopic;
                    systemParmBackupJsonDoc["attrpubtopic"] = "hostname/attribute";
                    systemParmBackupJsonDoc["subtopic"] = gSysParam.flServerSubTopic;
                    systemParmBackupJsonDoc["telnet"] = gSysParam.telnetEn;

                    serializeJsonPretty(systemParmBackupJsonDoc, outstream);

                    debugPrintln(outstream);
                    request->send(200, "application/json", outstream); });

    asyncWebServer.on(
        "/api/restore", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            IPAddress TempAddress;

            // char *hostname;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Data received: ");
            debugPrintln(body.c_str());

            DeserializationError error = deserializeJson(systemParmRestoreJsonDoc, body);
            if (error)
            {
                debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
                debugPrintln(error.c_str());
                request->send(400, "application/json", FALSE_RESPONSE);
                return;
            }
            else
            {

                if (systemParmRestoreJsonDoc.containsKey("hostname"))
                {
                    if (!systemParmRestoreJsonDoc["hostname"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["hostname"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.hostname, systemParmRestoreJsonDoc["hostname"]);
                        debugPrint("[WEBPAGE] Updating hostname as: ");
                        debugPrintln(gSysParam.hostname);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("lattitude"))
                {
                    if (!systemParmRestoreJsonDoc["lattitude"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["lattitude"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.defaultLat, systemParmRestoreJsonDoc["lattitude"]);
                        debugPrint("[WEBPAGE] Updating lattitude as: ");
                        debugPrintln(gSysParam.defaultLat);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("longitude"))
                {
                    if (!systemParmRestoreJsonDoc["longitude"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["longitude"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.defaultLong, systemParmRestoreJsonDoc["longitude"]);
                        debugPrint("[WEBPAGE] Updating longitude as: ");
                        debugPrintln(gSysParam.defaultLong);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("sitename"))
                {
                    if (!systemParmRestoreJsonDoc["sitename"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["sitename"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.sitename, systemParmRestoreJsonDoc["sitename"]);
                        debugPrint("[WEBPAGE] Updating sitename as: ");
                        debugPrintln(gSysParam.sitename);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("clientname"))
                {
                    if (!systemParmRestoreJsonDoc["clientname"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["clientname"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.clientname, systemParmRestoreJsonDoc["clientname"]);
                        debugPrint("[WEBPAGE] Updating clientname as: ");
                        debugPrintln(gSysParam.clientname);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("logstorage"))
                {
                    if (!systemParmRestoreJsonDoc["logstorage"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["logstorage"].as<String>().c_str(), URATE);
                        gSysParam.logSaveHttp = (int)systemParmRestoreJsonDoc["logstorage"];
                        debugPrint("[WEBPAGE] Updating logstorage as :");
                        debugPrintln(gSysParam.logSaveHttp);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("logsend"))
                {
                    if (!systemParmRestoreJsonDoc["logsend"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["logsend"].as<String>().c_str(), URATE);
                        gSysParam.logSendHttp = (int)systemParmRestoreJsonDoc["logsend"];
                        debugPrint("[WEBPAGE] Updating logsend as :");
                        debugPrintln(gSysParam.logSendHttp);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("conn"))
                {
                    if (!systemParmRestoreJsonDoc["conn"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["conn"].as<String>().c_str(), URATE);
                        gSysParam.ap4gOrWifiEn = (int)systemParmRestoreJsonDoc["conn"];
                        debugPrint("[WEBPAGE] Updating conn as :");
                        debugPrintln(gSysParam.ap4gOrWifiEn);
                        wifiApDoneFlag = false;
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("ssid"))
                {
                    if (!systemParmRestoreJsonDoc["ssid"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["ssid"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.staSsid, systemParmRestoreJsonDoc["ssid"]);
                        debugPrint("[WEBPAGE] Updating ssid as: ");
                        debugPrintln(gSysParam.staSsid);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("password"))
                {
                    if (!systemParmRestoreJsonDoc["password"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["password"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.staPass, systemParmRestoreJsonDoc["password"]);
                        debugPrint("[WEBPAGE] Updating password as: ");
                        debugPrintln(gSysParam.staPass);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("dhcp"))
                {
                    if (!systemParmRestoreJsonDoc["dhcp"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["dhcp"].as<String>().c_str(), URATE);
                        gSysParam.dhcp = (int)systemParmRestoreJsonDoc["dhcp"];
                        debugPrint("[WEBPAGE] Updating dhcp as :");
                        debugPrintln(gSysParam.dhcp);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("staticIP"))
                {
                    if (!systemParmRestoreJsonDoc["staticIP"].isNull())
                    {
                        if (!TempAddress.fromString(systemParmRestoreJsonDoc["staticIP"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["staticIP"], IPADDR);
                        strcpy(gSysParam.ipAddr, systemParmRestoreJsonDoc["staticIP"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating ipAddr as :");
                        debugPrintln(gSysParam.ipAddr);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("gatewayIP"))
                {
                    if (!systemParmRestoreJsonDoc["gatewayIP"].isNull())
                    {
                        if (!TempAddress.fromString(systemParmRestoreJsonDoc["gatewayIP"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["gatewayIP"], GATEWAY);
                        strcpy(gSysParam.gateway, systemParmRestoreJsonDoc["gatewayIP"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating gateway as :");
                        debugPrintln(gSysParam.gateway);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("subnetMask"))
                {
                    if (!systemParmRestoreJsonDoc["subnetMask"].isNull())
                    {
                        if (!TempAddress.fromString(systemParmRestoreJsonDoc["subnetMask"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["subnetMask"], SUBNET);
                        strcpy(gSysParam.subnet, systemParmRestoreJsonDoc["subnetMask"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating subnet as :");
                        debugPrintln(gSysParam.subnet);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("dns"))
                {
                    if (!systemParmRestoreJsonDoc["dns"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["dns"], DNS);
                        strcpy(gSysParam.dns, systemParmRestoreJsonDoc["dns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating dns as :");
                        debugPrintln(gSysParam.dns);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("pdns"))
                {
                    if (!systemParmRestoreJsonDoc["pdns"].isNull())
                    {
                        if (!TempAddress.fromString(systemParmRestoreJsonDoc["pdns"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["pdns"], PDNS);
                        strcpy(gSysParam.pDns, systemParmRestoreJsonDoc["pdns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating pdns as :");
                        debugPrintln(gSysParam.pDns);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("sdns"))
                {
                    if (!systemParmRestoreJsonDoc["sdns"].isNull())
                    {
                        if (!TempAddress.fromString(systemParmRestoreJsonDoc["sdns"].as<String>()))
                        {
                            request->send(500, "application/json", FALSE_RESPONSE);
                            return;
                        }
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["sdns"], SDNS);
                        strcpy(gSysParam.sDns, systemParmRestoreJsonDoc["sdns"].as<String>().c_str());
                        debugPrint("[WEBPAGE] Updating sdns as :");
                        debugPrintln(gSysParam.sDns);
                    }
                }

                if (systemParmRestoreJsonDoc.containsKey("autoApn"))
                {
                    if (!systemParmRestoreJsonDoc["autoApn"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["autoApn"].as<String>().c_str(), URATE);
                        gSysParam.apnMode = (int)systemParmRestoreJsonDoc["autoApn"];
                        debugPrint("[WEBPAGE] Updating autoApn as :");
                        debugPrintln(gSysParam.apnMode);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("sim1APN"))
                {
                    if (!systemParmRestoreJsonDoc["sim1APN"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["sim1APN"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.apn1, systemParmRestoreJsonDoc["sim1APN"]);
                        debugPrint("[WEBPAGE] Updating sim1APN as: ");
                        debugPrintln(gSysParam.apn1);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("serverIP"))
                {
                    if (!systemParmRestoreJsonDoc["serverIP"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["serverIP"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.dataServerIP, systemParmRestoreJsonDoc["serverIP"]);
                        debugPrint("[WEBPAGE] Updating serverIP as: ");
                        debugPrintln(gSysParam.dataServerIP);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("port"))
                {
                    if (systemParmRestoreJsonDoc["port"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["port"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.dataServerPort = (int)systemParmRestoreJsonDoc["port"];
                        systemPara.mqttPort = (int)systemParmRestoreJsonDoc["port"];
                        debugPrint("[WEBPAGE] Updating port as: ");
                        debugPrintln(gSysParam.dataServerPort);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("serverURL"))
                {
                    if (!systemParmRestoreJsonDoc["serverURL"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["serverURL"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.dataServerUrl, systemParmRestoreJsonDoc["serverURL"]);
                        debugPrint("[WEBPAGE] Updating serverURL as: ");
                        debugPrintln(gSysParam.dataServerUrl);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("token"))
                {
                    if (!systemParmRestoreJsonDoc["token"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["token"], DATA_SERV_TOKEN);
                        strcpy(gSysParam.dataServerToken, systemParmRestoreJsonDoc["token"]);
                        debugPrint("[WEBPAGE] Updating token as: ");
                        debugPrintln(gSysParam.dataServerToken);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("urate"))
                {
                    if (!systemParmRestoreJsonDoc["urate"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["urate"].as<String>().c_str(), URATE);
                        gSysParam.uRate = (int)systemParmRestoreJsonDoc["urate"];
                        debugPrint("[WEBPAGE] Updating urate as :");
                        debugPrintln(gSysParam.uRate);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("modaddr"))
                {
                    if (!systemParmRestoreJsonDoc["modaddr"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["modaddr"].as<String>(), MODBUSADDR);
                        gSysParam.sensor1ModAddress = (int)systemParmRestoreJsonDoc["modaddr"];
                        debugPrint("Updating modaddr as :");
                        debugPrintln(gSysParam.sensor1ModAddress);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("stinterval"))
                {
                    if (!systemParmRestoreJsonDoc["stinterval"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["stinterval"].as<String>().c_str(), STINTERVAL);
                        gSysParam.logTime = (int)systemParmRestoreJsonDoc["stinterval"];
                        debugPrint("[WEBPAGE] Updating log interval as :");
                        debugPrintln(gSysParam.logTime);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("broker"))
                {
                    if (!systemParmRestoreJsonDoc["broker"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["broker"].as<String>(), DATA_SERV_IP);
                        strcpy(gSysParam.flServerHost, systemParmRestoreJsonDoc["broker"]);
                        debugPrint("[WEBPAGE] Updating broker as: ");
                        debugPrintln(gSysParam.flServerHost);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("port"))
                {
                    if (systemParmRestoreJsonDoc["port"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["port"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttPort = (int)systemParmRestoreJsonDoc["port"];
                        debugPrint("[WEBPAGE] Updating port as: ");
                        debugPrintln(gSysParam.flServerMqttPort);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("user"))
                {
                    if (!systemParmRestoreJsonDoc["user"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["user"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttUname, systemParmRestoreJsonDoc["user"]);
                        debugPrint("[WEBPAGE] Updating user as: ");
                        debugPrintln(gSysParam.flServerMqttUname);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("pwd"))
                {
                    if (!systemParmRestoreJsonDoc["pwd"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["user"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttPass, systemParmRestoreJsonDoc["pwd"]);
                        debugPrint("[WEBPAGE] Updating pwd as: ");
                        debugPrintln(gSysParam.flServerMqttPass);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("cid"))
                {
                    if (!systemParmRestoreJsonDoc["cid"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["cid"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerMqttClientId, systemParmRestoreJsonDoc["cid"]);
                        debugPrint("[WEBPAGE] Updating cid as: ");
                        debugPrintln(gSysParam.flServerMqttClientId);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("qos"))
                {
                    if (systemParmRestoreJsonDoc["qos"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["qos"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttQos = (int)systemParmRestoreJsonDoc["qos"];
                        debugPrint("[WEBPAGE] Updating qos as: ");
                        debugPrintln(gSysParam.flServerMqttQos);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("retain"))
                {
                    if (systemParmRestoreJsonDoc["retain"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["retain"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttRetain = (int)systemParmRestoreJsonDoc["retain"];
                        debugPrint("[WEBPAGE] Updating retain as: ");
                        debugPrintln(gSysParam.flServerMqttRetain);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("keep_alive"))
                {
                    if (systemParmRestoreJsonDoc["keep_alive"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["keep_alive"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttKeepAlive = (int)systemParmRestoreJsonDoc["keep_alive"];
                        debugPrint("[WEBPAGE] Updating keep_alive as: ");
                        debugPrintln(gSysParam.flServerMqttKeepAlive);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("userSecureConnection"))
                {
                    if (systemParmRestoreJsonDoc["userSecureConnection"] > 0)
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["userSecureConnection"].as<String>().c_str(), DATA_SERV_PORT);
                        gSysParam.flServerMqttAuth = (int)systemParmRestoreJsonDoc["userSecureConnection"];
                        debugPrint("[WEBPAGE] Updating Auth as: ");
                        debugPrintln(gSysParam.flServerMqttAuth);
                    }
                }

                JsonObject certificates = systemParmRestoreJsonDoc["certificates"];
                const char *certificates_ca = certificates["ca"];   // nullptr
                const char *certificates_crt = certificates["crt"]; // nullptr
                const char *certificates_key = certificates["key"]; // nullptr

                if (systemParmRestoreJsonDoc.containsKey("pubtopic"))
                {
                    if (!systemParmRestoreJsonDoc["pubtopic"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["pubtopic"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerPubTopic, systemParmRestoreJsonDoc["pubtopic"]);
                        debugPrint("[WEBPAGE] Updating pubtopic as: ");
                        debugPrintln(gSysParam.flServerPubTopic);
                    }
                }
                const char *attrpubtopic = systemParmRestoreJsonDoc["attrpubtopic"]; // "hostname/attribute"
                if (systemParmRestoreJsonDoc.containsKey("subtopic"))
                {
                    if (!systemParmRestoreJsonDoc["subtopic"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["subtopic"].as<String>(), DATA_SERV_URL);
                        strcpy(gSysParam.flServerSubTopic, systemParmRestoreJsonDoc["subtopic"]);
                        debugPrint("[WEBPAGE] Updating subtopic as: ");
                        debugPrintln(gSysParam.flServerSubTopic);
                    }
                }
                if (systemParmRestoreJsonDoc.containsKey("telnet"))
                {
                    if (!systemParmRestoreJsonDoc["telnet"].isNull())
                    {
                        // spiffsFile.ChangeToken(systemParmRestoreJsonDoc["urate"].as<String>().c_str(), URATE);
                        gSysParam.telnetEn = (int)systemParmRestoreJsonDoc["telnet"];
                        debugPrint("[WEBPAGE] Updating telnet as :");
                        debugPrintln(gSysParam.telnetEn);
                    }
                }

                debugPrintln("[WEBPAGE] restore settings Succes!");
                debugPrintln();
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                if (false == AppSetConfigSysParams2(&gSysParam2))
                {
                }
                request->send(200, "application/json", TRUE_RESPONSE);
                delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
                ESP.restart();
            }
        });

    asyncWebServer.on(
        "/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Body received: ");
            debugPrintln(body.c_str());

            debugPrintln("[WEBPAGE] Reboot command received, Rebooting device...");
            request->send(200, "application/json", TRUE_RESPONSE);
            delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
            ESP.restart();
        });

    asyncWebServer.on(
        "/api/factroryReset", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            String body;
            for (size_t i = 0; i < len; i++)
            {
                body += (char)data[i];
            }
            debugPrint("Body received: ");
            debugPrintln(body.c_str());

            debugPrintln("[WEBPAGE] factory reset command received.");
            AppMakeDefaultConfigSystemParams();
            AppMakeDefaultSysParams2();

            request->send(200, "application/json", TRUE_RESPONSE);
            delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
            ESP.restart();
        });
    // OTA Update API:
    /*!
    @brief Function is used to perform Over The Air updates to the controller. The Controller
           Download the firmware from a remote asyncWebServer in packets and write to flash memeory.
           Atfer writing it to flash the controller resets, if the firware is downloaded correctly
           new firmware is loaded and if it has any error in downloading the old firmware is loaded
           back.
    @author Nivrutti Mahajan.
    */
    asyncWebServer.on(
        "/api/update", HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            // Replying with a positive or negative response on error scrnerio after the update process ends.
            debugPrintln("\n[WEBPAGE] Updating firmware done!\n");
            request->send((Update.hasError()) ? 500 : 200, "application/json", (Update.hasError()) ? FALSE_RESPONSE : TRUE_RESPONSE);
            asyncWebServer.end();
            delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
            ESP.restart();
        },
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            if (0 == index)
            {
                uploadStatus = OTA_UPLOAD_START;
                debugPrintln("\n[WEBPAGE] Update command received, Updating firmware...");
            }

            if (OTA_UPLOAD_START == uploadStatus)
            {
                if (filename == "firmware.bin")
                {
                    debugPrintln("[WEBPAGE] Found Firmware File!\n");
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
                    {
                        // start with max available size
                        Update.printError(Serial);
                        uploadStatus = OTA_UPLOAD_ABORTED;
                    }
                    else
                    {
                        uploadStatus = OTA_UPLOAD_WRITE;
                    }
                }
                else if (filename == "spiffs.bin")
                {
                    debugPrintln("[WEBPAGE] Found SPIFFS File!\n");
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
                    {
                        // start with max available size
                        Update.printError(Serial);
                        uploadStatus = OTA_UPLOAD_ABORTED;
                    }
                    else
                    {
                        uploadStatus = OTA_UPLOAD_WRITE;
                    }
                }
                else
                {
                    uploadStatus = OTA_UPLOAD_ABORTED;
                    debugPrintln("[WEBPAGE] Firmware update error!\n");
                }
            }

            if (OTA_UPLOAD_WRITE == uploadStatus)
            {
                if (Update.write(data, len) != len)
                {
                    Update.printError(Serial);
                    uploadStatus = OTA_UPLOAD_ABORTED;
                }
                else
                {
                    if (1 == final)
                    {
                        uploadStatus = OTA_UPLOAD_END;
                    }
                }
            }

            if (OTA_UPLOAD_END == uploadStatus)
            {
                if (Update.end(true))
                {
                    // true to set the size to the current progress
                    debugSmt(Serial.printf("[WEBPAGE] UploadEnd: %s, %u B\n", filename.c_str(), index + len););
                }
                else
                {
                    debugPrint("[WEBPAGE] Firmware update error: ");
                    Update.printError(Serial);
                    debugPrintln();
                    uploadStatus = OTA_UPLOAD_ABORTED;
                }
            }
        },
        NULL);
}

// void WebServerHandler()
// {
//     // Uncomment when we have web pages.
//     server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

//     // Authentication API:
//     server.on(
//         "/api/login", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             const size_t capacity = JSON_OBJECT_SIZE(2) + 50;
//             DynamicJsonDocument doc(capacity);

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 if (((doc["userid"] == uID) && (doc["password"] == uPass)) || ((doc["userid"] == mID) && (doc["password"] == mPass)))
//                 {
//                     request->send(200, "application/json", TRUE_RESPONSE);
//                 }
//                 else
//                 {
//                     request->send(401, "application/json", FALSE_RESPONSE);
//                 }
//             }
//         });

//     // Change Password API:
//     server.on(
//         "/api/changeP", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             const size_t capacity = JSON_OBJECT_SIZE(2) + 50;
//             DynamicJsonDocument doc(capacity);

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrintln(F("deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 if ((doc["oldP"] == uPass))
//                 {
//                     spiffsFile.ChangeToken(doc["newP"], UPASS);
//                     if (PASS != spiffsFile.writeFile(SPIFFS, CONFIG_FILE_PATH, configFileData))
//                     {
//                         flashErrorCount++;
//                         systemFlags.flashError = true;
//                     }
//                     else
//                     {
//                         systemFlags.flashError = false;
//                     }
//                     debugPrintln("[WEBPAGE] Password changed successfully");
//                     request->send(200, "application/json", TRUE_RESPONSE);
//                 }
//                 else
//                 {
//                     request->send(401, "application/json", FALSE_RESPONSE);
//                 }
//             }
//         });

//     // Mac Address API:
//     server.on("/api/getMAC", HTTP_GET, [](AsyncWebServerRequest *request)
//               {
//                   String outstream;
//                   const size_t capacity = JSON_OBJECT_SIZE(1);
//                   DynamicJsonDocument doc(capacity);

//                   doc["mac"] = deviceID.c_str();

//                   serializeJsonPretty(doc, outstream);
//                   //   debugPrintln(outstream);
//                   request->send(200, "application/json", outstream); });
//     server.on(
//         "/serial/cmd", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body, resp, outstream;
//             int status;
//             char ch;
//             static PARSE_ID_TYPE testType = CHECK_FOR_SEQ_ID;

//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             StaticJsonDocument<128> doc;
//             StaticJsonDocument<128> res;

//             DeserializationError error = deserializeJson(doc, body);

//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 const char *cmd = doc["cmd"]; // "SET DT 270721115200$0D"
//                 // debugPrint("[CMDLINE] Cmd Rxd: ");
//                 Serial.println(cmd);
//                 status = CmdLineProcess((char *)cmd, &respBuf, (void *)&testType, CheckForSerquenceNum);
//                 if (0 > status)
//                 {
//                     // resp = "[CMDLINE] Cmd Resp: ";
//                     while (0 == CbPop(&respBuf, &ch))
//                     {
//                         resp.concat(ch);
//                     }
//                     res["res"] = resp;
//                     res["sts"] = false;
//                     serializeJsonPretty(res, outstream);
//                     Serial.println(resp);
//                     // debugPrintln(outstream);
//                     request->send(200, "application/json", outstream);
//                 }
//                 else
//                 {
//                     // resp = "[CMDLINE] Cmd Resp: ";
//                     while (0 == CbPop(&respBuf, &ch))
//                     {
//                         resp.concat(ch);
//                     }
//                     res["res"] = resp;
//                     res["sts"] = true;
//                     serializeJsonPretty(res, outstream);
//                     Serial.println(resp);
//                     // debugPrintln(outstream);
//                     request->send(200, "application/json", outstream);
//                 }
//             }
//         });
//     // Interface Settings API:
//     server.on("/api/getinterface", HTTP_GET, [](AsyncWebServerRequest *request)
//               {
//                   String outstream;
//                   const size_t capacity = JSON_OBJECT_SIZE(1);
//                   DynamicJsonDocument doc(capacity);

//                   doc["SSID"] = stSsid.c_str();

//                   serializeJsonPretty(doc, outstream);
//                   //   debugPrintln(outstream);
//                   request->send(200, "application/json", outstream); });

//     server.on(
//         "/api/savinterface", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             const size_t capacity = JSON_OBJECT_SIZE(3) + 30;
//             DynamicJsonDocument doc(capacity);

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 if (!doc["ssid"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating ssid as :");
//                     debugPrintln(doc["ssid"].as<String>());
//                     spiffsFile.ChangeToken(doc["ssid"], STSSID);
//                 }

//                 if (!doc["password"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating password as :");
//                     debugPrintln(doc["password"].as<String>());
//                     spiffsFile.ChangeToken(doc["password"], STPASS);
//                 }
//                 debugPrintln("[WEBPAGE] WiFi credentials updated!");
//                 debugPrintln();
//                 wifiSettingsResetFlag = true;
//                 if (PASS != spiffsFile.writeFile(SPIFFS, CONFIG_FILE_PATH, configFileData))
//                 {
//                     flashErrorCount++;
//                     systemFlags.flashError = true;
//                 }
//                 else
//                 {
//                     systemFlags.flashError = false;
//                 }
//                 request->send(200, "application/json", TRUE_RESPONSE);
//                 return;
//             }
//         });
//     server.on("/api/getdevcon", HTTP_GET, [](AsyncWebServerRequest *request)
//               {
//                   String outstream;
//                   const size_t capacity = JSON_OBJECT_SIZE(2);
//                   DynamicJsonDocument doc(capacity);

//                   doc["UpdateRate"] = uRate;
//                   // doc["ModbusAddr"] = modbusAddr.c_str();

//                   serializeJsonPretty(doc, outstream);
//                   //   debugPrintln(outstream);
//                   request->send(200, "application/json", outstream); });

//     server.on(
//         "/api/savdevcon", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
//             DynamicJsonDocument doc(capacity);

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 if (!doc["updateRate"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating updateRate as :");
//                     debugPrintln(doc["updateRate"].as<String>());
//                     spiffsFile.ChangeToken(doc["updateRate"].as<String>().c_str(), URATE);
//                 }

//                 // if (!doc["modAddr"].isNull())
//                 // {
//                 //     debugPrint("Updating modAddr as :");
//                 //     debugPrintln(doc["modAddr"].as<String>());
//                 //     spiffsFile.ChangeToken(doc["modAddr"].as<String>(), MODBUSADDR);
//                 // }
//                 debugPrintln("[WEBPAGE] Device setting Success!");
//                 debugPrintln();
//                 if (PASS != spiffsFile.writeFile(SPIFFS, CONFIG_FILE_PATH, configFileData))
//                 {
//                     flashErrorCount++;
//                     systemFlags.flashError = true;
//                 }
//                 else
//                 {
//                     systemFlags.flashError = false;
//                 }
//                 request->send(200, "application/json", TRUE_RESPONSE);
//                 return;
//             }
//         });
//     // Network Setting API:
//     server.on("/api/getnetset", HTTP_GET, [](AsyncWebServerRequest *request)
//               {
//                   String outstream;
//                   const size_t capacity = JSON_OBJECT_SIZE(10);
//                   DynamicJsonDocument doc(capacity);

//                   if (wifiAPEnableFlag)
//                   {
//                       doc["DHCP"] = 0;
//                       doc["StaticIP"] = ipAddr.c_str();
//                       doc["GatewayIP"] = gateway.c_str();
//                       doc["SubnetMask"] = subnet.c_str();
//                       doc["PDNS"] = pDns.c_str();
//                       doc["SDNS"] = sDns.c_str();
//                       doc["DNS"] = dns.c_str();
//                   }
//                   else
//                   {
//                       if (1 == dhcp)
//                       {
//                           doc["DHCP"] = dhcp;
//                           doc["StaticIP"] = WiFi.localIP().toString();
//                           doc["GatewayIP"] = WiFi.gatewayIP().toString();
//                           doc["SubnetMask"] = WiFi.subnetMask().toString();
//                           doc["PDNS"] = pDns.c_str();
//                           doc["SDNS"] = sDns.c_str();
//                           doc["DNS"] = dns.c_str();
//                           // doc["port"] = tcpSerPort;
//                       }
//                       else
//                       {
//                           doc["DHCP"] = dhcp;
//                           doc["StaticIP"] = ipAddr.c_str();
//                           doc["GatewayIP"] = gateway.c_str();
//                           doc["SubnetMask"] = subnet.c_str();
//                           doc["PDNS"] = pDns.c_str();
//                           doc["SDNS"] = sDns.c_str();
//                           doc["DNS"] = dns.c_str();
//                           // doc["port"] = tcpSerPort;
//                       }
//                   }
//                   serializeJsonPretty(doc, outstream);

//                   //   debugPrintln(outstream);
//                   request->send(200, "application/json", outstream); });

//     server.on(
//         "/api/savnetset", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             IPAddress TempAddress;

//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             const size_t capacity = JSON_OBJECT_SIZE(8) + 210;
//             DynamicJsonDocument doc(capacity);

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {
//                 if (!doc["staticIP"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating ipAddr as :");
//                     debugPrintln(doc["staticIP"].as<String>());
//                     if (!TempAddress.fromString(doc["staticIP"].as<String>()))
//                     {
//                         request->send(500, "application/json", FALSE_RESPONSE);
//                         return;
//                     }
//                     spiffsFile.ChangeToken(doc["staticIP"], IPADDR);
//                 }
//                 if (!doc["gatewayIP"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating gateway as :");
//                     debugPrintln(doc["gatewayIP"].as<String>());
//                     if (!TempAddress.fromString(doc["gatewayIP"].as<String>()))
//                     {
//                         request->send(500, "application/json", FALSE_RESPONSE);
//                         return;
//                     }
//                     spiffsFile.ChangeToken(doc["gatewayIP"], GATEWAY);
//                 }
//                 if (!doc["subnetMask"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating subnet as :");
//                     debugPrintln(doc["subnetMask"].as<String>());
//                     if (!TempAddress.fromString(doc["subnetMask"].as<String>()))
//                     {
//                         request->send(500, "application/json", FALSE_RESPONSE);
//                         return;
//                     }
//                     spiffsFile.ChangeToken(doc["subnetMask"], SUBNET);
//                 }
//                 // if (!doc["port"].isNull())
//                 // {
//                 //     debugPrint("Updating port as :");
//                 //     debugPrintln(doc["port"].as<String>());

//                 //     if (!IsNum((CHAR *)doc["port"].as<String>().c_str()))
//                 //     {
//                 //         request->send(500, "application/json", FALSE_RESPONSE);
//                 //         return;
//                 //     }
//                 //     spiffsFile.ChangeToken(doc["port"].as<String>(), TCPCLIENTPORT);
//                 // }
//                 if (doc["dhcp"] >= 0)
//                 {
//                     debugPrint("[WEBPAGE] Updating dhcp as :");
//                     debugPrintln((int)doc["dhcp"]);
//                     spiffsFile.ChangeToken(doc["dhcp"].as<String>().c_str(), DHCP);
//                 }
//                 if (!doc["dns"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating dns as :");
//                     debugPrintln(doc["dns"].as<String>());

//                     spiffsFile.ChangeToken(doc["dns"], DNS);
//                 }
//                 if (!doc["PDNS"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating pdns as :");
//                     debugPrintln(doc["PDNS"].as<String>());
//                     if (!TempAddress.fromString(doc["PDNS"].as<String>()))
//                     {
//                         request->send(500, "application/json", FALSE_RESPONSE);
//                         return;
//                     }
//                     spiffsFile.ChangeToken(doc["PDNS"], PDNS);
//                 }
//                 if (!doc["SDNS"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating sdns as :");
//                     debugPrintln(doc["SDNS"].as<String>());
//                     if (!TempAddress.fromString(doc["SDNS"].as<String>()))
//                     {
//                         request->send(500, "application/json", FALSE_RESPONSE);
//                         return;
//                     }
//                     spiffsFile.ChangeToken(doc["SDNS"], SDNS);
//                 }
//                 debugPrintln("[WEBPAGE] Network settings updated!");
//                 debugPrintln();
//                 if (PASS != spiffsFile.writeFile(SPIFFS, CONFIG_FILE_PATH, configFileData))
//                 {
//                     flashErrorCount++;
//                     systemFlags.flashError = true;
//                 }
//                 else
//                 {
//                     systemFlags.flashError = false;
//                 }
//                 request->send(200, "application/json", TRUE_RESPONSE);
//                 delay(1000); // added to wait for to sent response to webpage and the restart controller (Nivrutti Mahajan)
//                 ESP.restart();
//             }
//         });

//     // Cloud Settings API:
//     server.on("/api/getcldset", HTTP_GET, [](AsyncWebServerRequest *request)
//               {
//                   String outstream;
//                   StaticJsonDocument<64> doc;

//                   doc["host"] = serv1MqttHostname.c_str();
//                   doc["port"] = serv1MqttPort;
//                   // doc["clientID"] = mqttClientId;
//                   serializeJsonPretty(doc, outstream);

//                   //   debugPrintln(outstream);
//                   request->send(200, "application/json", outstream); });

//     server.on(
//         "/api/savcldset", HTTP_POST, [](AsyncWebServerRequest *request) {},
//         NULL,
//         [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
//         {
//             String body;
//             // char *hostname;
//             for (size_t i = 0; i < len; i++)
//             {
//                 body += (char)data[i];
//             }

//             StaticJsonDocument<128> doc;

//             DeserializationError error = deserializeJson(doc, body);
//             if (error)
//             {
//                 debugPrint(F("[WEBPAGE] deserializeJson() failed: "));
//                 debugPrintln(error.c_str());
//                 request->send(400, "application/json", FALSE_RESPONSE);
//                 return;
//             }
//             else
//             {

//                 if (!doc["host"].isNull())
//                 {
//                     // strcpy(hostname, doc["host"]);
//                     // hostname = (char *)doc["host"];
//                     debugPrint("[WEBPAGE] Updating host as :");
//                     debugPrintln(doc["host"].as<String>());
//                     spiffsFile.ChangeToken(doc["host"].as<String>(), SERV1MQTTHOSTNAME);
//                     delay(100);
//                 }

//                 if (doc["port"] > 0)
//                 {
//                     debugPrint("[WEBPAGE] Updating port as :");
//                     debugPrintln((int)doc["port"]);
//                     spiffsFile.ChangeToken(doc["port"].as<String>().c_str(), SERV1MQTTPORT);
//                     systemPara.mqttPort = (int)doc["port"];
//                 }

//                 // if (!doc["uname"].isNull())
//                 // {
//                 //     debugPrint("Updating uname as :");
//                 //     debugPrintln(doc["uname"].as<String>());

//                 //     //TODO: VALIDATE
//                 //     if (('-' != *(doc["uname"].as<String>().c_str() + 8)) ||
//                 //         ('-' != *(doc["uname"].as<String>().c_str() + 13)) ||
//                 //         ('-' != *(doc["uname"].as<String>().c_str() + 18)) ||
//                 //         ('-' != *(doc["uname"].as<String>().c_str() + 23)))
//                 //     {
//                 //         request->send(500, "application/json", FALSE_RESPONSE);
//                 //         return;
//                 //     }

//                 //     // spiffsFile.ChangeToken(doc["uname"].as<String>(),MQTTUNAME);
//                 // }

//                 // if (!doc["upass"].isNull())
//                 // {
//                 //     debugPrint("Updating upass as :");
//                 //     debugPrintln(doc["upass"].as<String>());

//                 //     if (('-' != *(doc["upass"].as<String>().c_str() + 8)) ||
//                 //         ('-' != *(doc["upass"].as<String>().c_str() + 13)) ||
//                 //         ('-' != *(doc["upass"].as<String>().c_str() + 18)) ||
//                 //         ('-' != *(doc["upass"].as<String>().c_str() + 23)))
//                 //     {
//                 //         request->send(500, "application/json", FALSE_RESPONSE);
//                 //         return;
//                 //     }

//                 //     // spiffsFile.ChangeToken(doc["upass"].as<String>(),MQTTUPASS);
//                 // }

//                 if (!doc["token"].isNull())
//                 {
//                     debugPrint("[WEBPAGE] Updating token as :");
//                     debugPrintln(doc["token"].as<String>());

//                     // if (('-' != *(doc["token"].as<String>().c_str() + 8)) ||
//                     //     ('-' != *(doc["token"].as<String>().c_str() + 13)) ||
//                     //     ('-' != *(doc["token"].as<String>().c_str() + 18)) ||
//                     //     ('-' != *(doc["token"].as<String>().c_str() + 23)))
//                     // {
//                     //     request->send(500, "application/json", FALSE_RESPONSE);
//                     //     return;
//                     // }

//                     spiffsFile.ChangeToken(doc["token"], SERV1MQTTCLIENTID);
//                 }
//                 debugPrintln("[WEBPAGE] Cloud settings Succes!");
//                 debugPrintln();
//                 if (PASS != spiffsFile.writeFile(SPIFFS, CONFIG_FILE_PATH, configFileData))
//                 {
//                     flashErrorCount++;
//                     systemFlags.flashError = true;
//                 }
//                 else
//                 {
//                     systemFlags.flashError = false;
//                 }
//                 request->send(200, "application/json", TRUE_RESPONSE);
//                 return;
//             }
//         });

//     // OTA Update API:
//     /*!
//     @brief Function is used to perform Over The Air updates to the controller. The Controller
//            Download the firmware from a remote server in packets and write to flash memeory.
//            Atfer writing it to flash the controller resets, if the firware is downloaded correctly
//            new firmware is loaded and if it has any error in downloading the old firmware is loaded
//            back.
//     @author Nivrutti Mahajan.
//     */
//     server.on(
//         "/api/update", HTTP_POST,
//         [](AsyncWebServerRequest *request)
//         {
//             // Replying with a positive or negative response on error scrnerio after the update process ends.
//             request->send((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
//             server.end();
//             ESP.restart();
//         },
//         [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
//         {
//             if (0 == index)
//             {
//                 uploadStatus = OTA_UPLOAD_START;
//             }

//             if (OTA_UPLOAD_START == uploadStatus)
//             {
//                 if (filename == "firmware.bin")
//                 {
//                     debugPrintln("[WEBPAGE] Found Firmware File!");
//                     if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
//                     {
//                         // start with max available size
//                         Update.printError(Serial);
//                         uploadStatus = OTA_UPLOAD_ABORTED;
//                     }
//                     else
//                     {
//                         uploadStatus = OTA_UPLOAD_WRITE;
//                     }
//                 }
//                 else if (filename == "spiffs.bin")
//                 {
//                     debugPrintln("[WEBPAGE] Found SPIFFS File!");
//                     if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
//                     {
//                         // start with max available size
//                         Update.printError(Serial);
//                         uploadStatus = OTA_UPLOAD_ABORTED;
//                     }
//                     else
//                     {
//                         uploadStatus = OTA_UPLOAD_WRITE;
//                     }
//                 }
//                 else
//                 {
//                     uploadStatus = OTA_UPLOAD_ABORTED;
//                     debugPrintln("ERROR");
//                 }
//             }

//             if (OTA_UPLOAD_WRITE == uploadStatus)
//             {
//                 if (Update.write(data, len) != len)
//                 {
//                     Update.printError(Serial);
//                     uploadStatus = OTA_UPLOAD_ABORTED;
//                 }
//                 else
//                 {
//                     if (1 == final)
//                     {
//                         uploadStatus = OTA_UPLOAD_END;
//                     }
//                 }
//             }

//             if (OTA_UPLOAD_END == uploadStatus)
//             {
//                 if (Update.end(true))
//                 {
//                     // true to set the size to the current progress
//                     debugSmt(Serial.printf("[WEBPAGE] UploadEnd: %s, %u B\n", filename.c_str(), index + len););
//                 }
//                 else
//                 {
//                     Update.printError(Serial);
//                     uploadStatus = OTA_UPLOAD_ABORTED;
//                 }
//             }
//         },
//         NULL);
// }

static uint8_t convert_char(char x)
{
    uint8_t a = x; // Yes I read only upper case,
    if (a >= 'A')  // change 0x41 through to 0x45  to 0x3A to 0x3F
        a -= 7;
    return a - '0'; // get rid of the 0x30 part, leaving 0x00 through to 0x0F
}

int32_t TwosCompliment(char *buffer, uint8_t sizeOfvalue)
{
    int32_t value = 0;

    for (uint8_t i = 0; i < sizeOfvalue; i++)
    {
        uint32_t nibble = convert_char(buffer[i]);
        value = (value << 4) + nibble;
    }
    return value;
}

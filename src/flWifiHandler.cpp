#include "flWifiHandler.h"
#include "FLToolbox.h"
#include "Arduino.h"
#include "WiFi.h"
#include "ESPmDNS.h"
#include "user.h"
#include "global-vars.h"
#include "app.h"

// Set your Static IP address
IPAddress staticIpForAPMode(192, 168, 4, 1);

// bool wifiAPEnableFlag = false; // true = AP mode(hotspot mode), false = STA mode
bool wifiApDoneFlag = false;

flWifiHandler::flWifiHandler()
{
}

flWifiHandler::~flWifiHandler()
{
}

void flWifiHandler::WifiInit(const char *ssid, const char *pass)
{
    this->_ssid = ssid;
    this->_password = pass;
    this->_dhcp = false;
}

void flWifiHandler::WifiInit(const char *ssid, const char *pass, const char *apssid, const char *appass)
{
    this->_ssid = ssid;
    this->_password = pass;
    this->_apSsid = apssid;
    this->_apPass = appass;
}

void flWifiHandler::WifiInit(const char *ssid, const char *pass, bool dhcp,
                             IPAddress Ip, IPAddress gateway, IPAddress subnet,
                             IPAddress pDns, IPAddress sDns)
{
    this->_ssid = ssid;
    this->_password = pass;
    this->_dhcp = dhcp;
    this->_staticIp = Ip;
    this->_gatewayIp = gateway;
    this->_subnet = subnet;
    this->_pDns = pDns;
    this->_sDns = sDns;
}

void flWifiHandler::WiFiSelectMode(bool dhcp)
{
    this->_dhcp = dhcp;
}

void flWifiHandler::WiFiMainLoop()
{
    static uint8_t retryDisconnect = 0;
    String IP;
    uint32_t dummyLen;

    switch (this->_wifiStates)
    {
    case WIFI_STATE_IDLE:
        if (gSysParam.ap4gOrWifiEn) // switch to STA mode
        {
            retryDisconnect = 0;
            /*This was giving bind error [[E][AsyncTCP.cpp:1148] begin(): bind error: -8]
            commented by Nivrutti Mahajan */
            WebServerStop();
            serverConFlg = false;
            dnsStsFlg = false; // dns closed
            MDNS.end();
            addServiceFlag = true;
            TelnetSerialEnd();
            // if (true == wifiSettingsResetFlag)
            // {
            //     // delay(50);
            //     debugPrintln("[WiFi Log] WiFI settings changed, restarting WiFi....");
            if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
            {
                flashErrorCount++;
                systemFlags.flashError = true;
            }
            else
            {
                systemFlags.flashError = false;
            }
            local_IP.fromString(gSysParam.ipAddr);
            gateway_IP.fromString(gSysParam.gateway);
            subnet_Mask.fromString(gSysParam.subnet);
            p_dns.fromString(gSysParam.pDns);
            s_dns.fromString(gSysParam.sDns);
            // WiFi.disconnect();
            // FlWifi.WifiInit(gSysParam.staSsid, gSysParam.staPass, gSysParam.dhcp,
            //                 local_IP, gateway_IP, subnet_Mask, p_dns, s_dns);
            //     wifiSettingsResetFlag = false;
            // }
            FlWifi.WifiInit(gSysParam.staSsid, gSysParam.staPass, gSysParam.dhcp,
                            local_IP, gateway_IP, subnet_Mask, p_dns, s_dns);
            WiFi.mode(WIFI_OFF);
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_NEW_CONNECTION;
            break;
        }
        else if ((false == gSysParam.ap4gOrWifiEn) && (false == wifiApDoneFlag)) // check AP mode enabled already or not?
        {
            WebServerStop();
            serverConFlg = false;
            dnsStsFlg = false; // dns closed
            MDNS.end();
            addServiceFlag = true;
            // if (true == wifiSettingsResetFlag)
            // {
            //     // delay(50);
            //     debugPrintln("[WiFi Log] WiFI settings changed, restarting WiFi....");
            if (false == AppGetConfigSysParams(&gSysParam, &dummyLen))
            {
                flashErrorCount++;
                systemFlags.flashError = true;
            }
            else
            {
                systemFlags.flashError = false;
            }
            local_IP.fromString(gSysParam.ipAddr);
            gateway_IP.fromString(gSysParam.gateway);
            subnet_Mask.fromString(gSysParam.subnet);
            p_dns.fromString(gSysParam.pDns);
            s_dns.fromString(gSysParam.sDns);
            WiFi.disconnect();
            // FlWifi.WifiInit(gSysParam.staSsid, gSysParam.staPass, gSysParam.dhcp,
            //                 local_IP, gateway_IP, subnet_Mask, p_dns, s_dns);
            //     wifiSettingsResetFlag = false;
            // }
            FlWifi.WifiInit(gSysParam.staSsid, gSysParam.staPass, gSysParam.dhcp,
                            local_IP, gateway_IP, subnet_Mask, p_dns, s_dns);
            TelnetSerialEnd();
            this->_wifiStates = WIFI_STATE_SWITCH_TO_AP_MODE;
            break;
        }
        else
        {
            // wait here
        }
        break;

    case WIFI_STATE_NEW_CONNECTION:

        if (!this->_dhcp)
        {
            debugPrintln("[WiFi Log] Changing to Static IP Mode.");
            if (!WiFi.config(this->_staticIp, this->_gatewayIp, this->_subnet))
            {
                debugPrintln("[WiFi Log] WiFi STA Mode Failed to configure");
            }
        }
        WiFi.disconnect();
        debugPrint("[WiFi Log] Connecting to WiFi ");
        debugPrint(this->_ssid);
        debugPrintln("...");
        WiFi.hostname("fl-iot");
        WiFi.mode(WIFI_STA);
        WiFi.begin(this->_ssid, this->_password);
        this->_wifiCheckMillis = millis();
        retryDisconnect = 0;
        this->_wifiPrevState = this->_wifiStates;
        this->_wifiStates = WIFI_STATE_WAIT_FOR_SOMETIME;
        break;

    case WIFI_STATE_RETRY_CONNECTION:
        /**
         * The Below Are the Unhandled Cases:
         * WL_IDLE_STATUS
         * WL_SCAN_COMPLETED
         * WL_CONNECTION_LOST
         *
         * Not encountered yet. will Fix when issue comes.
         * */

        if (WL_NO_SHIELD == WiFi.status())
        {
            debugPrintln("[WiFi Log] No Shield");
            WiFi.disconnect(true);
            this->_myWiFiFirstConnect = true;
            // WiFi.begin wasn't called yet Trying a new connection.
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_NEW_CONNECTION;
            break;
        }
        else if (WL_CONNECT_FAILED == WiFi.status())
        {
            debugPrintln("[WiFi Log] No Connection");
            debugPrintln("[WiFi Log] Disconnecting WiFi");
            WiFi.disconnect(true);
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_NEW_CONNECTION;
            break;
        }
        else if ((WL_DISCONNECTED == WiFi.status()) || (WL_NO_SSID_AVAIL == WiFi.status()))
        {
            if (retryDisconnect < 4)
            {
                // debugPrintln("Trying to reconnect to WiFi");
                if (!this->_myWiFiFirstConnect)
                {
                    // Report only once
                    this->_myWiFiFirstConnect = true;
                    debugPrintln("[WiFi Log] WiFi disconnected");
                }
                // debugPrintln("prevState: " + (String)this->_wifiPrevState);
                retryDisconnect++;
                this->_wifiCheckMillis = millis();
                this->_wifiPrevState = this->_wifiStates;
                this->_wifiStates = WIFI_STATE_WAIT_FOR_SOMETIME;
                break;
            }
            else
            {
                // debugPrintln("Trying New Connection");
                this->_wifiPrevState = this->_wifiStates;
                this->_wifiStates = WIFI_STATE_IDLE;
                break;
            }
        }
        else
        {
            // debugPrint("Current WiFi State: ");
            // debugPrintln(WiFi.status());
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_IDLE;
        }
        break;

    case WIFI_STATE_MONITOR_CONNECTION:

        if (WL_CONNECTED != WiFi.status())
        {
            // debugPrintln("prevState: " + (String)this->_wifiPrevState);
            this->_wifiConnected = false;
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_RETRY_CONNECTION;
            break;
        }
        else if (false == gSysParam.ap4gOrWifiEn)
        {
            this->_myWiFiFirstConnect = true;
            this->_wifiStates = WIFI_STATE_IDLE;
        }
        else
        {
            this->_wifiConnected = true;
            if (this->_myWiFiFirstConnect)
            {
                debugPrint("[WiFi Log] WiFi Connected To:");
                debugPrintln(this->_ssid);
                debugPrint("[WiFi Log] IP address: ");
                debugPrintln(WiFi.localIP());
                debugPrint("[WiFi Log] RSSI: ");
                debugPrintln(WiFi.RSSI());
                strcpy(gSysParam.ipAddr, WiFi.localIP().toString().c_str());
                strcpy(gSysParam.gateway, WiFi.gatewayIP().toString().c_str());
                strcpy(gSysParam.subnet, WiFi.subnetMask().toString().c_str());
                if (false == AppSetConfigSysParams(&gSysParam))
                {
                }
                dnsInit(gSysParam.dns);
                TelnetSerialInit();
                this->_myWiFiFirstConnect = false;
            }
        }
        break;

    case WIFI_STATE_WAIT_FOR_SOMETIME:

        if (millis() - this->_wifiCheckMillis > 1000)
        {
            this->_wifiPrevState = this->_wifiStates;
            this->_wifiStates = WIFI_STATE_MONITOR_CONNECTION;
        }
        break;

    case WIFI_STATE_SWITCH_TO_AP_MODE:
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_AP);
        debugPrintln("[WiFi Log] Entered AP Mode");
        debugPrint("[WiFi Log] AP SSID: ");
        debugPrintln(apSSID.c_str());
        WiFi.softAP(apSSID.c_str(), apPASS.c_str()); // AP Mode connection
        WiFi.softAPConfig(staticIpForAPMode, staticIpForAPMode, IPAddress(255, 255, 255, 0));
        debugPrint("[WiFi Log] IP address: ");
        debugPrintln(WiFi.softAPIP());
        // strcpy(gSysParam.ipAddr, WiFi.softAPIP().toString().c_str());
        /*when the ESP32 is operating as an Access Point (AP) without a connected station,
        the WiFi.gatewayIP() function might return 0.0.0.0. This is because, in AP mode,
        there is no actual gateway IP address, as the ESP32 is not routing traffic to another network*/
        // strcpy(gSysParam.gateway, WiFi.gatewayIP().toString().c_str());
        // strcpy(gSysParam.subnet, WiFi.softAPSubnetMask().toString().c_str());
        if (false == AppSetConfigSysParams(&gSysParam))
        {
        }
        dnsInit(gSysParam.dns); // dns rmustar.local:3000
        TelnetSerialInit();
        wifiApDoneFlag = true;
        this->_wifiStates = WIFI_STATE_IDLE;
        break;
    default:
        this->_wifiPrevState = this->_wifiStates;
        this->_wifiStates = WIFI_STATE_IDLE;
        break;
    }
}

bool flWifiHandler::isConnected()
{
    return this->_wifiConnected;
}

flWifiHandler FlWifi;
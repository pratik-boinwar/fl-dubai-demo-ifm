#ifndef FL_WIFI_HANDLER
#define FL_WIFI_HANDLER
#include "IPAddress.h"

// extern bool wifiAPEnableFlag;
extern bool wifiApDoneFlag;

class flWifiHandler
{
public:
    explicit flWifiHandler();
    ~flWifiHandler();

    /**
     * @brief Function Used to pass SSID and PASS of wifi Access Point connection.
     * @param ssid 
     * @param pass
     * @return void.
     * */
    void WifiInit(const char *ssid, const char *pass);
    void WifiInit(const char *ssid, const char *pass, const char *apssid, const char *appass);
    void WifiInit(const char *ssid, const char *pass, bool dhcp,
                  IPAddress Ip, IPAddress gateway, IPAddress subnet,
                  IPAddress pDns, IPAddress sDns);

    void WiFiSelectMode(bool dhcp);
    // void WifiInit(const char *ssid, const char *pass, const char *apssid, const char *appass);
    void WiFiMainLoop();

    /**
     * @brief Function returns the status of wifi connection.
     * @return True if connected.
     * */
    bool isConnected();

protected:
    typedef enum WIFI_STATES
    {
        WIFI_STATE_IDLE,
        WIFI_STATE_NEW_CONNECTION,
        WIFI_STATE_RETRY_CONNECTION,
        WIFI_STATE_MONITOR_CONNECTION,
        WIFI_STATE_WAIT_FOR_SOMETIME,
        WIFI_STATE_SWITCH_TO_AP_MODE,
    } eWIFI_STATES;

    eWIFI_STATES _wifiStates = WIFI_STATE_IDLE, _wifiPrevState = WIFI_STATE_IDLE;

    bool _wifiConnected = false, _myWiFiFirstConnect = true;
    unsigned long _wifiCheckMillis = 0;
    const char *_ssid;
    const char *_password;
    const char *_apSsid;
    const char *_apPass;
    bool _dhcp;
    IPAddress _staticIp, _gatewayIp, _subnet, _pDns, _sDns;
};

extern flWifiHandler FlWifi;
#endif
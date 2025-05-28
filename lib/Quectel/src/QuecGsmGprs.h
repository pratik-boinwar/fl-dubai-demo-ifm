/**
 * @file       QuecGsmGprs.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright © 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QuecGsmGprs_H
#define QuecGsmGprs_H

/**
 * CellInfo_t, gsm serving cell information in integer format
 */
typedef struct
{
    uint16_t mcc;    /* serving cell mcc */
    uint16_t mnc;    /* serving cell mnc */
    uint32_t lac;    /* serving cell Location area code */
    uint32_t cellId; /* serving cell Cell ID */
    // int16_t rssi;
    // uint16_t timeAd;
} CellInfo_t;

/**
 * GsmDateTime_t, gsm network date time
 */
typedef struct
{
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t timezone; // one digit expresses a quarter of an hour, for example: 22 indicates "+5:30"
} GsmDateTime_t;

/**
 * SimSelect_t, sim selection.
 */
typedef enum _SimSelect
{
    SIM_SLOT_AUTO = 0, /* choose any availaible SIM slot */
    SIM_SLOT_1,        /* always SIM 1 */
    SIM_SLOT_2         /* always SIM 2*/
} SimSelect_t;

/**
 * SimApnSelect_t, APN selection.
 */
typedef enum _SimApnSelect
{
    SIM_AUTO_APN = 0, /* choose auto APN */
    SIM_1_APN,        /* apn for SIM 1 */
    SIM_2_APN         /* apn for SIM 2*/
} SimApnSelect_t;

/**
 * GsmGprsEvents_t, various callback events of gsm/gprs initialization process.
 */
typedef enum _GsmGprsEvents
{
    QUEC_EVENT_POWER_ON = 0,
    QUEC_EVENT_IMEI,
    QUEC_EVENT_SIM_SELECTED,
    QUEC_EVENT_GSM_NW_REGISTERATION,
    QUEC_EVENT_GPRS_NW_REGISTERATION,
    QUEC_EVENT_IP_ADDRESS,
    QUEC_EVENT_NW_TIME,
    QUEC_EVENT_NW_QUALITY,
    QUEC_EVENT_SIM_ID,
    QUEC_EVENT_CELL_INFO,
    QUEC_EVENT_CELL_LOCATION,
    QUEC_EVENT_NW_OPERATOR,
} GsmGprsEvents_t;

/**
 * function pointer to call back function to notify various process events.
 */
typedef void (*pfnGsmGprsEventCb)(GsmGprsEvents_t event, void *data, uint8_t dataLen);

/**
 * @fn bool QuectelSubscribeEvents(pfnGsmGprsEventCb cb)
 * @brief This function allows to subscribe various gsm/gprs events.
 * @return false: fail. subscription full , true: success. subscription accepted
 * @remark useful when user need to know various operation stages, events etc.
 */
bool QuectelSubscribeEvents(pfnGsmGprsEventCb cb);

/**
 * @fn bool ResetModem(void);
 * @brief This function restarting modem forcefully and starts re-initiate procedure.
 * @return false: fail, true: success
 * @remark useful when there is no exptected response since long time.
 */
bool ResetModem(void);

/**
 * @fn bool IsGsmNwConnected(void)
 * @brief This function returns the status of gsm network registration.
 * @return false: gsm nw not registered, true: gsm nw registered
 * @remark check this function always before calling any SMS, Calling related operations.
 */
bool IsGsmNwConnected(void);

/**
 * @fn bool IsGprsConnected(void)
 * @brief This function returns the status of gprs connection.
 * @return false: gprs not connected, true: gprs connected
 * @remark check this function always before calling any internet related operations.
 */
bool IsGprsConnected(void);

/**
 * @fn bool GetSignalQuality(uint8_t *num)
 * @brief This function gives the gsm signal quality number.
 * @param uint8_t *num, interger pointer to return signal quality data.
 * @return false: signal quality not found, true: signal quality found
 * @remark signal quality measured after gsm network registered.
 * @example
 * uint8_t quality = 99;
 * bool status = GetSignalQuality(&quality);
 */
bool GetSignalQuality(uint8_t *num);

/**
 * @fn bool GetIMEI(char *imei)
 * @brief This function returns the IMEI number of modem.
 * @param char *imei, pointer to buffer to hold IMEI number.
 * @return false: imei not found. means modem not responding, true: imei found
 * @remark imei buffer length must be greater than 15.
 * @example
 * char imei[16];
 * bool status = GetIMEI(imei);
 */
bool GetIMEI(char *imei);

/**
 * @fn bool GetSimICCD(char *iccd)
 * @brief This function returns the ICCD number of SIM Card in operation.
 * @param char *iccd, pointer to buffer to hold ICCD number.
 * @return false: iccd not found. means SIM not detected, true: iccd found
 * @remark iccd buffer length must be greater than 21.
 * @example
 * char iccd[22];
 * bool status = GetSimICCD(iccd);
 */
bool GetSimICCD(char *iccd);

/**
 * @fn bool GetCellLocation(CellInfo_t *cellInfo)
 * @brief This function returns the serving cell information.
 * @param CellInfo_t *cellInfo, pointer to structure to copy cell info data.
 * @return false: cell info not found, true: cell info found
 * @remark info will available after gsm network registration.
 * @example
 * CellInfo_t cellInfo;
 * bool status = GetCellLocation(&cellInfo);
 */
bool GetCellLocation(CellInfo_t *cellInfo);

/**
 * @fn bool GetGsmTime(GsmDateTime_t *datetime)
 * @brief This function returns the ICCD number of SIM Card in operation.
 * @param GsmDateTime_t *datetime, pointer to structure to copy date-time data.
 * @return false: date-time not available, true: date-time available
 * @remark info will available after gsm network registration.
 * @example
 * GsmDateTime_t datetime;
 * bool status = GetGsmTime(&datetime);
 */
bool GetGsmTime(GsmDateTime_t *datetime);

/**
 * @fn bool SetAPN(const char *apn, const char *username, const char *password, SimApnSelect_t sim)
 * @brief This function receives the bearer APN information.
 * @param const char *apn, pointer to buffer holding beare APN name.
 * @param const char *username, pointer to buffer holding APN username.
 * @param const char *password, pointer to buffer holding APN password.
 * @param  SimApnSelect_t sim, sim number to be used for this set of apn details or Auto APN select.
 * @return false: fail, true: success
 * @remark user need to hold the apn, username and password in buffer throughtout the program.
 *         In Auto APN mode, predefined APN will be selected according to operators name for the current using SIM.
 * @example no username and paasword for APN and information for SIM_1.
 * const char apn_name[]="www";
 * bool status = SetAPN(apn_name, NULL, NULL, SIM_1_APN);
 *
 * select Auto APN
 * bool status = SetAPN(NULL, NULL, NULL, SIM_AUTO_APN);
 */
bool SetAPN(const char *apn, const char *username, const char *password, SimApnSelect_t sim);

/**
 * @fn bool GetAPN(char *name, uint8_t len)
 * @brief This function returns the currently using apn name.
 * @param char *name, pointer to buffer to copy apn name.
 * @param uint8_t len, name buffer size.
 * @return false: error, true: success
 * @remark useful when Auto APN mode is selected and if user wants to know what APN is using.
 * @example
 * char apnName[18];
 * bool status = GetAPN(apnName, sizeof(apnName));
 */
bool GetApn(char *name, uint8_t len);

/**
 * @fn bool GetOperatorName(char *name, uint8_t len)
 * @brief This function returns the operator name.
 * @param char *name, pointer to buffer to copy operator name.
 * @param uint8_t len, name buffer size.
 * @return false: operator not found. true: operator name found
 * @remark
 * @example
 * char operatorName[17];
 * bool status = GetSimICCD(operatorName, sizeof(operatorName));
 */
bool GetOperatorName(char *name, uint8_t len);

/**
 * @fn bool SelectSIM(SimSelect_t simSlot)
 * @brief This function allows user to select SIM slot.
 * @param SimSelect_t simSlot, sim slot number.
 * @return false: no dual SIM support. true: SIM slot selected
 * @remark
 * SIM_SLOT_AUTO: SIM 1 & 2 are alternatiely gets selected when gprs signal dropped.
 * SIM_SLOT_1 : Only SIM 1 will be selected all the time.
 * SIM_SLOT_2 : Only SIM 2 will be selected all the time.
 * Note: operations are carried out on user selected SIM,
 *       if GSM/GPRS fail to initialize then also device will not switch to another SIM.
 *       User have to take control of SIM selection.
 * @example
 * bool status = SelectSIM(SIM_SLOT_AUTO);
 */
bool SelectSIM(SimSelect_t simSlot = SIM_SLOT_AUTO);

/**
 * @fn SimSelect_t GetSelectedSIM()
 * @brief This function returns currently selected sim slot.
 * @return SimSelect_t: identification sim slot
 * @remark returns SIM_SLOT_1 always when there is no dual sim support.
 * @example
 * SimSelect_t simSlot = GetSelectedSIM();
 */
SimSelect_t GetSelectedSIM();

/**
 * @fn bool CheckSIM_Presence(SimSelect_t simSlot)
 * @brief This function returns status of SIM Card detection/presence.
 *        When simSlot is SIM_SLOT_AUTO then both sim card detection will be checked.
 * @param SimSelect_t: identification sim slot
 * @remark returns, false: not present/detected, true: present/detected
 * @example
 * bool simDetect = CheckSIM_Presence(SIM_SLOT_1);
 */
bool CheckSIM_Presence(SimSelect_t simSlot);

/**
 * @fn bool CheckSIM_PresenceByPrevStatusAtRestart(SimSelect_t simSlot)
 * @brief This function returns status of SIM Card detection/presence.
 *        When simSlot is SIM_SLOT_AUTO then both sim card detection will be checked.
 * @param SimSelect_t: identification sim slot
 * @remark returns, false: not present/detected, true: present/detected
 * @example
 * bool simDetect = CheckSIM_PresenceByPrevStatusAtRestart(SIM_SLOT_1);
 */
bool CheckSIM_PresenceByPrevStatusAtRestart(SimSelect_t simSlot);

/**
 * @fn bool GetGPRSStatus(void)
 * @brief This function returns status of GPRS connection.
 * @remark returns, false: if gprs not connected/ local IP is not valid, true: valid local IP
 * @example
 * bool gprsConnectValidIP = GetGPRSStatus();
 */
bool GetGPRSStatus(void);

/**
 * @fn bool GetModemInfo(char *modemType)
 * @brief This function gived modem information.
 * @remark returns, false: modem not responding, true: valid modem information
 * @example
 * char modemInfo[100];
 * bool isValidModemInfo = GetModemInfo(modemInfo);
 */
bool GetModemInfo(char *modemType);
#endif // QuecGsmGprs_h
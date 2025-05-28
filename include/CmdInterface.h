#ifndef CMDINTERFACE_H
#define CMDINTERFACE_H
#include <cmds.h>

#define ERROR_RESP "ERROR"
#define OK_RESP "OK"
#define BAD_PARAM_RESP "BAD PARAMS"
#define BAD_ARG_RESP "BAD ARGS"
// Functions To Write Device Configs.

CMD_STATUS SetWiFiSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetWiFiPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetAPSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetAPPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetWiFiMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDataSendMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetUID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetUPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDhcp(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetSubnet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetGateway(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetPDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetSDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetModTcpClientIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModTcpClientIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModTcpClientPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModTcpClientPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetDateTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetLogTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetSendCurrentHttpLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetSendCurrentMqttLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetTestServer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetTcpZero(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetFileInfo(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetWiFiSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetWiFiPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetAPSSID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetAPPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetWiFiMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDataSendMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetUID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetUPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDhcp(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetSubnet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetIpAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetGateway(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetPDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetSDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDns(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetDateTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetLogTime(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetDevInfo(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetFlashDet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetSDCardDetetct(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS DeleteLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS ClearAllLogs(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS RestoreFactorySettings(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS FormatPartition(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetMac(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS ResetDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS DeepDebugDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS DebugDevice(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GsmDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS ModbusRtuDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS ModbusTcpDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS LogDebug(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SendTestMsg(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS BluetoothOn(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

// CMD_STATUS FlDeviceDiscovery(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS OtaUrlProcess(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS DownloadSSLRootCa(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS DownloadSSLKey(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS DownloadSSLClient(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetMaxFailedToConnectCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMaxFailedToConnectCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModemRestartCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModemRestartCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

// config system parameter set get commands
CMD_STATUS GetModbusSlave1Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave2Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave3Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave4Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave5Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetDo3ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetPh4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDo4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetPh5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDo5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetModbusSlave1Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave2Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave3Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave4Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave5Addr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetDo3ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetPh4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDo4ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetPh5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDo5ModbusAddr(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

// Slave Enable Or Disable Status
CMD_STATUS GetModbusSlave1EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave2EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave3EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave4EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetModbusSlave5EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave1EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave2EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave3EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave4EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModbusSlave5EnableStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetUpdateRate(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetUpdateRate(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetHBInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetHBInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetGpsLockDet(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetLocation(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetLocation(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetComPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetComPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetLogSend(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetLogSend(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetLogSave(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetLogSave(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetLogCount(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetLog(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetAPNMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetAPNMode(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetAPNSim1(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetAPNSim1(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetAPNSim2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetAPNSim2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetIMEI(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetIMEI(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetStInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetStInterval(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMQTTUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMQTTPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMqttSSL(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetMqttCertEnable(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMQTTUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMQTTPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMqttSSL(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMqttCertEnable(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetDataPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetHBPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetOtpSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetInfoSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetConfigPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetOndemandPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetConfigSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetOndemandSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDataPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetHBPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetOtpSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetInfoSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetConfigPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetOndemandPubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetConfigSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetOndemandSubTopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetFLMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLMqttUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLMqttPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLPubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLSubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLHttpUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLHttpPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFLHttpToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLMqttHost(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLMqttPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLMqttUser(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLMqttPass(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLMqttClientID(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLPubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLSubtopic(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLHttpUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLHttpPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFLHttpToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetDataTransferProtocol(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetServerIP(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetSerPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetDataServToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetIMEIasToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDataTransferProtocol(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetServerIP(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetServPort(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetUrl(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetDataServToken(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetConnectionDelay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetRTCStatus(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetModemResetCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetModemResetCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS RestoreFactorySettings2(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS DeleteFile(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS CreateFile(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS GetAppPendingPubCnt(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetFarm1Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFarm1Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFarm2Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFarm2Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFarm3Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFarm3Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFarm4Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFarm4Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetFarm5Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetFarm5Name(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);

CMD_STATUS SetPrevDay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetPrevDay(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS GetPrevDayTotalizer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);
CMD_STATUS SetPrevDayTotalizer(int argSize, char *argv[], CIRCULAR_BUFFER *respBuf);


#endif
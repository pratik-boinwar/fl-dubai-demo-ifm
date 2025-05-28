#include "ssl_cert.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "ota.h"

#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecHttp.h"
#include "QuecMqtt.h"
#include "QuecSsl.h"
#include "QuecFile.h"
#include "QuecFtp.h"

#include "globals.h"
#include "netclient.h"

#if OTA_DEBUG_SUPPORT
#define odebugPrint(x) debugPrint(x)
#define odebugPrintln(x) debugPrintln(x)
#define odebugPrintf(a, b) debugPrintf(a, b)
#else
#define odebugPrint(x)
#define odebugPrintln(x)
#define odebugPrintf(a, b)
#endif

// common variables
unsigned long _ssl_size = 0;          // total size of received bytes.
unsigned int _ssl_iterations = 0;     // number of time data received.
unsigned long _ssl_expected_size = 0; // expected size

// http related variables
char *_ssl_http_url;

bool _httpDownloadSSlFiles = false;
bool _httpDownloadRootCaFile = false;
bool _httpDownloadClientFile = false;
bool _httpDownloadKeyFile = false;

static const char _spiffs_rootCa_file[] = "/rootca0.pem"; // do not use this name elsewhere in program
static const char _spiffs_client_file[] = "/client0.crt"; // do not use this name elsewhere in program
static const char _spiffs_key_file[] = "/key0.key";       // do not use this name elsewhere in program

String certFileNameRxFromHttpResp;

// ssl related variales
static bool _sslRequired = false;
// static SslContextHandle _otaSslContext = -1;

// helper buffer
static char _helperBuffer[5 * 1024] = {0}; // keep this buffer size upto what module RX buffer size and consider baudrate as well.

String certificate;

static File _spiffsFile; // file handler

extern MqttBrokerHandler _dataBroker;

static int _sslCertDownloadHttpCallBack(HttpEvent_t httpEvent, const char *httpRxData, unsigned long httpRxDataLen);

// http call back
static int _sslCertDownloadHttpCallBack(HttpEvent_t httpEvent, const char *httpRxData, unsigned long httpRxDataLen)
{
    unsigned long _writtingBytes = 0;
    switch (httpEvent)
    {
    case HTTP_EVENT_DWNLD_START:
        if (true == downloadCertificateBySerialCmdFlag)
        {
            if (true == systemFlags.downloadSSlRootCaFile)
            {
                _spiffsFile = SPIFFS.open(_spiffs_rootCa_file, "a");
            }
            else if (true == systemFlags.downloadSSlClientFile)
            {
                _spiffsFile = SPIFFS.open(_spiffs_client_file, "a");
            }
            else if (true == systemFlags.downloadSSlkeyFile)
            {
                _spiffsFile = SPIFFS.open(_spiffs_key_file, "a");
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (true == systemFlags.downloadSSlRootCaFile)
            {
                _spiffsFile = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "a");
            }
            else if (true == systemFlags.downloadSSlClientFile)
            {
                _spiffsFile = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "a");
            }
            else if (true == systemFlags.downloadSSlkeyFile)
            {
                _spiffsFile = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "a");
            }
            else
            {
                return 0;
            }
        }
        if (!_spiffsFile)
        {
            debugPrintln("[OTA] file not opened");
        }

        QuectelDebug(false);
        _ssl_size = 0;
        _ssl_iterations = 0;
        _ssl_expected_size = httpRxDataLen;
        odebugPrintf("[SSL DOWNLOAD] download start, expecting %ld bytes\r\n", _ssl_expected_size);
        break;

    case HTTP_EVENT_DWNLD_NEW_DATA:
        if (_ssl_size >= _ssl_expected_size)
        {
            odebugPrintln("[SSL DOWNLOAD] bytes exceeded");
            return 0;
        }

        _writtingBytes = (_ssl_expected_size - _ssl_size) > httpRxDataLen ? httpRxDataLen : (_ssl_expected_size - _ssl_size);

        certificate += httpRxData;

        _ssl_size += httpRxDataLen;
        // buffer pointed by 'httpRxData' is going to be updated after leaving this function.
        // So, whatever we got write down immediately.
        _spiffsFile.write((uint8_t *)httpRxData, httpRxDataLen);
        delay(5);
        _spiffsFile.flush();

        _ssl_iterations++;
        // Serial.write((uint8_t *)httpRxData, httpRxDataLen);
        odebugPrintf("[SSL DOWNLOAD] received length %ld ", httpRxDataLen);
        odebugPrintf(", written %ld\r\n", _writtingBytes);
        break;

    case HTTP_EVENT_DWNLD_END:
        systemFlags.gsmDebugEnableFlag = false;
        QuectelDebug(systemFlags.gsmDebugEnableFlag);

        odebugPrintf("\r\n[SSL DOWNLOAD] total received %ld\r\n", _ssl_size);
        if (_ssl_size != _ssl_expected_size)
        {
            odebugPrintf("[SSL DOWNLOAD] expected %ld data bytes\n", _ssl_expected_size);
        }
        // odebugPrintln("CERTIFICATE: ");
        // odebugPrintln(certificate.c_str());

        odebugPrintf("[SSL DOWNLOAD] file size become %d\n", _spiffsFile.size());
        _spiffsFile.close();

        File root = SPIFFS.open("/");
        File file = root.openNextFile();

        if (true == systemFlags.downloadSSlRootCaFile)
        {
            while (file)
            {
                String certFile = String(file.name());
                bool flag = certFile.endsWith(".pem");
                if (true == downloadCertificateBySerialCmdFlag)
                {
                    if (flag && !certFile.equals(_spiffs_rootCa_file))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                else
                {
                    if (flag && !certFile.equals(certFileNameRxFromHttpResp.c_str()))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                file = root.openNextFile();
            }
            systemFlags.downloadSSlRootCaFile = false;
        }
        else if (true == systemFlags.downloadSSlClientFile)
        {
            while (file)
            {
                String certFile = String(file.name());
                bool flag = certFile.endsWith(".crt");
                if (true == downloadCertificateBySerialCmdFlag)
                {
                    if (flag && !certFile.equals(_spiffs_client_file))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                else
                {
                    if (flag && !certFile.equals(certFileNameRxFromHttpResp.c_str()))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                file = root.openNextFile();
            }
            systemFlags.downloadSSlClientFile = false;
        }
        else if (true == systemFlags.downloadSSlkeyFile)
        {
            while (file)
            {
                String certFile = String(file.name());
                bool flag = certFile.endsWith(".key");
                if (true == downloadCertificateBySerialCmdFlag)
                {
                    if (flag && !certFile.equals(_spiffs_key_file))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                else
                {
                    if (flag && !certFile.equals(certFileNameRxFromHttpResp.c_str()))
                    {
                        debugPrint("removing File: ");
                        debugPrintln(certFile.c_str());
                        SPIFFS.remove(certFile);
                    }
                }
                file = root.openNextFile();
            }
            systemFlags.downloadSSlkeyFile = false;
        }
        // close SPIFFS files opened
        root.close();

        debugPrintln("*********Restarting Device, Certificate download success!************");
        delay(10);
        ESP.restart();
        // MqttClose(_dataBroker);
        // ConfigConnectionToDataBroker(true);

        if (true == downloadCertificateBySerialCmdFlag)
        {
            // disable this flag
            downloadCertificateBySerialCmdFlag = false;
        }

        if (_httpDownloadSSlFiles)
        {
            // we are here from OTA over HTTP method. clear related parameters.
            _httpDownloadSSlFiles = false;

            free(_ssl_http_url);
            _ssl_http_url = NULL;
        }

        if (_sslRequired)
        {
            _sslRequired = false;

            // delete/un-subscribe ssl channel
        }
        break;
    }

    return 1;
}

bool SSLCertFromUrl(const char *sslUrl, String certFileName)
{
    /*
        URL format
        HTTP: http://hostname:port/filePath/fileName
        FTP: ftp://hostname/filePath/fileName:port@username:password
    */

    if (sslUrl == 0)
        return false;

    if ((0 != (strstr(sslUrl, "http://"))) || (0 != (strstr(sslUrl, "https://"))))
    {
        // add validations.
        // like if https, then set _sslRequired flag.. subscribe sslChannel etc..

        if (_ssl_http_url)
            free(_ssl_http_url);

        _ssl_http_url = strdup(sslUrl);

        File file;
        if (true == downloadCertificateBySerialCmdFlag)
        {
            if (true == systemFlags.downloadSSlRootCaFile)
            {
                file = SPIFFS.open(_spiffs_rootCa_file, "w+");
            }
            else if (true == systemFlags.downloadSSlClientFile)
            {
                file = SPIFFS.open(_spiffs_client_file, "w+");
            }
            else if (true == systemFlags.downloadSSlkeyFile)
            {
                file = SPIFFS.open(_spiffs_key_file, "w+");
            }
            else
            {
                return false;
            }
        }
        else
        {
            certFileNameRxFromHttpResp = certFileName;

            if (true == systemFlags.downloadSSlRootCaFile)
            {
                file = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "w+");
            }
            else if (true == systemFlags.downloadSSlClientFile)
            {
                file = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "w+");
            }
            else if (true == systemFlags.downloadSSlkeyFile)
            {
                file = SPIFFS.open(certFileNameRxFromHttpResp.c_str(), "w+");
            }
            else
            {
                return false;
            }
        }
        if (file)
        {
            file.close();
            if (true == downloadCertificateBySerialCmdFlag)
            {
                if (true == systemFlags.downloadSSlRootCaFile)
                {
                    if (!SPIFFS.exists(_spiffs_rootCa_file))
                    {
                        // this is not suppossed to happen.
                        // spiffs format required.
                        debugPrintln("[SSL] root ca certificate file not created. memory format required");
                        debugPrintln("[SSL] root ca certificate  download aborted, try another method");
                        return false;
                    }
                }
                else if (true == systemFlags.downloadSSlClientFile)
                {
                    if (!SPIFFS.exists(_spiffs_client_file))
                    {
                        // this is not suppossed to happen.
                        // spiffs format required.
                        debugPrintln("[SSL] client certificate file not created. memory format required");
                        debugPrintln("[SSL] client certificate download aborted, try another method");
                        return false;
                    }
                }
                else if (true == systemFlags.downloadSSlkeyFile)
                {
                    if (!SPIFFS.exists(_spiffs_key_file))
                    {
                        // this is not suppossed to happen.
                        // spiffs format required.
                        debugPrintln("[SSL] key file not created. memory format required");
                        debugPrintln("[SSL] key file download aborted, try another method");
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {

                if (!SPIFFS.exists(certFileNameRxFromHttpResp.c_str()))
                {
                    // this is not suppossed to happen.
                    // spiffs format required.
                    debugPrintln("[SSL] certificate file not created. memory format required");
                    debugPrintln("[SSL] certificate download aborted, try another method");
                    return false;
                }
            }
        }
        else
        {
            debugPrintln("[SSL] file not created");
        }
        _httpDownloadSSlFiles = true;

        // _ftpDownloadFirmware = false; // consider latest request

        // _sslRequired = false;

        // ToDo:
        // if 'https' session set _sslRequired flag and call ssl initializations
        // note that, after using ssl session, delete/un-subscribe particular channel.

        /*if(0 != (strstr(sslUrl, "https://")
        {
            // SSL Initializations if secure session

            _sslRequired = true;
            // _otaSslContext = SslChannelSubscribe(rootCACertificate, strlen(rootCACertificate), CLIENT_CERT_CRT, strlen(CLIENT_CERT_CRT),
            //                                   CLIENT_PRIVATE_KEY, strlen(CLIENT_PRIVATE_KEY), SSL_VER_ALL, All_CIPHER_SUITES, SERVER_CLIENT_AUTH);
        }*/

        return true;
    }
    else
    {
        debugPrintln("[SSL DOWNLOAD] url must start with http, https or ftp");
    }
    return false;
}

typedef enum
{
    SSL_DOWNLOAD_SM_INIT = 0,
    SSL_DOWNLOAD_SM_HTTP_EVENT,
    SSL_DOWNLOAD_SM_HTTP_DOWNLOAD_ROOT_CA,
    SSL_DOWNLOAD_SM_HTTP_DOWNLOAD_CLIENT,
    SSL_DOWNLOAD_SM_HTTP_DOWNLOAD_KEY,
    SSL_DOWNLOAD_SM_WAIT_HTTP_PROCESS_COMPLETE,
    SSL_DOWNLOAD_SM_FTP_EVENT,
    SSL_DOWNLOAD_SM_WAIT_FTP_PROCESS_COMPLETE,

    OTA_SM_IDLE
} OTA_SM;

void SSLCertLoop(void)
{
    static uint8_t _sslDownloadSmState = OTA_SM_IDLE;
    static uint32_t _generalTimeoutTick = 0;
    HttpStatus_t httpStatus;
    // FtpStatus_t ftpStatus;
    // uint8_t _ftpRetryCnt = 0;
    uint8_t _httpRetryCnt = 0;

    switch (_sslDownloadSmState)
    {
        // ======================== OTA HTTP PROCESSING =========================

    case SSL_DOWNLOAD_SM_HTTP_EVENT: // start http download session
        // check if ota dowanload flag is set and not other ota operation is running.
        if (!(_httpDownloadSSlFiles))
        {
            _sslDownloadSmState = OTA_SM_IDLE;
            break;
        }

        // check if http client and modem avaiaibilty
        if (!(IsHttpClientAvailable() && IsModemReady()))
        {
            break;
        }

        if (_sslRequired)
        {
            // wait till sslchannel is ready.
            // otherwise session is going to be fail anyways.

            // assign 'sslContextID' and 'secured' flag in 'HttpSendData'
        }

        if (!HttpSendData(_ssl_http_url, _helperBuffer, strlen(_helperBuffer), _helperBuffer, sizeof(_helperBuffer),
                          HTTP_METHOD_GET, HTTP_CONTENT_TEXT_PLAIN, _sslCertDownloadHttpCallBack, 0, 0))
        {
            odebugPrintln("[SSL DOWNLOAD] HTTP not initiated");
            _httpRetryCnt++;
            break;
        }
        memset(_helperBuffer, 0, sizeof(_helperBuffer));

        odebugPrintln("[SSL DOWNLOAD] Downloading certificate...");

        // clear the event
        // _httpDownloadSSlFiles = 0;

        _sslDownloadSmState = SSL_DOWNLOAD_SM_WAIT_HTTP_PROCESS_COMPLETE;
        _generalTimeoutTick = millis();
        // _sslDownloadSmState = NC_HTTP_WAIT_FOR_FIRMWARE_INFO;
        break;

    case SSL_DOWNLOAD_SM_WAIT_HTTP_PROCESS_COMPLETE: // wait till http session gets completed
        httpStatus = HttpGetLastStatus();
        if (httpStatus == HTTP_STATUS_SUCCESS)
        {
            odebugPrintln("[SSL DOWNLOAD] over HTTP Success");

            // clear the event
            _httpDownloadSSlFiles = false;
            _httpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.

            _generalTimeoutTick = millis();
            _sslDownloadSmState = OTA_SM_IDLE;
        }
        else if (httpStatus == HTTP_STATUS_FAILED)
        {
            _httpRetryCnt++; // try one more time
            odebugPrintf("[SSL DOWNLOAD] over HTTP fail, retry #%d", _httpRetryCnt);
            _generalTimeoutTick = millis();
            _sslDownloadSmState = OTA_SM_IDLE;
        }
        else if (httpStatus == HTTP_STATUS_DOWNLOADING)
        {
            _generalTimeoutTick = millis();
        }
        if ((millis() - _generalTimeoutTick) > 130000U)
        {
            _httpRetryCnt++; // try one more time
            odebugPrintf("[SSL DOWNLOAD] over HTTP timeout, retry #%d", _httpRetryCnt);
            _generalTimeoutTick = millis();
            _sslDownloadSmState = OTA_SM_IDLE;
            break;
        }
        break;

        // ======================== OTA FTP PROCESSING =========================

        // case SSL_DOWNLOAD_SM_FTP_EVENT: // start http download session
        //     // check if ota dowanload flag is set and not other ota operation is running.
        //     if (!(_ftpDownloadFirmware))
        //     {
        //         _sslDownloadSmState = OTA_SM_IDLE;
        //         break;
        //     }

        //     //check if ftp client and modem avaiaibilty
        //     if (!(IsFtpClientAvailable() && IsModemReady()))
        //     {
        //         break;
        //     }

        //     if (_sslRequired)
        //     {
        //         // wait till sslchannel is ready.
        //         // otherwise session is going to be fail anyways.
        //     }

        //     if (!FtpInitiate(_ota_host, _ota_port, _ota_ftp_user, _ota_ftp_pass, _otaDwnldLocalFileName, _ota_ftp_remote_filename,
        //                      _ota_ftp_remote_filepath, FTP_DOWNLOAD_FILE, FTP_FILE_OVERWRITE, QUEC_FILE_MEM_RAM))
        //     {
        //         debugPrintln("[OTA] FTP Downloading not initiated");
        //         _ftpRetryCnt++;
        //         _generalTimeoutTick = millis();
        //         _sslDownloadSmState = OTA_SM_IDLE;
        //         break;
        //     }
        //     // _ftpDownloadFirmware = 0;
        //     debugPrintf("[OTA] FTP Downloading %s \n", _ota_ftp_remote_filename);
        //     _generalTimeoutTick = millis();
        //     _sslDownloadSmState = SSL_DOWNLOAD_SM_WAIT_FTP_PROCESS_COMPLETE;
        //     break;

        // case SSL_DOWNLOAD_SM_WAIT_FTP_PROCESS_COMPLETE: // wait or ftp process get complete
        //     ftpStatus = FtpGetLastStatus();
        //     if (ftpStatus == FTP_STATUS_SUCCESS)
        //     {
        //         debugPrintln("[OTA] over FTP Download SUCCESS");

        //         _ftpRetryCnt = 0; // clear retry count
        //         // _ftpDownloadFirmware = false;

        //         QuecFileList(QUEC_FILE_MEM_RAM);
        //         QuecFileGetStorageSize(QUEC_FILE_MEM_RAM, NULL);

        //         // transfer Quectel RAM file content to local spiffs file.
        //         if (QuecFileDownload(_otaDwnldLocalFileName, (uint8_t *)_helperBuffer, sizeof(_helperBuffer), QUEC_FILE_MEM_RAM, _otaFiledownloader))
        //         {
        //             // the Quectel RAM file to SPIFFS file transfer is succeeded. try to download next file.
        //             if (_ftp_max_files_to_be_download == _ftp_current_file_downloading_index)
        //             {
        //                 debugPrintln("[OTA] all files downloaded. updating firmware");

        //                 // start updating firmware from local spiffs file.
        //                 _otaSpiffsToCoreTransfer();

        //                 // if everything we tried is acheived and we are calling auto reboot of device after successful upgrade
        //                 // then we shall not reach here,

        //                 // otherwise, we are here means our OTA is failed. clean up the things.
        //                 _ftpDownloadFirmware = false;
        //                 _ftpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.
        //             }
        //             else
        //             {
        //                 _ftp_current_file_downloading_index++; // move to next file index

        //                 // generate next file name to download
        //                 char appBin_fName[FTP_FILENAME_LEN] = {0x0};
        //                 sprintf(appBin_fName, "%s.%03d", _ftp_remote_fName_wo_ext, _ftp_current_file_downloading_index);

        //                 // reload new remote file name
        //                 if (_ota_ftp_remote_filename)
        //                     free(_ota_ftp_remote_filename);
        //                 _ota_ftp_remote_filename = strdup((char *)appBin_fName);

        //                 // _ftpDownloadFirmware = true;
        //                 // _httpDownloadSSlFiles = false;

        //                 debugPrintln("FTP Downloading next file");
        //                 _sslDownloadSmState = SSL_DOWNLOAD_SM_FTP_EVENT;
        //                 break;
        //             }
        //         }
        //         else
        //         {
        //             // if RAM file to SPIFFS transefer fail, then no point going ahead.
        //             // FOTA will not be completed. I suggest abbort or restart whole procedure.
        //             debugPrintln("[OTA] local File download fail");
        //             debugPrintln("[OTA] over FTP aborted");

        //             // clean up the things
        //             _ftpDownloadFirmware = false;
        //             _ftpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.
        //         }

        //         _generalTimeoutTick = millis();
        //         _sslDownloadSmState = OTA_SM_IDLE;
        //     }
        //     else if (ftpStatus == FTP_STATUS_FAILED)
        //     {
        //         debugPrintln("[OTA] over FTP fail");

        //         _generalTimeoutTick = millis();
        //         _sslDownloadSmState = OTA_SM_IDLE;
        //         _ftpRetryCnt++; // try one more time
        //     }

        //     if ((millis() - _generalTimeoutTick) > 125000)
        //     {
        //         debugPrintln("[OTA] over FTP timeout");

        //         _generalTimeoutTick = millis();
        //         _sslDownloadSmState = OTA_SM_IDLE;
        //         _ftpRetryCnt++; // try one more time
        //         break;
        //     }
        //     break;

    default:
        if ((millis() - _generalTimeoutTick) > 10000U)
        {
            _generalTimeoutTick = millis();

            if (_httpRetryCnt >= 3)
            {
                _httpRetryCnt = 0;
                _httpDownloadSSlFiles = false;
                // if (_otaCurrentMethod == OTA_OVER_HTTP)
                // {
                //     _otaCurrentMethod = OTA_OVER_NONE;
                // }

                if (_sslRequired)
                {
                    _sslRequired = false;

                    // delete/un-subscribe ssl channel
                }

                free(_ssl_http_url);
                _ssl_http_url = NULL;
            }

            // if (_ftpRetryCnt >= 3)
            // {
            //     _ftpRetryCnt = 0; // clear retry count
            //     if ((_otaCurrentMethod == OTA_OVER_FTP_MULTI_FILE) ||
            //         (_otaCurrentMethod == OTA_OVER_FTP_SINGLE_FILE))
            //     {
            //         _otaCurrentMethod = OTA_OVER_NONE;
            //     }
            //     _ftpDownloadFirmware = false; // stop downloading

            //     // if (_ota_host)
            //     free(_ota_host);
            //     _ota_host = NULL;
            //     // if (_ota_ftp_user)
            //     free(_ota_ftp_user);
            //     _ota_ftp_user = NULL;
            //     // if (_ota_ftp_pass)
            //     free(_ota_ftp_pass);
            //     _ota_ftp_pass = NULL;
            //     // if (_ota_ftp_remote_filepath)
            //     free(_ota_ftp_remote_filepath);
            //     _ota_ftp_remote_filepath = NULL;
            //     // if (_ota_ftp_remote_filename)
            //     free(_ota_ftp_remote_filename);
            //     _ota_ftp_remote_filename = NULL;
            // }

            // Note that we have checking http flag is first and then ftp.
            // because http is straight process, there are no multiple files to download.
            // even if we start http process in between of partial ftp files download.
            // we can resume it later after http process gets fail.
            // if http process succedded then we do not even require FTP process to continue.
            // because device will be rstarted after firmware updated.

            // check if ota download flag is set.
            if (_httpDownloadSSlFiles)
            {
                _sslDownloadSmState = SSL_DOWNLOAD_SM_HTTP_EVENT;
                break;
            }

            // if (_ftpDownloadFirmware && _otaCurrentMethod == OTA_OVER_NONE)
            // {
            //     _sslDownloadSmState = SSL_DOWNLOAD_SM_FTP_EVENT;
            //     break;
            // }

            _sslDownloadSmState = OTA_SM_IDLE;
        }
        break;

    } // switch (_sslDownloadSmState)
}

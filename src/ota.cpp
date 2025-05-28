#include <Arduino.h>
#include <ArduinoOTA.h>

#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecHttp.h"
#include "QuecSsl.h"
#include "QuecFile.h"
#include "QuecFtp.h"

#include "globals.h"
#include "ota.h"

#include "global-vars.h"

extern String imei;

#define ARDUINO_OTA_PORT 8266

#if OTA_DEBUG_SUPPORT
#define odebugPrint(x) debugPrint(x)
#define odebugPrintln(x) debugPrintln(x)
#define odebugPrintf(a, b) debugPrintf(a, b)
#else
#define odebugPrint(x)
#define odebugPrintln(x)
#define odebugPrintf(a, b)
#endif

typedef enum
{
    OTA_OVER_ARDUINO,
    OTA_OVER_HTTP,
    OTA_OVER_FTP_MULTI_FILE,
    OTA_OVER_FTP_SINGLE_FILE,

    OTA_OVER_NONE

} OTA_Method;

OTA_Method _otaCurrentMethod = OTA_OVER_NONE;

// common variables
unsigned long _ota_size = 0;          // total size of received bytes.
unsigned int _ota_iterations = 0;     // number of time data received.
unsigned long _ota_expected_size = 0; // expected size

uint8_t percentageOfOta = 0, percentageOfOtaOld = 0; // to show download percentage

// http related variables
char *_ota_http_url;
bool _httpDownloadFirmware = false;

// ftp related variables

// static uint8_t ftpHost[FTP_SERVERADD_LEN] = {0x0};    // ip string or domain name
// static uint8_t ftpFilePath[FTP_FILEPATH_LEN] = {0x0}; // file in the server path
// static uint8_t ftpUserName[FTP_USERNAME_LEN] = {0x0};
// static uint8_t ftpPassword[FTP_PASSWORD_LEN] = {0x0};
// static uint8_t appBin_fName[FTP_FILENAME_LEN] = {0x0};
static bool _ftpDownloadFirmware = false;
static File _spiffsFile; // file handler

static char *_ota_host = NULL;
static unsigned int _ota_port = 80;
static char *_ota_ftp_user = NULL;
static char *_ota_ftp_pass = NULL;
static char *_ota_ftp_remote_filepath = NULL;
static char *_ota_ftp_remote_filename = NULL;
static uint8_t _ftp_max_files_to_be_download = 1;
static uint8_t _ftp_current_file_downloading_index = 1;

static char _ftp_remote_fName_wo_ext[FTP_FILENAME_LEN] = {0x0}; // remote firmware file name without extension
static const char _otaDwnldLocalFileName[] = "firmware.bin";
static const char _spiffs_filename[] = "/firmware.bin"; // do not use this name elsewhere in program

// ssl related variales
static bool _sslRequired = false;
// static SslContextHandle _otaSslContext = -1;

// helper buffer
static char _helperBuffer[5 * 1024] = {0}; // keep this buffer size upto what module RX buffer size and consider baudrate as well.

// ota url
// const char otaHttpUrl[] = "https://raw.githubusercontent.com/umeshwalkar/esp32rfid-bin/master/README.md";
// const char otaHttpUrl[] = "https://raw.githubusercontent.com/umeshwalkar/esp32rfid-bin/master/wifirfid_1.0_v2.bin";
// const char otaHttpUrl[] = "https://raw.githubusercontent.com/umeshwalkar/esp32rfid-bin/master/QuectelAT_firmware.bin";
// const char otaHttpUrl[101] = "https://raw.githubusercontent.com/umeshwalkar/esp32rfid-bin/master/rmu_star_firmware.bin";
// const char otaFtpUrl[] = "ftp://165.22.221.43/ftp/test3/firmware.bin.005:21@rotosol:rotosol";
// const char otaFtpUrl[] = "ftp://165.22.221.43/rmu_star/update/firmware.bin.005:21@rotosol:rotosol";

static int _otaHttpCallBack(HttpEvent_t httpEvent, const char *httpRxData, unsigned long httpRxDataLen);
static void _otaFiledownloader(FileDownloadEvent_t fileEvent, const uint8_t *data, uint32_t dataLen);
static void _otaSpiffsToCoreTransfer(void);
static bool _FTP_DecodeURL(uint8_t *URL, int URLlength, uint8_t *serverAdd, unsigned int *serverPort,
                           uint8_t *ftpUserName, uint8_t *ftpUserPassword, uint8_t *filePath, uint8_t *binFile);

// http call back
static int _otaHttpCallBack(HttpEvent_t httpEvent, const char *httpRxData, unsigned long httpRxDataLen)
{
    unsigned long _writtingBytes = 0;
    switch (httpEvent)
    {
    case HTTP_EVENT_DWNLD_START:
        QuectelDebug(false);
        _ota_size = 0;
        _ota_iterations = 0;
        _ota_expected_size = httpRxDataLen;
        odebugPrintf("[OTA] download start, expecting %ld bytes\r\n", _ota_expected_size);
        break;

    case HTTP_EVENT_DWNLD_NEW_DATA:
        if (_ota_size >= _ota_expected_size)
        {
            odebugPrintln("[OTA] bytes exceeded");
            return 0;
        }

        _writtingBytes = (_ota_expected_size - _ota_size) > httpRxDataLen ? httpRxDataLen : (_ota_expected_size - _ota_size);

        if (_ota_size == 0)
        {
            if (!Update.begin())
            {
                Update.printError(Serial);
            }
        }
        if (!Update.hasError())
        {
            if (Update.write((uint8_t *)httpRxData, _writtingBytes) != _writtingBytes)
            {
                Update.printError(Serial);
            }
        }

        _ota_size += httpRxDataLen;
        _ota_iterations++;
        // Serial.write((uint8_t *)httpRxData, httpRxDataLen);
        odebugPrintf("[OTA] received length %ld ", httpRxDataLen);
        odebugPrintf(", written %ld\r\n", _writtingBytes);

        odebugPrintf("[OTA] recieved %ld bytes,", _ota_size);
        odebugPrintf("outof %ld bytes\r\n", _ota_expected_size);
        // map totol bytes recieved and maximum expected bytes with 0 to 100 %
        percentageOfOta = map(_ota_size, 0, _ota_expected_size, 0, 100);
        odebugPrintf("\n[OTA] Download percentage: %d%%\r\n\n", percentageOfOta);
        break;

    case HTTP_EVENT_DWNLD_END:
        QuectelDebug(systemFlags.gsmDebugEnableFlag);

        odebugPrintf("\r\n[OTA] total received %ld\r\n", _ota_size);
        if (_ota_size != _ota_expected_size)
        {
            odebugPrintf("[OTA] expected %ld data bytes\n", _ota_expected_size);
        }
        if (Update.end(true))
        {
            odebugPrintf(PSTR("[OTA] Success: %ld bytes\n"), _ota_size);
            SPIFFS.remove(_spiffs_filename);
            delay(500);
            esp_restart();
        }
        else
        {
            Update.printError(Serial);
            // update failed status
            systemFlags.otaUpdateFailedFlag = true;

            // Note:  I have noticed that sometimes spiffs file writing operations are not
            // carried out as expected. this is mainly due to memory corrupt.
            // we can see the list of files with proper sizes,
            // but those files were already written/present before memory
            // starts corrupting. here corrupting is the issue of framgmentation.
            // newly created file and newly written contents are not reflected properly.
            // also, in such condition we can see the writing operations takes more time
            // than ususal. memory formatting is the only solution i can suggest here.
            // memory fragmentation issue occure when frequent deleting, creating file operations with new file sizes.
            // memory corruption (file system partition corrupt issue) occured when multiple restart of device,
            // where we tend to initialize spiffs. with frequent restart we initialize it again and again and it gets corrrupt.
            // Here, in ESP the mainly reason is because device frequent restart and 'brown out' restart.
            // Simply, add few delays after first instruction execution and before spiffs init.
            // or any procedure to make delay the spiffs init.
            // because if device has power issue it will get reset again before we even try to spiffs init.
        }

        // Note:
        // Idealy we should not reach here if OTA is succedded
        // because we are rebooting the device.
        // If we are here means OTA file was not intact.
        // If this happens again and again, then we must format spiffs.
        if (_httpDownloadFirmware)
        {
            // we are here from OTA over HTTP method. clear related parameters.
            _httpDownloadFirmware = false;

            free(_ota_http_url);
            _ota_http_url = NULL;
        }

        if (_ftpDownloadFirmware)
        {
            // we are here from OTA over FTP method. clear related parameters.
            _ftpDownloadFirmware = false;

            free(_ota_host);
            _ota_host = NULL;
            free(_ota_ftp_user);
            _ota_ftp_user = NULL;
            free(_ota_ftp_pass);
            _ota_ftp_pass = NULL;
            free(_ota_ftp_remote_filepath);
            _ota_ftp_remote_filepath = NULL;
            free(_ota_ftp_remote_filename);
            _ota_ftp_remote_filename = NULL;
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

static void _otaFiledownloader(FileDownloadEvent_t fileEvent, const uint8_t *data, uint32_t dataLen)
{
    static uint32_t _readBytes = 0;
    static uint32_t _expectedSize = 0;
    switch (fileEvent)
    {
    case FILE_DWNLD_START:
        _readBytes = 0;
        _expectedSize = dataLen;
        debugPrintf("file download start, expecting %d\n", _expectedSize);

        // open file in read/write mode, but do not truncate or create new one
        // https://www.esp32.com/viewtopic.php?f=19&t=3229
        _spiffsFile = SPIFFS.open(_spiffs_filename, "a");
        if (!_spiffsFile)
        {
            debugPrintln("[OTA] file not opened");
        }
        // _spiffsFile.seek(0, SeekEnd);
        QuectelDebug(false);
        break;

    case FILE_DWNLD_NEW_DATA:
        _readBytes += dataLen;
        // buffer pointed by 'data' is going to be updated after leaving this function.
        // So, whatever we got write down immediately.
        _spiffsFile.write((uint8_t *)data, dataLen);
        delay(5);
        _spiffsFile.flush();

        debugPrintf("file data received length #%d\n", dataLen);
        // debugPrint(".");
        // debugPrintf("%s\n",data);
        // DBG("[OTA] rx #%d, file size become %d\n", dataLen, _spiffsFile.size());
        break;

    case FILE_DWNLD_END:
        debugPrintf("\nfile download end, total received #%d ", _readBytes);
        debugPrintf("/ %d\n", _expectedSize);
        DBG("[OTA] file size become %d\n", _spiffsFile.size());
        _spiffsFile.close();

        QuectelDebug(true);
        break;

    default:
        break;
    }
}

static void _otaSpiffsToCoreTransfer(void)
{
    // here we will make use of _otaHttpCallBack and HttpEvent_t httpEvent
    _spiffsFile.close(); // close if not previously closed

    // open in read mode
    _spiffsFile = SPIFFS.open(_spiffs_filename, "r");
    if (!_spiffsFile)
    {
        debugPrintln("[OTA] spiffs firmware fime not opened");
        return;
    }

    size_t fsize = _spiffsFile.size();
    size_t readBytes = 0, rsize;
    _otaHttpCallBack(HTTP_EVENT_DWNLD_START, 0, fsize);

    uint32_t _timeout = millis();
    do
    {
        _helperBuffer[0] = 0;
        rsize = 0;
        rsize = _spiffsFile.read((uint8_t *)_helperBuffer, sizeof(_helperBuffer));
        _otaHttpCallBack(HTTP_EVENT_DWNLD_NEW_DATA, (char *)_helperBuffer, rsize);
        readBytes += rsize;

        if ((readBytes >= fsize) || (rsize == 0))
        {
            _otaHttpCallBack(HTTP_EVENT_DWNLD_END, 0, 0);
            _spiffsFile.close();
            break;
        }
        delay(0);
    } while (millis() - _timeout < 60000);

    // if OTA file is intact and we are able to transfer all bytes.
    // we shall not reach here if we call auto reboot of device after successful upgrade.

    _spiffsFile.close(); // cloese the file.
}

static bool _FTP_DecodeURL(uint8_t *URL, int URLlength, uint8_t *serverAdd, unsigned int *serverPort,
                           uint8_t *ftpUserName, uint8_t *ftpUserPassword, uint8_t *filePath, uint8_t *binFile)
{
    uint8_t hstr[7];
    uint32_t j, i;
    uint8_t *phostnamehead;
    uint8_t *phostnameTail;
    char portStr[8];
    bool ret = false;
    uint8_t *puri = URL;
    uint32_t datalen = URLlength;
    uint32_t filePathLen, binfilenameLen;
    *serverPort = FTP_SERVICE_PORT;

    do
    {
        /* Resolve ftp:// */
        memset(hstr, 0, 7);

        if ((datalen) < 7)
            break;
        memcpy(hstr, URL, 7);
        for (i = 0; i < 6; i++)
        {
            if ((hstr[i] >= 'A') && (hstr[i] <= 'Z'))
                hstr[i] = tolower(hstr[i]);
        }
        if (NULL != strstr((char *)hstr, "ftp://"))
        {
            puri = URL + 6;
            datalen -= 6;
        }
        else
        {
            break;
        }
        i = 0;
        /* retrieve host name */
        phostnamehead = puri;
        phostnameTail = puri;
        while (i < datalen && puri[i] != '/' && puri[i] != ':')
        {
            i++;
            phostnameTail++;
        }
        if (i > FTP_SERVERADD_LEN)
        {
            debugPrintf("<--ftp server Addess is too long! limit is %d-->\r\n", FTP_SERVERADD_LEN);
            break;
        }
        memcpy((char *)serverAdd, (char *)phostnamehead, i); // here is server ip or domain name.
        // debugPrintf("<--!!serverAdd=%s-->\r\n",serverAdd);

        if (datalen >= i)
            datalen -= i;
        else
            break;

        /* retrieve  file path ,  image file name and ,port      eg /filepath/file/xxx.bin:8021@user......  or   /filepath/file/xxx.bin@user......  */
        puri += i;
        i = 0;
        phostnamehead = puri;
        phostnameTail = puri;
        while (puri[i] != ':' && puri[i] != '@' && i < datalen)
        {
            i++;
            phostnameTail++;
        }
        if (datalen >= i)
        {
            datalen -= i;
        }
        else // no @username:password     eg :  ftp://192.168.10.10/file/test.bin
        {
            j = i;
            while (puri[j] != '/' && j > 0) // the last '/' char
            {
                j--;
            }
            binfilenameLen = (i - j - 1);
            filePathLen = i - (i - j);
            if (binfilenameLen > FTP_FILENAME_LEN)
            {
                debugPrintf("<--!! bin file name length is to loog! limit is %d-->\r\n", FTP_FILENAME_LEN);
                break;
            }
            if (filePathLen > FTP_FILEPATH_LEN)
            {
                debugPrintf("<--!! filePath length is to loog! limit is %d-->\r\n", FTP_FILEPATH_LEN);
                break;
            }
            memcpy((char *)binFile, (char *)(puri + j + 1), binfilenameLen);
            memcpy((char *)filePath, (char *)phostnamehead, filePathLen);
            break;
        }

        /* retrieve  file path ,  image file name  /filepath/file/xxx.bin@user......  */
        if (puri[i] == '@') // no port number , it means port number is 21
        {
            j = i;
            while (puri[j] != '/' && j > 0) // the last '/' char
            {
                j--;
            }
            binfilenameLen = (i - j - 1);
            filePathLen = i - (i - j);
            if (binfilenameLen > FTP_FILENAME_LEN)
            {
                debugPrintf("<--@@ bin file name length is to loog! limit is %d-->\r\n", FTP_FILENAME_LEN);
                break;
            }
            if (filePathLen > FTP_FILEPATH_LEN)
            {
                debugPrintf("<--@@ filePath length is to loog! limit is %d-->\r\n", FTP_FILEPATH_LEN);
                break;
            }
            memcpy((char *)binFile, (char *)(puri + j + 1), binfilenameLen);
            memcpy((char *)filePath, (char *)phostnamehead, filePathLen);
            // debugPrintf("<--@@binFile=%s filePath=%s datalen=%d-->\r\n", binFile, filePath, datalen);
        }
        /* retrieve file path , image file name and port   /filepath/file/xxx.bin:8021@user......  */
        else // else if (puri[i] ==':')     ftp port number.
        {
            j = i;
            while (puri[j] != '/' && j > 0) // the last '/' char
            {
                j--;
            }
            binfilenameLen = (i - j - 1);
            filePathLen = i - (i - j);
            if (binfilenameLen > FTP_FILENAME_LEN)
            {
                debugPrintf("<--## bin file name length is to loog! limit is %d-->\r\n", FTP_FILENAME_LEN);
                break;
            }
            if (filePathLen > FTP_FILEPATH_LEN)
            {
                debugPrintf("<--## filePath length is to loog! limit is %d-->\r\n", FTP_FILEPATH_LEN);
                break;
            }
            memcpy((char *)binFile, (char *)(puri + j + 1), binfilenameLen);
            memcpy((char *)filePath, (char *)phostnamehead, filePathLen);

            puri += i; // puri = :port@username....
            i = 0;
            phostnamehead = puri;
            phostnameTail = puri;
            while (i < datalen && puri[i] != '@')
            {
                i++;
                phostnameTail++;
            }
            if (datalen >= i)
                datalen -= i;
            else
                break;
            memset(portStr, 0x00, sizeof(portStr));
            memcpy(portStr, phostnamehead + 1, i - 1);
            *serverPort = atoi(portStr);
            //   debugPrintf("<--&&portStr=%s server port=%d-->\r\n",portStr,*serverPort);
        }

        /* retrieve the ftp username and password      eg  @username:password  */
        puri += i;
        i = 0;
        phostnamehead = puri;
        phostnameTail = puri;
        while (puri[i] != ':' && i < datalen)
        {
            i++;
            phostnameTail++;
        }
        if (datalen >= i)
            datalen -= i;
        else
            break;
        if (0 == datalen) // no user password
        {
            if (i > FTP_USERNAME_LEN)
            {
                debugPrintf("<--@@ ftp user name is to loog! limit is %d-->\r\n", FTP_USERNAME_LEN);
                break;
            }
            memcpy(ftpUserName, phostnamehead + 1, i); // ftp user name
        }
        else
        {
            if (i - 1 > FTP_USERNAME_LEN)
            {
                debugPrintf("<--@@ ftp user name is to loog! limit is %d-->\r\n", FTP_USERNAME_LEN);
                break;
            }
            if (datalen > FTP_PASSWORD_LEN)
            {
                debugPrintf("<--@@ ftp user password is to loog! limit is %d-->\r\n", FTP_PASSWORD_LEN);
                break;
            }
            memcpy(ftpUserName, phostnamehead + 1, i - 1);           // ftp user name
            memcpy(ftpUserPassword, phostnamehead + i + 1, datalen); // user  password
        }

        ret = true;
    } while (false);

    if (*(filePath + strlen((const char *)filePath) - 1) != '/')
    {
        memcpy((void *)(filePath + strlen((const char *)filePath)), (const void *)"/", 1); //  file path end with '/'
    }
    // memcpy((void*)(filePath+strlen((const char*)filePath)), (const void*)"/", 1);  //  file path end with '/'
    // DEBUG_MSG("<--serverAdd=%s, file path=%s, image filename=%s-->\r\n", ftpHost, ftpFilePath, appBin_fName);
    // DEBUG_MSG("<--ftp user name=%s, user password =%s  serverPort=%d-->\r\n", ftpUserName, ftpPassword, *serverPort);
    // DEBUG_MSG("<--serverAdd=%s\n", ftpHost);
    // DEBUG_MSG(" file path=%s\n", ftpFilePath);
    // DEBUG_MSG("image filename=%s\n", appBin_fName);
    // DEBUG_MSG("\r\n");
    // DEBUG_MSG("<--ftp user name=%s", ftpUserName);
    // DEBUG_MSG(" user password =%s", ftpPassword);
    // DEBUG_MSG("  serverPort=xxx-->\r\n");
    return ret;
}

void OtaFromUrl(const char *otaUrl)
{
    /*
        URL format
        HTTP: http://hostname:port/filePath/fileName
        FTP: ftp://hostname/filePath/fileName:port@username:password
    */

    if (otaUrl == 0)
        return;

    if ((0 != (strstr(otaUrl, "http://"))) || (0 != (strstr(otaUrl, "https://"))))
    {
        // add validations.
        // like if https, then set _sslRequired flag.. subscribe sslChannel etc..

        if (_ota_http_url)
            free(_ota_http_url);

        _ota_http_url = strdup(otaUrl);

        _httpDownloadFirmware = true;
        // _ftpDownloadFirmware = false; // consider latest request
        _sslRequired = false;

        // ToDo:
        // if 'https' session set _sslRequired flag and call ssl initializations
        // note that, after using ssl session, delete/un-subscribe particular channel.

        /*if(0 != (strstr(otaUrl, "https://")
        {
            // SSL Initializations if secure session

            _sslRequired = true;
            // _otaSslContext = SslChannelSubscribe(rootCACertificate, strlen(rootCACertificate), CLIENT_CERT_CRT, strlen(CLIENT_CERT_CRT),
            //                                   CLIENT_PRIVATE_KEY, strlen(CLIENT_PRIVATE_KEY), SSL_VER_ALL, All_CIPHER_SUITES, SERVER_CLIENT_AUTH);
        }*/
    }
    else if (0 != (strstr(otaUrl, "ftp")))
    {
        // extract ftp parameters from given url.

        char url[200];
        strcpy(url, otaUrl);

        uint8_t ftpHost[FTP_SERVERADD_LEN] = {0x0};    // ip string or domain name
        uint8_t ftpFilePath[FTP_FILEPATH_LEN] = {0x0}; // file in the server path
        uint8_t ftpUserName[FTP_USERNAME_LEN] = {0x0};
        uint8_t ftpPassword[FTP_PASSWORD_LEN] = {0x0};
        uint8_t appBin_fName[FTP_FILENAME_LEN] = {0x0};

        if (_FTP_DecodeURL((uint8_t *)url, strlen(url), ftpHost, &_ota_port,
                           ftpUserName, ftpPassword, ftpFilePath, appBin_fName))
        {
            // check if multiple files are need to download.
            // extract file extension.
            char *ptr;
            ptr = strstr((char *)appBin_fName, "bin");
            if (ptr == 0)
            {
                // unknown file.
                ptr = strstr((char *)appBin_fName, "BIN");
                if (ptr == 0)
                {
                    debugPrintln("[OTA] invalid file name");
                    return;
                }
            }
            // search '.' after 'bin'
            ptr = strchr(ptr, '.');

            if (ptr == 0)
            {
                // download whole file.
                _ftp_max_files_to_be_download = 1;
                _ftp_current_file_downloading_index = 1;
            }
            else
            {
                // multiple files need to download.

                *ptr = 0;                                               // terminate string at second '.'
                strcpy(_ftp_remote_fName_wo_ext, (char *)appBin_fName); // copy file name without extension. it will required ahead.

                ptr++; // move to next position

                // extract number of files to be download.
                int len = atoi(ptr);

                // check file extension. it must be like .001 or .002 or .003 or .012 or .999 etc
                int strLen = strlen(ptr);
                do
                {
                    if ((*ptr < '0') || (*ptr > '9'))
                    {
                        debugPrintln("[OTA] invalid file extension");
                        return;
                    }
                    strLen--;
                    ptr++; // move to next position

                } while (strLen > 0);
                _ftp_max_files_to_be_download = len;

                _ftp_current_file_downloading_index = 1; // start with first index
                strcat((char *)appBin_fName, ".001");
                debugPrintf("[OTA] total #%d files to be download\n", _ftp_max_files_to_be_download);
            }

            if (_ota_host)
                free(_ota_host);
            if (_ota_ftp_user)
                free(_ota_ftp_user);
            if (_ota_ftp_pass)
                free(_ota_ftp_pass);
            if (_ota_ftp_remote_filepath)
                free(_ota_ftp_remote_filepath);
            if (_ota_ftp_remote_filename)
                free(_ota_ftp_remote_filename);

            _ota_host = strdup((char *)ftpHost);
            _ota_ftp_user = strdup((char *)ftpUserName);
            _ota_ftp_pass = strdup((char *)ftpPassword);
            _ota_ftp_remote_filepath = strdup((char *)ftpFilePath);
            _ota_ftp_remote_filename = strdup((char *)appBin_fName);

            debugPrintln("[OTA] over FTP");
            debugPrintf("host=%s\r\n", _ota_host);
            debugPrintf("port=%d\r\n", _ota_port);
            debugPrintf("username=%s\r\n", _ota_ftp_user);
            debugPrintf("password=%s\r\n", _ota_ftp_pass);
            debugPrintf("filepath=%s\r\n", _ota_ftp_remote_filepath);
            debugPrintf("filename=%s\r\n", _ota_ftp_remote_filename);

            // create a empty temporary file in which firmware image will be downloaded.
            SPIFFS.remove(_spiffs_filename);
            delay(10);
            File file = SPIFFS.open(_spiffs_filename, "w+");
            if (file)
            {
                file.close();
                if (!SPIFFS.exists(_spiffs_filename))
                {
                    // this is not suppoased to happen.
                    // spiffs format required.
                    debugPrintln("[OTA] file not created. memory format required");
                    debugPrintln("[OTA] firmware download aborted, try another method");
                    // SPIFFS.format(); // this will clear all other files as well
                    // delay(500);
                    // SPIFFS.begin(true);
                    // file = SPIFFS.open(_spiffs_filename, "w+");
                    // file.close();

                    return;
                }

                _ftpDownloadFirmware = true;
                // _httpDownloadFirmware = false; // consider latest request
            }
            else
            {
                debugPrintln("[OTA] file not created");
            }
        }
    }
    else
    {
        debugPrintln("[OTA] url must start with http, https or ftp");
    }
}

void OtaInit(void)
{
#if 1
    ArduinoOTA.setPort(ARDUINO_OTA_PORT);
    ArduinoOTA.setHostname(gSysParam.imei);
    ArduinoOTA.setPassword("admin1234"); // apPass // password shall be hardcoded.

    ArduinoOTA.onStart([]()
                       {
                           odebugPrintln(PSTR("[OTA] Start"));
                           _otaCurrentMethod = OTA_OVER_ARDUINO; });

    ArduinoOTA.onEnd([]()
                     {
                         odebugPrintln(PSTR("\n[OTA] End"));
                         _otaCurrentMethod = OTA_OVER_NONE;
                         delay(100);
                         esp_restart();
                         delay(100); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { odebugPrintf(PSTR("[OTA] Progress: %u%%%%\r"), (progress / (total / 100))); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
                           _otaCurrentMethod = OTA_OVER_NONE;
                           odebugPrintf(PSTR("\n[OTA] Error #%u: "), error);
                           if (error == OTA_AUTH_ERROR)
                           {
                               odebugPrintln(PSTR("\n[OTA] Auth Failed"));
                           }
                           else if (error == OTA_BEGIN_ERROR)
                           {
                               odebugPrintln(PSTR("\n[OTA] Begin Failed"));
                           }
                           else if (error == OTA_CONNECT_ERROR)
                           {
                               odebugPrintln(PSTR("\n[OTA] Connect Failed"));
                           }
                           else if (error == OTA_RECEIVE_ERROR)
                           {
                               odebugPrintln(PSTR("\n[OTA] Receive Failed"));
                           }
                           else if (error == OTA_END_ERROR)
                           {
                               odebugPrintln(PSTR("\n[OTA] End Failed"));
                           } });

    ArduinoOTA.begin();
#endif

    /*if (_sslRequired)
    {
        // SSL Initializations if secure session, it will take some time.
        // _otaSslContext = SslChannelSubscribe(rootCACertificate, strlen(rootCACertificate), CLIENT_CERT_CRT, strlen(CLIENT_CERT_CRT),
        //                                   CLIENT_PRIVATE_KEY, strlen(CLIENT_PRIVATE_KEY), SSL_VER_ALL, All_CIPHER_SUITES, SERVER_CLIENT_AUTH);
    }*/
}

typedef enum
{
    OTA_SM_INIT = 0,
    OTA_SM_HTTP_EVENT,
    OTA_SM_WAIT_HTTP_PROCESS_COMPLETE,
    OTA_SM_FTP_EVENT,
    OTA_SM_WAIT_FTP_PROCESS_COMPLETE,

    OTA_SM_IDLE
} OTA_SM;

static uint8_t _otaState = OTA_SM_IDLE;
void OtaLoop(void)
{

    static uint32_t _generalTimeoutTick = 0;
    HttpStatus_t httpStatus;
    FtpStatus_t ftpStatus;
    uint8_t _ftpRetryCnt = 0;
    uint8_t _httpRetryCnt = 0;

    ArduinoOTA.handle();

    switch (_otaState)
    {
        // ======================== OTA HTTP PROCESSING =========================

    case OTA_SM_HTTP_EVENT: // start http download session
        // check if ota dowanload flag is set and not other ota operation is running.
        if (!(_httpDownloadFirmware || _otaCurrentMethod == OTA_OVER_NONE))
        {
            _otaState = OTA_SM_IDLE;
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

        if (!HttpSendData(_ota_http_url, _helperBuffer, strlen(_helperBuffer), _helperBuffer, sizeof(_helperBuffer), HTTP_METHOD_GET, HTTP_CONTENT_TEXT_PLAIN, _otaHttpCallBack, 0, 0))
        {
            odebugPrintln("[OTA] HTTP not initiated");
            _httpRetryCnt++;
            break;
        }
        memset(_helperBuffer, 0, sizeof(_helperBuffer));

        odebugPrintln("[OTA] Downloading Firmware");

        // clear the event
        // _httpDownloadFirmware = 0;

        _otaState = OTA_SM_WAIT_HTTP_PROCESS_COMPLETE;
        _generalTimeoutTick = millis();
        // _otaState = NC_HTTP_WAIT_FOR_FIRMWARE_INFO;
        break;

    case OTA_SM_WAIT_HTTP_PROCESS_COMPLETE: // wait till http session gets completed
        httpStatus = HttpGetLastStatus();
        if (httpStatus == HTTP_STATUS_SUCCESS)
        {
            odebugPrintln("[OTA] over HTTP Success");

            // clear the event
            _httpDownloadFirmware = false;
            _httpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.

            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
        }
        else if (httpStatus == HTTP_STATUS_FAILED)
        {
            _httpRetryCnt++; // try one more time
            odebugPrintf("[OTA] over HTTP fail, retry #%d", _httpRetryCnt);
            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
        }
        else if (httpStatus == HTTP_STATUS_DOWNLOADING)
        {
            _generalTimeoutTick = millis();
        }
        if ((millis() - _generalTimeoutTick) > 130000U)
        {
            _httpRetryCnt++; // try one more time
            odebugPrintf("[OTA] over HTTP timeout, retry #%d", _httpRetryCnt);
            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
            break;
        }
        break;

        // ======================== OTA FTP PROCESSING =========================

    case OTA_SM_FTP_EVENT: // start http download session
        // check if ota dowanload flag is set and not other ota operation is running.
        if (!(_ftpDownloadFirmware || _otaCurrentMethod == OTA_OVER_NONE))
        {
            _otaState = OTA_SM_IDLE;
            break;
        }

        // check if ftp client and modem avaiaibilty
        if (!(IsFtpClientAvailable() && IsModemReady()))
        {
            break;
        }

        if (_sslRequired)
        {
            // wait till sslchannel is ready.
            // otherwise session is going to be fail anyways.
        }

        if (!FtpInitiate(_ota_host, _ota_port, _ota_ftp_user, _ota_ftp_pass, _otaDwnldLocalFileName, _ota_ftp_remote_filename,
                         _ota_ftp_remote_filepath, FTP_DOWNLOAD_FILE, FTP_FILE_OVERWRITE, QUEC_FILE_MEM_RAM))
        {
            debugPrintln("[OTA] FTP Downloading not initiated");
            _ftpRetryCnt++;
            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
            break;
        }
        // _ftpDownloadFirmware = 0;
        debugPrintf("[OTA] FTP Downloading %s \n", _ota_ftp_remote_filename);
        _generalTimeoutTick = millis();
        _otaState = OTA_SM_WAIT_FTP_PROCESS_COMPLETE;
        break;

    case OTA_SM_WAIT_FTP_PROCESS_COMPLETE: // wait or ftp process get complete
        ftpStatus = FtpGetLastStatus();
        if (ftpStatus == FTP_STATUS_SUCCESS)
        {
            debugPrintln("[OTA] over FTP Download SUCCESS");

            _ftpRetryCnt = 0; // clear retry count
            // _ftpDownloadFirmware = false;

            QuecFileList(QUEC_FILE_MEM_RAM);
            QuecFileGetStorageSize(QUEC_FILE_MEM_RAM, NULL);

            // transfer Quectel RAM file content to local spiffs file.
            if (QuecFileDownload(_otaDwnldLocalFileName, (uint8_t *)_helperBuffer, sizeof(_helperBuffer), QUEC_FILE_MEM_RAM, _otaFiledownloader))
            {
                // the Quectel RAM file to SPIFFS file transfer is succeeded. try to download next file.
                if (_ftp_max_files_to_be_download == _ftp_current_file_downloading_index)
                {
                    debugPrintln("[OTA] all files downloaded. updating firmware");

                    // start updating firmware from local spiffs file.
                    _otaSpiffsToCoreTransfer();

                    // if everything we tried is acheived and we are calling auto reboot of device after successful upgrade
                    // then we shall not reach here,

                    // otherwise, we are here means our OTA is failed. clean up the things.
                    _ftpDownloadFirmware = false;
                    _ftpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.
                }
                else
                {
                    _ftp_current_file_downloading_index++; // move to next file index

                    // generate next file name to download
                    char appBin_fName[FTP_FILENAME_LEN] = {0x0};
                    sprintf(appBin_fName, "%s.%03d", _ftp_remote_fName_wo_ext, _ftp_current_file_downloading_index);

                    // reload new remote file name
                    if (_ota_ftp_remote_filename)
                        free(_ota_ftp_remote_filename);
                    _ota_ftp_remote_filename = strdup((char *)appBin_fName);

                    // _ftpDownloadFirmware = true;
                    // _httpDownloadFirmware = false;

                    debugPrintln("FTP Downloading next file");
                    _otaState = OTA_SM_FTP_EVENT;
                    break;
                }
            }
            else
            {
                // if RAM file to SPIFFS transefer fail, then no point going ahead.
                // FOTA will not be completed. I suggest abbort or restart whole procedure.
                debugPrintln("[OTA] local File download fail");
                debugPrintln("[OTA] over FTP aborted");

                // clean up the things
                _ftpDownloadFirmware = false;
                _ftpRetryCnt = 10; // dummy. to let clean up the things in IDLE state.
            }

            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
        }
        else if (ftpStatus == FTP_STATUS_FAILED)
        {
            debugPrintln("[OTA] over FTP fail");

            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
            _ftpRetryCnt++; // try one more time
        }

        if ((millis() - _generalTimeoutTick) > 125000)
        {
            debugPrintln("[OTA] over FTP timeout");

            _generalTimeoutTick = millis();
            _otaState = OTA_SM_IDLE;
            _ftpRetryCnt++; // try one more time
            break;
        }
        break;

    default:
        if ((millis() - _generalTimeoutTick) > 10000U)
        {
            _generalTimeoutTick = millis();

            if (_httpRetryCnt >= 3)
            {
                _httpRetryCnt = 0;
                _httpDownloadFirmware = false;
                if (_otaCurrentMethod == OTA_OVER_HTTP)
                {
                    _otaCurrentMethod = OTA_OVER_NONE;
                }

                if (_sslRequired)
                {
                    _sslRequired = false;

                    // delete/un-subscribe ssl channel
                }

                free(_ota_http_url);
                _ota_http_url = NULL;
            }

            if (_ftpRetryCnt >= 3)
            {
                _ftpRetryCnt = 0; // clear retry count
                if ((_otaCurrentMethod == OTA_OVER_FTP_MULTI_FILE) ||
                    (_otaCurrentMethod == OTA_OVER_FTP_SINGLE_FILE))
                {
                    _otaCurrentMethod = OTA_OVER_NONE;
                }
                _ftpDownloadFirmware = false; // stop downloading

                // if (_ota_host)
                free(_ota_host);
                _ota_host = NULL;
                // if (_ota_ftp_user)
                free(_ota_ftp_user);
                _ota_ftp_user = NULL;
                // if (_ota_ftp_pass)
                free(_ota_ftp_pass);
                _ota_ftp_pass = NULL;
                // if (_ota_ftp_remote_filepath)
                free(_ota_ftp_remote_filepath);
                _ota_ftp_remote_filepath = NULL;
                // if (_ota_ftp_remote_filename)
                free(_ota_ftp_remote_filename);
                _ota_ftp_remote_filename = NULL;
            }

            // Note that we have checking http flag is first and then ftp.
            // because http is straight process, there are no multiple files to download.
            // even if we start http process in between of partial ftp files download.
            // we can resume it later after http process gets fail.
            // if http process succedded then we do not even require FTP process to continue.
            // because device will be rstarted after firmware updated.

            // check if ota download flag is set.
            if (_httpDownloadFirmware && _otaCurrentMethod == OTA_OVER_NONE)
            {
                _otaState = OTA_SM_HTTP_EVENT;
                break;
            }

            if (_ftpDownloadFirmware && _otaCurrentMethod == OTA_OVER_NONE)
            {
                _otaState = OTA_SM_FTP_EVENT;
                break;
            }

            _otaState = OTA_SM_IDLE;
        }
        break;

    } // switch (_otaState)
}

int OtaIsInProgress()
{
    return (_otaState != OTA_SM_IDLE);
}

int OtaGetPercentage()
{
    if (_otaState != OTA_SM_IDLE)
    {
        return percentageOfOta;
    }
    else
    {
        return 0;
    }
    return 0;
}

int OtaIsPercentChanged(int *pPercentageOfOta)
{
    // check if the otaUpdateFailedFlag is set and notifiy the app
    if (systemFlags.otaUpdateFailedFlag)
    {
        systemFlags.otaUpdateFailedFlag = false;
        percentageOfOta = 0;
        return -1;
    }

    if (1 == OtaIsInProgress())
    {
        // code to detect ota percentage change
        if (percentageOfOta != percentageOfOtaOld)
        {
            percentageOfOtaOld = percentageOfOta;
            *pPercentageOfOta = percentageOfOta;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return 0;
    }
}

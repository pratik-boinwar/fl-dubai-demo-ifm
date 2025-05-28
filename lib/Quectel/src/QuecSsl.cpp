/**
 * @file       QuecSsl.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecSsl.h"

const char nvramCA[] = "NVRAM:CA0";         // CA certificate store in NVRAM
const char nvramCC[] = "NVRAM:CC0";         // Client certificate store in NVRAM
const char nvramCK[] = "NVRAM:CK0";         // Client Key store in NVRAM
const char ramCA[] = "RAM:ca_cert.pem";     // CA certificate store in VRAM
const char ramCC[] = "RAM:client_cert.pem"; // Client certificate store in VRAM
const char ramCK[] = "RAM:client_key.pem";  // Client Key store in VRAM

#define MAX_SSL_CHANNELS 2     // DO NOT CHANGE. THIS LIBRARY DESIGNED TO HANDLE MAX 2 CHANNELS
#define MAX_SSL_FILE_SIZE 2017 // NVRAM can hold 2017 bytes, RAM can hold 32768 bytes but limited to 2017 bytes commonly

/**
 * CertStorage_t, certificates and key storage area
 */
typedef enum _CertStorage
{
    Ql_RAM = 0,
    Ql_NVRAM = 1,
    QL_FAT = 0xFF // not supported
} CertStorage_t;

/**
 * SslCnxStatus_t, varoius statuses of ssl configuration process
 */
typedef enum _SslCnxStatus
{
    CNX_INIT = 0,
    CNX_CHECK_CERT,
    CNX_LOAD_CA_CERT,
    CNX_LOAD_CLIENT_CERT,
    CNX_LOAD_CLIENT_KEY,
    CNX_DELETE_ALL_CERT,
    CNX_RENEW_ALL_CERT,
    CNX_CONFIG_CONTEXT,
    CNX_IDLE
} SslCnxStatus_t;

/**
 * SslChannel_t, structure holding various parameters of SSL channels
 */
typedef struct _SslChannel_t
{
    const char *certCA;
    const char *certClient;
    const char *clientKey;
    uint32_t certCAsize;
    uint32_t certClientSize;
    uint32_t clientKeySize;
    SslVersion_t version;
    SslCipherSuite_t cipher;
    SslSecurityLevel_t secLevel;
    CertStorage_t storageMemory;
    SslCnxStatus_t cnxStatus;
    bool isValid;
    bool isConfigured;
} SslChannel_t;

/**
 * SslState_t, various internal processing states
 */
typedef enum _SslState
{
    SSL_SM_NULL = 0,
    SSL_SM_INIT,
    SSL_SM_CHECK_EVENT,

    SSL_SM_LOAD_CA_CERT,
    SSL_SM_LOAD_CLIENT_CERT,
    SSL_SM_LOAD_CLIENT_KEY,

    SSL_SM_CONFIG_CONTEXT,
    SSL_SM_GENERIC_DELAY
} SslState_t;

//holds current pprcessing state
static SslState_t _sslState = SSL_SM_INIT;
//holds data of the SSL Channel
static SslChannel_t sslChannel[MAX_SSL_CHANNELS];

/**
 * @fn static int8_t ConfigContext(uint8_t handle)
 * @brief This function decides memory area of certificates and configures SSL context.
 * @param uint8_t handle, ssl channel
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark 
 */
static int8_t ConfigContext(uint8_t handle)
{
    int8_t ret;

    const char *nameCACert;
    const char *nameClientCert;
    const char *nameClientkey;

    if (sslChannel[handle].storageMemory == Ql_NVRAM)
    {
        nameCACert = nvramCA;
        nameClientCert = nvramCC;
        nameClientkey = nvramCK;
    }
    else
    {
        nameCACert = ramCA;
        nameClientCert = ramCC;
        nameClientkey = ramCK;
    }

    ret = SslConfigContext(handle, sslChannel[handle].version, sslChannel[handle].cipher, sslChannel[handle].secLevel,
                           nameCACert, nameClientCert, nameClientkey);

    return ret;
}

/**
 * @fn static int16_t WriteCACertificate(uint8_t handle)
 * @brief This function decides memory area of certificates and write CA certifcate.
 * @param uint8_t handle, ssl channel
 * @return -ve number: validation fail, 0: finish with fail, +ve number: success, bytes written
 * @remark 
 */
static int16_t WriteCACertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_AUTH) || (sslChannel[handle].secLevel == SERVER_CLIENT_AUTH))
    {
        int16_t ret;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
            ret = SslWriteCertificate(nvramCA, sslChannel[handle].certCA, sslChannel[handle].certCAsize);
        else
            ret = SslWriteCertificate(ramCA, sslChannel[handle].certCA, sslChannel[handle].certCAsize);

        return ret;
    }

    return true;
}

/**
 * @fn static int16_t WriteClientCertificate(uint8_t handle)
 * @brief This function decides memory area of certificates and write Client certifcate.
 * @param uint8_t handle, ssl channel
 * @return -ve number: validation fail, 0: finish with fail, +ve number: success, bytes written
 * @remark 
 */
static int16_t WriteClientCertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) // || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int16_t ret;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
            ret = SslWriteCertificate(nvramCC, sslChannel[handle].certClient, sslChannel[handle].certClientSize);
        else
            ret = SslWriteCertificate(ramCC, sslChannel[handle].certClient, sslChannel[handle].certClientSize);

        return ret;
    }

    return true;
}

/**
 * @fn static int16_t WriteClientKey(uint8_t handle)
 * @brief This function decides memory area of key and write Client Key.
 * @param uint8_t handle, ssl channel
 * @return -ve number: validation fail, 0: finish with fail, +ve number: success, bytes written
 * @remark 
 */
static int16_t WriteClientKey(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) //  || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int16_t ret;
        if (sslChannel[handle].storageMemory == Ql_NVRAM)
            ret = SslWriteCertificate(nvramCK, sslChannel[handle].clientKey, sslChannel[handle].clientKeySize);
        else
            ret = SslWriteCertificate(ramCK, sslChannel[handle].clientKey, sslChannel[handle].clientKeySize);
        return ret;
    }

    return true;
}

/**
 * @fn static bool CheckCACertificate(uint8_t handle)
 * @brief This function decides memory area of certificate and check CA certifcate.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool CheckCACertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_AUTH) || (sslChannel[handle].secLevel == SERVER_CLIENT_AUTH))
    {
        int16_t status;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //chek server certificate in NVRAM
            status = SslReadCertificate(nvramCA);
            if (status <= 0)
                return false;
        }
        else
        {
            //chek server certificate in RAM
            status = SslReadCertificate(ramCA);
            if (status <= 0)
                return false;
        }
    }
    return true;
}

/**
 * @fn static bool CheckClientCertificate(uint8_t handle)
 * @brief This function decides memory area of certificate and check Client certifcate.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool CheckClientCertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) // || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int status;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //cehck client certificate in NVRAM
            status = SslReadCertificate(nvramCC);
            if (status <= 0)
                return false;
        }
        else
        {
            //cehck client certificate in RAM
            status = SslReadCertificate(ramCC);
            if (status <= 0)
                return false;
        }
    }
    return true;
}

/**
 * @fn static bool CheckClientKey(uint8_t handle)
 * @brief This function decides memory area of key and check Client Key.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool CheckClientKey(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) // || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int status;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //check client key in NVRAM
            status = SslReadCertificate(nvramCK);
            if (status <= 0)
                return false;
        }
        else
        {
            //check client key in RAM
            status = SslReadCertificate(ramCK);
            if (status <= 0)
                return false;
        }
    }

    return true;
}

/**
 * @fn static bool CheckAllCertificates(uint8_t handle)
 * @brief This function checking all Certificates and Client Key.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool CheckAllCertificates(uint8_t handle)
{
    if (!CheckCACertificate(handle))
        return false;

    if (!CheckClientCertificate(handle))
        return false;

    if (!CheckClientKey(handle))
        return false;

    return true;
}

/**
 * @fn static bool DeleteCACertificate(uint8_t handle)
 * @brief This function decides memory area of certificate and deleting CA certifcate.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool DeleteCACertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_AUTH) || (sslChannel[handle].secLevel == SERVER_CLIENT_AUTH))
    {
        int8_t ret;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //chek server certificate
            ret = SslDeleteCertificate(nvramCA);
            if (ret <= 0)
                return false;
        }
        else
        {
            //chek server certificate
            ret = SslDeleteCertificate(ramCA);
            if (ret <= 0)
                return false;
        }
    }

    return true;
}

/**
 * @fn static bool DeleteClientCertificate(uint8_t handle)
 * @brief This function decides memory area of certificate and deleting Client certifcate.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool DeleteClientCertificate(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) // || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int8_t ret;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //cehck client certificate
            ret = SslDeleteCertificate(nvramCC);
            if (ret <= 0)
                return false;
        }
        else
        {
            //cehck client certificate
            ret = SslDeleteCertificate(ramCC);
            if (ret <= 0)
                return false;
        }
    }

    return true;
}

/**
 * @fn static bool DeleteClientKey(uint8_t handle)
 * @brief This function decides memory area of certificate and deleting Client Key.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
static bool DeleteClientKey(uint8_t handle)
{
    if ((sslChannel[handle].secLevel == SERVER_CLIENT_AUTH)) // || (sslChannel[handle].secLevel == SERVER_AUTH)
    {
        int8_t ret;

        if (sslChannel[handle].storageMemory == Ql_NVRAM)
        {
            //check client key
            ret = SslDeleteCertificate(nvramCK);
            if (ret <= 0)
                return false;
        }
        else
        {
            //check client key
            ret = SslDeleteCertificate(ramCK);
            if (ret <= 0)
                return false;
        }
    }

    return true;
}

/**
 * @fn static bool DeleteAllCertificates(uint8_t handle)
 * @brief This function deleting all Certificates and Client Key.
 * @param uint8_t handle, ssl channel
 * @return false: fail, true: success
 * @remark 
 */
bool DeleteAllCertificates(uint8_t handle)
{
    if (!DeleteCACertificate(handle))
        return false;

    if (!DeleteClientCertificate(handle))
        return false;

    if (!DeleteClientKey(handle))
        return false;

    return true;
}

/**
 * @fn static void SslManualDisconnection(void)
 * @brief This function is being called to set/clear associated flags to re-initiated process.
 * @remark 
 */
static void SslManualDisconnection(void)
{
    static bool _firstTime = true;

    DBG("[SSL] manual disconnection\n");

    for (uint8_t handle = 0; handle < MAX_SSL_CHANNELS; handle++)
    {
        sslChannel[handle].isConfigured = false;
        if (_firstTime == true)
        {
            _firstTime = false;
            sslChannel[handle].cnxStatus = CNX_RENEW_ALL_CERT;
        }
        else
        {
            sslChannel[handle].cnxStatus = CNX_CHECK_CERT;
        }
    }
}

/**
 * @fn void QuectelSslSm(void)
 * @brief This function handles the process of SSL channel configurations.
 * @remark this function is required to be called frequently. 
 *         Do not make direct call. This is called within QuectelLoop() function
 */
void QuectelSslSm(void)
{
    static int8_t _curSslHandle = -1;
    static uint32_t _generalTimeoutTick = 0;

    if (!IsATavailable())
        return;

    // if modem is initializing then wait till finish it
    // and re-init SSL
    if (GetModemCurrentOp() == MDM_INITIALIZING)
        _sslState = SSL_SM_INIT;

    if (!IsModemReady())
        return;

    switch (_sslState)
    {
    case SSL_SM_INIT: // wait till gprs connected
        if (!IsGprsConnected())
        {
            break;
        }

        //clear all flags.
        SslManualDisconnection();

        _curSslHandle = -1;
        _sslState = SSL_SM_CHECK_EVENT;
        break;

    case SSL_SM_CHECK_EVENT:

        // if (!IsATavailable())
        //     break;

        // if (!IsModemReady())
        //     break;

        if (!IsGprsConnected())
        {
            _sslState = SSL_SM_INIT;
            break;
        }

        if (++_curSslHandle >= MAX_SSL_CHANNELS)
        {
            // check ssl config here if required
            _curSslHandle = -1;
            _sslState = SSL_SM_GENERIC_DELAY;
            _generalTimeoutTick = millis();
        }

        if (sslChannel[_curSslHandle].isValid)
        {
            if (sslChannel[_curSslHandle].cnxStatus == CNX_RENEW_ALL_CERT)
            {
                DeleteAllCertificates(_curSslHandle);
                sslChannel[_curSslHandle].cnxStatus = CNX_LOAD_CA_CERT;
                sslChannel[_curSslHandle].isConfigured = false;
            }

            if (sslChannel[_curSslHandle].isConfigured)
            {
                if (sslChannel[_curSslHandle].cnxStatus == CNX_DELETE_ALL_CERT)
                {
                    if (DeleteAllCertificates(_curSslHandle))
                    {
                        sslChannel[_curSslHandle].isValid = false;
                        sslChannel[_curSslHandle].isConfigured = false;
                    }
                }
            }
            else
            {
                if (sslChannel[_curSslHandle].cnxStatus == CNX_CHECK_CERT)
                {
                    if (CheckAllCertificates(_curSslHandle))
                    {
                        // if all certificates are intacet
                        //sslChannel[_curSslHandle].isConfigured = true;
                        sslChannel[_curSslHandle].cnxStatus = CNX_CONFIG_CONTEXT;
                    }
                    else
                    {
                        // reload certificates
                        sslChannel[_curSslHandle].cnxStatus = CNX_LOAD_CA_CERT;
                        _sslState = SSL_SM_LOAD_CA_CERT;
                    }
                }
                else if (sslChannel[_curSslHandle].cnxStatus == CNX_LOAD_CA_CERT)
                {
                    _sslState = SSL_SM_LOAD_CA_CERT;
                }
                else if (sslChannel[_curSslHandle].cnxStatus == CNX_LOAD_CLIENT_CERT)
                {
                    _sslState = SSL_SM_LOAD_CLIENT_CERT;
                }
                else if (sslChannel[_curSslHandle].cnxStatus == CNX_LOAD_CLIENT_KEY)
                {
                    _sslState = SSL_SM_LOAD_CLIENT_KEY;
                }
                else if (sslChannel[_curSslHandle].cnxStatus == CNX_CONFIG_CONTEXT)
                {
                    _sslState = SSL_SM_CONFIG_CONTEXT;
                }
                else if (sslChannel[_curSslHandle].cnxStatus == CNX_INIT)
                {
                    // this can happen when new channel subscribed after SSL init
                    // and power cycle
                    sslChannel[_curSslHandle].cnxStatus = CNX_CHECK_CERT;
                }
            }
        }
        _generalTimeoutTick = millis();
        break;

    case SSL_SM_LOAD_CA_CERT: // load CA certificate
        if (IsATavailable())
        {
            //safe side we blindly try to delete existing one if not previously
            DeleteCACertificate(_curSslHandle);

            if (WriteCACertificate(_curSslHandle) > 0)
            {
                sslChannel[_curSslHandle].cnxStatus = CNX_LOAD_CLIENT_CERT;
                _sslState = SSL_SM_LOAD_CLIENT_CERT;
                _generalTimeoutTick = millis();
            }
            else
                _sslState = SSL_SM_GENERIC_DELAY; //_sslState = SSL_SM_CHECK_EVENT;
        }
        if (millis() - _generalTimeoutTick > 10000)
        {
            // try later
            _sslState = SSL_SM_CHECK_EVENT;
        }
        break;

    case SSL_SM_LOAD_CLIENT_CERT: // load Client certificate
        if (IsATavailable())
        {
            //safe side we blindly try to delete existing one if not previously
            DeleteClientCertificate(_curSslHandle);

            if (WriteClientCertificate(_curSslHandle) > 0)
            {
                sslChannel[_curSslHandle].cnxStatus = CNX_LOAD_CLIENT_KEY;
                _sslState = SSL_SM_LOAD_CLIENT_KEY;
                _generalTimeoutTick = millis();
            }
            else
                _sslState = SSL_SM_GENERIC_DELAY; //_sslState = SSL_SM_CHECK_EVENT;
        }
        if (millis() - _generalTimeoutTick > 10000)
        {
            // try later
            _sslState = SSL_SM_CHECK_EVENT;
        }
        break;

    case SSL_SM_LOAD_CLIENT_KEY: // load Client Key certificate
        if (IsATavailable())
        {
            //safe side we blindly try to delete existing one if not previously
            DeleteClientKey(_curSslHandle);

            if (WriteClientKey(_curSslHandle) > 0)
            {
                sslChannel[_curSslHandle].cnxStatus = CNX_CONFIG_CONTEXT;
                _sslState = SSL_SM_CONFIG_CONTEXT;
                _generalTimeoutTick = millis();
            }
            else
                _sslState = SSL_SM_GENERIC_DELAY; //_sslState = SSL_SM_CHECK_EVENT;
        }
        if (millis() - _generalTimeoutTick > 10000)
        {
            // try later
            _sslState = SSL_SM_CHECK_EVENT;
        }
        break;

    case SSL_SM_CONFIG_CONTEXT:
        if (IsATavailable())
        {
            if (ConfigContext(_curSslHandle))
            {
                // greate, we have configured ssl channel and context for it
                sslChannel[_curSslHandle].isConfigured = true;
                sslChannel[_curSslHandle].cnxStatus = CNX_IDLE;
                _sslState = SSL_SM_CHECK_EVENT;
            }
            else
            {
                _sslState = SSL_SM_GENERIC_DELAY; //_sslState = SSL_SM_CHECK_EVENT;
            }
        }
        if (millis() - _generalTimeoutTick > 10000)
        {
            // try later
            _sslState = SSL_SM_CHECK_EVENT;
        }
        break;

    case SSL_SM_GENERIC_DELAY:
        if (millis() - _generalTimeoutTick > 30000)
        {
            _sslState = SSL_SM_CHECK_EVENT;
        }
        break;

    default:
        _sslState = SSL_SM_CHECK_EVENT;
        break;
    }
}

/**
 * @fn SslContextHandle SslChannelSubscribe(const char *certCA, uint32_t certCAsize, const char *certClient, uint32_t certClientsize,
 *                                          const char *clientKey, uint32_t clientKeySize, SslVersion_t version, SslCipherSuite_t cipher,
 *                                          SslSecurityLevel_t secLevel)
 * @brief This function creates new SSL channel using given parameters
 * @param const char *certCA, pointer to data buffer holding server/CA certificate
 * @param uint32_t certCAsize, size of server/CA certificate
 * @param const char *certClient, pointer to data buffer holding client certificate 
 * @param uint32_t certClientsize, size of client certificate
 * @param const char *clientKey, pointer to data buffer holding client key
 * @param  uint32_t clientKeySize, size of client key
 * @param SslVersion_t version, ssl version
 * @param SslCipherSuite_t cipher, ssl cipher suite
 * @param SslSecurityLevel_t secLevel, ssl security level
 * @return SslContextHandle handle, -ve number: validation fail, SSL channel identifier: success
 * @remark user need to hold certCA, certClient, clientKey in buffer all the time.
 *         process automatically re-configures ssl in case of module restarts.
 */
SslContextHandle SslChannelSubscribe(const char *certCA, uint32_t certCAsize, const char *certClient, uint32_t certClientsize,
                                     const char *clientKey, uint32_t clientKeySize, SslVersion_t version, SslCipherSuite_t cipher,
                                     SslSecurityLevel_t secLevel)
{

    SslContextHandle handle = 0;

    while (true == sslChannel[handle].isValid)
    {
        handle++;
        if (handle >= MAX_SSL_CHANNELS)
        {
            DBG("\r\nSSL_ERROR: No SSL Channel Left\r\n");
            return -1;
        }
    }

    //validate
    if (secLevel == NO_AUTH)
    {
        //ToDo:
        //need more exploration, what is NO_AUTH level and how module behave with this
        return -3;
    }
    else if (secLevel == SERVER_AUTH)
    {
        if (certCA == NULL)
            return -2;
    }
    else if (secLevel == SERVER_CLIENT_AUTH)
    {
        if (certCA == NULL)
            return -2;

        if (certClient == NULL)
            return -2;

        if (clientKey == NULL)
            return -2;
    }

    if ((certCAsize == 0) || (certCAsize > MAX_SSL_FILE_SIZE))
        return -2;

    if ((certClientsize == 0) || (certClientsize > MAX_SSL_FILE_SIZE))
        return -2;

    if ((clientKeySize == 0) || (clientKeySize > MAX_SSL_FILE_SIZE))
        return -2;

    sslChannel[handle].certCA = certCA;
    sslChannel[handle].certCAsize = certCAsize;
    sslChannel[handle].certClient = certClient;
    sslChannel[handle].certClientSize = certClientsize;
    sslChannel[handle].clientKey = clientKey;
    sslChannel[handle].clientKeySize = clientKeySize;
    sslChannel[handle].version = version;
    sslChannel[handle].cipher = cipher;
    sslChannel[handle].secLevel = secLevel;

    // we can store only one set of certs in NVRAM due to requirement of specific file name.
    if (0 == handle)
        sslChannel[handle].storageMemory = Ql_NVRAM;
    else
        sslChannel[handle].storageMemory = Ql_RAM;

    // if (secLevel == NO_AUTH)
    // {
    //     //ToDo:
    //     //need more exploration, what is NO_AUTH level and how module behave with this
    //     sslChannel[handle].isConfigured = true;
    //     sslChannel[handle].cnxStatus = CNX_IDLE;
    // }
    // else
    {
        sslChannel[handle].isConfigured = false;
        sslChannel[handle].cnxStatus = CNX_INIT;
    }

    sslChannel[handle].isValid = true;

    return handle;
}

/**
 * @fn int8_t SslRenewChannelSubscribed(SslContextHandle handle, const char *certCA, uint32_t certCAsize, const char *certClient, uint32_t certClientsize,
 *                                      const char *clientKey, uint32_t clientKeySize, SslVersion_t version, SslCipherSuite_t cipher,
 *                                      SslSecurityLevel_t secLevel)
 * @brief This function updates exisiting SSL channel with given parameters
 * @param SslContextHandle handle, SSL channel identifier
 * @param const char *certCA, pointer to data buffer holding server/CA certificate
 * @param uint32_t certCAsize, size of server/CA certificate
 * @param const char *certClient, pointer to data buffer holding client certificate 
 * @param uint32_t certClientsize, size of client certificate
 * @param const char *clientKey, pointer to data buffer holding client key
 * @param  uint32_t clientKeySize, size of client key
 * @param SslVersion_t version, ssl version
 * @param SslCipherSuite_t cipher, ssl cipher suite
 * @param SslSecurityLevel_t secLevel, ssl security level
 * @return SslContextHandle handle, -ve number: validation fail, SSL channel identifier: success
 * @remark user need to hold certCA, certClient, clientKey in buffer all the time.
 *         process automatically re-configures ssl in case of module restarts.
 *         call this function when it happens to change parameters in running program
 */
int8_t SslRenewChannelSubscribed(SslContextHandle handle, const char *certCA, uint32_t certCAsize, const char *certClient, uint32_t certClientsize,
                                 const char *clientKey, uint32_t clientKeySize, SslVersion_t version, SslCipherSuite_t cipher,
                                 SslSecurityLevel_t secLevel)
{
    if (handle < 0 || handle >= MAX_SSL_CHANNELS)
        return -1;

    if (!sslChannel[handle].isValid)
    {
        // channel is previously not subscribed
        return -2;
    }

    //validate
    if (secLevel == NO_AUTH)
    {
        //ToDo:
        //need more exploration, what is NO_AUTH level and how module behave with this
        return -3;
    }
    else if (secLevel == SERVER_AUTH)
    {
        if (certCA == NULL)
            return -2;
    }
    else if (secLevel == SERVER_CLIENT_AUTH)
    {
        if (certCA == NULL)
            return -2;

        if (certClient == NULL)
            return -2;

        if (clientKey == NULL)
            return -2;
    }

    if ((certCAsize == 0) || (certCAsize > MAX_SSL_FILE_SIZE))
        return -2;

    if ((certClientsize == 0) || (certClientsize > MAX_SSL_FILE_SIZE))
        return -2;

    if ((clientKeySize == 0) || (clientKeySize > MAX_SSL_FILE_SIZE))
        return -2;

    sslChannel[handle].certCA = certCA;
    sslChannel[handle].certCAsize = certCAsize;
    sslChannel[handle].certClient = certClient;
    sslChannel[handle].certClientSize = certClientsize;
    sslChannel[handle].clientKey = clientKey;
    sslChannel[handle].clientKeySize = clientKeySize;
    sslChannel[handle].version = version;
    sslChannel[handle].cipher = cipher;
    sslChannel[handle].secLevel = secLevel;

    // we can store only one set of certs in NVRAM due to requirement of specific file name.
    // if (0 == handle)
    //     sslChannel[handle].storageMemory = Ql_NVRAM;
    // else
    //     sslChannel[handle].storageMemory = Ql_RAM;

    // if (secLevel == NO_AUTH)
    // {
    //     //ToDo:
    //     //need more exploration, what is NO_AUTH level and how module behave with this
    //     sslChannel[handle].isConfigured = true;
    //     sslChannel[handle].cnxStatus = CNX_IDLE;
    // }
    // else
    {
        sslChannel[handle].isConfigured = false;
        sslChannel[handle].cnxStatus = CNX_RENEW_ALL_CERT;
    }

    return 1;
}

/**
 * @fn bool IsSslChannelAvailable(SslContextHandle handle)
 * @brief This function returns the configuration status of SSL channel
 * @param SslContextHandle handle, SSL channel identifier
 * @return false: not available, true: available
 * @remark user need to first subscribe SSL channel
 */
bool IsSslChannelAvailable(SslContextHandle handle)
{
    if (handle < 0 || handle >= MAX_SSL_CHANNELS)
        return 0;

    return sslChannel[handle].isConfigured;
}

/**
 * @fn int16_t SslWriteCertificate(const char *certName, const char *data, int16_t dataLen)
 * @brief This function writing the certifacte to memory
 * @param const char *certName, pointer to certificate name.
 * @param const char *data, pointer to certificate data
 * @param int16_t dataLen, certificate data size
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int16_t SslWriteCertificate(const char *certName, const char *data, int16_t dataLen)
{
    /*Response
    CONNECT
    ..... data upload ..... 
    +QSECWRITE: <uploadsize>,<checksum>
    OK
    */

    if (!IsATavailable())
        return -1;

    if (certName == NULL)
        return -2;

    if (data == NULL)
        return -3;

    // NVRAM can hold 2017 bytes, RAM can hold 32768 bytes
    if (dataLen == 0 || dataLen > MAX_SSL_FILE_SIZE)
        return -4;

    char strAT[100];

    //AT+QSECWRITE=<filename>,<filesize> [,<timeout>]
    sprintf(strAT, "+QSECWRITE=\"%s\",%d,10", certName, dataLen);
    sendAT(strAT);

    int8_t ret = waitResponse2(2000, GSM_CONNECT, GSM_ERROR);
    if (ret != 1)
    {
        // escape from data mode, if we missed the CONNECT
        streamAT().print("+++");
        waitResponse3(GSM_OK); // read remianing
        return 0;
    }

    int16_t Bytes = streamAT().write(data, dataLen);
    if (Bytes != dataLen)
    {
        // mismatched written bytes
        // escape from data mode, if module is waiting more bytes
        streamAT().print("+++");
        waitResponse3(GSM_OK); // read remianing
        return 0;
    }

    // expecting +QSECWRITE: <uploadsize>,<checksum>
    ret = waitResponse2(2000, "+QSECWRITE: ", GSM_ERROR);
    if (1 == ret)
    {
        Bytes = streamGetIntBefore(',');
        if (Bytes == dataLen)
        {
            //uploadsize and dataLen matched. greate.

            //we can retrive checksum as well to more validation,
            //int16_t checksum = streamGetIntBefore('\r');  //note that checksum is in Hex and it may compute as -ve number for 'return'

            waitResponse3(GSM_OK); // read remianing

            // return checksum;
            return Bytes; // returning number of bytes written which is +ve number
        }
    }

    return 0;
}

/**
 * @fn int16_t SslReadCertificate(const char *certName)
 * @brief This function checks the status of the certifacte in memory
 * @param const char *certName, pointer to certificate name.
 * @return -ve number: validation fail, 0: finish with fail, 1: file intact
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int16_t SslReadCertificate(const char *certName)
{
    /*Response
    +QSECREAD: 1,6640
    */

    if (!IsATavailable())
        return -1;

    char strAT[100];

    sprintf(strAT, "+QSECREAD=\"%s\"", certName);
    sendAT(strAT);
    int8_t ret = waitResponse3("+QSECREAD: ", GSM_ERROR);
    if (1 == ret)
    {
        // received +QSECREAD: <good>,<checksum>
        int16_t status = streamGetIntBefore(',');
        if (0 == status)
        {
            // certificate or key is wrong
            return 0;
        }
        // int16_t checksum = streamGetIntBefore('\r');
        waitResponse3(GSM_OK); // read remianing
        // return checksum;
        return status;
    }

    return 0;
}

/**
 * @fn int8_t SslDeleteCertificate(const char *certName)
 * @brief This function deletes the certifacte from memory
 * @param const char *certName, pointer to certificate name.
 * @return -ve number: validation fail, 0: finish with fail, 1: file intact
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int8_t SslDeleteCertificate(const char *certName)
{
    /*Response
    OK
    */

    if (!IsATavailable())
        return -1;

    char strAT[100];

    sprintf(strAT, "+QSECDEL=\"%s\"", certName);
    sendAT(strAT);
    int8_t ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (1 == ret)
    {
        return 1;
    }

    return 0;
}

/**
 * @fn int8_t SslConfigContext(uint8_t contextID, SslVersion_t sslVersion, SslCipherSuite_t cipherSuite, 
 *                             SslSecurityLevel_t secLevel, const char *nameCACert, const char *nameClientCert,
 *                             const char *nameClientkey)
 * @brief This function configures the SSL context for the given contextID with given parameters
 * @param uint8_t contextID, SSL context ID to be configured
 * @param SslVersion_t sslVersion, SSL version
 * @param SslCipherSuite_t cipherSuite, SSL cipher suite
 * @param const char *nameCACert, pointer to server/CA certificate name stored in meomry
 * @param const char *nameClientCert, pointer to client certificate name stored in memory
 * @param const char *nameClientkey, pointer to client key name stored in memory
 * @return -ve number: validation fail, 0: finish with fail, 1: file intact
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 *         certificates and keys must be already written in appropriate memory area.
 */
int8_t SslConfigContext(uint8_t contextID, SslVersion_t sslVersion, SslCipherSuite_t cipherSuite, SslSecurityLevel_t secLevel,
                        const char *nameCACert, const char *nameClientCert, const char *nameClientkey)
{
    if (!IsATavailable())
        return -1;

    int8_t ret;
    sendAT("+QSSLCFG=\"ignorertctime\",1");
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    char strAT[100];

    sprintf(strAT, "+QSSLCFG=\"sslversion\",%d,%d", contextID, sslVersion);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    sprintf(strAT, "+QSSLCFG=\"ciphersuite\",%d,\"0x%04X\"", contextID, cipherSuite);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    sprintf(strAT, "+QSSLCFG=\"seclevel\",%d,%d", contextID, secLevel);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    sprintf(strAT, "+QSSLCFG=\"cacert\",%d,\"%s\"", contextID, nameCACert);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    sprintf(strAT, "+QSSLCFG=\"clientcert\",%d,\"%s\"", contextID, nameClientCert);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    sprintf(strAT, "+QSSLCFG=\"clientkey\",%d,\"%s\"", contextID, nameClientkey);
    sendAT(strAT);
    ret = waitResponse3(GSM_OK, GSM_ERROR);
    if (ret != 1)
    {
        return 0;
    }

    return 1;
}

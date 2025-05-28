/**
 * @file       QuecSsl.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECSSL_H
#define QUECSSL_H

/**
 * SslVersion_t, available SSL versions
 */
typedef enum _SslVersion
{
    SSL_VER_3_0 = 0, // SSL 3.0
    SSL_VER_1_0,     // TLS 1.0
    SSL_VER_1_1,     // TLS 1.1
    SSL_VER_1_2,     // TLS 1.2
    SSL_VER_ALL      // All Support
} SslVersion_t;

/**
 * SslCipherSuite_t, available cipher suites
 */
typedef enum _SslCipherSuite
{
    TLS_RSA_WITH_AES_256_CBC_SHA = 0X0035,
    TLS_RSA_WITH_AES_128_CBC_SHA = 0X002F,
    TLS_RSA_WITH_RC4_128_SHA = 0X0005,
    TLS_RSA_WITH_RC4_128_MD5 = 0X0004,
    TLS_RSA_WITH_3DES_EDE_CBC_SHA = 0X000A,
    TLS_RSA_WITH_AES_256_CBC_SHA256 = 0X003D,
    All_CIPHER_SUITES = 0XFFFF
} SslCipherSuite_t;

/**
 * SslSecurityLevel_t, security level
 */
typedef enum _SslSecurityLevel
{
    NO_AUTH = 0,       // No Authentication
    SERVER_AUTH,       // Manage Server Authentication
    SERVER_CLIENT_AUTH // Manage Server and Client Authentication if requested by the remote server
} SslSecurityLevel_t;

/**
 * SslContextHandle, represents SSL Channel handle
 */
typedef int8_t SslContextHandle;

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
                                     SslSecurityLevel_t secLevel);

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
                                 SslSecurityLevel_t secLevel);

/**
 * @fn bool IsSslChannelAvailable(SslContextHandle handle)
 * @brief This function returns the configuration status of SSL channel
 * @param SslContextHandle handle, SSL channel identifier
 * @return false: not available, true: available
 * @remark user need to first subscribe SSL channel
 */
bool IsSslChannelAvailable(SslContextHandle handle);

/**
 * @fn int16_t SslWriteCertificate(const char *certName, const char *data, int16_t dataLen)
 * @brief This function writing the certifacte to memory
 * @param const char *certName, pointer to certificate name.
 * @param const char *data, pointer to certificate data
 * @param int16_t dataLen, certificate data size
 * @return -ve number: validation fail, 0: finish with fail, 1: success
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int16_t SslWriteCertificate(const char *certName, const char *data, int16_t dataLen);

/**
 * @fn int16_t SslReadCertificate(const char *certName)
 * @brief This function checks the status of the certifacte in memory
 * @param const char *certName, pointer to certificate name.
 * @return -ve number: validation fail, 0: finish with fail, 1: file intact
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int16_t SslReadCertificate(const char *certName);

/**
 * @fn int8_t SslDeleteCertificate(const char *certName)
 * @brief This function deletes the certifacte from memory
 * @param const char *certName, pointer to certificate name.
 * @return -ve number: validation fail, 0: finish with fail, 1: file intact
 * @remark certName shall contain memory area, such as NVRAM:CA0, RAM:ca_cert.pem etc
 */
int8_t SslDeleteCertificate(const char *certName);

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
                        const char *nameCACert, const char *nameClientCert, const char *nameClientkey);
/*
IMPORTANT NOTE:
Max size for Certificates and Key file/data is 2017 bytes.
There are two methodology to use SSL certificates.

1] Let the Library handle its configuration.
--> In this, user need to create channel subscription (i.e. SSL Context creation) and Library will handle further.
--> User can get a Handler (i.e. SSL Context ID) to use or to associate with SSL Client such as MQTTS, HTTPS, TCP(S), SMPTS
--> User can check if particular handle is successfully configured or not, so that he can use it further. 
--> Note that there can be only TWO such handles (or Context) can be created. do not try to alter this number ever.
--> Note that all the certificates are taking as pointer to array. So User must hold passing array thorught out the program.

2] User can handle these Context by his own
--> In this case DO NOT USE "SslChannelSubscribe" function call.
--> Read, Write, Delete certificates and configure SSL Context appropriately using respective functions provided by library.
--> Example certName can be "NVRAM:CA0", "NVRAM:CC0", "NVRAM:CK0" or "RAM:ca_cert.pem", "RAM:client_cert.pem", "RAM:client_key.pem"
--> Note that if NVRAM choose as storage location then names shall be "NVRAM:CA0" (server cert), "NVRAM:CC0" (client cert), "NVRAM:CK0" (client key)
--> Note that if RAM choose as storage location then names shall start with "RAM:*****"
--> Note that if in runtime configurations are happen to update then don't forget to disconnect and re-connect to MQTT, TCP.
         that means before MQTTS or TCP(S) always configure SSL first.
--> NOTE IMPORTANT: call associated functions when "IsATavailable()" and "IsModemReady()" always returns true.
*/

bool DeleteAllCertificates(uint8_t handle);

#endif // QUECSSL_H

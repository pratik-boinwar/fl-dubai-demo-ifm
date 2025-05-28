/**
 * @file       QuecHttp.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECHTTP_H
#define QUECHTTP_H

/**
 * HttpMethod_t is http GET/POST method
 */
typedef enum _HttpMethod
{
    HTTP_METHOD_GET,
    HTTP_METHOD_POST
} HttpMethod_t;

/**
 * HttpContentType_t, available http header 'Content-Type' of data being send/receive
 * refer: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types
 * https://stackoverflow.com/questions/23714383/what-are-all-the-possible-values-for-http-content-type-header
 * https://stackoverflow.com/questions/9870523/what-are-the-differences-between-application-json-and-application-x-www-form-url
 */
typedef enum _HttpContentType
{
    HTTP_CONTENT_X_WWW_FORM_URLENCODED = 0,
    HTTP_CONTENT_TEXT_PLAIN = 1,
    HTTP_APPLICATION_OCTET_STREAM = 2,
    HTTP_MULTIPART_FORM_DATA = 3,
    // DO NOT CHANGE ABOVE THIS LINE

    HTTP_CONTENT_APPLICATION_JSON
} HttpContentType_t;

/**
 * HttpStatus_t states current status of http process
 */
typedef enum _HttpStatus
{
    HTTP_STATUS_IDLE,
    HTTP_STATUS_INTIATED,
    HTTP_STATUS_BUSY,
    HTTP_STATUS_DOWNLOADING,
    HTTP_STATUS_SUCCESS,
    HTTP_STATUS_FAILED
} HttpStatus_t;

/**
 * HttpEvent_t, various Http callback events
 */
typedef enum _HttpEvent
{
    HTTP_EVENT_DWNLD_START,
    HTTP_EVENT_DWNLD_NEW_DATA,
    HTTP_EVENT_DWNLD_END
} HttpEvent_t;

/**
 * function pointer to call back function to collect http data
 */
typedef int (*pfnMdmHttpResStoreCb)(HttpEvent_t httpEvent, const char *httpRxData, unsigned long httpRxDataLen);

/**
 * @fn int8_t HttpSendData(const char *httpServerUrl, const char *httpTxData, uint32_t httpTxDataLen,
                 char *httpRxDatBuff, uint32_t httpRxDatBuffSize, HttpMethod_t httpMethod,
                 HttpContentType_t httpCntType, pfnMdmHttpResStoreCb httpResStoreFuncCb,
                 bool secured, uint8_t sslContextID)
 * @brief This function initiates the http task. it holds the information passed by calling method
 *          through out the process
 * @param char *httpServerUrl, points to the http server url
 * @param char *httpTxData, points to buffer holding http data body
 * @param uint32_t httpTxDataLen, size of http data body
 * @param char *httpRxDatBuff, pointer to buffer which will hold http received response
 * @param uint32_t httpRxDatBuffSize, is rx buffer size
 * @param HttpMethod_t httpMethod, current http procedure http GET/POST
 * @param HttpContentType_t httpCntType, http data conntent-type
 * @param pfnMdmHttpResStoreCb httpResStoreFuncCb, pointer to function to transfer received http data.
 *                              either this or httpRxDatBuff must be valid to complete read process.
 * @param bool secured, enable/disable SSL i.e. http or https session
 * @param uint8_t sslContextID, ssl context id in case https session
 * @return int8_t, -ve number: validation fail, 0: not availaible, 1:success
 * @remark It is required to hold all the buffers passed to this function.
 */
int8_t HttpSendData(const char *httpServerUrl, const char *httpTxData, uint32_t httpTxDataLen,
                    char *httpRxDatBuff, uint32_t httpRxDatBuffSize, HttpMethod_t httpMethod,
                    HttpContentType_t httpCntType, pfnMdmHttpResStoreCb httpResStoreFuncCb,
                    bool secured, uint8_t sslContextID);

/**
 * @fn HttpStatus_t HttpGetLastStatus(void)
 * @brief This function returns the http process current status.
 * @return status of http process
 * @remark
 */
HttpStatus_t HttpGetLastStatus(void);

/**
 * @fn bool IsHttpClientAvailable(void)
 * @brief This function returns the availability of http process
 * @return 0: not available, 1: available
 * @remark
 */
bool IsHttpClientAvailable(void);

/**
 * @fn void HttpGetPostCmdResRxd(void);
 * @brief This function will handle the response of Http Get command
 * @remark
 */
void HttpGetPostCmdResRxd(void);

uint32_t HttpGetPostCmdRespError(void);
uint32_t HttpGetPostCmdRespStatus(void);
uint32_t HttpGetPostCmdRespContentLength(void);

#endif // QUECHTTP_H

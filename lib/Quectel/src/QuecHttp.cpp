/**
 * @file       QuecHttp.cpp
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecHttp.h"

extern void freezeAT(void);
extern void unfreezeAT(void);

static uint32_t _httpGetPostCmdResError, _httpGetPostCmdResHttpStatus, _httpGetPostCmdResContentLength;
/**
 * HTTP_SM states of http client
 */
typedef enum _HTTP_SM
{
    HTTP_SM_NULL = 0,

    /************ HTTP States ************/
    HTTP_SM_INIT,
    HTTP_SM_SSL_CONFIG,
    HTTP_SM_SET_URL,
    HTTP_SM_WAIT_FOR_URL_ACCEPT,
    HTTP_SM_SEND_GET_REQUEST,
    HTTP_SM_WAIT_FOR_GET_REQUEST_ACCEPT,
    HTTP_SM_SET_HEADER,
    HTTP_SM_SEND_POST_REQUEST,
    HTTP_SM_WAIT_FOR_POST_DATA_ACCEPT,
    HTTP_SM_SEND_READ_REQUEST,
    HTTP_SM_WAIT_FOR_DATA_RECEIVED,

    HTTP_SM_SEND_POST_REQUEST2,
    E_ESP_HTTP_SM_WAIT_FOR_POST_DATA_ACCEPT,

    HTTP_SM_GENERIC_DELAY,
    HTTP_SM_CLEAN_UP,
    HTTP_SM_IDLE

} HTTP_SM;

/// holds the current and previous operating state
static HTTP_SM _eHttpSmState = HTTP_SM_IDLE, _eHttpSmPrevState = HTTP_SM_IDLE;
/// general ticks used in processing
static uint32_t _localGenericTick = 0, _genericDelayTick = 0;
/// holds the current status of process
static HttpStatus_t _mdmHttpStatus = HTTP_STATUS_IDLE;

/// holds the http parameters for the current http process
static const char *pHttpServerUrl;
static const char *_pHttpTxData;
static char *pHttpRxData;
static uint32_t _httpTxDataLen, _httpRxBuffsize;
static HttpMethod_t _httpMethod = HTTP_METHOD_POST;
static HttpContentType_t _httpCntType;
static pfnMdmHttpResStoreCb mdmHttpResStoreFuncCb;
static bool _sslEnabled = false;
static uint8_t _sslContextID = 0;
static bool _responseHeaderReceived = false;
static long _responseContentLength = 0;

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
 * @param uint32_t httpRxDatBuffSize, rx buffer size
 * @param HttpMethod_t httpMethod, current http procedure http GET/POST
 * @param HttpContentType_t httpCntType, http data conntent-type
 * @param pfnMdmHttpResStoreCb httpResStoreFuncCb, pointer to function to transfer received http data.
 *                              either CB or httpRxDatBuff must be valid to complete read process.
 * @param bool secured, enable/disable SSL i.e. http or https session
 * @param uint8_t sslContextID, ssl context id in case https session
 * @return int8_t, -ve number: validation fail, 0: not availaible, 1:success
 * @remark It is required to hold all the buffers passed to this function.
 */
int8_t HttpSendData(const char *httpServerUrl, const char *httpTxData, uint32_t httpTxDataLen,
                    char *httpRxDatBuff, uint32_t httpRxDatBuffSize, HttpMethod_t httpMethod,
                    HttpContentType_t httpCntType, pfnMdmHttpResStoreCb httpResStoreFuncCb,
                    bool secured, uint8_t sslContextID)
{
    // check gprs connection
    if (!IsGprsConnected())
    {
        return 0;
    }

    // check modem availability
    if (!IsModemReady())
        return 0;

    // we received modem access, don't forget to release resource when finished
    if (_eHttpSmState != HTTP_SM_IDLE)
    {
        // http already in process. Idealy this should not happen
        // as we have already got access by GetModemAccess()
        //  ReleaseModemAccess(MDM_RUNNING_HTTP);
        return 0;
    }

    if (httpResStoreFuncCb == NULL)
    {
        // if no callback assign, then process must have assigned RXbuffer for received data

        // rx buffer required
        if (httpRxDatBuff == NULL)
            return -1;

        // rx buffer length must be defined
        if (httpRxDatBuffSize == 0)
            return -2;
    }

    if (!GetModemAccess(MDM_RUNNING_HTTP))
        return 0;

    _mdmHttpStatus = HTTP_STATUS_INTIATED;

    pHttpServerUrl = httpServerUrl;
    _pHttpTxData = httpTxData;
    _httpTxDataLen = httpTxDataLen;
    pHttpRxData = httpRxDatBuff;
    _httpRxBuffsize = httpRxDatBuffSize;
    _httpMethod = httpMethod;
    _httpCntType = httpCntType;
    _eHttpSmState = HTTP_SM_INIT;

    _sslEnabled = secured;
    _sslContextID = sslContextID;

    mdmHttpResStoreFuncCb = httpResStoreFuncCb;

    _responseContentLength = 0;
    _responseHeaderReceived = false;

    return 1;
}

/**
 * @fn HttpStatus_t HttpGetLastStatus(void)
 * @brief This function returns the http process current status.
 * @return status of http process
 * @remark
 */
HttpStatus_t HttpGetLastStatus(void)
{
    return (_mdmHttpStatus);
}

/**
 * @fn bool IsHttpClientAvailable(void)
 * @brief This function returns the availability of http process
 * @return 0: not available, 1: available
 * @remark
 */
bool IsHttpClientAvailable(void)
{
    if (_eHttpSmState == HTTP_SM_IDLE)
        return true;
    else
        return false;
}

/**
 * @fn static void HttpSmChangeState(HTTP_SM curState)
 * @brief This function changes the http process state
 * @param HTTP_SM curState the state to process
 * @remark
 */
static void HttpSmChangeState(HTTP_SM curState)
{
    _eHttpSmPrevState = _eHttpSmState;
    _eHttpSmState = curState;
}

/**
 * @fn static void HttpSmDelay(uint32_t delayTicks, HTTP_SM nextState)
 * @brief This function suspends current task for given ticks
 *        move to another state after given ticks
 * @param uint32_t delayTicks ticks to suspend
 * @param HTTP_SM nextState state to process after delay
 * @remark
 */
static void HttpSmDelay(uint32_t delayTicks, HTTP_SM nextState)
{
    _genericDelayTick = delayTicks;
    _localGenericTick = millis();
    HttpSmChangeState(HTTP_SM_GENERIC_DELAY);
    if (HTTP_SM_NULL != nextState)
        _eHttpSmPrevState = nextState;
}

/**
 * @fn void QuectelHttpClientSm(void)
 * @brief This function is handling http process.this function is required to be called frequently
 * @remark
 */
void QuectelHttpClientSm(void)
{
    static int status = 0;
    // char str[55];
    static uint32_t _generalTimeoutTick = 0;
    uint32_t httpResByteCnt = 0;
    static uint32_t httpResByteCntWaitRetry = 0;

    switch (_eHttpSmState)
    {
    case HTTP_SM_GENERIC_DELAY:
        if ((millis() - _localGenericTick) > _genericDelayTick)
        {
            HttpSmChangeState(_eHttpSmPrevState);
            break;
        }
        break;

    case HTTP_SM_INIT: // intialte flags and timeouts
        _generalTimeoutTick = millis();
        HttpSmChangeState(HTTP_SM_SSL_CONFIG);
        break;

    case HTTP_SM_SSL_CONFIG:
        if (IsATavailable())
        {
            char strAt[25];

#ifdef ENABLE_4G
#else
            // Enable HTTPS function
            snprintf(strAt, sizeof(strAt), "+QSSLCFG=\"https\",%d", _sslEnabled);
            sendAT(strAt);
            waitResponse3(GSM_OK);
#endif
            if (_sslEnabled)
            {
#ifdef ENABLE_4G
                snprintf(strAt, sizeof(strAt), "+QSSLCFG=\"sslctxid\",%d", _sslContextID);
#else
                snprintf(strAt, sizeof(strAt), "+QSSLCFG=\"httpsctxi\",%d", _sslContextID);
#endif

                sendAT(strAt);
                waitResponse3(GSM_OK);
            }

            _generalTimeoutTick = millis();
            HttpSmChangeState(HTTP_SM_SET_URL);
        }
        if ((millis() - _generalTimeoutTick) > 5000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_SET_URL:
        if (IsATavailable())
        {
            //            _DPRINT_TCPSM("SET_HTTP_URL\r\n");
            _mdmHttpStatus = HTTP_STATUS_BUSY;
            // mdmCurrentOperation = RUNNING_HTTP;
            // _M95ClearHttpUnrFlags();

            // SetM95InDataMode();

            char strAt[250];

            snprintf(strAt, sizeof(strAt), "+QHTTPURL=%d,30", strlen(pHttpServerUrl));
            sendAT(strAt);

            HttpSmChangeState(HTTP_SM_WAIT_FOR_URL_ACCEPT);

            _generalTimeoutTick = millis();
        }

        if ((millis() - _generalTimeoutTick) > 5000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_WAIT_FOR_URL_ACCEPT:
        // CONNECT, OK takes longer time, we are leaving in 1Sec and come again to check
        status = waitResponse2(1000, GSM_CONNECT, GSM_OK, GSM_ERROR);
        if (1 == status)
        {
            // received "CONNECT". send URL
            // DBG("%s", pHttpServerUrl);
            streamAT().print(pHttpServerUrl);
            _generalTimeoutTick = millis();
            break;
        }
        if (2 == status)
        {
            // received "OK". URL is accepted
            HttpSmChangeState(HTTP_SM_SET_HEADER); // POST Request, set header
            // SetM95InCommandMode();
            break;
        }
        if (3 == status)
        {
            // received ERROR
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] Error\n");
            break;
        }

        if ((millis() - _generalTimeoutTick) > 15000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_SET_HEADER: // set http header if any
        if (IsATavailable())
        {
            if (_httpMethod == HTTP_METHOD_GET)
            {
                // if callback provided, enable response header.
                if (mdmHttpResStoreFuncCb != NULL)
                    sendAT("+QHTTPCFG=\"responseheader\",1"); //"\"requestheader\",1"
                else
                    sendAT("+QHTTPCFG=\"responseheader\",0"); //"\"requestheader\",1"
                status = waitResponse3(GSM_OK);
            }
            else
            {
#ifdef ENABLE_4G
                status = 1;

                sendAT("+QHTTPCFG=\"requestheader\",0"); //"\"requestheader\",1"
                status = waitResponse3(GSM_OK);

                char strAt[200];

                strcpy(strAt, "+QHTTPCFG=\"contenttype\",");

                switch (_httpCntType)
                {
                case HTTP_CONTENT_APPLICATION_JSON:
                    strcat(strAt, "4"); // only this is tested
                    break;

                case HTTP_CONTENT_TEXT_PLAIN:
                    strcat(strAt, "1");
                    break;

                case HTTP_APPLICATION_OCTET_STREAM:
                    strcat(strAt, "2");
                    break;

                case HTTP_MULTIPART_FORM_DATA:
                    strcat(strAt, "3");
                    break;

                default:
                    strcat(strAt, "0");
                    break;
                }

                sendAT(strAt);
                status = waitResponse3(GSM_OK);
#else
#if defined(USE_QUECTEL_M95) || defined(USE_QUECTEL_M66)
                char strAt[200];

                strcpy(strAt, "+QHTTPCFG=\"Content-Type\",");

                switch (_httpCntType)
                {
                case HTTP_CONTENT_APPLICATION_JSON:
                    strcat(strAt, "\"application/json\""); // only this is tested
                    break;

                case HTTP_CONTENT_TEXT_PLAIN:
                    strcat(strAt, "\"text/plain\"");
                    break;

                case HTTP_APPLICATION_OCTET_STREAM:
                    strcat(strAt, "\"application/octet-stream\"");
                    break;

                case HTTP_MULTIPART_FORM_DATA:
                    strcat(strAt, "\"multipart/form-data\"");
                    break;

                default:
                    strcat(strAt, "\"application/x-www-form-urlencoded\"");
                    break;
                }

                sendAT(strAt);
                status = waitResponse3(GSM_OK);

#elif defined(USE_QUECTEL_BG96)
                // refer: BG96_HTTP(S)_AT_Commands_Manual_V1.0
                if (_httpCntType > 3)
                {
                    _httpCntType = 0; // override content-type. I tested "json" with this type=0
                }

                char strAt[50];

                snprintf(strAt, sizeof(strAt), "+QHTTPCFG=\"Content-Type\",%d", _httpCntType);
                status = waitResponse3(GSM_OK);

#else
                /* UNSUPORTED MODULE */
                _mdmHttpStatus = HTTP_STATUS_FAILED;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                break;
#endif
#endif
            }

            if (1 == status)
            {
                // received OK for +QHTTPCFG
                if (_httpMethod == HTTP_METHOD_GET)
                    HttpSmChangeState(HTTP_SM_SEND_GET_REQUEST); // GET Request
                else
                    HttpSmChangeState(HTTP_SM_SEND_POST_REQUEST); // POST Request

                _generalTimeoutTick = millis();
                break;
            }
            else
            {
                // not recceived OK for +QHTTPCFG
                _mdmHttpStatus = HTTP_STATUS_FAILED;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                break;
            }
        }

        if ((millis() - _generalTimeoutTick) > 15000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_SEND_GET_REQUEST:
        if (IsATavailable())
        {
            sendAT("+QHTTPGET=15,30");
#if 1
            _generalTimeoutTick = millis();
            HttpSmChangeState(HTTP_SM_WAIT_FOR_GET_REQUEST_ACCEPT);
#else
            status = waitResponse2(35000, GSM_OK);
            if (1 == status)
            {
                // received "OK". httpTxData is accepted
                HttpSmChangeState(HTTP_SM_SEND_READ_REQUEST); // check and read any data received from server
                // SetM95InCommandMode();
                _generalTimeoutTick = millis();
                break;
            }
            else
            {
                _mdmHttpStatus = HTTP_STATUS_FAILED;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
            }
#endif
        }

        if ((millis() - _generalTimeoutTick) > 15000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_WAIT_FOR_GET_REQUEST_ACCEPT:
        status = waitResponse2(1000, GSM_OK);
        if (1 == status)
        {
#ifdef ENABLE_4G
            status = waitResponse2(10000);
            if (10 == status)
            {
                if (0 != HttpGetPostCmdRespError())
                {
                    _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
                if (HttpGetPostCmdRespStatus() == 200 || HttpGetPostCmdRespStatus() == 201)
                {
                    // success
                }
                else
                {
                    _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
            }
            else
            {
                _mdmHttpStatus = HTTP_STATUS_FAILED;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                break;
            }
#endif
            // received "OK". httpTxData is accepted
            HttpSmChangeState(HTTP_SM_SEND_READ_REQUEST); // check and read any data received from server
            // SetM95InCommandMode();
            _generalTimeoutTick = millis();
            break;
        }
        else if (2 == status)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
        }

        // added by neev: CME Error was not handled for HTTP GET request
        else if (3 == status)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
        }

        if ((millis() - _generalTimeoutTick) > 120000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_SEND_POST_REQUEST:
        if (IsATavailable())
        {
            //            _DPRINT_TCPSM("E_M95_HTTP_SEND_POST_REQUEST\r\n");
            // _M95ClearHttpUnrFlags();
            // SetM95InDataMode();

            char strAt[50];

            sprintf(strAt, "+QHTTPPOST=%d,55,50", _httpTxDataLen); // strlen(pM95HttpTxBody)
            sendAT(strAt);

            HttpSmDelay(100, HTTP_SM_WAIT_FOR_POST_DATA_ACCEPT);

            _generalTimeoutTick = millis();
        }

        if ((millis() - _generalTimeoutTick) > 15000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_WAIT_FOR_POST_DATA_ACCEPT:
        // CONNECT, OK takes longer time, we are leaving in 1Sec and come again to check
        status = waitResponse2(1000, GSM_CONNECT, GSM_OK);
        if (1 == status)
        {
            // received "CONNECT". send httpTxData
            streamAT().write(reinterpret_cast<const uint8_t *>(_pHttpTxData), _httpTxDataLen);
            delay(5);
            // streamAT().flush();

            // DBG("%s", _pHttpTxData);

            _generalTimeoutTick = millis();
            break;
        }
        if (2 == status)
        {
#ifdef ENABLE_4G
            status = waitResponse2(10000);
            if (10 == status)
            {
                if (0 != HttpGetPostCmdRespError())
                {
                    _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
                if (HttpGetPostCmdRespStatus() == 200 || HttpGetPostCmdRespStatus() == 201)
                {
                    // success
                }
                else
                {
                    _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
            }
            else
            {
                _mdmHttpStatus = HTTP_STATUS_FAILED;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                break;
            }
#endif
            // received "OK". httpTxData is accepted
            HttpSmChangeState(HTTP_SM_SEND_READ_REQUEST); // check and read any data received from server
            // SetM95InCommandMode();
            _generalTimeoutTick = millis();
            break;
        }
        if (3 == status)
        {
            // received ERROR
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] Error\n");
            break;
        }

        if ((millis() - _generalTimeoutTick) > 120000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_SEND_READ_REQUEST:
        if (IsATavailable())
        {
            //            _DPRINT_TCPSM("E_M95_HTTP_SEND_POST_REQUEST\r\n");
            // _M95ClearHttpUnrFlags();
            // SetM95InDataMode();

            sendAT("+QHTTPREAD=60");
            status = waitResponse3(GSM_CONNECT);
            if (1 == status)
            {
                // received "CONNECT", after that data is expected
                // freezeAT();
                if (mdmHttpResStoreFuncCb != NULL)
                {
                    // huge amount of data is expected to receive from HTTP server in response.
                    // such big amount of data can not be collected in single buffer.
                    // so we will collect in chunk of bytes and transfer to the local memory storage.
                    // as we are communicating with http server (with Connection Close type), the HTTP will release the socket.
                    // we will processed data after socket is been close from http server.

                    HttpSmDelay(50, HTTP_SM_WAIT_FOR_DATA_RECEIVED);
                    _mdmHttpStatus = HTTP_STATUS_DOWNLOADING;
                    // mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_START, 0, 0);
                    _generalTimeoutTick = millis();
                    httpResByteCntWaitRetry = 0;
                    break;
                }
                else
                {
                    httpResByteCnt = streamAT().readBytes(pHttpRxData, (_httpRxBuffsize - 1));

                    if (httpResByteCnt > 0)
                    {
                        //                char *ptrch = 0;
                        //                if ((ptrch = strstr(pM95HttpRxBody, "\r\nOK\r\n")) != 0)
                        //                {
                        //                    *ptrch = '\0';
                        //                }
                        _mdmHttpStatus = HTTP_STATUS_SUCCESS;
                    }
                    else
                    {
                        // there might some application cases, where server not sending any data to GET request which is totally OK
                        // So, SUCCESS or FAIL here is application dependent.

                        _mdmHttpStatus = HTTP_STATUS_FAILED;
                    }

                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    unfreezeAT();
                    break;
                }
            }
            else
            {
                // already finished HTTP GET/POST session

                if (_httpMethod == HTTP_METHOD_POST)
                {
                    // if we are here means, data is been POSTed. but no response from seerver
                    // it is depend on application requirement how to take this.
                    // either called it as SUCCESS or FAIL session
                    _mdmHttpStatus = HTTP_STATUS_SUCCESS;
                    // _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
                else
                {
                    // if this is GET request, then we are expecting data from server.
                    // but it is application depenedent. So, act accordingly
                    _mdmHttpStatus = HTTP_STATUS_FAILED;
                    HttpSmChangeState(HTTP_SM_CLEAN_UP);
                    break;
                }
            }
            // HttpSmDelay(100, HTTP_SM_WAIT_FOR_POST_DATA_ACCEPT);

            _generalTimeoutTick = millis();
        }

        if ((millis() - _generalTimeoutTick) > 30000)
        {
            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }
        break;

    case HTTP_SM_WAIT_FOR_DATA_RECEIVED:
        if ((millis() - _generalTimeoutTick) > 600000U)
        {
            // read one last time before ending.
            httpResByteCnt = streamAT().readBytes(pHttpRxData, _httpRxBuffsize);
            mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_END, pHttpRxData, httpResByteCnt);

            _mdmHttpStatus = HTTP_STATUS_FAILED;
            HttpSmChangeState(HTTP_SM_CLEAN_UP);
            DBG("[HTTP] timeout\n");
            break;
        }

        httpResByteCnt = streamAT().available();
        if (httpResByteCnt > 0)
        {
            // if bytes received, collect them so that let more bytes to receive from server.

            // find content length and wait till it received.
            if (_responseContentLength == 0)
            {
                int8_t ret = waitResponse3("-Length: ");
                if (1 == ret)
                {
                    _responseContentLength = streamGetLongIntBefore('\r');
                    DBG(" ==== Content-Length: %ld ======\n", _responseContentLength);
                    do
                    {
                        // required sequence '\n' , '\r' and '\n'
                        bool ret = streamSkipUntil('\n');
                        if (ret == true)
                        {
                            char a = streamAT().read();

                            if (a == '\r')
                            {
                                a = streamAT().read();
                                if (a == '\n')
                                {
                                    // hit the spot.
                                    _responseHeaderReceived = true;
                                    DBG(" ===== response header received =====\n");
                                    freezeAT();
                                    mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_START, 0, _responseContentLength);
                                    break;
                                }
                            }
                        }
                        delay(0);
                    } while ((millis() - _generalTimeoutTick) < 7000); // (_responseHeaderReceived == false) &&

                    if ((millis() - _generalTimeoutTick) > 7000)
                    {
                        _mdmHttpStatus = HTTP_STATUS_FAILED;
                        HttpSmChangeState(HTTP_SM_CLEAN_UP);
                        DBG("[HTTP] header timeout\n");
                        break;
                    }
                }
                else
                {
                    // DBG("==\n");
                    break;
                }
            }

            httpResByteCnt = streamAT().readBytes(pHttpRxData, _httpRxBuffsize);

            uint32_t len = 0;

            if (_responseContentLength > httpResByteCnt)
            {
                len = httpResByteCnt;
            }
            else
            {
                len = _responseContentLength;
            }
            _responseContentLength -= len;

            DBG("\r\n[HTTP] received %d / remaining %ld\n", len, _responseContentLength);

            // if ((pHttpRxData[len - 1] == '\n' && pHttpRxData[len - 2] == '\r' && pHttpRxData[len - 3] == 'K' &&
            //      pHttpRxData[len - 4] == 'O' && pHttpRxData[len - 5] == '\n' && pHttpRxData[len - 6] == '\r') ||
            if (_responseContentLength <= 0)
            {
                // received OK. end of data.

                // pHttpRxData[len - 6] = 0;  // terminate string.

                // check string length and pass to function.
                mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_NEW_DATA, pHttpRxData, len);

                // finish the process
                mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_END, 0, 0);
                *pHttpRxData = 0; // clear buffer.

                _mdmHttpStatus = HTTP_STATUS_SUCCESS;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                DBG("[HTTP] Download Completed\n");
                break;
            }
            else
            {
                // StoreHttpServerResponse(_pEnAppHttpClient->pHttpRxDataBuff, httpResByteCnt);
                mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_NEW_DATA, pHttpRxData, httpResByteCnt);
                httpResByteCntWaitRetry = 0;
            }

            /** we can not collect data forever if server fails and keep sending data
             * we have allocated max. 30 sec
             */
            // _generalTimeoutTick = millis();

            break; // if bytes collected, come back again imediately to read another chunk.
        }
        else
        {
            // retry to get some more data for some time
            httpResByteCntWaitRetry++;
            if (httpResByteCntWaitRetry > 3000U)
            {
                mdmHttpResStoreFuncCb(HTTP_EVENT_DWNLD_END, 0, 0);
                *pHttpRxData = 0; // clear buffer.

                _mdmHttpStatus = HTTP_STATUS_SUCCESS;
                HttpSmChangeState(HTTP_SM_CLEAN_UP);
                // SetM95InCommandMode();
                // M95ReleaseRxHandling();
                DBG("[HTTP] Download finished abruptly\n");
                break;
            }
        }
        HttpSmDelay(5, HTTP_SM_NULL);
        break;

    case HTTP_SM_CLEAN_UP: // do some cleanup job before conclude this process
        HttpSmChangeState(HTTP_SM_IDLE);
        // SetM95InCommandMode();
        unfreezeAT();
        streamAT().write(0x1A);
        streamAT().flush();
        delay(10);
        streamAT().write("+++");
        delay(10);
        waitResponse3(); // read out remaining
        ReleaseModemAccess(MDM_RUNNING_HTTP);
        break;

    default: // IDLE
        break;
    }
}

void HttpGetPostCmdResRxd(void)
{
    _httpGetPostCmdResError = streamGetIntBefore(',');
    _httpGetPostCmdResHttpStatus = streamGetIntBefore(',');
    _httpGetPostCmdResContentLength = streamGetIntBefore(',');
}

uint32_t HttpGetPostCmdRespError(void)
{
    return _httpGetPostCmdResError;
}

uint32_t HttpGetPostCmdRespStatus(void)
{
    return _httpGetPostCmdResHttpStatus;
}

uint32_t HttpGetPostCmdRespContentLength(void)
{
    return _httpGetPostCmdResContentLength;
}
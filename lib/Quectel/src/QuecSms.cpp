
#include "Quectel.h"
#include "QuecGsmGprs.h"
#include "QuecSms.h"

#define MAX_SMS_INBOX 20 // can not be greater than 31

#define SMS_AUTO_DELETE_READ 1 // 1: delete sms which are already read, 0: let the user decide, but it has to delete to make space.
#define SMS_AUTO_DELETE_SENT 1 // 1: delete sms which are already sent, 0: let the user decide, but it has to delete to make space.

#define SMS_MAX_RETRY_COUNT 3 // after this much count, it is good to make decision on deleting outbox to make space for new sms.

// each bit represents inbox number.
static uint32_t _inboxAvailable = 0xFFFFE; // Initially reading all inboxes.
static uint32_t _outboxAvailable = 0;

// holds the parameters for the sms process.
static uint8_t _newInbox = 0, _outboxProcessing = 0;
static uint8_t _smsSendRetryCount[MAX_SMS_INBOX + 2] = {0}; // each index/element represents attepmpted retry count for outbox.
static QuecSmsCallBack _smsCb = NULL;
static Sms_t _newSMS;

/**
 * @fn void SmsReceived(SmsUrcType_t smsType)
 * @brief This function is being called when new SMS received.
 * @remark this function is called from waitResponse() function
 */
void SmsReceived(SmsUrcType_t smsType)
{
    if (smsType == SMS_URC_TYPE_CMTI)
    {
        // AT+CNMI=2,1 message indication.

        /*
            +CMTI: "SM",1
        */

        streamSkipUntil(',');
        uint8_t _newInbox = streamGetIntBefore('\r');
        streamSkipUntil('\n');
        _inboxAvailable |= (1 << _newInbox);
        _outboxAvailable &= ~(1 << _newInbox);

        DBG("[SMS] #%d recevied\n", _newInbox);
    }
    else
    {
        // AT+CNMI=2,2 message indication.
        // read AT+CMGF status and then decide received message is in Text or PDU format.

        if (smsType == SMS_URC_TYPE_CMTI)
        {
            /*
                +CMT: xxxx
            */

            // ToDo:
            //  received SMS text/pdu.
            //  collect and call callback.
        }

        // rest type of AT+CNMI=2,2 messages are not handled.
    }
}

/**
 * @fn static uint8_t smsFindOutbox(void)
 * @brief This functions return pending outbox number if any.
 * @return 0: no pending sms in queue, >0 : outbox number
 * @remark this is used in conjuction with SmsSendToQueue
 */
static uint8_t smsFindOutbox(void)
{
    if (_outboxAvailable)
    {
        for (uint8_t i = 1; (i < MAX_SMS_INBOX && i < 32); i++)
        {
            if ((1 << i) & _outboxAvailable)
            {
                return i;
            }
        }
    }

    return 0;
}

/**
 * SMS_SM states for SMS handling process
 */
typedef enum _SMS_SM
{
    SMS_SM_NULL = 0,
    SMS_SM_INIT,
    SMS_SM_WAIT_FOR_SMS_READY,
    SMS_SM_GENERIC_DELAY,
    SMS_SM_CHECK_INBOX,
    SMS_SM_CHECK_OUTBOX,
    SMS_SM_WAIT_OUTBOX_SENT,

} SMS_SM;

// holds the current operating state
SMS_SM _smsState = SMS_SM_INIT;

/**
 * @fn void QuectelSmsClientSm(void)
 * @brief This function is handling sms process. This function is required to be called frequently.
 * @remark
 */
void QuectelSmsClientSm(void)
{
    static uint32_t _generalDelayTick = 0;

    if (!IsGsmNwConnected())
    {
        _smsState = SMS_SM_INIT;
        return;
    }

    switch (_smsState)
    {
    case SMS_SM_INIT: // first time after module reset.
        _inboxAvailable = _inboxAvailable & 0x3FFFFFFE;
        _outboxAvailable = _outboxAvailable & 0x3FFFFFFE;

        // we are taking a nap after network registered.
        // because module required some time to initialize internal SMS process.
        _generalDelayTick = millis();
        _smsState = SMS_SM_WAIT_FOR_SMS_READY;
        break;

    case SMS_SM_WAIT_FOR_SMS_READY: // wait for URC 'SMS Ready'
        if (millis() - _generalDelayTick > 15000)
        {
            _smsState = SMS_SM_CHECK_INBOX;
        }
        break;

    case SMS_SM_GENERIC_DELAY: // this is power nap to release modem access for other processes.
        if (millis() - _generalDelayTick > 5000)
        {
            _smsState = SMS_SM_CHECK_INBOX;
        }
        break;

    case SMS_SM_CHECK_INBOX: // check and read inbox
        if (_smsCb != NULL)
        {
            // read sms and send to user thorugh provided callback.
            if (_newInbox > 0)
            {
                // giving priority to newly received sms.
                if (SmsRead(_newInbox, _newSMS.number, _newSMS.msg, sizeof(_newSMS.msg)))
                {
                    // deleting first, to make sure there is atleast one space for new sms
                    // if user wish to send sms in queue.
                    SmsDelete(_newInbox);
                    _smsCb(_newSMS.number, _newSMS.msg, strlen(_newSMS.msg));
                }

                _newInbox = 0;
            }
            else if (IsSmsAvailable(&_newInbox))
            {
                // reading pending sms
                if (SmsRead(_newInbox, _newSMS.number, _newSMS.msg, sizeof(_newSMS.msg)))
                {
                    // deleting first, to make sure there is atleast one space for new sms
                    // if user wish to send sms in queue.
                    SmsDelete(_newInbox);
                    _smsCb(_newSMS.number, _newSMS.msg, strlen(_newSMS.msg));
                }

                _newInbox = 0;
            }
        }

        _generalDelayTick = millis();
        _smsState = SMS_SM_CHECK_OUTBOX;
        break;

    case SMS_SM_CHECK_OUTBOX: // check if any sms in queue to send.
        _generalDelayTick = millis();
        _outboxProcessing = smsFindOutbox();
        if (_outboxProcessing > 0)
        {
            if (GetModemAccess(MDM_SENDING_SMS))
            {
                streamAT().print("AT+CMSS=");
                streamAT().println(_outboxProcessing);
                _smsState = SMS_SM_WAIT_OUTBOX_SENT;
                break;
            }
        }

        // either no sms to send in queue or Modem is busy. come again later.
        _smsState = SMS_SM_GENERIC_DELAY;
        break;

    case SMS_SM_WAIT_OUTBOX_SENT: // module take about 120 sec to return result, depends on network strength
        if (millis() - _generalDelayTick > 120000)
        {
            // sms sending timeout
            DBG("[SMS] sending timeout\n");

            // release modem access
            ReleaseModemAccess(MDM_SENDING_SMS);

            // ToDo:
            // take action on what to do, to such unsent sms.
            // 1. keep on retry endlessly.
            // 2. keep on retry for specific count and then delete if retry count overflows to make space for new sms.
            _smsSendRetryCount[_outboxProcessing] += 1;
            if (_smsSendRetryCount[_outboxProcessing] >= SMS_MAX_RETRY_COUNT)
            {
                SmsDelete(_outboxProcessing);
            }

            _generalDelayTick = millis();
            _smsState = SMS_SM_GENERIC_DELAY;
        }
        else
        {
            int8_t ret = waitResponse2(2000, "+CMSS: ", GSM_ERROR);
            if (1 == ret)
            {
                // message sent.

                // release modem access
                ReleaseModemAccess(MDM_SENDING_SMS);

                SmsDelete(_outboxProcessing);

                // though we got success, we will take some nap and let other processes take module resources.
                _generalDelayTick = millis();
                _smsState = SMS_SM_GENERIC_DELAY;
            }
            else if (ret >= 2)
            {
                // message not sent.
                DBG("[SMS] sending fail\n");

                waitResponse3(); // read remaining

                // release modem access
                ReleaseModemAccess(MDM_SENDING_SMS);

                // ToDo:
                // take action on what to do, to such unsent sms.
                // 1. keep on retry endlessly.
                // 2. keep on retry for specific count and then delete if retry count overflows to make space for new sms.
                _smsSendRetryCount[_outboxProcessing] += 1;
                if (_smsSendRetryCount[_outboxProcessing] >= SMS_MAX_RETRY_COUNT)
                {
                    SmsDelete(_outboxProcessing);
                }

                _generalDelayTick = millis();
                _smsState = SMS_SM_GENERIC_DELAY;
            }
        }
        break;

    default:
        _smsState = SMS_SM_GENERIC_DELAY;
        break;
    }
}

/**
 * @fn bool SmsSubscribeCallback(QuecSmsCallBack cb)
 * @brief This function notify to user about new sms through call back. user need not to keep on reading inboxes.
 * @param QuecSmsCallBack cb, pointer to function notifying new sms text and sender number.
 * @return false: fail, true: successs
 * @remark either use callback method or IsSmsAvailable and ReadSms method.
 * @example
 * void smsArrived(const char *number, const char *sms, uint8_t smsLen)
 * {
 *    ...
 * }
 * bool status = SmsSubscribeCallback(smsArrived);
 */
bool SmsSubscribeCallback(QuecSmsCallBack cb)
{
    if (cb != NULL)
    {
        _smsCb = cb;
        return true;
    }

    return false;
}

/**
 * @fn bool IsSmsAvailable(uint8_t *inbox)
 * @brief This function notify if sms is in inbox or not.
 * @param uint8_t *inbox, pointer to integer number to which available inbox number will be assigned.
 * @return false: no sms, true: sms available
 * @remark this function rely on sms URC indication. AT+CNMI=2,1 command.
 * @example
 * uint8_t inbox = 0;
 * bool status = IsSmsAvailable(&inbox);
 * if(status == true)
 * {
 *    'inbox' must be greater than 0 and assigned a inboxnumber ready to be read.
 * }
 */
bool IsSmsAvailable(uint8_t *inbox)
{
    static uint8_t _inbox = 1;

    if (_inboxAvailable)
    {
        for (uint8_t i = 1; (i < MAX_SMS_INBOX && i < 32); i++)
        {
            if ((1 << i) & _inboxAvailable)
            {
                *inbox = i;
                _inbox = i; // begin with last succesfful read
                return true;
            }
        }
    }

#if (1)
    // if no new sms indications is received,
    // try to read incremental inbox
    _inbox++;
    if (_inbox > MAX_SMS_INBOX)
        _inbox = 1;

    *inbox = _inbox;
    return true;
#else
    return false;
#endif
}

/**
 * @fn bool SmsSendNow(const char *number, const char *msg)
 * @brief This function transmitting message to given number using SMS service.
 * @param const char *number, pointer to buffer holding receiving mobile number.
 * @param const char *msg, pointer to buffer holding text message to be send.
 * @return false: fail, true: success
 * @remark max text limit is modem dependent and it could be generally 150 bytes.
 *         Note. SMS sending takes few to 120 seconds depends on network strength.
 *         It is suggested to use SmsSendToQueue method if the sms sending is not urgent.
 * @example
 * const char moile_num[]="+919123457890";
 * const char text_msg[]="sample text message";
 * bool status = SmsSendNow(mobile_num, text_msg);
 */
bool SmsSendNow(const char *number, const char *msg)
{
    /*
    AT+CMGS="number"
    > msg ctrl+z
    +CMGW: <index>
    OK
    */

    if (!IsATavailable())
        return false;

    if (!IsModemReady())
        return false;

    streamAT().print("AT+CMGS=\"");
    streamAT().print(number);
    streamAT().print("\"\r\n");

    if (waitResponse2(2000, ">") != 1)
    {
        streamAT().write(0x1B); // ESC char
        return false;
    }

    streamAT().print(msg);
    streamAT().write(0x1A); // CTRL+Z char
    streamAT().flush();

    // module takes about 120 sec to return result. it is depends on network strength.
    int8_t ret = waitResponse2(30000, "+CMGS: ", GSM_ERROR);
    waitResponse3(GSM_OK);

    if (1 == ret)
    {
        return true;
    }

    return false;
}

/**
 * @fn bool SmsSendToQueue(const char *number, const char *msg)
 * @brief This function adding sms to outbox queue and it will be send later.
 * @param const char *number, pointer to buffer holding receiver's mobile number.
 * @param const char *msg, pointer to buffer holding text message to be send.
 * @return false: fail, true: success
 * @remark max text limit is modem dependent and it could be generally 150 bytes
 * @example
 * const char moile_num[]="+919123457890";
 * const char text_msg[]="sample text message";
 * bool status = SmsSendToQueue(mobile_num, text_msg);
 */
bool SmsSendToQueue(const char *number, const char *msg)
{
    /*
    AT+CMGW="number"
    > msg ctrl+z
    +CMGW: <index>
    OK
    */

    if (!IsATavailable())
        return false;

    if (!IsModemReady())
        return false;

    streamAT().print("AT+CMGW=\"");
    streamAT().print(number);
    streamAT().print("\"\r\n");

    if (waitResponse2(2000, ">") != 1)
    {
        streamAT().write(0x1B); // ESC char
        return false;
    }

    streamAT().print(msg);
    streamAT().write(0x1A); // CTRL+Z char
    streamAT().flush();

    int8_t ret = waitResponse2(10000, "+CMGW: ", GSM_ERROR);

    if (1 == ret)
    {
        uint8_t index = streamGetIntBefore('\r');
        waitResponse3(GSM_OK); // read out remaining

        _outboxAvailable |= (1 << index);
        _smsSendRetryCount[index] = 0;

        DBG("[SMS] added in Queue no. #%d\n", index);

        return true;
    }

    return false;
}

/**
 * @fn bool SmsRead(uint8_t inboxNum, char *senderNum, char *msgBuf, uint8_t msgBufSize)
 * @brief This function reading text SMS from given inbox number.
 * @param uint8_t inboxNum, sms inbox number to be read
 * @param char *senderNum, pointer to buffer to copy sender's mobile number.
 * @param char *msgBuf, pointer to buffer to copy text message.
 * @param uint16_t msgBufSize, size of msgBuff into which text message will be copy.
 * @return false: fail, true: success
 * @remark max text limit is modem dependent and it could be generally 150 bytes.
 *         sender's number could be about 15 bytes. keep buffer sizes accordingly.
 * @example
 * const char moile_num[16]="";
 * const char text_msg[160]="";
 * bool status = SmsRead(1, mobile_num, text_msg, sizeof(text_msg));
 */
bool SmsRead(uint8_t inboxNum, char *senderNum, char *msgBuf, uint8_t msgBufSize)
{
    /* Response:
    +CMGR: "REC UNREAD","+917028017882","","16/03/23,18:48:36+22"
    Text Message

    OK

    */

    if (!IsGsmNwConnected())
        return false;

    if (!IsATavailable())
        return false;

    if (!((GetModemCurrentOp() == MDM_INITIALIZING) || (GetModemCurrentOp() == MDM_IDLE)))
        return false;

    // if (!IsModemReady())
    //     return false;

    if (inboxNum > 15) // Neev: this is added just because white listed sim card has 15 msg store limit
    {
        inboxNum = 1;
    }

    streamAT().print("AT+CMGR=");
    streamAT().print(inboxNum);
    streamAT().print("\r\n");

    int8_t ret;
    ret = waitResponse2(2000, "+CMGR: ", GSM_OK, GSM_ERROR);
    if (ret == 1)
    {
        String state = streamAT().readStringUntil(',');

        streamSkipUntil('\"');
        int8_t size = streamAT().readBytesUntil('\"', senderNum, 15);
        senderNum[size] = 0;

        streamSkipUntil('\n');
        size = streamAT().readBytesUntil('\r', msgBuf, msgBufSize);
        msgBuf[size] = 0;

        waitResponse3(GSM_OK); // read out "OK"

        if (state.endsWith("\"REC UNREAD\""))
        {
            // we are considering only UNREAD messagess
            _outboxAvailable &= ~(1 << inboxNum);
            return true;
        }
        else if (state.endsWith("\"STO UNSENT\""))
        {
            _inboxAvailable &= ~(1 << inboxNum);
            _outboxAvailable |= (1 << inboxNum);
        }
        else
#if !(SMS_AUTO_DELETE_READ)
            if (state.endsWith("\"STO READ\""))
        {
            // sms is already read. user need to delete it to make space for new sms.
            return true;
        }
        else
#endif

#if !(SMS_AUTO_DELETE_SENT)
            if (state.endsWith("\"STO SENT\""))
        {
            // sms is already sent. user need to delete it to make space for new sms.
            return true;
        }
        else
#endif
        {
            // not required this state
            SmsDelete(inboxNum);
        }
    }

    if (ret == 2)
    {
        // OK received. SMS not present
        _inboxAvailable &= ~(1 << inboxNum);
        _outboxAvailable &= ~(1 << inboxNum);
    }

    return false;
}

/**
 * @fn bool SmsDelete(uint8_t inboxNum)
 * @brief This function deleting text SMS from given inbox number.
 * @param uint8_t inboxNum, sms inbox number to be read
 * @return false: fail, true: success
 * @remark this function deleting message specified by 'inboxNum'.
 * @example
 * bool status = Smsdelete(1);
 */
bool SmsDelete(uint8_t inboxNum)
{
    if (!IsATavailable())
        return false;

    if (!IsModemReady())
        return false;

    if (inboxNum > 15) // Neev: this is added just because white listed sim card has 15 msg store limit
    {
        inboxNum = 1;
    }

    streamAT().print("AT+CMGD=");
    streamAT().print(inboxNum);
    streamAT().print("\r\n");

    if (waitResponse2(2000, GSM_OK, GSM_ERROR) != 1)
        return false;

    _inboxAvailable &= ~(1 << inboxNum);
    _outboxAvailable &= ~(1 << inboxNum);
    _smsSendRetryCount[inboxNum] = 0;

    DBG("[SMS] #%d deleted\n", inboxNum);

    return true;
}

/**
 * @fn bool SmsDeleteAll(uint8_t deleteType)
 * @brief This function deleting text SMS from <mem1> message storage according to given choice.
 * @param uint8_t deleteType, 1: delete all read.
 *                            2: delete all read & sent.
 *                            3: delete all read, sent & unsent.
 *                            4: delete all read, sent,unsent & unread. means all messages
 * @return false: fail, true: success
 * @remark this function deleting message specified by 'inboxNum'.
 * @example deleteing all messages
 * bool status = Smsdelete(4);
 */
bool SmsDeleteAll(uint8_t deleteType)
{
    if (!IsATavailable())
        return false;

    if (!IsModemReady())
        return false;

    streamAT().print("AT+CMGD=1,");
    streamAT().print(deleteType);
    streamAT().print("\r\n");

    if (waitResponse2(2000, GSM_OK, GSM_ERROR) != 1)
        return false;

    _inboxAvailable = 0;
    _outboxAvailable = 0;
    memset(_smsSendRetryCount, 0, sizeof(_smsSendRetryCount));

    return true;
}

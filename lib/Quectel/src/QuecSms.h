#ifndef _QUECSMS_H
#define _QUECSMS_H

/**
 * SmsUrcType_t, various SMS URC types. 
 * AT+CNMI=2,1 New message is arrived and saved to memory.
 * AT+CNMI=2,2 New short message is received and output directly. 
              It could be Text/PDU format and currently not handled.
 */
typedef enum _SmsUrcType
{
    SMS_URC_TYPE_CMTI = 0, // AT+CNMI=2,1.
    SMS_URC_TYPE_CMT,      // AT+CNMI=2,2.
    SMS_URC_TYPE_CBM,      // AT+CNMI=2,2.
    SMS_URC_TYPE_CDS       // AT+CNMI=2,2.
} SmsUrcType_t;

/**
 * SmsStat_t, state of sms in memory. 
 */
typedef enum _SmsStat
{
    SMS_STAT_UNREAD = 0, // Received unread messages
    SMS_STAT_READ,       // Received read messages
    SMS_STAT_STO_UNSENT, // Stored unsent messages
    SMS_STAT_STO_SENT,   // Stored sent messages
    SMS_STAT_UNHANDLED
} SmsStat_t;

/**
 * Sms_t, structure to hold SMS infomration. 
 */
typedef struct _Sms
{
    char msg[160];     // sms text
    char number[15];   // sender number
    SmsStat_t smsStat; // sms stat
} Sms_t;

typedef void (*QuecSmsCallBack)(const char *number, const char *sms, uint8_t smsLen);

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
bool SmsSubscribeCallback(QuecSmsCallBack cb);

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
bool IsSmsAvailable(uint8_t *inbox);

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
bool SmsSendNow(const char *number, const char *msg);

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
bool SmsSendToQueue(const char *number, const char *msg);

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
bool SmsRead(uint8_t inboxNum, char *senderNum, char *msgBuf, uint8_t msgBufSize);

/**
 * @fn bool SmsDelete(uint8_t inboxNum)
 * @brief This function deleting text SMS from given inbox number.
 * @param uint8_t inboxNum, sms inbox number to be read
 * @return false: fail, true: success
 * @remark this function deleting message specified by 'inboxNum'.
 * @example 
 * bool status = Smsdelete(1);
 */
bool SmsDelete(uint8_t inboxNum);

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
bool SmsDeleteAll(uint8_t deleteType);

#endif // _QUECSMS_H

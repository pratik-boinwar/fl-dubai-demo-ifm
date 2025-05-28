/*
 *Zerowav Framework
 *Created on April 3, 2019
 *Support for Error handling.
 *Support for Timed loop operations.
 *Support for enabling and disabling debug mode.
 */
#ifndef ERRORS_H
#define ERRORS_H

#ifndef errChk
#define errChk(fCall)             \
  if (error = (fCall), error < 0) \
  {                               \
    goto Error;                   \
  }                               \
  else
#endif

enum ERRORS
{
  PASS = 0,
  NOP = -1,
  FILE_NOT_FOUND = -100,
  FILE_READ_ERROR = -101,
  FILE_WRITE_ERROR = -102,
  FILE_DELETE_ERROR = -103,
  FILE_APPEND_ERROR = -104,
  FILE_RENAME_ERROR = -105,
  DATABASE_CORRUTP = -106,
  SPIFFS_NOT_FOUND = -150,
  NO_CARD = -200,
  STA_MODE_ERROR = -300,
  AP_MODE_ERROR = -301,
  RTC_UPDATE_FAIL = -400,
  MQTT_CONNECTION_ERROR = -500,
  MQTT_BAD_PACKET = -501,
  MQTT_WRONG_COMMAND = -502,
  RECORD_NOT_FOUND = -600,
  RECORD_UPDATE_ERROR = -601,
  OFFLINE_SYNC_ERROR = -602,
  OTA_ERROR = -700,
  SQL_DB_OPEN_ERROR = -800,
  DESERIALZATION_ERROR = -900
};

/*=========================================================
  Function to Fetch Msg According to Error.
   Publisher: Zerowav
   Author: Salman Sadique
   Date Modified: April 8, 2019
  =========================================================*/
//    void errorMsg(int16_t errorCode, String *status,String *msg)
//    {
//         switch(errorCode)
//         {
//             case PASS:
//             {
//                 *msg = "Success";
//                 *status = "Pass";
//                 break;
//             }
//             case NOP:
//             {
//                 *msg = "No Operation";
//                 *status = "Null";
//                 break;
//             }
//             case FILE_NOT_FOUND:
//             {
//                 *msg = "File Not Found";
//                 *status = "Fail";
//                 break;
//             }
//              case FILE_READ_ERROR:
//             {
//                 *msg = "Error In Reading File";
//                 *status = "Fail";
//                 break;
//             }
//              case FILE_WRITE_ERROR:
//             {
//                 *msg = "Error In Writng File";
//                 *status = "Fail";
//                 break;
//             }
//              case FILE_DELETE_ERROR:
//             {
//                 *msg = "Error In Deleting File";
//                 *status = "Fail";
//                 break;
//             }
//              case FILE_APPEND_ERROR:
//             {
//                 *msg = "Error In Appending File";
//                 *status = "Fail";
//                 break;
//             }
//              case FILE_RENAME_ERROR:
//             {
//                 *msg = "Error In Renaming File";
//                 *status = "Fail";
//                 break;
//             }
//              case DATABASE_CORRUTP:
//             {
//                 *msg = "Error Database is Corrupt";
//                 *status = "Fail";
//                 break;
//             }
//              case SPIFFS_NOT_FOUND:
//             {
//                 *msg = "SPIFFS error";
//                 *status = "Fail";
//                 break;
//             }
//              case NO_CARD:
//             {
//                 *msg = "Card Detect Error";
//                 *status = "Fail";
//                 break;
//             }
//              case STA_MODE_ERROR:
//             {
//                 *msg = "Unable to Go to Station Mode";
//                 *status = "Fail";
//                 break;
//             }
//              case AP_MODE_ERROR:
//             {
//                 *msg = "Unable to Go to AP Mode";
//                 *status = "Fail";
//                 break;
//             }
//              case RTC_UPDATE_FAIL:
//             {
//                 *msg = "Unable to Update RTC";
//                 *status = "Fail";
//                 break;
//             }
//              case MQTT_CONNECTION_ERROR:
//             {
//                 *msg = "Unable to Connect to MQTT Broker";
//                 *status = "Fail";
//                 break;
//             }
//              case RECORD_NOT_FOUND:
//             {
//                 *msg = "Data not found in Local Database";
//                 *status = "Fail";
//                 break;
//             }
//              case RECORD_UPDATE_ERROR:
//             {
//                 *msg = "Data not updated in Local Database";
//                 *status = "Fail";
//                 break;
//             }
//             case OFFLINE_SYNC_ERROR:
//             {
//                 *msg = "Offline Sync error";
//                 *status = "Fail";
//                 break;
//             }
//             case MQTT_BAD_PACKET:
//             {
//                 *msg = "Recieved Packet Is corrupt";
//                 *status = "Fail";
//                 break;
//             }
//             case MQTT_WRONG_COMMAND:
//             {
//                 *msg = "Invalid Command";
//                 *status = "Fail";
//                 break;
//             }
//             case SQL_DB_OPEN_ERROR:
//             {
//                 *msg = "SQLite Open Error";
//                 *status = "Fail";
//                 break;
//             }
//             default:
//             {
//                 *msg = "Unknown Error";
//                 *status = "Fail";
//                 break;
//             }
//         }
//     }
#endif
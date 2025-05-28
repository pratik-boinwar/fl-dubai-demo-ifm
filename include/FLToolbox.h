#include "globals.h"
#include "WiFiClient.h"

extern WiFiClient Telnet;

/*
 *Zerowav Framework
 *Created on April 3, 2019
 *Support Error handling.
 *Support Timed loop operations.
 *Support Conted
 *Support Debugging.
 */

/*=========================================================
   DEBUG VARIALBE TO ACTIVATE DEBUG MODE. DEFINE IT 1 FOR
   ACTIVATING DEBUG MODE.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef DEBUG
#define DEBUG systemFlags.debugEnableFlag
#endif

/*=========================================================
   HANDEL FUNCTION WHEN NULL VALUE IS REFRENCED.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef nullChk
#define nullChk(fCall)             \
  if (error = (fCall), error == 0) \
  {                                \
    goto Error;                    \
  }                                \
  else
#endif

/*=========================================================
   HANDEL FUNCTION WHEN ERROR IS RETURNED. ACCEPTS ONLY
   NEGATIVE VALUES AS ERRORS.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef errChk
#define errChk(fCall)             \
  if (error = (fCall), error < 0) \
  {                               \
    goto Error;                   \
  }                               \
  else
#endif

/*=========================================================
   CALL FUNCTIONS WHEN DEBUG MODE IS ENABLED. FOR ENABLING
   DEBUG MODE DEFINE 'DEBUG 1' TO DISBALE DEFINE 'DEBUG 0'.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef debugSmt
#define debugSmt(funcall) \
  {                       \
    if (DEBUG)            \
    {                     \
      funcall             \
    }                     \
  }
#endif

/*=========================================================
   CALL FUNCTIONS WHEN DEBUG MODE IS ENABLED. FOR ENABLING
   DEBUG MODE DEFINE 'DEBUG 1' TO DISBALE DEFINE 'DEBUG 0'.
   Publisher: Zerowav
   Author: Manohar Reddy
   Modified by: Nivrutti Mahajan
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef debugPrint
#define debugPrint(param)     \
  {                           \
    if (DEBUG)                \
    {                         \
      Serial.print(param);    \
      if (Telnet.connected()) \
      {                       \
        Telnet.print(param);  \
      }                       \
    }                         \
  }
#endif

/*=========================================================
   CALL FUNCTIONS WHEN DEBUG MODE IS ENABLED. FOR ENABLING
   DEBUG MODE DEFINE 'DEBUG 1' TO DISBALE DEFINE 'DEBUG 0'.
   Publisher: Zerowav
   Author: Manohar Reddy
   Modified by: Nivrutti Mahajan
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef debugPrintln
#define debugPrintln(param)    \
  do                           \
  {                            \
    if (DEBUG)                 \
    {                          \
      Serial.println(param);   \
      if (Telnet.connected())  \
      {                        \
        Telnet.println(param); \
      }                        \
    }                          \
  } while (0);
#endif

/*=========================================================
   CALL FUNCTIONS WHEN DEBUG MODE IS ENABLED. FOR ENABLING
   DEBUG MODE DEFINE 'DEBUG 1' TO DISBALE DEFINE 'DEBUG 0'.
   Publisher: FountLab
   Author: Nivrutti Mahajan
   Date Modified: January 10, 2021
  =========================================================*/
#ifndef debugPrintf
#define debugPrintf(param, var)    \
  do                               \
  {                                \
    if (DEBUG)                     \
    {                              \
      Serial.printf(param, var);   \
      if (Telnet.connected())      \
      {                            \
        Telnet.printf(param, var); \
      }                            \
    }                              \
  } while (0);
#endif
/*=========================================================
   MAKE THE FUNCTION WAIT FOR DEFINED TIME. TIMEOUT IS ALWAYS
   IN MILLI SECONDS.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef timedWhile
#define timedWhile(funcall, timeout)           \
  {                                            \
    if (timeout > 0)                           \
    {                                          \
      unsigned long startTimer = millis();     \
      while (millis() - startTimer <= timeout) \
      {                                        \
        funcall                                \
      }                                        \
    }                                          \
    else                                       \
    {                                          \
      while (1)                                \
      {                                        \
        funcall                                \
      };                                       \
    }                                          \
  }
#endif

/*=========================================================
   MAKE THE FUNCTION RETY EXECUTION FOR DEFINED COUNT.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 6, 2019
  =========================================================*/
#ifndef reTry
#define reTry(funcall, count)             \
  {                                       \
    if (count > 0)                        \
    {                                     \
      for (uint8_t i = 0; i < count; i++) \
      {                                   \
        funcall                           \
      }                                   \
    }                                     \
  }
#endif

/*=========================================================
   MAKE THE FUNCTION WAIT FOR DEFINED TIME. TIMEOUT IS ALWAYS
   IN MILLI SECONDS.
   Publisher: Zerowav
   Author: Manohar Reddy
   Date Modified: April 3, 2019
  =========================================================*/
#ifndef APP_DEBUG
#define AppDebug(state)        \
  {                            \
    if (state == true)         \
    {                          \
      systemFlags.debugEnableFlag = true;  \
    }                          \
    else                       \
    {                          \
      systemFlags.debugEnableFlag = false; \
    }                          \
  }
#endif

/*!
 * @brief This class checks the status of reset switch and on continous press
 * of switch erases the current  wifi settings of the device and resets the controller.
 * @author Salman Sadique.
 */

#include "resetButton.h"
#include "FLToolbox.h"
#include "globals.h"

/**<Constructor*/
AppSwitch::AppSwitch()
{
    pinMode(BUTTON_PIN, INPUT);
}

/**<Destructor*/
AppSwitch::~AppSwitch()
{
}

/*!
 * @brief State Machine which Checks the state of switch and Checks for timer to trigger.
 * @param void (*func)()
 */
void AppSwitch::CheckButtonState(void (*func)())
{
    switch (APP_BUTTON_STATE)
    {

    case STATE_IDLE:

        if (SWITCH_OFF == digitalRead(BUTTON_PIN))
        {
            debugPrintln("Button Pressed");
            this->Timer(1);
            APP_BUTTON_STATE = IS_BUTTON_PRESSED;
        }

        break;

    case IS_BUTTON_PRESSED:

        if (SWITCH_ON == digitalRead(BUTTON_PIN))
        {
            debugPrintln("Button Released");
            this->Timer(0);
            APP_BUTTON_STATE = BUTTON_RELEASED;
        }

        this->CheckTimerValue(func);
        break;

    case BUTTON_RELEASED:
        APP_BUTTON_STATE = STATE_IDLE;
        break;

    default:
        debugPrintln("Invalid State");
        APP_BUTTON_STATE = STATE_IDLE;
        break;
    }
}

/*!
 * @brief Function is used to start or stop timer.
 * @param status 0 to stop timer, 1 to start timer.
 */
void AppSwitch::Timer(char status)
{

    if (status == 0)
    {
        debugPrintln("Timer Stopped");
        this->timerstate = 0;
        this->lastmillis = 0;
    }
    else if (status == 1)
    {
        this->timerstate = 1;
        this->lastmillis = millis();
        debugPrint("Timer Started at :");
        debugPrintln(this->lastmillis);
    }
    else
    {
    }
}

/*!
 *@brief Checks the timer continously and after successfull timeout calls the desired function.
 *@param void(*func)()
 */
void AppSwitch::CheckTimerValue(void (*func)())
{
    if (this->timerstate == 1)
    {
        if (millis() - this->lastmillis > MAX_TIMER_VAL)
        {
            APP_BUTTON_STATE = STATE_IDLE;
            gSystemFlag.btOrWifiAPEnableFlag = !gSystemFlag.btOrWifiAPEnableFlag;
            func();
            this->timerstate = 0;
            lastmillis = 0;
        }
    }
}

AppSwitch appSwitch;

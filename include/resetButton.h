#ifndef RESETBUTTON_H
#define RESETBUTTON_H

#include "Arduino.h"

#define MAX_TIMER_VAL 10000
#define SWITCH_ON 1
#define SWITCH_OFF 0
typedef enum _BUTTON_STATE_
{
    STATE_IDLE,
    IS_BUTTON_PRESSED,
    BUTTON_RELEASED,
} eBUTTON_STATE;

class AppSwitch
{
public:
    explicit AppSwitch();
    ~AppSwitch();
    void CheckButtonState(void (*func)());

protected:
    char timerstate = 0;
    unsigned long lastmillis = 0;
    eBUTTON_STATE APP_BUTTON_STATE = STATE_IDLE;
    void Timer(char status);
    void CheckTimerValue(void (*func)());
};

extern AppSwitch appSwitch;
#endif
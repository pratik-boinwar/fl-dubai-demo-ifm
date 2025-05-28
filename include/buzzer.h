#ifndef _BUZZER_H
#define _BUZZER_H

#include "Arduino.h"
#include <ESP32Servo.h>

#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 3000

void BuzzerInit(void);

void startup_tone();
void success_tone();
void error_tone();
void WiFiStart_tone();
void WiFiStop_tone();
void HotspotStart_tone();
void HotspotStop_tone();
#endif
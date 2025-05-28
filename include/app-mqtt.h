#ifndef APP_MQTT_H
#define APP_MQTT_H

#include "Arduino.h"

void AppMqttInit(void);
void AppMqttLoop(void);
bool AppMqttIsConnected(void);
bool AppMqttGetMqttStatus(void);
bool AppMqttSubscribe(const char *topic);
bool AppMqttPublish(const char *topic, const char *payload);

#endif // APP_MQTT_H
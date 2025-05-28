#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "Arduino.h"
#include "CircularBuffer.h"

void netclientSetup();
void NetclientLoop();

extern CIRCULAR_BUFFER smsRespBuf;

#endif // NETCLIENT_H
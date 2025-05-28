#ifndef SSL_CERT_H
#define SSL_CERT_H

#include "Arduino.h"

bool SSLCertFromUrl(const char *sslUrl, String certFileName = "");
void SSLCertInit(void);
void SSLCertLoop(void);

#endif // SSL_CERT_H
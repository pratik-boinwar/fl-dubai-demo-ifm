#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

// #include "EnteslaTCP.h"
#ifdef	__cplusplus
extern "C" {
#endif


// #define TCP_TX_BUFFER_SIZE 500
// #define TCP_RX_BUFFER_SIZE 500
// #define TCP_TERMINAL_RX_BUFFER_SIZE 500
// #define TERMINAL1_RX_BUFFER_SIZE 500
// #define TERMINAL1_TX_BUFFER_SIZE 500
// #define TERMINAL2_RX_BUFFER_SIZE 500
// #define TERMINAL2_TX_BUFFER_SIZE 500
// #define TERMINAL3_RX_BUFFER_SIZE 500
// #define TERMINAL3_TX_BUFFER_SIZE 500
// #define TERMINAL4_RX_BUFFER_SIZE 500
// #define TERMINAL4_TX_BUFFER_SIZE 500
#define MAIN_RESP_BUFFER_SIZE 500
// #define TCP_SERVER_TX_BUFFER_SIZE 500

#define BUFFER_SIZE 2500

typedef struct {
    volatile unsigned int size;
    volatile unsigned int start;
    volatile unsigned int end;
    volatile unsigned int current;
    volatile unsigned int startIndex;
    volatile char IsValid;
}  CIRCULAR_BUFFER;

extern unsigned int CbCreate(CIRCULAR_BUFFER *cirBuff, unsigned int size);
extern unsigned int CbPush(CIRCULAR_BUFFER *cirBuff, char ch);
extern unsigned int CbPop(CIRCULAR_BUFFER *cirBuff, char *pCh);
extern unsigned int CbPushS(CIRCULAR_BUFFER *cirBuff, const char *str);
extern unsigned int CbSize(CIRCULAR_BUFFER *cirBuff);
extern unsigned int CbSizeEmpty(CIRCULAR_BUFFER *cirBuff);
extern void CbClear(CIRCULAR_BUFFER *cirBuff);
extern unsigned int CbPeek(CIRCULAR_BUFFER *cirBuff, char *str, unsigned int size);
extern unsigned int CbGet(CIRCULAR_BUFFER *cirBuff, char *pCh, unsigned int loc);
extern unsigned int CbStrCpy(char *DestStr, CIRCULAR_BUFFER *cirBuff);
extern unsigned int CbStrStr(char *searchStr, CIRCULAR_BUFFER *cirBuff);


#ifdef	__cplusplus
}
#endif

#endif	// _CIRCULAR_BUFFER_H

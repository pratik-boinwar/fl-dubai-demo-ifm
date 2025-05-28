#include "CircularBuffer.h"
#include <string.h>
volatile static char _mainbuffer[BUFFER_SIZE];
static unsigned int _mainBufIndex = 0;

unsigned int CbCreate(CIRCULAR_BUFFER *cirBuff, unsigned int size)
{
    // validate whether we have available size
    if (!((BUFFER_SIZE - _mainBufIndex) >= size))
    {
        cirBuff->IsValid = 0;
        return 1;
    }
    cirBuff->start = 0;
    cirBuff->end = 0;
    cirBuff->current = 0;
    cirBuff->startIndex = _mainBufIndex;
    cirBuff->size = size;
    _mainBufIndex += size;
    cirBuff->IsValid = 1;

    return 0;
}

void CbClear(CIRCULAR_BUFFER *cirBuff)
{
    cirBuff->start = 0;
    cirBuff->end = 0;
    cirBuff->current = 0;
}

unsigned int CbPushS(CIRCULAR_BUFFER *cirBuff, const char *str)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    while (*str)
    {
        CbPush(cirBuff, *str++);
    }
    return 0;
}

unsigned int CbPush(CIRCULAR_BUFFER *cirBuff, char ch)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    _mainbuffer[cirBuff->end + cirBuff->startIndex] = ch;
    cirBuff->end = (cirBuff->end + 1) % cirBuff->size;

    if (cirBuff->start >= cirBuff->size)
    {
        return 1;
    }

    if (cirBuff->current < cirBuff->size)
    {
        cirBuff->current++;
    }
    else
    {
        // Overwriting the oldest. Move start to next-oldest
        cirBuff->start = (cirBuff->start + 1) % cirBuff->size;
        if (cirBuff->start >= cirBuff->size)
        {
            return 1;
        }
    }
    return 0;
}

unsigned int CbPop(CIRCULAR_BUFFER *cirBuff, char *pCh)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    if (!cirBuff->current)
    {
        return 1;
    }

    if (cirBuff->start >= cirBuff->size)
    {
        return 1;
    }
    *pCh = _mainbuffer[cirBuff->start + cirBuff->startIndex];

    cirBuff->start = (cirBuff->start + 1) % cirBuff->size;

    if (cirBuff->start >= cirBuff->size)
    {
        return 1;
    }
    cirBuff->current--;
    return 0;
}

// returns size of string peeked

unsigned int CbPeek(CIRCULAR_BUFFER *cirBuff, char *str, unsigned int size)
{
    unsigned int current = cirBuff->current, start = cirBuff->start;
    unsigned int i;

    if (cirBuff->IsValid != 1)
    {
        return 0;
    }
    
    if (!current)
    {
        return 0;
    }

    if (current > size)
    {
        current = size;
    }

    for (i = 0; i < current; i++)
    {
        str[i] = _mainbuffer[start + cirBuff->startIndex];
        start = (start + 1) % cirBuff->size;
    }
    //str[i] = 0;
    return i;
}

unsigned int CbSize(CIRCULAR_BUFFER *cirBuff)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    return cirBuff->current;
}

unsigned int CbSizeEmpty(CIRCULAR_BUFFER *cirBuff)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    
    return cirBuff->size - cirBuff->current;
}

unsigned int CbGet(CIRCULAR_BUFFER *cirBuff, char *pCh, unsigned int loc)
{
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    
    if (!cirBuff->current)
    {
        return 1;
    }

    if (loc >= cirBuff->current) //previously it was loc >= cirBuff->current changed by neeraj
    {
        return 1;
    }

    *pCh = _mainbuffer[((cirBuff->start + loc) % cirBuff->size) + cirBuff->startIndex];


    return 0;
}

unsigned int CbStrCpy(char *destStr, CIRCULAR_BUFFER *cirBuff)
{
    unsigned int bytesCopied;
    
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    
    bytesCopied = 0;
    while (!CbPop(cirBuff, destStr + bytesCopied))
    {
        bytesCopied++;
    }

    destStr[bytesCopied] = 0;

    return (bytesCopied);

}

unsigned int CbStrStr(char *searchStr, CIRCULAR_BUFFER *cirBuff)
{
    int strLen, startLoc = 0, i = 0, stat = 0;
    char ch;
    
    if (cirBuff->IsValid != 1)
    {
        return 1;
    }
    
    strLen = strlen(searchStr);

    do
    {
        stat = CbGet(cirBuff, &ch, startLoc);
        startLoc++;
        if (ch == searchStr[i])
        {
            for (i = 1; i < strLen; i++)
            {
                CbGet(cirBuff, &ch, startLoc);
                if (ch != searchStr[i]) //no match
                    break; //return (0);
                startLoc++;
            }
            if (i == strLen)
                return (startLoc - strLen);
            i = 0;
        }

    }
    while (0 == stat);

    return (0);
}
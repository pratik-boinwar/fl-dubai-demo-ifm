#include <string.h>
#include <math.h>
#include "helper.h"
#include <stdlib.h>
#include <ctype.h>
#include <Arduino.h>

char hexTab[16] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
};

int HexCharToByte(char strData)
{
    int x = 0;
    char c = strData;
    if (c >= '0' && c <= '9')
    {
        x *= 16;
        x += c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
        x *= 16;
        x += (c - 'A') + 10;
    }
    else if (c >= 'a' && c <= 'f')
    {
        x *= 16;
        x += (c - 'a') + 10;
    }
    return x;
}

bool HexStrToByteArr(char *str, BYTE *arr)
{
    unsigned char i, j;
    unsigned char strLength;

    strLength = strlen(str);

    if (strLength == 0)
    {
        return false;
    }

    i = 0;
    j = 0;
    // check whether string length is even or odd
    if ((strLength) % 2 != 0)
    {
        if (isxdigit(str[0]))
        {
            arr[0] = HexCharToByte(str[0]);
            i = 1;
            j = 1;
        }
        else
        {
            return false;
        }
    }
    for (; i < strLength - 1; i = i + 2, j++)
    {
        if (isxdigit(str[i]) && isxdigit(str[i + 1]))
        {
            arr[j] = (HexCharToByte(str[i]) << 4) | (HexCharToByte(str[i + 1]));
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool HexByteArrToStr(char *str, BYTE *arr, unsigned char size)
{
    unsigned char i;
    BYTE value;
    for (i = 0; i < size; i++)
    {
        value = *arr;
        *str++ = hexTab[value >> 4];
        *str++ = hexTab[value & 0x0f];
        arr++;
    }
    *str = '\0';
    return true;
}

/**
 * @fn void ToUpper(CHAR *s)
 * @brief Converts string to Upper case
 * @param CHAR *s
 * @remark
 */
void ToUpper(CHAR *s)
{
    UINT32 strLen, i;

    strLen = strlen(s);

    for (i = 0; i < strLen; i++)
    {
        if (s[i] >= 'a' && s[i] <= 'z')
        {
            s[i] -= 32; // 'a' - 'A' = 32
        }
    }
}

// reverses a string 'str' of length 'len'

/**
 * @fn void reversen(CHAR *str, INT32 len)
 * @brief reverses a string till given length
 * @param CHAR *str
 * @param INT32 len
 * @remark
 */
void reversen(CHAR *str, INT32 len)
{
    INT32 i = 0, j = len - 1, temp;
    while (i < j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

/**
 * @fn void reverse(CHAR *s)
 * @brief reverse a whole string
 * @param CHAR *s
 * @remark
 */
void reverse(CHAR *s)
{
    UINT32 i, j, strLen;
    UINT8 c;

    strLen = strlen(s);
    for (i = 0, j = strLen - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/**
 * @fn void IntToAscii(INT32 n, CHAR *s)
 * @brief Converts integer to ASCII
 * @param INT32 n
 * @param CHAR *s
 * @remark
 */
void IntToAscii(INT32 n, CHAR *s)
{
    INT32 sign;
    UINT32 i;

    if ((sign = n) < 0) // record sign
        n = -n;         // make n positive
    i = 0;
    do
    {                          // generate digits in reverse order
        s[i++] = n % 10 + '0'; // get next digit
    } while ((n /= 10) > 0);   // delete it
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

CHAR *IntToStr(INT32 n)
{
    INT32 sign;
    UINT32 i;
    CHAR *s = NULL;
    if ((sign = n) < 0) // record sign
        n = -n;         // make n positive
    i = 0;
    do
    {                          // generate digits in reverse order
        s[i++] = n % 10 + '0'; // get next digit
    } while ((n /= 10) > 0);   // delete it
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    return s;
}

// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.

/**
 * @fn INT32 intToStr(INT32 x, CHAR str[], INT32 d)
 * @brief Converts a given integer x to string str[].  d is the number
 *        of digits required in output. If d is more than the number
 *        of digits in x, then 0s are added at the beginning.
 * @param INT32 x
 * @param CHAR str[]
 * @param INT32 d
 * @remark
 */
INT32 intToStr(INT32 x, CHAR str[], INT32 d)
{
    INT32 i = 0;
    INT32 sign;

    if ((sign = x) < 0) // record sign
        x = -x;         // make n positive
    while (x)
    {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
    if (sign < 0)
        str[i++] = '-';
    reversen(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating point number to string.

/**
 * @fn void Ftoa(float n, CHAR *res, INT32 afterpoint)
 * @brief Converts a floating point number to string.
 * @param float n
 * @param CHAR *res
 * @param INT32 afterpoint
 * @remark
 */
void Ftoa(float n, CHAR *res, INT32 afterpoint)
{
    // Extract integer part
    INT32 ipart = (INT32)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    INT32 i = intToStr(ipart, res, 0);

    if (fpart < 0)
    {
        fpart = -fpart;
    }
    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.'; // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((INT32)fpart, res + i + 1, afterpoint);
    }
}

/**
 * @fn void LongToAscii(INT64 n, CHAR *s)
 * @brief Converts Long to ASCII
 * @param INT64 n
 * @param CHAR *s
 * @remark
 */
void LongToAscii(INT64 n, CHAR *s)
{
    INT64 sign;
    UINT32 i;

    if ((sign = n) < 0) // record sign
        n = -n;         // make n positive
    i = 0;
    do
    {                          // generate digits in reverse order
        s[i++] = n % 10 + '0'; // get next digit
    } while ((n /= 10) > 0);   // delete it
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/**
 * @fn UINT32 AtoIn(CHAR *str, UINT32 len)
 * @brief Convert Char array to integer till the given length
 * @param CHAR *str
 * @param UINT32 len
 * @remark
 */
UINT32 AtoIn(CHAR *str, UINT32 len)
{
    INT32 res = 0; // Initialize result
    UINT32 i;
    INT32 sign = 1;

    for (i = 0; str[i] && (i < len); i++)
    {
        if (str[i] == ' ')
            continue;
        else if (str[i] == '-')
            sign = -1;
        else if (str[i] == '+')
            sign = 1;
        else
            break;
    }
    // Iterate through all characters of input string and
    // update result
    for (; (str[i] != '\0') && (i < len); ++i)
    {
        res = res * 10 + str[i] - '0';
    }

    // return result.
    return res * sign;
}

/**
 * @fn UINT8 ConvertToHexLowerNibble(UINT8 num)
 * @brief Extract Hex Lower nibble from integer
 * @param UINt8 num
 * @remark
 */
UINT8 ConvertToHexLowerNibble(UINT8 num)
{
    num = num & 0x0F;

    if (num < 10)
    {
        return ('0' + num);
    }
    else
    {
        num = num - 10;
        return ('A' + num);
    }
}

/**
 * @fn UINT8 ConvertToHexHigherNibble(UINT8 num)
 * @brief Extract Hex Higher nibble from integer
 * @param UINT8 num
 * @remark
 */
UINT8 ConvertToHexHigherNibble(UINT8 num)
{
    num = num >> 4;

    if (num < 10)
    {
        return ('0' + num);
    }
    else
    {
        num = num - 10;
        return ('A' + num);
    }
}

/**
 * @fn UINT32 CpyChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt)
 * @brief Copies "cnt" number of bytes from array 2 to array 1
 * @param UINT8 *arr1
 * @param UINT8 *arr2
 * @param UINT32 cnt
 * @remark
 */
UINT32 CpyChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt)
{
    UINT32 i;

    for (i = 0; i < cnt; i++)
    {
        arr1[i] = arr2[i];
    }

    return 1;
}

/**
 * @fn UINT32 CmpChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt)
 * @brief Compares two array till "cnt" number of bytes
 * @param UINT8 *arr1
 * @param UINT8 *arr2
 * @param UINT32 cnt
 * @remark
 */
UINT32 CmpChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt)
{
    UINT32 i;

    for (i = 0; i < cnt; i++)
    {
        if (arr1[i] != arr2[i])
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @fn UINT32 IntStrPadZeros(CHAR *str, UINT8 size)
 * @brief Zero padding in a string till size length
 * @param CHAR *str
 * @param UINT8 size
 * @remark
 */
UINT32 IntStrPadZeros(CHAR *str, UINT8 size)
{
    CHAR zeroStr[] = "0000000000";
    CHAR _size;
    CHAR _sizeof;

    _sizeof = strlen(str);

    _size = (CHAR)size - _sizeof;
    if (_size <= 0)
    {
        return 0;
    }
    else
    {
        zeroStr[(UINT32)_size] = 0;
        strcat(zeroStr, str);
        strcpy(str, zeroStr);
        return 1;
    }
}

/**
 * @fn UINT32 IsDecimalNum(CHAR *s)
 * @brief Check if string contains decimal numbers
 * @param CHAR *s
 * @remark
 */
UINT32 IsDecimalNum(CHAR *s)
{
    UINT32 strLen, i;

    strLen = strlen(s);

    for (i = 0; i < strLen; i++)
    {
        if (!((s[i] >= '0') && (s[i] <= '9')))
        {
            if (s[i] != '.')
            {
                if (s[i] != '-')
                {
                    if (s[i] != '+')
                    {
                        return 0;
                    }
                }
            }
        }
    }

    return 1;
}

/**
 * @fn UINT32 IsNumN(CHAR *s, UINT32 len)
 * @brief Check if string contains only Numerical till "len" length
 * @param CHAR *s
 * @remark
 */
UINT32 IsNumN(CHAR *s, UINT32 len)
{
    UINT32 i;

    for (i = 0; s[i] && (i < len); i++)
    {
        if (s[i] == ' ')
            continue;
        else if (s[i] == '-')
            continue;
        else if (s[i] == '+')
            continue;
        else
            break;
    }
    for (; i < len; i++)
    {

        if (!((s[i] >= '0') && (s[i] <= '9')))
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @fn void ToUpper(CHAR *s)
 * @brief Check if string contains only Numerical
 * @param CHAR *s
 * @remark
 */
UINT32 IsNum(CHAR *s)
{
    UINT32 strLen, i;

    strLen = strlen(s);

    for (i = 0; i < strLen; i++)
    {

        if (!((s[i] >= '0') && (s[i] <= '9')))
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @fn UINT8* StringToken(UINT8 *str, UINT8 *token)
 * @brief Finds token in a string
 * @param UINT8 *str
 * @param UINT8 *token
 * @remark
 */
UINT8 *StringToken(UINT8 *str, UINT8 *token)
{
    static UINT8 *_pStr = NULL;
    static UINT8 flagDone = 0;

    UINT8 *start;

    if (NULL == str)
    {
        if (1 == flagDone)
        {
            return NULL;
        }

        _pStr++;
        start = _pStr;
    }
    else
    {
        flagDone = 0;
        _pStr = str;
        start = _pStr;
    }

    while ((*_pStr != *token))
    {
        if ((*_pStr == 0))
        {
            flagDone = 1;
            return start;
        }
        _pStr++;
    }
    *_pStr = 0;

    return start;
}

// Function to replace a string with another
// string
bool ReplaceWordInString(const char *string, const char *oldW,
                         const char *newW, char *output)
{
    char result[256];
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word
    // occur in the string
    for (i = 0; string[i] != '\0'; i++)
    {
        if (strstr(&string[i], oldW) == &string[i])
        {
            cnt++;

            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }

    // Making new string of enough length
    // result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);
    if (sizeof(result) < (i + cnt * (newWlen - oldWlen) + 1))
    {
        return false;
    }
    i = 0;
    while (*string)
    {
        // compare the substring with the result
        if (strstr(string, oldW) == string)
        {
            strcpy(&result[i], newW);
            i += newWlen;
            string += oldWlen;
        }
        else
            result[i++] = *string++;
    }

    result[i] = '\0';
    strncpy(output, result, strlen(result));
    return true;
}

// void printDigit(int N)
// {
//     // To store the digit
//     // of the number N
//     int arr[MAX];
//     int i = 0;
//     int j, r;

//     // Till N becomes 0
//     while (N != 0) {

//         // Extract the last digit of N
//         r = N % 100;

//         // Put the digit in arr[]
//         arr[i] = r;
//         i++;

//         // Update N to N/10 to extract
//         // next last digit
//         N = N / 100;
//     }

// }

// // Routine for checking if string is a valid number
// bool isFloat(String str)
// {
//     String tBuf;
//     bool decPt = false;

//     if (str.charAt(0) == '+' || str.charAt(0) == '-')
//         tBuf = &str[1];
//     else
//         tBuf = str;

//     for (int x = 0; x < tBuf.length(); x++)
//     {
//         if (tBuf.charAt(x) == '.')
//         {
//             if (decPt)
//                 return false;
//             else
//                 decPt = true;
//         }
//         else if (tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9')
//             return false;
//     }
//     return true;
// }

float ToFloat(uint16_t value1, uint16_t value2)
{
    unsigned long temp = (value1 << 16 | value2);
    return *(float *)&temp;
}

long HexStrtoInt(char *hex)
{
    long decimal, place;
    int i = 0, val, len;

    decimal = 0;
    place = 1;

    /* Find the length of total number of hex digit */
    len = strlen(hex);
    len--;

    /*
     * Iterate over each hex digit
     */
    for (i = 0; hex[i] != '\0'; i++)
    {

        /* Find the decimal representation of hex[i] */
        if (hex[i] >= '0' && hex[i] <= '9')
        {
            val = hex[i] - 48;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f')
        {
            val = hex[i] - 97 + 10;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F')
        {
            val = hex[i] - 65 + 10;
        }

        decimal += val * pow(16, len);
        len--;
    }

    return decimal;
}

void RemoveQuotes(char *str)
{
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"')
    {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// Function to extract the last numeric value from a string
int ExtractLastNumericValue(const char *input)
{
    int length = strlen(input);
    int result = 0;     // Initialize the result to 0
    int multiplier = 1; // Used to calculate the numeric value

    for (int i = length - 1; i >= 0; i--)
    {
        if (isdigit(input[i]))
        {
            // If the character is a digit, calculate its contribution to the result
            result += (input[i] - '0') * multiplier;
            multiplier *= 10; // Update the multiplier for the next digit
        }
        else if (result > 0)
        {
            // If a non-numeric character is encountered after numeric characters,
            // it means we've collected the numeric value.
            break;
        }
    }

    return result;
}

float RoundToDecimalPlaces(float number, int decimalPlaces)
{
    float multiplier = pow(10.0, decimalPlaces);
    return round(number * multiplier) / multiplier;
}

double MapFloat(double val, double in_min, double in_max, double out_min, double out_max)
{
    return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void stringToUint8Array(const char *str, uint8_t *uint8Array, size_t arraySize)
{
    size_t strLen = strlen(str);
    if (strLen > arraySize)
    {
        strLen = arraySize; // Ensure not to overflow the array
    }
    for (size_t i = 0; i < strLen; i++)
    {
        uint8Array[i] = (uint8_t)str[i];
    }
}
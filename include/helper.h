#ifndef _HELPER_H
#define _HELPER_H

#include "generic_typedefs.h"

extern int HexCharToByte(char strData);
extern bool HexStrToByteArr(char *str, BYTE *arr);
extern bool HexByteArrToStr(char *str, BYTE *arr, unsigned char size);
extern void IntToAscii(INT32 n, CHAR *s);
extern CHAR *IntToStr(INT32 n);
extern void LongToAscii(INT64 n, CHAR *s);
extern UINT32 AtoIn(CHAR *str, UINT32 len);
extern UINT8 ConvertToHexLowerNibble(UINT8 num);
extern UINT8 ConvertToHexHigherNibble(UINT8 num);
extern UINT32 CmpChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt);
extern UINT32 CpyChArray(UINT8 *arr1, UINT8 *arr2, UINT32 cnt);
extern UINT32 IntStrPadZeros(CHAR *str, UINT8 size);
extern void ToUpper(CHAR *s);
extern UINT32 IsNumN(CHAR *s, UINT32 len);
extern UINT32 IsNum(CHAR *s);
// // Routine for checking if string is a valid number
// bool isFloat(String tString);
extern UINT32 IsDecimalNum(CHAR *s);
extern UINT8 *StringToken(UINT8 *str, UINT8 *token);
extern void Ftoa(float n, CHAR *res, INT32 afterpoint);
// Function to replace a string with another
// string
bool ReplaceWordInString(const char *string, const char *oldW,
                         const char *newW, char *output);

/*!
 *  @brief  REAL4 format to Float Conversion
 *
 *  It will convert 32 bit REAL4 format to float value
 *  By combining two 16 bit values
 *
 *  @param value1 16 bit value
 *  @param value2 16 bit value
 *
 */
float ToFloat(UINT16 value1, UINT16 value2);

long HexStrtoInt(char *hex);
void RemoveQuotes(char *str);
// Function to extract the last numeric value from a string
int ExtractLastNumericValue(const char *input);
float RoundToDecimalPlaces(float number, int decimalPlaces);
double MapFloat(double val, double in_min, double in_max, double out_min, double out_max);

void stringToUint8Array(const char *str, UINT8 *uint8Array, size_t arraySize);
#endif //  _HELPER_H
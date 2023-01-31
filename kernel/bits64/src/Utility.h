#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"
#include <stdarg.h>

void kMemSet(void* pvDestination, BYTE bData, int iSize);
int kMemCpy(void* pvDestination, const void* pvSource, int iSize);
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

BOOL kSetInterruptFlag(BOOL bEnableInterrupt);

void kCheckTotalRAMSize(void);
QWORD kGetTotalRAMSize(void);
void kReverseString(char* pcBuffer);

#endif
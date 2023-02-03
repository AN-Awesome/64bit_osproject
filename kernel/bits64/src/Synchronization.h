#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

// Structure
// 1Byte Sort

#pragma pack(push, 1)

// Mutex Data Structure
typedef struct kMuteStruct {
    volatile QWORD qwTaskID;    // Task ID
    volatile DWORD dwLockCount; // Number of Locks
    volatile BOOL bLockFlag;    // LockFlag
    BYTE vbPadding[3];          // field : Set the size of a data structure to 8 bytes
} MUTEX;

#pragma pack(pop)

// Function
BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL bInterruptFlag);
void kInitializeMutex(MUTEX* pstMutex);
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);

#endif
#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"

// Locking functions for system-wide data
BOOL kLockForSystemData(void) {
    return kSetInterruptFlag(FALSE);
}

// Unlock function for system-wide data
void kUnlockForSystemData(BOOL bInterruptFlag) {
    kSetInterruptFlag(bInterruptFlag);
}

// Init MUTEX
void kInitializeMutex(MUTEX* pstMutex) {
    pstMutex->bLockFlag = FALSE;
    pstMutex->dwLockCount = 0;              // Locked flag and count
    pstMutex->qwTaskID = TASK_INVALIDID;    // Init Task ID
}

// Locking functions for data used between tasks
void kLock(MUTEX* pstMutex) {
    // If it's already locked, check if I'm locked, increase the number of locks, and pay the total
    if(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE ) {
        // If you locked yourself, only increase the number of times
        if(pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID) { 
            pstMutex->dwLockCount++;
            return ;
        }

        // If it is not you, wait until the locked thing is unlocked
        while(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) kSchedule();
    }

    // Setting locks, locked flags are handled by the kTestAndSet() function above.
    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask( )->stLink.qwID;
}

// Unlock function for data used between tasks
void kUnlock(MUTEX* pstMutex) {
    // Fail unless the task locks the mutex.
    if((pstMutex->bLockFlag == FALSE) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID)) return;

    // If the mutex is locked redundantly, only the number of locks is reduced.
    if(pstMutex->dwLockCount > 1) {
        pstMutex->dwLockCount--;
        return ;
    }

    pstMutex->qwTaskID = TASK_INVALIDID;    // set as off
    pstMutex->dwLockCount = 0;
    pstMutex->bLockFlag = FALSE;            // Locked flags must be unlocked last
}
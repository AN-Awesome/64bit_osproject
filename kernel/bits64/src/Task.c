#include "Task.h"
#include "Descriptor.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

//==========================
// Task Pool & Task-related
//==========================
// Init Task Pool
static void kInitializeTCBPool(void) {
    int i;
    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));
    
    // Set Task Pool Address and Init
    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);
    // Allocate ID at TCB
    for( i = 0; i < TASK_MAXCOUNT; i++) gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    // Init TCB's Max Count & Allocate Count
    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}
// Allocate TCB
static TCB* kAllocateTCB(void) {
    TCB* pstEmptyTCB;
    int i;

    if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) return NULL;
    
    for(i = 0; i < gs_stTCBPoolManager.iMaxCount; i++) {
        // Unallocated TCB if ID's Upper 32 bit is 0
        if ((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
            pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
            break;
        }
    }
    // Set Upper 32 bit non-0 to allocated TCB
    pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if(gs_stTCBPoolManager.iAllocatedCount == 0) gs_stTCBPoolManager.iAllocatedCount = 1;
    return pstEmptyTCB;
}
// Clear TCB
static void kFreeTCB(QWORD qwID) {
    int i;
    // Lower 32 bit of Task ID act as Index
    i = GETTCBOFFSET(qwID);

    // Init TCB & Set ID
    kMemSet( &(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

// Create Task
// Stack auto Allocate from Stack Pool according to Task ID
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB* pstTask;
    void* pvStackAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    pstTask = kAllocateTCB();
    if(pstTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }
    kUnlockForSystemData(bPreviousFlag);


    // Calculate Stack Address with Task ID, Lower 32 bit act as offset of Stack Pool
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));
    // Set up TCB and insert it into the list so it can be Scheduled
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    bPreviousFlag = kLockForSystemData();   // EntryPoint
    kAddTaskToReadyList(pstTask);           // Add Task to Read List
    kUnlockForSystemData(bPreviousFlag);    // EndPoint

    return pstTask;
}

/*
// Use Parameter to set TCB
void kSetupTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize) {
    // 생략된 부분 부록 파일 Task.c 참조
}

*/

//===================
// Scheduler-related
//===================
// Init Scheduler
void kInitializeScheduler(void) {
    int i;
    // Task Pool Init
    kInitializeTCBPool();
    
    // Init ReadyList & Execution Count by Priority & WaitList
    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i] = 0;
    } kInitializeList(&(gs_stScheduler.stWaitList));

    // Assign TCB and set it as a running task
    // Preparing a TCB to store booted tasks
    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    // Init Data Structure (Calculate processor utilization)
    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;
}
// Set Current Perform Task
void kSetRunningTask(TCB* pstTask) {
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();       // EntryPoint
    gs_stScheduler.pstRunningTask = pstTask;
    kUnlockForSystemData(bPreviousFlag);        // EndPoint
}
// Return Current Perform Task
TCB* kGetRunningTask(void) {
    BOOL bPreviouisFlag;
    TCB *pstRunningTask;

    bPreviouisFlag = kLockForSystemData();              // EntryPoint
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kUnlockForSystemData(bPreviouisFlag);               // Endoint
}

// Get the next task to run in the task list
static TCB* kGetNextTaskToRun(void) {
    TCB* pstTarget = NULL;
    int iTaskCount, i, j ;

    // When all queue tasks are executed once -> may not be able to select a task
    // So In case of NULL, execute one more time
    for(j = 0; j < 2; j++) {
        // Choose task to schedule (Check the list from high priority to low priority)
        for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

            // If the number of tasks in the list is greater than the number of executions
            // Execute the task of the current priority
            if(gs_stScheduler.viExecuteCount[i] < iTaskCount) {
                pstTarget = (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
                gs_stScheduler.viExecuteCount[i]++;
                break;
            } else gs_stScheduler.viExecuteCount[i] = 0; // If the number of executions is higher, the number of executions is initialized and yielded to priority.
        }
        // If task to be performed is found -> Exit
        if(pstTarget != NULL) break;
    }
    return pstTarget;
}

// Insert the task into the scheduler's ready list
static BOOL kAddTaskToReadyList(TCB* pstTask) {
    BYTE bPriority;

    bPriority = GETPRIORITY(pstTask->qwFlags);
    if(bPriority >= TASK_MAXREADYLISTCOUNT) return FALSE;

    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
    return TRUE;
}

// Remove task from ready queue
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID) {
    TCB* pstTarget;
    BYTE bPriority;

    // If task ID is invalid -> FAIL
    if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT) return NULL;

    // Find the TCB of the task in the TCB pool and check if the ID actually matches
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if(pstTarget->stLink.qwID != qwTaskID) return NULL;

    // Remove a task from the ready list where the task exists
    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
    return pstTarget;
}

// Change the priority of a task
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    TCB* pstTarget;
    BOOL bPreviousFlag;

    if(bPriority > TASK_MAXREADYLISTCOUNT) return FALSE;

    bPreviousFlag = kLockForSystemData();   // EntryPoint

    // If the task is currently running, only change the priority
    // When an interrupt (IRQ 0) of the PIT controller occurs and task switching is performed, move to the list of changed priorities.
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) SETPRIORITY(pstTarget->qwFlags, bPriority);
    else { // if it is not a witness-to-execute task, find it in the ready list and move it to the list at that priority.
        // If the task is not found in the ready list, it finds the task manually and sets the priority.
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
        if(pstTarget == NULL) {
            // Find and set directly by task ID
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL) SETPRIORITY(pstTarget->qwFlags, bPriority); // Set priorities
            else {
                // Set priority and insert back into ready list
                SETPRIORITY(pstTarget->qwFlags, bPriority);
                kAddTaskToReadyList(pstTarget);
            }
        }
    }
    kUnlockForSystemData(bPreviousFlag);    // EndPoint
    return TRUE;
}


// Find another Task and Switch
void kSchedule(void) {
    TCB* pstRunningTask, * pstNextTask;
    BOOL bPreviousFlag;
    // Need Task to Switch to
    if(kGetReadyTaskCount() < 1) return;


    // Prevent Interrupt during switchover which is troublesome if the task switchover occurs again
    bPreviousFlag = kLockForSystemData();   // EntryPoint

    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag); // EndPoint
        return;
    }
    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    // Increase processor time used if diverted from idle task
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;

    // If the task end flag is set, the context does not need to be saved, so insert into the waiting list and switch the context
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kSwitchContext(NULL, &(pstNextTask->stContext));
    } else {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    // Update Processor Using time
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    kUnlockForSystemData(bPreviousFlag);
}
// Find another Task and Switch when Interrupt Occured
BOOL kScheduleInInterrupt(void) {
    TCB* pstRunningTask, *pstNextTask;
    char* pcContextAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();       // EntryPoint

    // Exit if there is no Task to Switch
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);    // EndPoint
        return FALSE;
    }

    //==============
    // Task Switch
    //==============
    pcContextAddress = (char*) IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
    // Get current Task, Copy Context in IST & Move current task to Ready List
    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    //Increase processor time used if diverted from idle task
    if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;

    // If the task end flag is set, do not save the context, only insert into the waiting list
    if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    else { // If the task is not finished, copy the context in IST and move the current task to the ready list.
        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kUnlockForSystemData(bPreviousFlag);    // EndPoint

    // Set the task to be switched and executed as Running Task and copy the context to IST so that task switching occurs automatically
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    // Update Processor Using Time
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    return TRUE;
}
// Decrease Processor Time
void kDecreaseProcessorTime(void) {
    if(gs_stScheduler.iProcessorTime > 0) gs_stScheduler.iProcessorTime--;
}
// Return whether Processor is out of time to use
BOOL kIsProcessorTimeExpired(void) {
    if(gs_stScheduler.iProcessorTime <= 0) return TRUE;
    return FALSE;
}

// Set TCB using Parameter
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize) {
    // Init Context
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));
    // Set RSP, RBP Register related on Stack
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD) pvStackAddress + qwStackSize;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD) pvStackAddress + qwStackSize;
    // Set Segment Selector
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;
    // Set RIP Register & Interrupt Flag
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;
    // Enable Interrupt by setting IF bit (bit 9) of RFLAGS Register to 1.
    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

    // Stack, Flag
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

// TASK EXIT
BOOL kEndTask(QWORD qwTaskID) {
    TCB* pstTarget;
    BYTE bPriority;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();   // EntryPoint

    // If the task is currently running, set the EndTask bit and switch the task.
    pstTarget = gs_stScheduler.pstRunningTask;
    if(pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSystemData(bPreviousFlag);    // EndPoint

        kSchedule();

        // The code below never runs because the task has been switched
        while(1);
    } else { // If the task is not found in the ready list, it directly finds the task and sets the task end bit.
        pstTarget = kRemoveTaskFromReadyList(qwTaskID);
        if(pstTarget == NULL) {
            pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
            if(pstTarget != NULL) {
                // Find and set directly by task ID
                pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
                SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
            }
            kUnlockForSystemData(bPreviousFlag);    // EndPoint
            return TRUE;
        }
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
    }
    return TRUE;
}

// Task terminates itself
void kExitTask(void) {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

// Returns the number of all tasks in the ready queue
int kGetReadyTaskCount(void) {
    int iTotalCount = 0;
    int i;

    // Check all ready queues to get the number of tasks
    for(i=0; i<TASK_MAXREADYLISTCOUNT; i++) iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    return iTotalCount;
}

// Returns the total number of tasks
int kGetTaskCount(void) {
    int iTotalCount;
    BOOL bPreviousCount;

    // After getting the number of tasks in the ready queue, add the number of tasks in the waiting queue and the number of currently running tasks.
    iTotalCount = kGetReadyTaskCount();

    bPreviousCount = kLockForSystemData();  // EntryPoint

    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    kUnlockForSystemData(bPreviousCount);   // EndPoint
    return iTotalCount;
}

// Returns the TCB at that offset from the TCB pool
TCB* kGetTCBInTCBPool(int iOffset) {
    if((iOffset<-1) && (iOffset>TASK_MAXCOUNT)) return NULL;
    return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

// Returns whether the task exists
BOOL kIsTaskExist(QWORD qwID) {
    TCB* pstTCB;

    // Return TCB by ID
    pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

    // If there is no TCB or the ID does not match, it is not present.
    if((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID)) return FALSE;
    return TRUE;
}

// Returns the percentage utilization of the processor
QWORD kGetProcessorLoad(void) {
    return gs_stScheduler.qwProcessorLoad;
}

// idle task
// Clean up tasks waiting to be deleted in the waiting queue
void kIdleTask(void) {
    TCB* pstTask;
    QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
    QWORD qwTaskID;
    BOOL bPreviousFlag;

    // Stores baseline information for calculating processor usage
    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while(1) {
        // save current state
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        // Calculate processor usage
        // 100-(Processor time used by idle tasks)*100/ (Processor time used system-wide)
        if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0) gs_stScheduler.qwProcessorLoad = 0;
        else gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 /(qwCurrentMeasureTickCount - qwLastMeasureTickCount);

        // Keep the current state in the previous state
        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        // Rest according to the load of the processor
        kHaltProcessorByLoad();

        // If there is a task waiting in the wait queue, terminate the task
        if(kGetListCount(&(gs_stScheduler.stWaitList)) >= 0) {
            while(1) {
                bPreviousFlag = kLockForSystemData();   // EntryPoint
                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if(pstTask == NULL) {
                    kUnlockForSystemData(bPreviousFlag);    // EndPoint
                    break;
                }
                qwTaskID = pstTask->stLink.qwID;
                kFreeTCB(qwTaskID);

                kUnlockForSystemData(bPreviousFlag);    // EndPoint
                kPrintf( "IDLE: Task ID is completely ended. \n", pstTask->stLink.qwID);
            }
        }
        kSchedule();
    }
}

// Rest the processor according to the measured processor load
void kHaltProcessorByLoad(void) {
    if(gs_stScheduler.qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    } else if(gs_stScheduler.qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    } else if(gs_stScheduler.qwProcessorLoad < 95) kHlt();
}
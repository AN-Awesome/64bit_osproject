#include "Task.h"
#include "Descriptor.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

//==========================
// Task Pool & Task-related
//==========================
// Init Task Pool
void kInitializeTCBPool(void) {
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
TCB* kAllocateTCB(void) {
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
void kFreeTCB(QWORD qwID) {
    int i;
    // Lower 32 bit of Task ID act as Index
    i = qwID & 0xFFFFFFFF;

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
    pstTask = kAllocateTCB();
    if(pstTask == NULL) return NULL;

    // Calculate Stack Address with Task ID, Lower 32 bit act as offset of Stack Pool
    pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));
    // Set up TCB and insert it into the list so it can be Scheduled
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
    kAddTaskToReadyList(pstTask);
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
    // Task Pool Init
    kInitializeTCBPool();
    // Ready List INit
    kInitializeList( &(gs_stScheduler.stReadyList));
    // Set TCB as a running task with the assignment of 
    // TCB to prepare TCB to store the task that performed the boot
    gs_stScheduler.pstRunningTask = kAllocateTCB();
}
// Set Current Perform Task
void kSetRunningTask(TCB* pstTask) {
    gs_stScheduler.pstRunningTask = pstTask;
}
// Return Current Perform Task
TCB* kGetRunningTask(void) {
    return gs_stScheduler.pstRunningTask;
}
// Get Next Task to Run from Task List
TCB* kGetNextTaskToRun(void) {
    if(kGetListCount( &(gs_stScheduler.stReadyList)) == 0) return NULL;
    return(TCB*)kRemoveListFromHeader( &(gs_stScheduler.stReadyList));
}
// Insert Task to Scheduler Ready List
void kAddTaskToReadyList(TCB* pstTask) {
    kAddListToTail( &(gs_stScheduler.stReadyList), pstTask);
}
// Find another Task and Switch
void kSchedule(void) {
    TCB* pstRunningTask, * pstNextTask;
    BOOL bPreviousFlag;
    // Need Task to Switch to
    if(kGetListCount( &(gs_stScheduler.stReadyList)) == 0) return;
    // Prevent Interrupt during switchover which is troublesome if the task switchover occurs again
    bPreviousFlag = kSetInterruptFlag(FALSE);
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) {
        kSetInterruptFlag(bPreviousFlag);
        return;
    }
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kAddTaskToReadyList(pstRunningTask);
    // Switch context after setting Next Task as currently running task 
    gs_stScheduler.pstRunningTask = pstNextTask;
    kSwitchContext( &(pstRunningTask->stContext), &(pstNextTask->stContext));

    // Update Processor Using time
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    kSetInterruptFlag(bPreviousFlag);
}
// Find another Task and Switch when Interrupt Occured
BOOL kScheduleInInterrupt(void) {
    TCB* pstRunningTask, *pstNextTask;
    char* pcContextAddress;

    // Exit if there is no Task to Switch
    pstNextTask = kGetNextTaskToRun();
    if(pstNextTask == NULL) return FALSE;

    //==============
    // Task Switch
    //==============
    pcContextAddress = (char*) IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
    // Get current Task, Copy Context in IST & Move current task to Ready List
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kMemCpy( &(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
    kAddTaskToReadyList(pstRunningTask);
    gs_stScheduler.pstRunningTask = pstNextTask;
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
void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize) {
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
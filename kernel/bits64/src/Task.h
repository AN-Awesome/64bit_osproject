#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

// Macro
// SS, RSP, RFLAGS, CS, RIP + ISR Save 19 Register
#define TASK_REGISTERCOUNT (5 + 19)
#define TASK_REGISTERSIZE   8

// Context Data Structure Register Offset
#define TASK_GSOFFSET       0
#define TASK_FSOFFSET       1
#define TASK_ESOFFSET       2
#define TASK_DSOFFSET       3
#define TASK_R15OFFSET      4
#define TASK_R14OFFSET      5
#define TASK_R13OFFSET      6
#define TASK_R12OFFSET      7
#define TASK_R11OFFSET      8
#define TASK_R10OFFSET      9
#define TASK_R9OFFSET       10
#define TASK_R8OFFSET       11
#define TASK_RSIOFFSET      12
#define TASK_RDIOFFSET      13
#define TASK_RDXOFFSET      14
#define TASK_RCXOFFSET      15
#define TASK_RBXOFFSET      16
#define TASK_RAXOFFSET      17
#define TASK_RBPOFFSET      18
#define TASK_RIPOFFSET      19
#define TASK_CSOFFSET       20
#define TASK_RFLAGSOFFSET   21
#define TASK_RSPOFFSET      22
#define TASK_SSOFFSET       23

// Task Pool Address
#define TASK_TCBPOOLADDRESS 0x800000
#define TASK_MAXCOUNT       1024
// Stack Pool Stack Size
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE      8192
// Invalid Task ID
#define TASK_INVALIDID      0xFFFFFFFFFFFFFFFF
// Maximum amount Processor time Task can use(5ms)
#define TASK_PROCESSORTIME  5
// Sturcture
#pragma pack (push, 1)

// Context Data Structure
typedef struct kContextStruct {
    QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;

// TASK Condtion Check Data Structure
typedef struct kTaskControlBlockStruct {
    // Next Data Location & ID
    LISTLINK stLink;
    // Flag
    QWORD qwFlags;
    // Context
    CONTEXT stContext;
    // ID, Flag
    QWORD qwID;
    QWORD qwFlags;

    // Stack Address & Size
    void* pvStackAddress;
    QWORD qwStackSize;
} TCB;
// TCB Pool Status Manage DataStruct
typedef struct kTCBPoolManagerStruct {
    // Task Pool Info.
    TCB* pstStartAddress;
    int iMaxCount;
    int iUseCount;

    // TCB Allocated Numbers
    int iAllocatedCount;
} TCBPOOLMANAGER;
// Scheduler Status Manage DataStructure
typedef struct kSchedulerStruct {
    // Current Perform Task
    TCB* pstRunningTask;
    // Processor time that Current Perform Task can use
    int iProcessorTime;
    //  List of Task being prepared to run
    LIST stReadyList;
} SCHEDULER;

#pragma pack(pop)
// function
//==========================
// Task Pool & Task-related
//==========================
void kInitializeTCBPool(void);
TCB* kAllocateTCB(void);
void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress);
void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD, qwStackSize);

//===================
// Scheduler-related
//===================
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun(void);
void kAddTaskToReadyList(TCB* pstTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
#endif
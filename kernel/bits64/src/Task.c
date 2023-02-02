#include "Task.h"
#include "Descriptor.h"

// Set TCB using Parameter
void kSetUpTask(TCB* pstTCB, QWORD qwID, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize) {
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

    // Save ID, Stack, Flag
    pstTCB->qwID = qwID;
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}
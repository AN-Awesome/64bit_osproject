#include "InterruptHandler.h"
#include "PIC.h"
#include "Utility.h"
#include "Keyboard.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h" // 22 chapter
#include "TextColor.h"
#include "Console.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = {0, };

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintStringXY(0, 0, "==============================", YELLOW);
    kPrintStringXY(0, 0, "    Exception Occur~!!!!", YELLOW);        // (0, 1, " ")
    kPrintStringXY(0, 0, "          Vector:", YELLOW);               // (0, 2, " ")
    kPrintStringXY(27, 2, vcBuffer, YELLOW);
    kPrintStringXY(0, 0, "==============================", YELLOW);

    while(1);
}

void kCommonInterruptHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer, YELLOW);                                 // (0, 0, , , )

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintStringXY(70, 1, vcBuffer, SKY_BR);

    if(kIsOutputBufferFull() == TRUE) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }
    
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

// Handler of Timer Interrupt
void kTimerHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iTimerInterruptCount = 0;

    // Output Text
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    // Count
    vcBuffer[8] = '0' + g_iTimerInterruptCount;
    g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
    kPrintStringXY(70, 0, vcBuffer, GREEN);

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

    // Increment
    g_qwTickCount++;

    // Reduce use time
    kDecreaseProcessorTime();

    // Switch Task(Next Period Time__TASK)
    if(kIsProcessorTimeExpired() == TRUE) kScheduleInInterrupt();
}

// Device Not Available Handler (22 chapter)
void kDeviceNotAvailableHandler(int iVectorNumber) {
    TCB* pstFPUTask, *pstCurrentTask;
    QWORD qwLastFPUTaskID;

    //================================================================================
    // Print message : FPU exception occurred
    char vcBuffer[] = "[EXC:  , ]";
    static int g_iFPUInterruptCount = 0;

    // Print message : exception vector(top right of screen as a two-digit integer)
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    // Print : number of occurrences
    vcBuffer[8] = '0' + g_iFPUInterruptCount;
    g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;
    kPrintStringXY(70, 2, vcBuffer, BLUE_BR);
    //================================================================================

    // SET : TS bit = 0 (CR0 control register)
    kClearTS();

    // SAVE : State of FPU to task
    qwLastFPUTaskID = kGetLastFPUUsedTaskID();
    pstCurrentTask = kGetRunningTask();

    // Do NotThing : If used the FPU before
    if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID) return;

    // SAVE : FPU status (if task using FPU)
    else if(qwLastFPUTaskID != TASK_INVALIDID) {
        pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
        if((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)) kSaveFPUContext(pstFPUTask->vqwFPUContext);
    }

    // INIT FPU : if task not used FPU
    // Restore FPU : if task used FPU
    if(pstCurrentTask->bFPUUsed == FALSE) {
        kInitializeFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    } else kLoadFPUContext(pstCurrentTask->vqwFPUContext);

    // Change : FPU ID  to current task
    kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}
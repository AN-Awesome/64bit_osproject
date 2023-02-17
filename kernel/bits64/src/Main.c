#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "TextColor.h"
#include "FileSystem.h"
#include "PIC.h"
#include "PIT.h"
#include "ConsoleShell.h"
#include "Console.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"

void kPrintString(int iX, int iY, const char* pcString, int color);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0;
    int iCursorX, iCursorY;

    kInitializeConsole(0, 2);
    kGetCursor(&iCursorX, &iCursorY);

    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);

    kLoadTR(GDT_TSSSEGMENT);

    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);

    kCheckTotalRAMSize();
    kSetCursor(1, 3);
    kPrintf("RAM Size = %d MB\n", kGetTotalRAMSize());

    kInitializeScheduler();

    kInitializeDynamicMemory();

    kInitializePIT(MSTOCOUNT(1), 1);

    if(kInitializeKeyboard() == TRUE) kChangeKeyboardLED(FALSE, FALSE, FALSE);
    else while(1);

    kInitializePIC();
    kMaskPICInterrupt(0);

    kEnableInterrupt();


    // Init File System
    if(kInitializeFileSystem() == TRUE) ;
    else kPrintStringXY(0, 1, "FILESYSTEM ERROR", RED);

    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
    kStartConsoleShell();
}

void kPrintString(int iX, int iY, const char* pcString, int color) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
        pstScreen[i].bAttribute = color;
    }
}
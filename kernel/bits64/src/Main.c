#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "TextColor.h"
#include "PIC.h"
#include "PIT.h"
#include "ConsoleShell.h"
#include "Console.h"
#include "Task.h"

void kPrintString(int iX, int iY, const char* pcString, int color);

void Main(void) {
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
    kInitializePIT(MSTOCOUNT(1), 1);

    if(kInitializeKeyboard() == TRUE) kChangeKeyboardLED(FALSE, FALSE, FALSE);
    else while(1);

    kInitializePIC();
    kMaskPICInterrupt(0);

    kEnableInterrupt();
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
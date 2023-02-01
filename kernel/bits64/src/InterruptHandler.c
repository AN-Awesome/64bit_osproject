#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "TextColor.h"


void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = {0, };

    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintString(0, 0, "==============================", YELLOW);
    kPrintString(0, 0, "    Exception Occur~!!!!", YELLOW);        // (0, 1, " ")
    kPrintString(0, 0, "          Vector:", YELLOW);               // (0, 2, " ")
    kPrintString(27, 2, vcBuffer, YELLOW);
    kPrintString(0, 0, "==============================", YELLOW);

    while(1);
}

void kCommonInterruptHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintString(70, 0, vcBuffer, YELLOW);                                 // (0, 0, , , )

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
    kPrintString(70, 1, vcBuffer, SKY_BR);

    if(kIsOutputBufferFull() == TRUE) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }
    
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
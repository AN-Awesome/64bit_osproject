#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "TextColor.h"
#include "PIC.h"

void kPrintString(int iX, int iY, const char* pcString, int color);

void Main(void) {
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    KEYDATA stData;

    /*
    kPrintString(35, 17, ">> COMPLETE <<", GREEN);
    kPrintString(1, 19, ">> Now In IA-32e Mode", PINK_BR);

    kPrintString(1, 20, "INITIALIZE - ", PINK);
    kPrintString(14, 20, "GDT", BLUE_BR);
    kPrintString(17, 20, ", IDT", BLUE_BR);

    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);

    kPrintString(35, 20, ">> COMPLETE <<", GREEN);
    //-----------

    kPrintString(1, 21, "SEGMENT TSS LOAD", PINK);
    kLoadTR(GDT_TSSSEGMENT);
    kPrintString(35, 21, ">> COMPLETE <<", GREEN);

    kPrintString(1, 22, "Keyboard Activate", PINK);


    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);

    */
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);

    kLoadTR(GDT_TSSSEGMENT);

    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);


   kPrintString(1, 4, "Keyboard Activate... ", PINK_BR);
   /*
   if(kActivateKeyboard() == TRUE) {
        kPrintString(30, 4, ">> COMPLETE <<", GREEN);
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    } else {
        kPrintString(30, 4, ">> ERROR <<", RED_BR);
        while(1);
    }
    */
    if(kInitializeKeyboard() == TRUE) {
        kPrintString(30, 4, ">> COMPLETE <<", GREEN);
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    } else {
        kPrintString(30, 4, ">> ERROR <<", RED_BR);
        while(1);
    }

    kInitializePIC();
    kMaskPICInterrupt(0);
    kEnableInterrupt();

    int i = 1;
    while(1) {
        if(kGetKeyFromKeyQueue(&stData) == TRUE) {
            if(stData.bFlags & KEY_FLAGS_DOWN) {
                vcTemp[0] = stData.bASCIICode;
                kPrintString(i++, 10, vcTemp);

                if(vcTemp[0] == '0') bTemp /= 0;
            }
        }
    }
}

void kPrintString(int iX, int iY, const char* pcString, int color) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
        pstScreen[i].bAttribute = color;
    }
}
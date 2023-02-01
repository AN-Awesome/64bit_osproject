#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "TextColor.h"
#include "PIC.h"
#include "ConsoleShell.h"
#include "Console.h"

void kPrintString(int iX, int iY, const char* pcString, int color);

void Main(void) {
    int iCursorX, iCursorY;

    kInitializeConsole(0, 10);
}

void kPrintString(int iX, int iY, const char* pcString, int color) {
    CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
    pstScreen += (iY * 80) + iX;
    for (int i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharactor = pcString[i];
        pstScreen[i].bAttribute = color;
    }
}
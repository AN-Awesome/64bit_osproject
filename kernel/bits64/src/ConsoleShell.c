#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "TextColor.h"

// Command Table def.
SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
};

//==============
// Compose Shell
//==============

// Main loop
void kStartConsoleShell(void) {
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    // Prompt Output
    kSetColor(PINK_BR);
    kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
    
    kSetColor(WHITE);
    kPrintf("%s", CONSOLESHELL_SPLIT);

    while(1) {
        // Wait Until Key is recieved
        bKey = kGetCh();
        // Backspace key
        if(bKey == KEY_BACKSPACE) {
            if(iCommandBufferIndex > 0) {       //  Remove Last letter from Command Buffer
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX - 1, iCursorY, " ", WHITE);
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferIndex--;
            }
        } else if (bKey == KEY_ENTER) { // Enter key
            kPrintf("\n");

            if(iCommandBufferIndex > 0) {       //  Implement Command from Command Buffer
                vcCommandBuffer [iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            // Prompt Output & Init Command Buffer
            kSetColor(PINK_BR);
            kPrintf("\n%s", CONSOLESHELL_PROMPTMESSAGE);
            
            kSetColor(WHITE);
            kPrintf("%s", CONSOLESHELL_SPLIT);


            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        } else if ((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) ||(bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLLOCK)) ; // Ignores Shift key, Caps Lock, Num Lock, Scroll Lock
        else {
            // Tab to Blank
            if (bKey == KEY_TAB) bKey = ' ';
            // Only if there is space left in Buffer
            if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT) {
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            } 
        }
    }
}

void kExecuteCommand(const char* pcCommandBuffer) {
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    iCommandBufferLength = kStrLen(pcCommandBuffer);
    for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++) {
        if(pcCommandBuffer[iSpaceIndex] == ' ') break;
    }

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for(i = 0; i < iCount; i++) {
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        // Check that the length and content of the command match completely
        if((iCommandLength == iSpaceIndex) && (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)) {
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }
    kSetColor(RED_BR);
    if (i >= iCount) kPrintf(" '%s' is not found.\n", pcCommandBuffer);
}

// Parameter Data Structure Init.
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter) {
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

// Returns the content and length of a space-separated parameter 
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter) {
    int i;
    int iLength;

    if(pstList->iLength <= pstList->iCurrentPosition) return 0;

    for(i = pstList->iCurrentPosition; i < pstList->iLength; i++) if(pstList->pcBuffer[i] == ' ') break;

    // Copy Parameter and return length
    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    // Update Parameter location
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}

//======================
// Process Command Code
//======================
void kHelp(const char* pcCommandBuffer) {
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kSetColor(BROWN);
    kPrintf("\n");

    kPrintf(" =====================================\n");
    kPrintf("         Awesome OS SHELL Help        \n");
    kPrintf(" =====================================\n");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    
    for(i = 0; i < iCount; i++) {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength) iMaxCommandLength = iLength; // Check Code Style plz 
    }

    // Help Output
    for(i = 0; i < iCount; i++) {
        kPrintf(" %s", gs_vstCommandTable[i].pcCommand);
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);
    }
}

// Clear the Screen
void kCls(const char* pcParameterBuffer) {
    kClearScreen();
    kSetCursor(0, 1);
} 

// Total Memory Size Output
void kShowTotalRAMSize(const char* pcParameterBuffer) {
    kPrintf( "Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

// Convert String into Number & Output
void kStringToDecimalHexTest(const char* pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;
    

    // Init Parameter
    kInitializeParameter(&stList, pcParameterBuffer);

    while(1) {
        iLength = kGetNextParameter(&stList, vcParameter);
        if(iLength == 0) break;
        // Parameter Info Output / Check Hex or Decimal
        kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);
        // 0x = Hex, Others = Decimal
        if(kMemCmp(vcParameter, "0x", 2) == 0) {
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("Hex Value = %q\n", lValue);
        } else {
            lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }
        iCount++;
    }
}

// Restart PC
void kShutdown(const char* pcParameterBuffer) {
    kSetColor(SKY_BR);
    kPrintf("\n System Shutdown Start...\n");

    // Restart PC via Keyboard Controller
    kPrintf(" Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}
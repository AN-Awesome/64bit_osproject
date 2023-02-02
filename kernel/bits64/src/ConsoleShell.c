#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "TextColor.h"
#include "ConsoleEster.h"

// Command Table def.
SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"ram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decial/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},

    {"settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT, ex)wait 100(ms)",  kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"now", "Show Date And Time", kShowDateAndTime},
    {"dev", "Show Project Members", KShowTeamMember},
    {"createtask", "Create Task, ex)createtask 1(type 10(count)", kCreateTestTask}
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

    kPrintf("\n =====================================\n");
    kPrintf("         Awesome OS SHELL Help        \n");
    kPrintf(" =====================================\n");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    
    for(i = 0; i < iCount; i++) {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength) iMaxCommandLength = iLength; // Check Code Style plz 
    }
    iMaxCommandLength += 2;

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
    kSetColor(SKY);
    kPrintf("\n Total RAM Size = %d MB\n", kGetTotalRAMSize());
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
        kPrintf("\n Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);
        // 0x = Hex, Others = Decimal
        if(kMemCmp(vcParameter, "0x", 2) == 0) {
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("Hex Value = %q\n", lValue);
        } else {
            lValue = kAToI(vcParameter, 10);
            kPrintf("\n Decimal Value = %d\n", lValue);
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

void kSetTimer(const char* pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    //Init
    kInitializeParameter(&stList, pcParameterBuffer);

    // Extract 'ms'
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    lValue = kAToI(vcParameter, 10);

    // Extract Periodic
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    bPeriodic = kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("\n Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

void kWaitUsingPIT(const char* pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    
    // Init
    kInitializeParameter(&stList, pcParameterBuffer);
    if(kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf(" ex)wait 100(ms)\n");
        return;
    }

    lMillisecond = kAToI(pcParameterBuffer, 10);
    kSetColor(GRAY);
    kPrintf("\n %d ms Sleep Start...\n", lMillisecond);

    // Disable Interrupt - Direct time measurement via PIT controller
    kDisableInterrupt();
    for(int i = 0; i < lMillisecond / 30; i++) kWaitUsingDirectPIT(MSTOCOUNT(30));
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();
    kPrintf(" %d ms Sleep Complete\n", lMillisecond);

    kInitializePIT(MSTOCOUNT(1), TRUE);
}

void kReadTimeStampCounter(const char* pcParameterBuffer) {
    QWORD qwTSC;

    qwTSC = kReadTSC();
    kSetColor(GREEN);
    kPrintf("\n Time Stamp Counter = %q\n", qwTSC);
}

void kMeasureProcessorSpeed(const char* pcParameterBuffer) {
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;
    kSetColor(BLUE_BR);
    kPrintf("\n Now Measuring.");

    // Indirect measurement of processor speed through TSC
    kDisableInterrupt();
    for(i = 0; i < 200; i++) {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        // kPrintf(".");
    }
    // Restore Timer
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    
    kPrintf(" CPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
    
}

void kShowDateAndTime(const char* pcParameterBuffer) {
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // Read Date and Time From RTC
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kSetColor(GREEN);

    kPrintf("\n Date: %d/%d/%d %s\n", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf(" Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

static TCB gs_vstTask[2] = {0, };
static QWORD gs_vstStack[1024] = {0, };

void kTestTask1(void) {
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    CHARACTER *pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;

    // ID -> Screen Offset
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    while(1) {
        switch(i) {
            case 0:
                iX++;
                if(iX >= (CONSOLE_WIDTH - iMargin)) i = 1;
                break;
            
            case 1:
                iY++;
                if(iY >= (CONSOLE_HEIGHT - iMargin)) i = 2;
                break;

            case 2:
                iX--;
                if(iX < iMargin) i = 3;
                break;

            case 3:
                iY--;
                if(iY < iMargin) i = 0;
                break;

            pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
            pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
            bData++;

            // Switch Task
            kSchedule();
        }
    }
}

void kTestTask2(void) {
    int i = 0, iOffset;
    CHARACTER *pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    // ID -> Screen Offset
    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(1) {
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        kSchedule();
    }
}

void kCreateTestTask(const char* pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;

    // Extract Params
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    switch(kAToI(vcType, 10)) {
        case 1:
            for(i = 0; i < kAToI(vcCount, 10); i++) if(kCreateTask(0, (QWORD)kTestTask1) == NULL) break;
            kPrintf("Task1 %d Created\n", i);
            break;

        case 2:
            for(i = 0; i < kAToI(vcCount, 10); i++) if(kCreateTask(0, (QWORD)kTestTask2) == NULL) break;
            kPrintf("Task2 %d Created\n", i);
            break;
    }
}


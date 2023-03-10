#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

// Macro
#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 300
#define CONSOLESHELL_PROMPTMESSAGE         " AWESOME OS"
#define CONSOLESHELL_SPLIT                 ": "

// Define Type of Function Pointer that accpets String Pointer as Parameter
typedef void(*CommandFunction) (const char* pcParameter);

// Structure
// Align 1 byte
#pragma pack(push, 1)

// Structure that stores SHELL COMMAND
typedef struct kShellCommandEntryStruct {
    char* pcCommand;                        // Command String
    char* pcHelp;                           // Command Help
    CommandFunction pfFunction;             // Command Function Pointer
} SHELLCOMMANDENTRY;

// Structure that stores Info. to process Parameter
typedef struct kParameterListStruct {
    const char* pcBuffer;                   // Parameter Buffer Address
    int iLength;                            // Parameter Length
    int iCurrentPosition;                   // where the Parameter to be processed starts
} PARAMETERLIST;

#pragma pack(pop)

// Function
// ACTUAL SHELL CODE
void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);

// Function that process Command
static void kHelp(const char* pcParameterBuffer);
static void kCls(const char* pcParameterBuffer);
static void kShowTotalRAMSize(const char* pcParameterBuffer);
static void kStringToDecimalHexTest(const char* pcParameterBuffer);
static void kShutdown(const char* pcParameterBuffer);

static void kSetTimer(const char* pcParameterBuffer);
static void kWaitUsingPIT(const char* pcParameterBuffer);
static void kReadTimeStampCounter(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);

static void kCreateTestTask(const char* pcParameterBuffer);
static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);
static void kTestMutex(const char* pcParameterBuffer);

static void kCreateThreadTask(void);
static void kTestThread(const char* pcParameterBuffer);

static void kShowMatrix(const char* pcParameterBuffer);
static void kTestPIE(const char* pcParameterBuffer);
static void kShowDynamicMemoryInformation(const char* pcParameterBuffer);
static void kTestSequentialAllocation(const char* pcParameterBuffer);
static void kTestRandomAllocation(const char* pcParameterBuffer);
static void kRandomAllocationTask(void);

static void kShowHDDInformation(const char* pcParameterBuffer);
static void kReadSector(const char* pcParameterBuffer);
static void kWriteSector(const char* pcParameterBuffer);

static void kMountHDD(const char* pcParameterBuffer);
static void kFormatHDD(const char* pcParameterBuffer);
static void kShowFileSystemInformation(const char* pcParameterBuffer);
static void kCreateFileInRootDirectory(const char* pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer);
static void kShowRootDirectory(const char* pcParameterBuffer);

static void kWriteDataToFile(const char* pcParameterBuffer);
static void kReadDataFromFile(const char* pcParameterBuffer);
static void kTestFileIO(const char* pcParameterBuffer);

#endif

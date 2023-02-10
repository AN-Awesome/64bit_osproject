#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"

// Macro
// Dynamic memory area Start address 
// Sort by 1 MB
#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)

// Buddy block Min Size (1kb)
#define DYNAMICMEMORY_MIN_SIZE      (1 * 1024)

// Bitmap Flag
#define DYNAMICMEMORY_EXIST         0x01
#define DYNAMICMEMORY_EMPTY         0x00

// Structure
// Data Structure : Manage bitmaps
typedef struct kBitmapStruct {
    BYTE* pbBitmap;
    QWORD qwExistBitCount;
} BITMAP;

// Data Structure : Manage Buddy block
typedef struct kDynamicMemoryManagerStruct {
    int iMaxLevelCount;             // Total number of block lists
    int iBlockCountOfSmallestBlock; // Number of blocks with the smallest size
    QWORD qwUsedSize;               // Allocated memory size

    QWORD qwStartAddress;           // Start address of Block Pool
    QWORD qwEndAddress;             // End address of Block Pool

    BYTE* pbAllocateBlockListIndex; // Area to store the index
    BITMAP* pstBitmapOfLevel;       // Address of bitmap data structure
} DYNAMICMEMORY;

// Function
void kInitializeDynamicMemory(void);
void* kAllocateMemory(QWORD qwSize);
BOOL kFreeMemory(void* pvAddress);
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);

static QWORD kCalculateDynamicMemorySize(void);
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
static int kAllocationBuddyBlock(QWORD qwAlignedSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);

#endif
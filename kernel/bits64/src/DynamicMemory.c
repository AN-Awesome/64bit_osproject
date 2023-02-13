#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"

static DYNAMICMEMORY gs_stDynamicMemory;

// Init Dynamic Memory Area
void kInitializeDynamicMemory(void) {
    QWORD qwDynamicMemorySize;
    int i, j;
    BYTE* pbCurrentBitmapPosition;
    int iBlockCountOfLevel, iMetaBlockCount;

    // Calculate memory size in smallest blocks
    qwDynamicMemorySize = kCalculateDynamicMemorySize();
    iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

    // Organize meta information
    gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

    // Calculate : Composed of up to several block lists
    for(i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++); // DO NOTHING
    gs_stDynamicMemory.iMaxLevelCount = i;

    // Init : Area to store the index of the block list
    gs_stDynamicMemory.pbAllocateBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
    for(i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++) gs_stDynamicMemory.pbAllocateBlockListIndex[i] = 0xFF;

    // Specify start address : Bitmap Data Structure
    gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS + (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));

    // Specifies the address : actual bitmap
    pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) + (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);

    // Creating a bitmap by looping through each block list
    // set to empty
    for(j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++) {
        gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
        gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;
        iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

        // set all to empty
        for(i = 0; i < iBlockCountOfLevel / 8; i++) {
            *pbCurrentBitmapPosition = 0x00;
            pbCurrentBitmapPosition++;
        }

        // Processing the rest of the block : Blocks not divisible by 8
        if((iBlockCountOfLevel % 8) != 0) {
            *pbCurrentBitmapPosition = 0x00;

            // Last block is set as a scrap block
            i = iBlockCountOfLevel % 8;
            if((i % 2) == 1) {
                *pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
                gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
            }
            pbCurrentBitmapPosition++;
        }
    }

    // Address Setting : Block Pool 
    // Set : Amount of memory used
    gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
    gs_stDynamicMemory.qwEndAddress = kCalculateDynamicMemorySize() + DYNAMICMEMORY_START_ADDRESS;
    gs_stDynamicMemory.qwUsedSize = 0;
}

// Calculate : Dynamic Memory Area Size
static QWORD kCalculateDynamicMemorySize(void) {
    QWORD qwRAMSize;

    // Use up to 3GB
    qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);
    if(qwRAMSize > (QWORD) 3 * 1024 * 1024 * 1024) qwRAMSize = (QWORD) 3 * 1024 * 1024 * 1024;
    return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

// Calculate Area Size : Information needed to manage dynamic memory areas
// Return : Sort by smallest block
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize) {
    long iBlockCountOfSmallestBlock; // ================= 책에는 l로 헤더에는 i로 나와있음
    DWORD dwSizeOfAllocatedBlockListIndex;
    DWORD dwSizeOfBitmap;
    long i;

    // Calculate : Area to store allocated size
    // Calculate : Bitmap Area
    iBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
    // Calculate : Area required to store the index
    dwSizeOfAllocatedBlockListIndex = iBlockCountOfSmallestBlock * sizeof(BYTE);

    // Calculate : Space required to store bitmap
    dwSizeOfBitmap = 0;
    for(i=0; (iBlockCountOfSmallestBlock >> i)>0; i++) {
        dwSizeOfBitmap += sizeof(BITMAP);                               // Space for a bitmap pointer in the block list
        dwSizeOfBitmap += ((iBlockCountOfSmallestBlock >> i) + 7) / 8;  // Bitmap size of block list, rounded up in bytes
    }
    return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}

// Allocate Memory
void* kAllocateMemory(QWORD qwSize) {
    QWORD qwAlignedSize;
    QWORD qwRelativeAddress;
    long iOffset;
    int iSizeArrayOffset;
    int iIndexOfBlockList;

    // SET : Memory Size = Buddy Block Size
    qwAlignedSize = kGetBuddyBlockSize(qwSize);
    if(qwAlignedSize == 0) return NULL;

    // If not enough free space -> Fail
    if(gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + qwAlignedSize > gs_stDynamicMemory.qwEndAddress) return NULL;

    // RETURN : Block list index (Allocate a buddy block and to which the allocated block belongs)
    iOffset = kAllocationBuddyBlock(qwAlignedSize);
    if(iOffset == -1) return NULL;

    iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

    // SAVE : Block List Index (The area where the block size is stored belongs to the actually allocated buddy block)
    // When freeing memory, use the index of the block list
    qwRelativeAddress = qwAlignedSize = iOffset;
    iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

    gs_stDynamicMemory.pbAllocateBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;
    gs_stDynamicMemory.qwUsedSize += qwAlignedSize;
    return (void*)(qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}

// RETURN : sorted size (sorted by the size of the nearest buddy block)
static QWORD kGetBuddyBlockSize(QWORD qwSize) {
    long i;
    for(i=0; i<gs_stDynamicMemory.iMaxLevelCount; i++) {
        if(qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)) return (DYNAMICMEMORY_MIN_SIZE << i);
    }
    return 0;
}

// Allocate : Memory block with buddy block algorithm
// The memory size must be requested as the size of the buddy block
static int kAllocationBuddyBlock(QWORD qwAlignedSize) {
    int iBlockListIndex, iFreeOffset;
    int i;
    BOOL bPreviousInterruptFlag;

    // Retrieve the index of the block list that satisfies the block size
    iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
    if(iBlockListIndex == -1) return -1;

    // sync processing
    bPreviousInterruptFlag = kLockForSystemData();

    // Select Block : Search from the satisfied block list to the top block list
    for(i=iBlockListIndex; i<gs_stDynamicMemory.iMaxLevelCount; i++) {
        iFreeOffset = kFindFreeBlockInBitmap(i); // Check the bitmap in the block list to see if a block is present
        if(iFreeOffset != -1) break;
    }

    // Even after searching up to the last block list, if it is not found -> Fail
    if(iFreeOffset == -1) {
        kUnlockForSystemData(bPreviousInterruptFlag);
        return -1;
    }

    // Mark as an empty block
    kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

    // Split the block : If you found a block in the parent block
    if(i > iBlockListIndex) {
        for(i=i-1; i>=iBlockListIndex; i--) {
            kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
            kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);
            iFreeOffset = iFreeOffset * 2;
        }
    }
    kUnlockForSystemData(bPreviousInterruptFlag);
    return iFreeOffset;
}

// RETURN : Block List Index (Size passed and list of closest blocks)
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize) {
    int i;
    for(i=0; i<gs_stDynamicMemory.iMaxLevelCount; i++) {
        if(qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i)) return i;
    }
    return -1;
}

// RETURN : Block Offset (After searching the bitmap in the block list, if a block exists)
static int kFindFreeBlockInBitmap(int iBlockListIndex) {
    int i, iMaxCount;
    BYTE* pbBitmap;
    QWORD* pqwBitmap;

    // If no data in the bitmap -> Fail
    if(gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0) return -1;

    // Get the total number of blocks in the block list -> Search bitmap by number
    iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
    pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    for(i=0; i<iMaxCount; ) {
        // QWORD = 8 * 8 = 64(bit)
        // Check all at once -> Check if 1bit exists
        if(((iMaxCount - i) / 64) > 0) {
            pqwBitmap = (QWORD*) &(pbBitmap[i / 8]);
            if(*pqwBitmap == 0) { // If all 8 bytes are 0, exclude all 8 bytes
                i += 64;
                continue;
            }
        }

        if((pbBitmap[i / 8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0) return i;
        i++;
    }
    return -1;
}

// Set Bitmap Falg
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag) {
    BYTE* pbBitMap;
    pbBitMap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    if(bFlag == DYNAMICMEMORY_EXIST) {
        // If the data is empty at that location, the count is incremented.
        if((pbBitMap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0) gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
        pbBitMap[iOffset / 8] |= (0x01 << (iOffset % 8));
    } else {
        // Decrease the count if data exists at that location
        if((pbBitMap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0) gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
        pbBitMap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
    }
}

// Free the allocated memory
BOOL kFreeMemory(void* pvAddress) {
    QWORD qwRelativeAddress;
    int iSizeArrayOffset;
    QWORD qwBlockSize;
    int iBlockListIndex;
    int iBitmapOffset;

    if(pvAddress == NULL) return FALSE;

    // Retrieve the size of the allocated block
    qwRelativeAddress = ((QWORD)pvAddress) - gs_stDynamicMemory.qwStartAddress;
    iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

    // Do not return if not assigned
    if(gs_stDynamicMemory.pbAllocateBlockListIndex[iSizeArrayOffset] == 0xFF) return FALSE;

    // Init : Where the index of the block list is stored
    // Retrieves a block list containing allocated blocks
    iBlockListIndex = (int)gs_stDynamicMemory.pbAllocateBlockListIndex[iSizeArrayOffset];
    gs_stDynamicMemory.pbAllocateBlockListIndex[iSizeArrayOffset] = 0xFF;

    // Calculate the size of the allocated block
    qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

    // Unblock by finding the block offset in the block list
    iBitmapOffset = qwRelativeAddress / qwBlockSize;
    if(kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE) {
        gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
        return TRUE;
    }
    return FALSE;
}

// Release the buddy block in the block list
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset) {
    int iBuddyBlockOffset;
    int i;
    BOOL bFlag;
    BOOL bPreviousInterruptFlag;

    // sync processing
    bPreviousInterruptFlag = kLockForSystemData();

    // Repeat : until we can't connect
    for(i=iBlockListIndex; i<gs_stDynamicMemory.iMaxLevelCount; i++) {
        // set the current block to the existing state
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

        // Block Offset = even (left) -> check odd (right)
        // Block Offset = odd (right) -> check even (left)
        // Check the bitmap and combine adjacent blocks if present
        if((iBlockOffset % 2) == 0) iBuddyBlockOffset = iBlockOffset + 1;
        else iBuddyBlockOffset = iBlockOffset - 1;
        bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

        // Exit if the block is empty
        if(bFlag == DYNAMICMEMORY_EMPTY) break;

        // combine blocks
        // make it an empty block and move to parent block
        kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

        // Change to the block offset of the parent block list & repeat again
        iBlockOffset = iBlockOffset / 2;
    }
    kUnlockForSystemData(bPreviousInterruptFlag);
    return TRUE;
}

// Returns a bitmap at that position in the block list
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset) {
    BYTE* pbBitmap;

    pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
    if((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00) return DYNAMICMEMORY_EXIST;
    return DYNAMICMEMORY_EMPTY;
}

// Return information from dynamic memory area
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUseMemorySize) {
    *pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
    *pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
    *pqwMetaDataSize = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize) * DYNAMICMEMORY_MIN_SIZE;
    *pqwUseMemorySize = gs_stDynamicMemory.qwUsedSize;
}

// Returns a data structure that manages a dynamic memory area
DYNAMICMEMORY* kGetDynamicMemoryManager(void) {
    return &gs_stDynamicMemory;
}
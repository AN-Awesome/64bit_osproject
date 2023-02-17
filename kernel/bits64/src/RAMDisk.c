#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"

// datastructure : manage ram disk
static RDDMANAGER gs_stRDDManager;

// function : init ram disk driver
BOOL kInitializeRDD(DWORD dwTotalSectorCount) {
    // init data structure
    kMemSet(&gs_stRDDManager, 0, sizeof(gs_stRDDManager));

    // allocate memory -> use ram disk
    gs_stRDDManager.pbBuffer = (BYTE*) kAllocateMemory(dwTotalSectorCount * 512);
    if(gs_stRDDManager.pbBuffer == NULL) return FALSE;

    // set : total sector count & sync object
    gs_stRDDManager.dwTotalSectorCount = dwTotalSectorCount;
    kInitializeMutex( &gs_stRDDManager.stMutex);
    return TRUE;
}

// return : ram disk info
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation) {
    // init data structure
    kMemSet(pstHDDInformation, 0, sizeof(HDDINFORMATION));

    // set : total sector count & model number
    pstHDDInformation->dwTotalSectors = gs_stRDDManager.dwTotalSectorCount;
    kMemCpy(pstHDDInformation->vwSerialNumber, "0000-0000", 9);
    kMemCpy(pstHDDInformation->vwModelNumber, "AWESOME RAM Disk v1.0", 18);

    return TRUE;
}

// return : read sector in ram disk
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    int iRealReadCount;

    // compare sector count(that can read first to end in lba address) & sector count(that to read)
    // calculate actual number of read
    iRealReadCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);

    kMemCpy(pcBuffer, gs_stRDDManager.pbBuffer + (dwLBA * 512), iRealReadCount * 512);
    return iRealReadCount;
}

// us multiple sectors (by ramdisk)
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    int iRealWriteCount;

    iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);

    kMemCpy(gs_stRDDManager.pbBuffer + (dwLBA * 512), pcBuffer, iRealWriteCount * 512);

    return iRealWriteCount;
}
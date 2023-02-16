#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"

static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL kInitializeFileSystem(void) {
    kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
    kInitializeMutex(&(gs_stFileSystemManager.stMutex));

    if(kInitializeHDD() == TRUE) {
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_pfReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;
    } else return FALSE;

    // Connect File System
    if(kMount() == FALSE) return FALSE;
    // Allocate Space for Handle
    gs_stFileSystemManager.pstHandlePool = (FILE*)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    // Set HardDisk as Unrecognized if Memory Allocation Fail
    if(gs_stFileSystemManager.pstHandPool == NULL) {
        gs_stFileSystemManager.bMounted = FALSE;
        return FALSE;
    }
    // Handle Pool Init to 0
    kMemset(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    return TRUE;
}

// LOW LEVEL FUNCTION
// CHECK MBR(IS AWESOMEOS'S FILESYSTEM)
BOOL kMount(void) {
    MBR* pstMBR;

    kLock(&(gs_stFileSystemManager.stMutex));
    if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR*)gs_vbTempBuffer;
    if(pstMBR->dwSignature != FILESYSTEM_SIGNATURE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    gs_stFileSystemManager.bMounted = TRUE;

    gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress = pstMBR->dwReservedSectorCount + 1;
    gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
    gs_stFileSystemManager.dwDataAreaStartAddress = pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount + 1;
    gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

    // Process synchronization
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return TRUE;
}

// Generate filesystem to HDD
BOOL kFormat(void) {
    HDDINFORMATION* pstHDD;
    MBR* pstMBR;
    DWORD dwTotalSectorCount, dwRemainSectorCount;
    DWORD dwMaxClusterCount, dwClusterCount;
    DWORD dwClusterLinkSectorCount;
    DWORD i;

    kLock(&(gs_stFileSystemManager.stMutex));

    pstHDD = (HDDINFORMATION*)gs_vbTempBuffer;
    if(gs_pfReadHDDInformation(TRUE, TRUE, pstHDD) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    dwTotalSectorCount = pstHDD->dwTotalSectors;

    // Calculate Max Cluster Count(TOTALSECTOR / 4KB(SIZEOF CLUSTER))
    dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

    // Calculate Sector Count of Cluster Link
    dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

    // Calculate Number of Remain Cluster
    dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;
    dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

    dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;

    if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR*)gs_vbTempBuffer;
    kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClusterCount;

    if(gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    kMemSet(gs_vbTempBuffer, 0, 512);
    for(i = 0; i < (dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER); i++) {
        if(i == 0) ((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_LASTCLUSTER;
        else ((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_FREECLUSTER;

        if(gs_pfWriteHDDSector(TRUE, TRUE, i + 1, 1, gs_vbTempBuffer) == FALSE) {
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return FALSE;
        }
    }
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return TRUE;
}

BOOL kGetHDDInformation(HDDINFORMATION* pstInformation) {
    BOOL bResult;

    kLock(&(gs_stFileSystemManager.stMutex));
    bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return bResult;
}

static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

static BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

static DWORD kFindFreeCluster(void) {
    DWORD dwLinkCountInSector;
    DWORD dwLastSectorOffset, dwCurrentSectorOffset;
    DWORD i, j;

    if(gs_stFileSystemManager.bMounted == FALSE) return FILESYSTEM_LASTCLUSTER;

    dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;
    for(i = 0; i < gs_stFileSystemManager.dwClusterLinkAreaSize; i++) {
        if((dwLastSectorOffset + i) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1)) dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
        else dwLinkCountInSector = 128;

        dwCurrentSectorOffset = (dwLastSectorOffset + i) % gs_stFileSystemManager.dwClusterLinkAreaSize;
        if(kReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer) == FALSE) return FILESYSTEM_LASTCLUSTER;

        // search enpty cluster
        for(j = 0; j < dwLinkCountInSector; j++) {
            if(((DWORD*)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER) break;
        }

        // if find than..
        if(j != dwLinkCountInSector) {
            gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;
            return (dwCurrentSectorOffset * 128) + j;
        }
    }
    return FILESYSTEM_LASTCLUSTER;
}

static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData) {
    DWORD dwSectorOffset;

    if(gs_stFileSystemManager.bMounted == FALSE) return FALSE;

    dwSectorOffset = dwClusterIndex / 128;

    // set link information and save
    if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;
    ((DWORD*)gs_vbTempBuffer) [dwClusterIndex % 128] = dwData;

    if(kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;
    return TRUE;
}

static BOOL kGetClulsterLinkData(DWORD dwClusterIndex, DWORD* pdwData) {
    DWORD dwSectorOffset;

    if(gs_stFileSystemManager.bMounted == FALSE) return FALSE;

    dwSectorOffset = dwClusterIndex / 128;
    if(dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize) return FALSE;

    if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;

    *pdwData = ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128];
    return TRUE;
}

static int kFindFreeDirectoryEntry(void) {
    DIRECTORYENTRY* pstEntry;
    int i;

    if(gs_stFileSystemManager.bMounted == FALSE) return -1;

    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return -1;

    pstEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    for(i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if(pstEntry[i].dwStartClusterIndex == 0) return i;
    }
    return -1;
}

static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >=FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;
    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    if(kWriteCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    return TRUE;
}

static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;

    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;

    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}

static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    int i, iLength;

    if(gs_stFileSystemManager.bMounted == FALSE) return -1;
    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return -1;

    iLength = kStrLen(pcFileName);
    pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;

    for(i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if(kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0) {
            kMemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
            return i;
        }
    }
    return -1;
}

void kGetFileSystemInformation(FILESYSTEMMANAGER *pstManager) {
    kMemCpy(pstManager, &gs_stFileSystemManager, sizeof(gs_stFileSystemManager));
}
//======================
// High Level Function
//======================
static void* kAllocateFileDirectoryHandle(void) {
    int i;
    FILE* pstFile;
    // Search All Handle Pool & Return Empty Handle 
    pstFile = gs_stFileSystemManager.pstHandlePool;
    for(i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        // Return if Empty
        if(pstFile->bType == FILESYSTEM_TYPE_FREE) {
            pstFile ->bType == FILESYSTEM_TYPE_FILE;
            return pstFile;
        }
        // Move Next
        pstFile++;
    }
    return NULL;
}
// Return Used Handle
static void kFreeFileDirectoryHandle(FILE* pstFile) {
    // Init All area
    kMemSet(pstFile, 0, sizeof(FILE));
    // Set Empty Type
    pstFile->bType = FILESYSTEM_TYPE_FREE;
}
// Make File
static BOOL kCreateFile(cosnt char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex) {
    DWORD dwCluster;
    // Find Empty Cluster and Set Allocated
    dwCluster = kFindFreeCluster();
    if((dwCluster == FILESYSTEM_LASTCLUSTER) || (kSeetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE)) return FALSE;
    // Search Empty Directory Entry
    *piDirectoryEntryIndex = kFindFreeDirectoryEntry();
    if(*piDirectoryEntryIndex == -1) {
        // Return Allocated Cluster if it Fails
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }
    // Set Directory Entry
    kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName) +1);
    pstEntry->dwStartClusterIndex = dwCluster;
    pstEntry->dwFileSize = 0;
    // Register Directory Entry
    if(kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE) {
        // Return Allocated Cluster if it Fails
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        return FALSE;
    }
    return TRUE;
}
// Return All Connected Cluster from Passed to Parameter to the end of File
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex) {
    DWORD dwCurrentClusterIndex;
    DWORD dwNextClusterIndex;
    // Cluster Index Init
    dwCurrentClusterIndex = dwClusterIndex;
    
    while(dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER) {
        // Get Next Cluster's Index
        if(kGetClulsterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) return FALSE;
        // Disable by Setting Current Cluster Empty
        if(kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE) return FALSE;
        // Current Cluster's Index changes to Next Cluster's Index
        dwCurrentClusterIndex = dwNextClusterIndex;
    }
    return TRUE;
}
// Create or Open File
FILE* kOpenFile(const char* pcFileName, const char* pcMode) {
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;
    DWORD dwSecondCluster;
    FILE* pstFile;

    // Verify File Name
    iFileNameLength = kStrLen(pcFileName);
    if((iFileNameLength >(sizof(stEntry.vcFileName) - 1)) || (iFileNameLength == 0)) return NULL;
    // Sync.
    kLock(&(gs_stFileSystemManager.stMutex));
    //================================================================
    // Verify the File Exists; if not, check Options and Create File
    //================================================================
    iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
    if(iDirectoryEntryOffset == -1) {
        // If not exists, Read Option Failed
        if(pcMode[0] == 'r') {
            // Sync.
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return NULL;
        }
        // Rest Options create File
        if(kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset) == FALSE) {
            // Sync.
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return NULL;
        }
    }
    //=================================================
    // If Option to empty contents of File, remove all clusters attached to the file and set the file size to zero
}

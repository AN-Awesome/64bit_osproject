#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"

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

BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}

BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

DWORD kFindFreeCluster(void) {
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

BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData) {
    DWORD dwSectorOffset;

    if(gs_stFileSystemManager.bMounted == FALSE) return FALSE;

    dwSectorOffset = dwClusterIndex / 128;

    // set link information and save
    if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;
    ((DWORD*)gs_vbTempBuffer) [dwClusterIndex % 128] = dwData;

    if(kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;
    return TRUE;
}

BOOL kGetClulsterLinkData(DWORD dwClusterIndex, DWORD* pdwData) {
    DWORD dwSectorOffset;

    if(gs_stFileSystemManager.bMounted == FALSE) return FALSE;

    dwSectorOffset = dwClusterIndex / 128;
    if(dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize) return FALSE;

    if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) return FALSE;

    *pdwData = ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128];
    return TRUE;
}

int kFindFreeDirectoryEntry(void) {
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

BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >=FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;
    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    if(kWriteCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    return TRUE;
}

BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;

    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;

    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}

int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY *pstEntry) {
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


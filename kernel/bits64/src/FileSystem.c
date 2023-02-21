#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"
#include "CacheManager.h"
#include "RAMDisk.h"

static FILESYSTEMMANAGER gs_stFileSystemManager;
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];

fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

BOOL kInitializeFileSystem(void) {
    BOOL bCacheEnable = FALSE;
    
    kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
    kInitializeMutex(&(gs_stFileSystemManager.stMutex));

    if(kInitializeHDD() == TRUE) {
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_pfReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;

        bCacheEnable = TRUE;
    } else if(kInitializeRDD(RDD_TOTALSECTORCOUNT) == TRUE) {       // 8MB Size RAMDisk Create when HardDisk Init Failed
        gs_pfReadHDDInformation = kReadRDDInformation;
        gs_pfReadHDDSector = kReadRDDSector;
        gs_pfWriteHDDSector = kWriteRDDSector;
        
        if(kFormat() == FALSE) return FALSE;
    } else return FALSE;

    // Connect File System
    if(kMount() == FALSE) return FALSE;
    // Allocate Space for Handle
    gs_stFileSystemManager.pstHandlePool = (FILE*)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    // Set HardDisk as Unrecognized if Memory Allocation Fail
    if(gs_stFileSystemManager.pstHandlePool == NULL) {
        gs_stFileSystemManager.bMounted = FALSE;
        return FALSE;
    }
    // Handle Pool Init to 0
    kMemSet(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
    if(bCacheEnable == TRUE) gs_stFileSystemManager.bCacheEnable = kInitializeCacheManager();
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
    // Discard All Cache Buffer
    if(gs_stFileSystemManager.bCacheEnable == TRUE) {
        kDiscardAllCacheBuffer(CACHE_CLUSTERLINKTABLEAREA);
        kDiscardAllCacheBuffer(CACHE_DATAAREA);
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
    if(gs_stFileSystemManager.bCacheEnable == FALSE) kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer);
    else kInternalReadClusterLinkTableWithCache(dwOffset, pbBuffer);
}
// Read 1 Sector from Cluster Link Table / No use Cache 
static BOOL kInternalReadClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}
// Read 1 Sector from Cluster Link Table / Use Cache
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer) {
    CACHEBUFFER* pstCacheBuffer;
    pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);
    if(pstCacheBuffer != NULL) {
        kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, 512);
        return TRUE;
    }
    if(kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer) == FALSE) return FALSE;
    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
    if(pstCacheBuffer == NULL) return FALSE;
    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
    pstCacheBuffer->dwTag = dwOffset;
    pstCacheBuffer->bChanged = FALSE;
    return TRUE;
}

// Allocate from Cluster Link Table Area Cache Buffer or Data Area Cache Buffer
// If there is no empty cache buffer, choose one of old ones and empty it before use.
static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex) {
    CACHEBUFFER* pstCacheBuffer;
    pstCacheBuffer = kAllocateCacheBuffer(iCacheTableIndex);
    if(pstCacheBuffer == NULL) {
        pstCacheBuffer = kGetVictimInCacheBuffer(iCacheTableIndex);
        if(pstCacheBuffer == NULL) {
            kPrintf("Cache Allocate Fail.\n");
            return NULL;
        }
        if(pstCacheBuffer->bChanged == TRUE) switch(iCacheTableIndex) {
            case CACHE_CLUSTERLINKTABLEAREA:
                if(kInternalWriteClusterLinkTableWithoutCache(pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer) == FALSE) {
                    kPrintf("Cache Buffer Write Fail.\n");
                    return NULL;
                } break;
            case CACHE_DATAAREA:
                if(kInternalWriteClusterWithoutCache(pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer) == FALSE) {
                    kPrintf("Cache Buffer Write Fail.\n");
                    return NULL;
                } break;
            default:
                kPrintf("kAllocateCacheBufferWithFlush Fail\n");
                return NULL;
                break;
        }
    }
    return pstCacheBuffer;
}
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    if(gs_stFileSystemManager.bCacheEnable == FALSE) return kInternalWriteClusterLinkTableWithoutCache(dwOffset, pbBuffer);
    else return kInternalWriteClusterLinkTableWithCache(dwOffset, pbBuffer);
}
// Write 1 Sector to Cluster Link Table 
// Internal Func. / No use Cache
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress, 1, pbBuffer);
}
// Write 1 Sector to Cluster Link Table 
// Internal Func. / Use Cache
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer) {
    CACHEBUFFER* pstCacheBuffer;
    pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);
    if(pstCacheBuffer != NULL) {
        kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
        pstCacheBuffer->bChanged = TRUE;
        return TRUE;
    }
    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
    if(pstCacheBuffer == NULL) return FALSE;
    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
    pstCacheBuffer->dwTag = dwOffset;
    pstCacheBuffer->bChanged = TRUE;

    return TRUE;
}
static BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer) {
    if(gs_stFileSystemManager.bCacheEnable == FALSE) kInternalReadClusterWithOutCache(dwOffset, pbBuffer);
    else kInternalReadClusterWithCache(dwOffset, pbBuffer);
}
// Read 1 Cluster from Data Area Offset
// Inernal Func. / No use Cache
static BOOL kInternalReadClusterWithOutCache(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfReadHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}
// Read 1 Cluster from Data Area Offset
// Inernal Func. / Use Cache 
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer) {
    CACHEBUFFER* pstCacheBuffer;
    pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);
    if(pstCacheBuffer != NULL) {
        kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, FILESYSTEM_CLUSTERSIZE);
        return TRUE;
    }
    if(kInternalReadClusterWithOutCache(dwOffset, pbBuffer) == FALSE) return FALSE;
    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);
    if(pstCacheBuffer == NULL) return FALSE;
    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
    pstCacheBuffer->dwTag = dwOffset;
    pstCacheBuffer->bChanged = FALSE;
    return TRUE;
}
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer) {
    if(gs_stFileSystemManager.bCacheEnable == FALSE) kInternalWriteClusterWithoutCache(dwOffset, pbBuffer);
    else kInternalWriteClusterWithCache(dwOffset, pbBuffer);
}
// Write 1 Sector to Data Area Offset
// Internal Func. / No use Cache
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer) {
    return gs_pfWriteHDDSector(TRUE, TRUE, (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress, FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}
// Write 1 Sector to Data Area Offset
// Internal Func. / No use Cache
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer) {
    CACHEBUFFER* pstCacheBuffer;
    pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);
    if(pstCacheBuffer != NULL) {
        kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
        pstCacheBuffer->bChanged = TRUE;
        return TRUE;
    }
    pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);
    if(pstCacheBuffer == NULL) return FALSE;
    kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
    pstCacheBuffer->dwTag = dwOffset;
    pstCacheBuffer->bChanged = TRUE;
    return TRUE;
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

static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData) {
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

static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry) {
    DIRECTORYENTRY* pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;
    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    if(kWriteCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;
    return TRUE;
}

static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry) {
    DIRECTORYENTRY* pstRootEntry;
    if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return FALSE;

    if(kReadCluster(0, gs_vbTempBuffer) == FALSE) return FALSE;

    pstRootEntry = (DIRECTORYENTRY*) gs_vbTempBuffer;
    kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}

static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry) {
    DIRECTORYENTRY* pstRootEntry;
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

void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager) {
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
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex) {
    DWORD dwCluster;
    // Find Empty Cluster and Set Allocated
    dwCluster = kFindFreeCluster();
    if((dwCluster == FILESYSTEM_LASTCLUSTER) || (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE)) return FALSE;
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
        if(kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) return FALSE;
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
    if((iFileNameLength >(sizeof(stEntry.vcFileName) - 1)) || (iFileNameLength == 0)) return NULL;
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
    //====================================================================================================
    // If Option to empty contents of File, Remove all clusters attached to file and set file size to Zero
    //====================================================================================================
    else if(pcMode[0] == 'w') {
        // Find Start Cluster's Next Cluster
        if(kGetClusterLinkData(stEntry.dwStartClusterIndex, &dwSecondCluster) == FALSE) {
            kUnlock(&(gs_stFileSystemManager.stMutex)); // Sync.
            return NULL;
        } 
        // Make Start Cluster to Last Cluster
        if(kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE) {
        // Sync.
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return NULL;
        }
        // Disable Next Cluster to Last Cluster
        if(kFreeClusterUntilEnd(dwSecondCluster) == FALSE) {
        // Sync.
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return NULL;
        }
        // Set Size to 0 beacause All Contents of File are Empty
        stEntry.dwFileSize = 0;
        if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE) {
        // Sync.
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return NULL;
        }
    }
    //=============================================
    // File Handle Assigned, Set Data, and Return
    //=============================================
    pstFile = kAllocateFileDirectoryHandle();
    if(pstFile == NULL) {
        // Sync.
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return NULL;
    }
    // Set Info. at File Handle
    pstFile->bType = FILESYSTEM_TYPE_FILE;
    pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
    pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
    pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentOffset = 0;
    
    if(pcMode[0] == 'a') kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
    // Sync.
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return pstFile;
}
// Read File & Copy to Buffer
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile) {
    DWORD dwTotalCount;
    DWORD dwReadCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    FILEHANDLE* pstFileHandle;
    DWORD dwNextClusterIndex;

    if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) return 0;
    pstFileHandle = &(pstFile->stFileHandle);
    // Exit if it is End of File or Last Cluster
    if((pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize) || (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER)) return 0;
    dwTotalCount = MIN(dwSize * dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset);
    // Sync.
    kLock(&(gs_stFileSystemManager.stMutex));
    // Repeat Until Calculated Value is read out
    dwReadCount = 0;
    while(dwReadCount != dwTotalCount) {
        //================================
        // Read Cluster and Copy to Buffer
        //================================
        if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) break;
        dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);
        kMemCpy((char*) pvBuffer + dwReadCount, gs_vbTempBuffer + dwOffsetInCluster, dwCopySize);
        dwReadCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;

        //======================================================================
        // Once you have finished reading current Cluster, move to next Cluster.
        //======================================================================
        if((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {
            if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) break;
            pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }
    // Sync.
    kUnlock(&(gs_stFileSystemManager.stMutex));
    // Return Read Byte Count
    return dwReadCount;
}
// Renew Directory Entry Value from Root Directory
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle) {
    DIRECTORYENTRY stEntry;
    // Seatch Directory Entry
    if((pstFileHandle == NULL) || (kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE)) return FALSE;
    // Change File Size & Start Cluster
    stEntry.dwFileSize = pstFileHandle->dwFileSize;
    stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
    // Set Changed Directory Entry
    if(kSetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE) return FALSE;
    return TRUE;
} 
// Read Buffer Data at File
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile) {
    DWORD dwWriteCount;
    DWORD dwTotalCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    DWORD dwAllocatedClusterIndex;
    DWORD dwNextClusterIndex;
    FILEHANDLE* pstFileHandle;

    // If Handle is not a File Type, it fails.
    if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) return 0;
    pstFileHandle = &(pstFile->stFileHandle);
    // Total Byte
    dwTotalCount = dwSize * dwCount;
    // Sync.
    kLock(&(gs_stFileSystemManager.stMutex));
    // Repeat Until Done
    dwWriteCount = 0;
    while(dwWriteCount != dwTotalCount) {
        //===============================================================
        // If Current Cluster is end of a file, Assign and Attach Cluster
        //===============================================================
        if(pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) {
            // Search Empty Cluster
            dwAllocatedClusterIndex = kFindFreeCluster();
            if(dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER) break;
            // Set discovered Cluster to Last Cluster
            if(kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE) break;
            // Connect Allocated Cluster to File Last Cluster
            if(kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex) == FALSE) {
                kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
                break;
            }
            // Change Current Cluster is Allocated
            pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;
            // kMemSet(gs_vbTempBuffer, 0, FILESYSTEM_LASTCLUSTER);
            kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
        }
        //====================================================================================
        // If Cluster fails to Fill up, Read Cluster and Copy it to Temporary Cluster Buffer
        //====================================================================================
        else if(((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0) || ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE)) {
            if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) break;
        }
        dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;
        dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount);
        kMemCpy(gs_vbTempBuffer + dwOffsetInCluster, (char*)pvBuffer + dwWriteCount, dwCopySize);
        if(kWriteCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE) break;
        // Renew File Point Location & Wrote Byte
        dwWriteCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;
        //==================================================
        // Move Next Cluster if Current Cluster is Filled up
        //==================================================
        if((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {
            if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE) break;
            // Renew Cluster Info.
            pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }
    //========================================================================
    // If File Size changed, Renew Directory Entry Info friom Root Directory
    //========================================================================
    if(pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset) {
        pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
        kUpdateDirectoryEntry(pstFileHandle);
    }
    // Sync.
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return (dwWriteCount / dwSize);
}
// Fill File with 0 by Count
BOOL kWriteZero(FILE* pstFile, DWORD dwCount) {
    BYTE* pbBuffer;
    DWORD dwRemainCount;
    DWORD dwWriteCount;
    // If Handle is NULL, if fails
    if(pstFile == NULL) return FALSE;
    
    pbBuffer = (BYTE*) kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if(pbBuffer == NULL) return FALSE;
    // Fill with 0
    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
    dwRemainCount = dwCount;

    while(dwRemainCount != 0) {
        dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
        if(kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount) {
            kFreeMemory(pbBuffer);
            return FALSE;
        }
        dwRemainCount -= dwWriteCount;
    }
    kFreeMemory(pbBuffer);
    return TRUE;
}
// Move file pointer location
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin) {
    DWORD dwRealOffset;
    DWORD dwClusterOffsetToMove;
    DWORD dwCurrentClusterOffset;
    DWORD dwLastClusterOffset;
    DWORD dwMoveCount;
    DWORD i;
    DWORD dwStartClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentClusterIndex;
    FILEHANDLE* pstFileHandle;

    // if(handle != file type) -> exit
    if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) return 0;
    pstFileHandle = &(pstFile->stFileHandle);

    //============================================================
    // Combining Origin and Offset to calculate the position to move the file pointer based on the start of the file
    //============================================================
    // Calculate actual position according to options

    // If negative, move beginning of the file
    // If positive, move end of the file
    switch(iOrigin) {
    case FILESYSTEM_SEEK_SET:
        if(iOffset <= 0) dwRealOffset = 0;
        else dwRealOffset = iOffset;
        break;
        
    case FILESYSTEM_SEEK_CUR:
        if((iOffset < 0) && (pstFileHandle->dwCurrentOffset <= (DWORD) -iOffset)) dwRealOffset = 0;
        else dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
        break;

    case FILESYSTEM_SEEK_END:
        if((iOffset < 0) && (pstFileHandle->dwFileSize <= (DWORD) -iOffset)) dwRealOffset = 0;
        else dwRealOffset = pstFileHandle->dwFileSize + iOffset;
        break;
    }

    //============================================================
    //============================================================
    // offset of the last cluster in the file
    dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
    // The cluster offset of where the file pointer is to be moved
    dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
    // The offset of the cluster where the current file pointer is located
    dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

    if(dwLastClusterOffset < dwClusterOffsetToMove) {
        dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    } else if(dwCurrentClusterOffset <= dwClusterOffsetToMove) {
        dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    } else {
        dwMoveCount = dwClusterOffsetToMove;
        dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    // sync
    kLock( &(gs_stFileSystemManager.stMutex));

    // move cluster
    dwCurrentClusterIndex = dwStartClusterIndex;
    for(i = 0; i < dwMoveCount; i++) {
        dwPreviousClusterIndex = dwCurrentClusterIndex;
        if(kGetClusterLinkData(dwPreviousClusterIndex, &dwCurrentClusterIndex) == FALSE) {
            // sync
            kUnlock( &(gs_stFileSystemManager.stMutex));
            return -1;
        }
    }
    if(dwMoveCount > 0) {
        pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
    } else if(dwStartClusterIndex == pstFileHandle->dwStartClusterIndex) {
        pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    if(dwLastClusterOffset < dwClusterOffsetToMove) {
        pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        if(kWriteZero(pstFile, dwRealOffset - pstFileHandle->dwFileSize) == FALSE) return 0;
    }
    pstFileHandle->dwCurrentOffset = dwRealOffset;
    
    //sync
    kUnlock( &(gs_stFileSystemManager.stMutex));
    return 0;
}

// Close File
int kCloseFile(FILE* pstFile) {
    // if(handle type != file) -> Fail
    if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE)) return -1;

    // Return handle
    kFreeFileDirectoryHandle(pstFile);
    return 0;
}

// Check handlepool -> see if file is open
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry) {
    int i;
    FILE* pstFile;

    // Search open file
    pstFile = gs_stFileSystemManager.pstHandlePool;
    for(i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        // Match : starting cluster & file type -> return
        if((pstFile[i].bType == FILESYSTEM_TYPE_FILE) && (pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex)) return TRUE;
    }
    return FALSE;
}

// Delete File
int kRemoveFile(const char* pcFileName) {
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;

    // Check file name
    iFileNameLength = kStrLen(pcFileName);
    if((iFileNameLength > (sizeof(stEntry.vcFileName) - 1)) || (iFileNameLength == 0)) return NULL;

    // sync
    kLock( &(gs_stFileSystemManager.stMutex));

    // Check File Exists
    iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
    if(iDirectoryEntryOffset == -1) {
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return -1;
    }

    // if file open -> can not delete
    if(kIsFileOpened(&stEntry) == TRUE) {
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return -1;
    }

    // Free all cluster
    if(kFreeClusterUntilEnd(stEntry.dwStartClusterIndex) == FALSE) {
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return -1;
    }

    // Set directory entry to empty
    kMemSet( &stEntry, 0, sizeof(stEntry));
    if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE) {
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return -1;
    }

    // sync
    kUnlock( &(gs_stFileSystemManager.stMutex));
    return 0;
}

// Open directory
DIR* kOpenDirectory(const char* pcDirectoryName) {
    DIR* pstDirectory;
    DIRECTORYENTRY* pstDirectoryBuffer;

    // sync
    kLock(&(gs_stFileSystemManager.stMutex));

    // ignore directory name
    // allocate hande and return
    pstDirectory = kAllocateFileDirectoryHandle();
    if(pstDirectory == NULL) {
        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return NULL;
    }

    // Allocate Buffer (that save root directory)
    pstDirectoryBuffer = (DIRECTORYENTRY*) kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if(pstDirectory == NULL) {
        kFreeFileDirectoryHandle(pstDirectory); // if Faile -> return handle
        kUnlock( &(gs_stFileSystemManager.stMutex)); // sync
        return NULL;
    }

    // Read root dirctory
    if(kReadCluster(0, (BYTE*)pstDirectoryBuffer) == FALSE) {
        // if Fail -> return all memory
        kFreeFileDirectoryHandle(pstDirectory);
        kFreeMemory(pstDirectoryBuffer);

        // sync
        kUnlock( &(gs_stFileSystemManager.stMutex));
        return NULL;
    }

    // Set directory type
    // Init current dirctory entry offset
    pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
    pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
    pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

    // sync
    kUnlock( &(gs_stFileSystemManager.stMutex));
    return pstDirectory;
}

// Return directory entry
// Move next
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory) {
    DIRECTORYHANDLE* pstDirectoryHandle;
    DIRECTORYENTRY* pstEntry;

    // (handle type != directory) -> Fail
    if((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) return NULL;
    pstDirectoryHandle =  &(pstDirectory->stDirectoryHandle);

    // (Range of Offset > Maximum Cluster) -> Fail 
    if((pstDirectoryHandle->iCurrentOffset < 0) || (pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)) return NULL;

    // sync
    kLock( &(gs_stFileSystemManager.stMutex));

    // Search : maximum number of directory entries in the root directory
    pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
    while(pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        // if file exist -> return directory entry
        if(pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex != 0) {
            // sync
            kUnlock( &(gs_stFileSystemManager.stMutex));
            return &(pstEntry[pstDirectoryHandle->iCurrentOffset++]);
        }
        pstDirectoryHandle->iCurrentOffset++;
    }
    // sync
    kUnlock( &(gs_stFileSystemManager.stMutex));
    return NULL;
}

// Move : directory pointer -> beginning of directory
void kRewindDirectory(DIR* pstDirectory) {
    DIRECTORYHANDLE* pstDirectoryHandle;

    // If handle type is not directory -> Fail
    if((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) return;
    pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

    // sync
    kLock( &(gs_stFileSystemManager.stMutex));

    // Change directory entry point to '0'
    pstDirectoryHandle->iCurrentOffset = 0;

    //sync
    kUnlock( &(gs_stFileSystemManager.stMutex));
}

// Close Open Directory
int kCloseDirectory(DIR* pstDirectory) {
    DIRECTORYHANDLE* pstDirectoryHandle;

    // If handle type is not directory -> Fail
    if((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)) return -1;
    pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

    //sync
    kLock( &(gs_stFileSystemManager.stMutex));

    // Free buffer of Root directory
    // return handle
    kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);
    kFreeFileDirectoryHandle(pstDirectory);

    //sync
    kUnlock( &(gs_stFileSystemManager.stMutex));

    return 0;
}
// Write File System Cache at HardDisk
BOOL kFlushFileSystemCache(void) {
    CACHEBUFFER* pstCacheBuffer;
    int iCacheCount;
    int i;

    if(gs_stFileSystemManager.bCacheEnable == FALSE) return TRUE;

    kLock(&(gs_stFileSystemManager.stMutex));
    kGetCacheBufferAndCount(CACHE_CLUSTERLINKTABLEAREA, &pstCacheBuffer, &iCacheCount);
    for(i = 0; i < iCacheCount; i++) {
        if(pstCacheBuffer[i].bChanged == TRUE) {
            if(kInternalWriteClusterLinkTableWithoutCache(pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer) == FALSE) return FALSE;
            pstCacheBuffer[i].bChanged = FALSE; 
        }
    }
    kGetCacheBufferAndCount(CACHE_DATAAREA, &pstCacheBuffer, &iCacheCount);
    for(i = 0; i < iCacheCount; i++) {
        if(pstCacheBuffer[i].bChanged == TRUE) {
            if(kInternalWriteClusterWithoutCache(pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer) == FALSE) return FALSE;
            pstCacheBuffer[i].bChanged = FALSE;
        }
    }
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return TRUE;
}
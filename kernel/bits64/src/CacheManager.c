#include "CacheManager.h"
#include "FileSystem.h"
#include "DynamicMemory.h"
#include "Utility.h"

static CACHEMANAGER gs_stCacheManager;

// init file system cache
BOOL kInitializeCacheManager(void) {
    int i;

    // init data struct
    kMemSet(&gs_stCacheManager, 0, sizeof(gs_stCacheManager));

    // init access time
    gs_stCacheManager.vdwAccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
    gs_stCacheManager.vdwAccessTime[CACHE_DATAAREA] = 0;
    
    // set maximum of cache buffer
    gs_stCacheManager.vdwMaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
    gs_stCacheManager.vdwMaxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;

    // manage as 512 bytes
    gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] = (BYTE*)kAllocateMemory(CACHE_MAXCLUSTERLINKTABLEAREACOUNT * 512);
    if(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] == NULL) return FALSE;

    // register to cache buffer
    for(i = 0; i < CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++) {
        // allocate memory into cache buffer
        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].pbBuffer = gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);

        // make into blank area(invalid tag)
        gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].dwTag = CACHE_INVALIDTAG;
    }

    // ---------------------------------------
    // manage as 4KB
    gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] = (BYTE*)kAllocateMemory(CACHE_MAXDATAAREACOUNT * FILESYSTEM_CLUSTERSIZE);
    if(gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] == NULL) {
        kFreeMemory(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA]);
        return FALSE;
    }

    // register to cache buffer
    for(i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
        // allocate memory into cache buffer
        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].pbBuffer = gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] + (i * FILESYSTEM_CLUSTERSIZE);

        // make into blank area(invalid tag)
        gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].dwTag = CACHE_INVALIDTAG;
    }
    return TRUE;
}

CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) return FALSE;

    // reduce access time
    kCutDownAccessTime(iCacheTableIndex);

    // return empty entity
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG - 1;

            // refresh access time
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;
            return &(pstCacheBuffer[i]);
        }
    }
    return NULL;
}

CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) return FALSE;

    kCutDownAccessTime(iCacheTableIndex);

    // return empty entity
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        if(pstCacheBuffer[i].dwTag == dwTag) {
            // refresh access time
            pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;
            return &(pstCacheBuffer[i]);
        }
    }
    return NULL;
}

static void kCutDownAccessTime(int iCacheTableIndex) {
    CACHEBUFFER stTemp;
    CACHEBUFFER *pstCacheBuffer;
    BOOL bSorted;
    int i, j;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) return;

    if(gs_stCacheManager.vdwAccessTime[iCacheTableIndex] < 0xFFFFFFFE) return;

    // sort(bubble sort)
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(j = 0; j < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1; j++) {
        bSorted = TRUE;
        for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1 - j; i++) {
            if(pstCacheBuffer[i].dwAccessTime > pstCacheBuffer[i + 1].dwAccessTime) {
                bSorted = FALSE;

                kMemCpy(&stTemp, &(pstCacheBuffer[i]), sizeof(CACHEBUFFER));
                kMemCpy(&(pstCacheBuffer[i]), &(pstCacheBuffer[i + 1]), sizeof(CACHEBUFFER));
                kMemCpy(&(pstCacheBuffer[i + 1]), &stTemp, sizeof(CACHEBUFFER));
            }
        }
        if(bSorted == TRUE) break;
    }

    // refresh data
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) pstCacheBuffer[i].dwAccessTime = i;
    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = i;
}

CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex) {
    DWORD dwOldTime;
    CACHEBUFFER *pstCacheBuffer;
    int iOldIndex;
    int i;

    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) return FALSE;

    // search oldest cache buffer
    iOldIndex = -1;
    dwOldTime = 0xFFFFFFFF;

    // return oldest one..
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];

    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) {
        // return empty one
        if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG) {
            iOldIndex = i;
            break;
        }

        if(pstCacheBuffer[i].dwAccessTime < dwOldTime) {
            dwOldTime = pstCacheBuffer[i].dwAccessTime;
            iOldIndex = i;
        }
    }

    // cannot search..??
    if(iOldIndex == -1) {
        kPrintf("Cache Buffer Find Error\n");
        return NULL;
    }
    pstCacheBuffer[iOldIndex].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

    return &(pstCacheBuffer[iOldIndex]);
}

// remove all entity of cache buffer
void kDiscardAllCacheBuffer(int iCacheTableIndex) {
    CACHEBUFFER *pstCacheBuffer;
    int i;

    // empty state
    pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    for(i = 0; i < gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++) pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG;

    // init access time
    gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = 0;
}

BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER **ppstCacheBuffer, int *piMaxCount) {
    if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX) return FALSE;

    *ppstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
    *piMaxCount = gs_stCacheManager.vdwMaxCount[iCacheTableIndex];
    return TRUE;
}
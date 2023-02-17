#ifndef __RAMDISK_H__ // 27 chapter
#define __RAMDISK_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

// macro
// Set 8Mb size (Ram disk total sector count)
#define RDD_TOTALSECTORCOUNT    (8 * 1024 * 1024 / 512)

// structure
// sort by 1 byte
#pragma pack(push, 1)

// structure : save ram disk structure
typedef struct kRDDManagerStruct {
    BYTE* pbBuffer;             // memory space that for ram disk
    DWORD dwTotalSectorCount;   // total sector count
    MUTEX stMutex;              // sync object
} RDDMANAGER;

#pragma pack(pop)

// function
BOOL kInitializeRDD(DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

#endif
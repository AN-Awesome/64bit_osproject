#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

// Macro Function Pointer
#define FILESYSTEM_SIGNATURE                0x7E38CF10  // Awesome File System Signature
#define FILESYSTEM_SECTORSPERCLUSTER        8           // Cluster Size
#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF  // File Cluster Last
#define FILESYSTEM_FREECLUSTER              0x00        // Empty Cluster
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT    ((FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY))    // Max Directory Entry at Loot Directory
#define FILESYSTEM_CLUSTERSIZE              (FILESYSTEM_SECTORSPERCLUSTER * 512)    // Cluster Size
#define FILESYSTEM_MAXFILENAMELENGTH        24          // File name Max Length
// HardDisk Controll Related Function Pointer Type
typedef BOOL(* fReadHDDInformation) (BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int(* fReadHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int(* fWriteHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
// Structure
#pragma pack(push, 1)
// Partition Data Structure
typedef struct kPartitionStruct {
    BYTE bBootableFlag;     // Bootable Flag
    BYTE vbStartingCHSAddress[3];   // Partition Start Address
    // Partition Type
    BYTE bPartitionType;
    BYTE vbEndingCHSAddress[3];
    DWORD dwStartingLBAAddress;
    DWORD dwSizeInSector;
} PARTITION;

// MBR Data Structure
typedef struct kMBRStruct {
    // Boot Loader Code
    BYTE vbBootCode[430];
    // File System Signature
    DWORD dwSignature;
    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkSectorCount;
    DWORD dwTotalClusterCount; 

    PARTITION vstPartition[4];     // Partition Table
    BYTE vbBootLoaderSignature[2]; // Boot Loader Signature 
} MBR;

// Directory Entry Data Structure
typedef struct kDirectoryEntryStruct {
    char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];
    DWORD dwFileSize;
    DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack(pop)
// File System Management Structure
typedef struct kFileSystemManagerStruct {
    BOOL bMounted;
    // Each Sector & Start LBA Address
    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;
    DWORD dwDataAreaStartAddress;
    DWORD dwTotalClusterCount;  // Data Cluster Number
    DWORD dwLastAllocatedClusterLinkSectorOffset;   // Save offset of Last cluster-assigned link table

    MUTEX stMutex;
} FILESYSTEMMANAGER;
// Function
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
DWORD kFindFreeCluster(void);
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData);
int kFindFreeDirectoryEntry(void);
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

#endif
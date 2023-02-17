#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "CacheManager.h"
// Macro Function Pointer
#define FILESYSTEM_SIGNATURE                0x7E38CF10  // Awesome File System Signature
#define FILESYSTEM_SECTORSPERCLUSTER        8           // Cluster Size
#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF  // File Cluster Last
#define FILESYSTEM_FREECLUSTER              0x00        // Empty Cluster
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT    ((FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY))    // Max Directory Entry at Loot Directory
#define FILESYSTEM_CLUSTERSIZE              (FILESYSTEM_SECTORSPERCLUSTER * 512)    // Cluster Size
#define FILESYSTEM_HANDLE_MAXCOUNT          (TASK_MAXCOUNT * 3)     // 26 chapter
#define FILESYSTEM_MAXFILENAMELENGTH        24          // File name Max Length

// define handle type // 26 chapter
#define FILESYSTEM_TYPE_FREE        0
#define FILESYSTEM_TYPE_FILE        1
#define FILESYSTEM_TYPE_DIRECTORY   2

// Definition Seek option   // 26 chapter
#define FILESYSTEM_SEEK_SET          0
#define FILESYSTEM_SEEK_CUR          1
#define FILESYSTEM_SEEK_END          2

// HardDisk Controll Related Function Pointer Type
typedef BOOL(* fReadHDDInformation) (BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int(* fReadHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int(* fWriteHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

// Redefine File system function // 26 chapter
#define fopen       kOpenFile
#define fread       kReadFile
#define fwrite      kWriteFile
#define fseek       kSeekFile
#define fclose      kCloseFile
#define remove      kRemoveFile
#define opendir     kOpenDirectory
#define readdir     kReadDirectory
#define rewinddir   kRewindDirectory
#define closedir    kCloseDirectory

// Redefine File system macro // 26chapter
#define SEEK_SET    FILESYSTEM_SEEK_SET
#define SEEK_CUR    FILESYSTEM_SEEK_CUR
#define SEEK_END    FILESYSTEM_SEEK_END

// Redefine File system type & fild
#define size_t  DWORD
#define dirent  kDirectoryEntryStruct
#define d_name  vcFileName

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

// Data Structure : file handle // 26 chapter
typedef struct kFileHandleStruct {
    int iDirectoryEntryOffset;      // entry offset
    DWORD dwFileSize;               // file size
    DWORD dwStartClusterIndex;      // file start cluster index
    DWORD dwCurrentClusterIndex;    // progress cluster index
    DWORD dwPreviousClusterIndex;    // before cluster index
    DWORD dwCurrentOffset;          // file pointer location
} FILEHANDLE;

// Directory Data Sturcture : manage directories // 26 chapter
typedef struct kDirectoryHandleStruct {
    DIRECTORYENTRY* pstDirectoryBuffer; // Buffer : save Root directory
    int iCurrentOffset                  // directory pointer current location
} DIRECTORYHANDLE;

// Data Sturcture : file & directory information
typedef struct kFileDirectoryHandleStruct {
    BYTE bType; 
    union {
        FILEHANDLE stFileHandle;    // File handle
        DIRECTORYHANDLE stDirectoryHandle;  // directory handle
    };
} FILE, DIR;

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

    FILE* pstHandlePool;    // handpool address // 26chpater

    BOOL bCacheEnable;      // Whether Cache is enabled
} FILESYSTEMMANAGER;
// Function
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

// Low Level Function // 26 chapter
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData);
static int kFindFreeDirectoryEntry(void);
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

// Cache-Related Func.
static BOOL kInternalReadClusterLinkTableWithoutCahce(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithOutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);

static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex);
BOOL kFlushFileSystemCache(void);

// High Level Function // 26 chapter
FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int kCloseFile(FILE* pstFile);
int kRemoveFile(const char* pcFileName);
DIR* kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void kRewindDirectory(DIR* pstDirectory);
int kCloseDirectory(DIR* pstDirectory);
BOOL kWriteZero(FILE* pstFile, DWORD dwCount);
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry);

static void* kAllocateFileDirectoryHandle(void);
static void kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);

#endif
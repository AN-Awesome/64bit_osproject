#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Synchronization.h"

// Macro
// 1st PATA Port(Primary PATA Port) & 2nd PATA Port(Secondary PATA Port) Info.
#define HDD_PORT_PRIMARYBASE                0x1F0
#define HDD_PORT_SECONDARYBASE              0x170

// Port Index Macro
#define HDD_PORT_INDEX_DATA                 0x00
#define HDD_PORT_INDEX_SECTORCOUNT          0x02
#define HDD_PORT_INDEX_SECTORNUMBER         0x03
#define HDD_PORT_INDEX_CYLINDERLSB          0x04
#define HDD_PORT_INDEX_CYLINDERMSB          0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD         0x06
#define HDD_PORT_INDEX_STATUS               0x07
#define HDD_PORT_INDEX_COMMAND              0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT        0x206

// Command Register Macro
#define HDD_COMMAND_READ                    0x20
#define HDD_COMMAND_WRITE                   0x30
#define HDD_COMMAND_IDENTIFY                0xEC

// Status Register Macro
#define HDD_STATUS_ERROR                    0x01
#define HDD_STATUS_INDEX                    0x02
#define HDD_STATUS_CORRECTEDDATA            0x04
#define HDD_STATUS_DATAREQUEST              0x08
#define HDD_STATUS_SEEKCOMPLETE             0x10
#define HDD_STATUS_WRITEFAULT               0x20
#define HDD_STATUS_READY                    0x40
#define HDD_STATUS_BUSY                     0x80

// Drive/Head Register Macro
#define HDD_DRIVEANDHEAD_LBA                0xE0
#define HDD_DRIVEANDHEAD_SLAVE              0x10

// Digital Output Register Macro
#define HDD_DIGITALOUTPUT_RESET             0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT  0x01

// HardDisk Response Wait Time(Milisecond)
#define HDD_WAITTIME                        500
// HDD Read/Write Sector Count
#define HDD_MAXBULKSECTORCOUNT              256

// Structure
// 1 byte alligned
#pragma pack(push, 1)

typedef struct kHDDInformationStruct {
    WORD wConfiguation;
    // Cylinder
    WORD wNumberOfCylinder;
    WORD wReserved1;
    // Head
    WORD wNumberOfHead;
    WORD wUnformattedBytesPerTrack;
    WORD wUnformattedBytesPerSector;
    // Cylinder Sector
    WORD wNumberOfSectorPerCylinder;
    WORD wInterSectorGap;
    WORD wBytesInPhaseLock;
    WORD wNumberOfVendorUniqueStatusWord;
    // HardDisk Serial Number
    WORD vwSerialNumber[10];
    WORD wControllerType;
    WORD wBufferSize;
    WORD wNumberOfECCBytes;
    WORD vwFirmwareRevision[4];
    // HardDisk Model Num.
    WORD vwModelNumber[20];
    WORD vwReserved2[13];
    // DISk Sector
    DWORD dwTotalSectors;
    WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

// HardDisk management Structure
typedef struct kHDDManagerStruct {
    // HDD Presence & Perform Write
    BOOL bHDDDetected;
    BOOL bCanWrite;

    // Interrupt & Sync Object
    volatile BOOL bPrimaryInterruptOccur;
    volatile BOOL bSecondaryInterrruptOccur;
    MUTEX stMutex;

    // HDD Info
    HDDINFORMATION stHDDInformation;
} HDDMANAGER;

// Function
BOOL kInitializeHDD(void);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);

static void kSwapByteInWord(WORD* pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);
static BOOL kIsHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);

#endif
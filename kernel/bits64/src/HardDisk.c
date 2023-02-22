#include "HardDisk.h"

// HardDisk management Data Structure
static HDDMANAGER gs_stHDDManager;
// HardDisk Device Driver Init.
BOOL kInitializeHDD(void) {
    // Mutex Init.
    kInitializeMutex(&(gs_stHDDManager.stMutex));
    // Interrupt Flag Init.
    gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
    gs_stHDDManager.bSecondaryInterrruptOccur = FALSE;
    // 1st 2nd PATA Port Digital Output Register Print 0
    // Activate Hard Disk Controller Interrupt
    kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    // Request HardDisk Info.
    if (kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation)) == FALSE) {
        gs_stHDDManager.bHDDDetected = FALSE;
        gs_stHDDManager.bCanWrite = FALSE;
        return FALSE;
    }
    // Set HardDisk once Detected, To be Used only QEMU.
    gs_stHDDManager.bHDDDetected = TRUE;
    if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0) gs_stHDDManager.bCanWrite = TRUE;
    else gs_stHDDManager.bCanWrite = FALSE;
    return TRUE;
}
// Return HardDisk Status
static BYTE kReadHDDStatus(BOOL bPrimary) {
    if(bPrimary == TRUE) return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}
// Wait Until HardDisk Busy disable
static BOOL kWaitForHDDNoBusy(BOOL bPrimary) {
    QWORD qwStartTickCount;
    BYTE bStatus;
    // Save Time started Waiting
    qwStartTickCount = kGetTickCount();
    // Wait Until HardDisk Busy disable
    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
        // Return HDD Status
        bStatus = kReadHDDStatus(bPrimary);
        // Exit when Busy bit is not set up
        if (( bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) return TRUE;
        kSleep(1);
    }
    return FALSE;
}
// Wait Until HardDisk is prepared
static BOOL kWaitForHDDReady(BOOL bPrimary) {
    QWORD qwStartTickCount;
    BYTE bStatus;

    // Save Time started Waiting
    qwStartTickCount = kGetTickCount();
    // Wait Until HardDisk is Prepared
    while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME) {
        // Retrun HDD Status
        bStatus = kReadHDDStatus(bPrimary);
        // Exit when Ready bit is not set up
        if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY) return TRUE;
        kSleep(1);
    }
    return FALSE;
}
// Set Interrupt Occur
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag) {
    if(bPrimary == TRUE) gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
    else gs_stHDDManager.bSecondaryInterrruptOccur = bFlag;
}
// Wait Until Interrupt Occur
static BOOL kWaitForHDDInterrupt(BOOL bPrimary) {
    QWORD qwTickCount;
    // Save Time started Waiting
    qwTickCount = kGetTickCount();
    // Wait Until HardDisk Interrupt Occur
     while(kGetTickCount() - qwTickCount <= HDD_WAITTIME) {
        // Check Interrupt Occur Flag at HardDisk DataStructure
        if((bPrimary == TRUE) && (gs_stHDDManager.bPrimaryInterruptOccur == TRUE)) return TRUE;
        else if((bPrimary == FALSE) && (gs_stHDDManager.bSecondaryInterrruptOccur == TRUE)) return TRUE;
     }
     return FALSE;
}
// Read HardDisk Info.
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformaiton) {
    WORD wPortBase;
    QWORD qwLastTickCount;
    BYTE bStatus;
    BYTE bDriveFlag;
    int i;
    WORD wTemp;
    BOOL bWaitResult;
    // Set Default Address of I/O Port according to PATA Port
    if(bPrimary == TRUE) wPortBase = HDD_PORT_PRIMARYBASE;
    else wPortBase = HDD_PORT_SECONDARYBASE;
    // Sync. Process
    kLock(&(gs_stHDDManager.stMutex));

    if(kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }
    // =======================================
    // Set LBA Address, Drive & Head Register
    // =======================================
    // Set Drive & Head Data
    if(bMaster == TRUE) bDriveFlag = HDD_DRIVEANDHEAD_LBA; // If Master set only LBA Flag
    else bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE; // If Slave set Slave Flag at LBA Flag
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);
    //===============================
    // Send Command & Wait Interrupt
    //===============================
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        // Sync. Process
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }
    // Interrupt Flag Init.
    kSetHDDInterruptFlag(bPrimary, FALSE);
    // Send Drive Recognition Command to Command Register
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

    // Wait Until Intterupt Occur
    bWaitResult = kWaitForHDDInterrupt(bPrimary);
    // Exit if Error or Interrupt is not Occured
    bStatus = kReadHDDStatus(bPrimary);
    if((bWaitResult == FALSE) || ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)) {
        // Sync. Process
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }
    //===============
    // Recieve Data
    //===============
    // Read 1 Sector
    for(i = 0; i < 512 / 2; i++) ((WORD*)pstHDDInformaiton)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA); 
    
    // Convert String to Byte
    kSwapByteInWord(pstHDDInformaiton->vwModelNumber, sizeof(pstHDDInformaiton->vwModelNumber) / 2);
    kSwapByteInWord(pstHDDInformaiton->vwSerialNumber, sizeof(pstHDDInformaiton->vwSerialNumber) / 2);
    // Sync. Process
    kUnlock(&(gs_stHDDManager.stMutex));
    return TRUE;
}
// Change WORD Byte Order
static void kSwapByteInWord(WORD* pwData, int iWordCount) {
    int i;
    WORD wTemp;

    for(i = 0; i < iWordCount; i++) {
        wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}

int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    WORD wPortBase;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    // Check Range.
    if((gs_stHDDManager.bHDDDetected == FALSE) || (iSectorCount <= 0) || (256 < iSectorCount) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)) return 0;

    // Set I/O Defalut Address
    if(bPrimary == TRUE) wPortBase = HDD_PORT_PRIMARYBASE;
    else wPortBase = HDD_PORT_SECONDARYBASE;

    // Sync.
    kLock(&(gs_stHDDManager.stMutex));
    
    if(kWaitForHDDNoBusy(bPrimary) == FALSE) {
        // Sync.
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }
    //=====================
    // Set Data Register
    //=====================
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // Set Drive Head Data
    if(bMaster == TRUE) bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    else bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;

    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));
    //===============
    // Send Command
    //===============
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        // Sync.
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }
    // Interrupt Flag Init.
    kSetHDDInterruptFlag(bPrimary, FALSE);
    // Send Read(0x20) to Command Register
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

    //===============================
    // Wait Interrupt & Recieve Data
    //===============================
    for(i = 0; i < iSectorCount; i++) {
        // Exit if Error Occur
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) { 
            kPrintf("Error Occur\n");
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }
        // DATA Request
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            if(bWaitResult == FALSE) {
                kPrintf("Interrupt Not Occur\n");
                kUnlock(&(gs_stHDDManager.stMutex));
                return FALSE;
            }
        }
        for(j = 0; j < 512 / 2; j++) ((WORD*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}

int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    WORD wPortBase;
    WORD wTemp;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;

    // Check Range
    if((gs_stHDDManager.bCanWrite == FALSE) || (iSectorCount <= 0) || (256 < iSectorCount) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)) return 0;
    // Set Defalut Address
    if(bPrimary == TRUE) wPortBase = HDD_PORT_PRIMARYBASE;
    else wPortBase = HDD_PORT_SECONDARYBASE;

    if(kWaitForHDDNoBusy(bPrimary) == FALSE) return FALSE;
    kLock(&(gs_stHDDManager.stMutex));
    //===================
    // Set Data Register
    //===================
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // Set Drive & Head Data
    if(bMaster == TRUE) bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    else bDriveFlag =HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    // Send Set Value
    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));
    //=====================================================
    // Wait for data to be sent after command transmission
    //=====================================================
    if(kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // Send Command
    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);
    // Wait Until Data to be sent
    while(1) {
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock( &(gs_stHDDManager.stMutex));
            return 0;
        }
        if((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST) break;
        kSleep(1);
    }
    //================================
    // Wait Interrupt after Data Sent
    //================================
    for(i = 0; i < iSectorCount; i++) {
        kSetHDDInterruptFlag(bPrimary, FALSE);
        for(j = 0; j < 512 / 2; j++) kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lReadCount++]);
        bStatus = kReadHDDStatus(bPrimary);
        if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }
        if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            if(bWaitResult == FALSE) {
                kUnlock(&(gs_stHDDManager.stMutex));
                return FALSE;
            }
        }
    }
    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}

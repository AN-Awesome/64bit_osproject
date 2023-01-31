#include "Utility.h"
#include "AssemblyUtility.h"

void kMemSet(void* pvDestination, BYTE bData, int iSize) {
    int i;
    for (i = 0; i < iSize; i++) ((char*)pvDestination)[i] = bData;
}

int kMemCpy(void* pvDestination, const void* pvSource, int iSize) {
    int i;
    for (i = 0; i < iSize; i++) ((char*)pvDestination)[i] = ((char*)pvSource)[i];
    return iSize;
}

int kMemCmp(const void* pvDestination, const void* pvSource, int iSize) {
    int i;
    char cTemp;
    for(i = 0; i < iSize; i++) {
        cTemp = ((char*)pvDestination)[i] - ((char*)pvSource)[i];
        if(cTemp != 0) return (int)cTemp;
    }
    return 0;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
    QWORD qwRFLAGS;
    
    // RFLAGS Read
    qwRFLAGS = kReadRFLAGS();
    if(bEnableInterrupt == TRUE) kEnableInterrupt();
    else kDisableInterrupt();
    
    // Check previous interrupt status through RFLAGS(BITS 9_IF) register
    // Return previous state
    if(qwRFLAGS & 0x0200) return TRUE;
    return FALSE;
}
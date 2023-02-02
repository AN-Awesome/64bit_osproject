#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

// Structure
// Sort 1Byte
#pragma pack(push, 1)

// Data Structure of Link Data
// Must be placed at the beginning of the Data
typedef struct kListlinkStruct{
    // ID to identify
    void* pvNext;
    QWORD qwID;
} LISTLINK;

// Data Structure for Managing List
typedef struct kListManagerStruct{
    // Number of List Data
    int iItemCount;
    // List First Address
    void* pvHeader;
    // List Last Address
    void* pvTail;
} LIST;

#pragma pack(pop)

//Function
void kInitializeList(LIST* pstList);
int kGetListCount(const LIST* pstList );
void kAddListToTail(LIST* pstList, void* pvItem );
void kAddListToHeader(LIST* pstList, void* pvItem );
void* kRemoveList(LIST* pstList, QWORD qwID);
void* kRemoveListFromHeader(LIST* pstList);
void* kRemoveListFromTail (LIST* pstList );
void* kFindList(const LIST* pstList, QWORD qwID);
void* kGetHeaderFromList(const LIST* pstList);
void* kGetTailFromList(const LIST* pstList );
void* kGetNextFromList(const LIST* pstList, void* pstCurrent);

#endif
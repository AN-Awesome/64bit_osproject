#include "List.h"

// List init
void kInitializeList(LIST* pstList){
    pstList->iItemCount = 0;
    pstList->pvHeader = NULL;
    pstList->pvTail = NULL;
}

// Returns the number of items in a List
int kGetListCount(const LIST* pstList) {
    return pstList->iItemCount;
}

// Add data to List
void kAddListToTail(LIST* pstList, void* pvItem) {
    LISTLINK* pstLink;

    pstLink = (LISTLINK*) pvItem;
    pstLink->pvNext = NULL;

    // if LIst is Empty -> Set Header and Tail to added data
    if(pstList->pvHeader == NULL) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return ;
    }

    // Get the LISTLINK location of the last data and set the next data to the added data
    pstLink = (LISTLINK*) pstList->pvTail;
    pstLink->pvNext = pvItem;

    // Change the last data in the list to the added data
    pstList->pvTail = pvItem;
    pstList->iItemCount++;
}

// Add data to the first part of the list
void kAddListToHeader(LIST* pstList, void* pvItem) {
    LISTLINK* pstLink;

    // Set the address of the next data as Header
    pstLink = (LISTLINK*) pvItem;
    pstLink->pvNext = pstList->pvHeader;

    // If List is Empty -> Set Header and Tail to Added Data
    if(pstList->pvHeader == NULL){
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return ;
    }

    // Change : List First Data -> Added Data
    pstList->pvHeader = pvItem;
    pstList->iItemCount++;
}

// Delete Data from List
// Return Data Pointer
void* kRemoveList(LIST* pstList, QWORD qwID) {
    LISTLINK* pstLink;
    LISTLINK* pstPreviousLink;

    pstPreviousLink = (LISTLINK*) pstList->pvHeader;
    for(pstLink = pstPreviousLink; pstLink != NULL; pstLink = pstLink->pvNext){
        // Match ID is Exist -> Delete
        if(pstLink->qwID == qwID){
            // if Data is only one -> List reset
            if((pstLink == pstList->pvHeader) && (pstLink == pstList->pvTail)) {
                pstList->pvHeader = NULL;
                pstList->pvTail = NULL;
            }
            // if First Data -> Change Second data
            else if(pstLink == pstList->pvHeader) pstList->pvHeader = pstLink->pvNext;

            // if Last Data -> Move Tail to before the Last Data
            else if(pstLink == pstList->pvTail) pstList->pvTail = pstPreviousLink;
            else pstPreviousLink->pvNext = pstLink->pvNext;
            pstList->iItemCount--;
            return pstLink;
        }
        pstPreviousLink = pstLink;
    }
    return NULL;
}

// Delete List First Data and Return
void* kRemoveListFromHeader(LIST* pstList) {
    LISTLINK* pstLink;

    if(pstList->iItemCount == 0) return NULL;

    // Delete Header and Return
    pstLink = (LISTLINK*) pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}

// Delete Last Data of List and Return
void* kRemoveListFromTail(LIST* pstList) {
    LISTLINK* pstLink;

    if(pstList->iItemCount == 0) return NULL;

    // Delete Tail and Return
    pstLink = (LISTLINK*) pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}

// Fine Item in List
void* kFindList(const LIST* pstList, QWORD qwID) {
    LISTLINK* pstLink;

    for(pstLink = (LISTLINK*) pstList->pvHeader; pstLink != NULL; pstLink = pstLink->pvNext){
        // if there is a Match -> Return
        if(pstLink->qwID == qwID) return pstLink;
    }
    return NULL;
}

// Return List Header
void* kGetHeaderFromList(const LIST* pstList) {
    return pstList->pvHeader;
}

// Return LIst Tail
void* kGetTailFromList(const LIST* pstList) {
    return pstList->pvTail;
}

// Return Next Item
void* kGetNextFromList(const LIST* pstList, void* pstCurrent) {
    LISTLINK* pstLink;
    pstLink = (LISTLINK*) pstCurrent;
    return pstLink->pvNext;
}
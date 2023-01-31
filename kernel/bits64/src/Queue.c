#include "Queue.h"

// Init Queue
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize) {
    // Buffer
    pstQueue->iMaxDataCount = iMaxDataCount;
    pstQueue->iDataSize = iDataSize;
    pstQueue->pvQueueArray = pvQueueBuffer;

    // 큐의 삽입 위치와 제거 위치를 초기화하고 마지막으로 수행된 명령을 제거로 설정하여 큐를 빈 상태로 만듦
    pstQueue->iPutIndex = 0;
    pstQueue->iGetIndex = 0;
    pstQueue->bLastOperationPut = FALSE;
}

// Check and Return Queue-Full State
BOOL kIsQueueFull(const QUEUE* pstQueue) {
    // Index of Add == Index of Remove
    // Last Command == Add();
    // ===> Queue is Full ==> Cannot Add
    if((pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == TRUE)) return TRUE;
    return FALSE;    
}

// Check and Return Queue-Empty State
BOOL kIsQueueEmpty(const QUEUE* pstQueue) {
    // Index of Add == Index of Remove
    // Last Command == Remove();
    // ===> Queue is Empty ==> Cannot Remove
    if((pstQueue->iGetIndex == pstQueue->iPutIndex) && (pstQueue->bLastOperationPut == FALSE)) return TRUE;
    return FALSE;
}

// Add Items to Queue
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData) {
    // Is Queue Full?
    if(kIsQueueFull(pstQueue) == TRUE) return FALSE;

    // Pointer + sizeof(data) ==> copy
    kMemCpy((char*) pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex), pvData, pstQueue->iDataSize);
    
    // add log(Add Process)
    pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = TRUE;
    return TRUE;
}

// Remove Items from Queue
BOOL kGetQueue(QUEUE* pstQueue, void* pvData) {
    // Is Queue Empty?
    if(kIsQueueEmpty(pstQueue) == TRUE) return FALSE;

    // Pointer + sizeof(data) ==> copy
    kMemCpy(pvData, (char*) pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize);
    
    // add log(Remove Process)
    pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = FALSE;

    return TRUE;
}
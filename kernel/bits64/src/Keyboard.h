#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

// Define the number of scan codes that should be ignored when processing Puase keys
#define KEY_SKIPCOUNTFORPAUSE 2

//Flags containing values for key state(REGISTRY__)
#define KEY_FLAGS_UP 0x00
#define KEY_FLAGS_DOWN 0x01
#define KEY_FLAGS_EXTENDEDKEY 0x02

// Macro for Scan Code Mapping Table
#define KEY_MAPPINGTABLEMAXCOUNT 89

#define KEY_NONE 0x00
#define KEY_ENTER '\n'

#define KEY_TAB '\t'

#define KEY_ESC 0x1B
#define KEY_BACKSPACE 0x08

#define KEY_CTRL 0x81
#define KEY_LSHIFT 0x82
#define KEY_RSHIFT 0x83

/* LALT: LEFT ALT*/
#define KEY_LALT 0x85
#define KEY_CAPSLOCK 0x86

#define KEY_F1 0x87
#define KEY_F2 0x88
#define KEY_F3 0x89
#define KEY_F4 0x8A
#define KEY_F5 0x8B
#define KEY_F6 0x8C
#define KEY_F7 0x8D
#define KEY_F8 0x8E
#define KEY_F9 0x8F
#define KEY_F10 0x90
#define KEY_F11 0x9E
#define KEY_F12 0x9F

#define KEY_NUMLOCK 0x91

#define KEY_PAUSE 0xA0
#define KEY_SCROLLLOCK 0x92
#define KEY_PRINTSCREEN 0x84

#define KEY_HOME 0x93
#define KEY_END 0x99
#define KEY_PAGEUP 0x95
#define KEY_PAGEDOWN 0x9B

#define KEY_INS 0x9C
#define KEY_DEL 0x9D

#define KEY_UP 0x94
#define KEY_DOWN 0x9A
#define KEY_CENTER 0x97
#define KEY_LEFT 0x96
#define KEY_RIGHT 0x98

// 키 큐의 최대 크기
#define KEY_MAXQUEUECOUNT 100

#pragma pack (push, 1)

typedef struct kKeyMappingEntryStruct {
    // Not Combined with Shift or Caps Lock
    BYTE bNormalCode;
    // Combined with Shift or Caps Lock
    BYTE bCombinedCode;
} KEYMAPPINGENTRY;

#pragma pack(pop)

typedef struct kKeyboardManagerStruct {
    // Combination key Info.
    BOOL bShiftDown;
    BOOL bCapsLockOn;
    BOOL bNumLockOn;
    BOOL bScrollLockOn;
    // Extended key Info.
    BOOL bExtendedCodeIn;
    int iSkipCountForPause;
} KEYBOARDMANAGER;

// 키 큐에 삽입할 데이터 구조체
typedef struct kKeyDataStruct{
    //키보드에서 전달된 스캔 코드
    BYTE bScanCode;
    
    //스캔 코드를 변환한 ASCII 코드
    BYTE bASCIICode;

    //키 상태를 저장하는 플래그(눌림/떨어짐/확징 키 여부)
    BYTE bFlags;
}KEYDATA;

// Function
BOOL kIsOutputBufferFull(void);
BOOL kIsInputBufferFull(void);
BOOL kActivateKeyboard(void);
BOOL kGetKeyboardScanCode(void);
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn);
void kEnableA20Gate(void);
void kReboot(void);
BOOL kIsAlphabetScanCode(BYTE bScanCode);
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode);
BOOL kIsNumberPadScanCode(BYTE bScanCode);
BOOL kIsUseCombinedCode(BYTE bScanCode);
void UpdateCombinationKeyStatusAndLED(BYTE bScanCode);

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICODE, BOOL* pbFlags);
BOOL kInitializeKeyboard(void);
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode);
BOOL kGetKeyFromKeyQueue(KEYDATA* pstData);
BOOL kWaitForACKAndPutOtherScanCode(void);

#endif
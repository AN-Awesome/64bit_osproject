#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"

BOOL kIsOutputBufferFull(void) {
    // 1: Input Data Exists..
    if(kInPortByte(0x64) & 0x01) return TRUE;
    return FALSE;
}

BOOL kIsInputBufferFull(void) {
    // 1: There is data not taken by the keyboard.
    if(kInPortByte(0x64) & 0x02) return TRUE;
    return FALSE;
}

// Wait ACK
BOOL kWaitForACKAndPutOtherScanCode(void) {
    int i, j;
    BYTE bData;
    BOOL bResult = FALSE;

    for(j=0; j<100; j++) {
        for(i=0; i<0xFFFF; i++) {
            if(kIsOutputBufferFull() == TRUE) break;
        }

    // Output buffer data ACK
    bData = kInPortByte(0x60);
    if(bData == 0xFA) {
        bResult = TRUE;
        break;
    }
    // NOT ACK = Convert to ASCII code
    else kConvertScanCodeAndPutQueue(bData);
    }
    return bResult;
}

BOOL kActivateKeyboard(void) {
    int i;
    int j;
    
    kOutPortByte(0x64, 0xAE); // (0x64, 0xAE) :: Parameters for activating the keyboard device
    
    for(j=0; j<100; j++){
        for(i = 0; i<0xFFFF; i++) if(kIsOutputBufferFull() == TRUE) break;
        if(kInPortByte(0x60) == 0xFA) return TRUE;
    }
    return FALSE;
}

BYTE kGetKeyboardScanCode(void) {
    while(kIsOutputBufferFull() == FALSE) break;
    return kInPortByte(0x60);
}

//============================================================
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn) {
    int i, j;

    for(i = 0; i < 0xFFFF; i++) if(kIsInputBufferFull() == FALSE) break; // Commands can be sent when the input buffer (port 0x60) is empty

    kOutPortByte(0x60, 0xED);
    for(i = 0; i < 0xFFFF; i++) if(kIsInputBufferFull() == FALSE) break; // When a command is obtained from the keyboard, the input buffer (0x60) is emptied.
    for(j = 0; j<100; j++){
        for(i = 0; i < 0xFFFF; i++) if(kIsOutputBufferFull() == TRUE) break;
        if(kInPortByte(0x60) == 0xFA) break;
    } 
    if(j >= 100) return FALSE;

    kOutPortByte(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | bScrollLockOn);
    for(i = 0; i < 0xFFFF; i++) if(kIsInputBufferFull() == FALSE) break;

    for(j = 0; j < 100; j++) {
        for(i = 0; i < 0xFFFF; i++) if(kIsOutputBufferFull() == TRUE) break;

        // If the data read from the output buffer(Port 0x60) is ACK(0xFA), it works normally.
        if(kInPortByte(0x60) == 0xFA) break;    // 0x60: Port number of the input buffer
                                                // 0xFA: Return value for normal processing
    }
    if(j >= 100) return FALSE;
    return TRUE;
}

void kEnableA20Gate(void) {
    BYTE bOutputPortData;
    int i;

    // Send 0xD0(Read output value of Keyboard Controller) command to Port64(Control Register)
    kOutPortByte(0x64, 0xD0);

    // Read the data of the output port.
    for(i = 0; i < 0xFFFF; i++) if(kIsOutputBufferFull() == TRUE) break;

    // Read the output port value of the keyboard controller received at the output port (port 0x60)
    bOutputPortData = kInPortByte(0x60);

    // A20 gate bit setting
    bOutputPortData |= 0x01;    // OR Operation

    // If data is empty in the input buffer (port 0x60), a command to write a value to the output port and data transmission to the output port
    for(i = 0; i < 0xFFFF; i++) if(kIsInputBufferFull() == FALSE) break;

    // Transmits the output port setting command (0xD1) to the command register (0x64)
    kOutPortByte(0x64, 0xD1);

    // Passes the value with the A20 gate bit set to 1 to the input buffer (0x60)
    kOutPortByte(0x60, bOutputPortData);
}

// Reset the processor
// Keyboard.h
void kReboot(void) {
    int i;

    // If data is empty in the input buffer (port 0x60), a command to write a value to the output port and data transmission to the output port
    for(i = 0; i < 0xFFFF; i++) if(kIsInputBufferFull() == FALSE) break;

    // Transmits the output port setting command (0xD1) to the command register (0x64)
    kOutPortByte(0x64, 0xD1);

    // Reset the processor by passing 0 to the input buffer (0x60)
    kOutPortByte(0x60, 0x00);

    while(1) { ; }
}

// Functions related to the ability to convert scan codes to ASCII codes
static KEYBOARDMANAGER gs_stKeyboardManager = {0,}; // Keyboard state manager

static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
    /* 0 */  {KEY_NONE, KEY_NONE},
    /* 1 */  {KEY_ESC, KEY_ESC},
    /* 2 */  {'1', '!'},
    /* 3 */  {'2', '@'},
    /* 4 */  {'3', '#'},
    /* 5 */  {'4', '$'},
    /* 6 */  {'5', '%'},
    /* 7 */  {'6', '^'},
    /* 8 */  {'7', '&'},
    /* 9 */  {'8', '*'},
    /* 10 */ {'9', '('},
    /* 11 */ {'0', ')'},
    /* 12 */ {'-', '_'},
    /* 13 */ {'=', '+'},
    /* 14 */ {KEY_BACKSPACE, KEY_BACKSPACE},
    /* 15 */ {KEY_TAB, KEY_TAB},
    /* 16 */ {'q', 'Q'},
    /* 17 */ {'w', 'W'},
    /* 18 */ {'e', 'E'},
    /* 19 */ {'r', 'R'},
    /* 20 */ {'t', 'T'},
    /* 21 */ {'y', 'Y'},
    /* 22 */ {'u', 'U'},
    /* 23 */ {'i', 'I'},
    /* 24 */ {'o', 'O'},
    /* 25 */ {'p', 'P'},
    /* 26 */ {'[', '{'},
    /* 27 */ {']', '}'},
    /* 28 */ {'\n', '\n'},
    /* 29 */ {KEY_CTRL, KEY_CTRL},
    /* 30 */ {'a', 'A'},
    /* 31 */ {'s', 'S'},
    /* 32 */ {'d', 'D'},
    /* 33 */ {'f', 'F'},
    /* 34 */ {'g', 'G'},
    /* 35 */ {'h', 'H'},
    /* 36 */ {'j', 'J'},
    /* 37 */ {'k', 'K'},
    /* 38 */ {'l', 'L'},
    /* 39 */ {';', ':'},
    /* 40 */ {'\'', '\"'},
    /* 41 */ {'`', '~'},
    /* 42 */ {KEY_LSHIFT, KEY_LSHIFT},
    /* 43 */ {'\\', '|'},
    /* 44 */ {'z', 'Z'},
    /* 45 */ {'x', 'X'},
    /* 46 */ {'c', 'C'},
    /* 47 */ {'v', 'V'},
    /* 48 */ {'b', 'B'},
    /* 49 */ {'n', 'N'},
    /* 50 */ {'m', 'M'},
    /* 51 */ {',', '<'},
    /* 52 */ {'.', '>'},
    /* 53 */ {'/', '?'},
    /* 54 */ {KEY_RSHIFT, KEY_RSHIFT},
    /* 55 */ {'*', '*'},
    /* 56 */ {KEY_LALT, KEY_LALT},
    /* 57 */ {' ', ' '},
    /* 58 */ {KEY_CAPSLOCK, KEY_CAPSLOCK},
    /* 59 */ {KEY_F1, KEY_F1},
    /* 60 */ {KEY_F2, KEY_F2},
    /* 61 */ {KEY_F3, KEY_F3},
    /* 62 */ {KEY_F4, KEY_F4},
    /* 63 */ {KEY_F5, KEY_F5},
    /* 64 */ {KEY_F6, KEY_F6},
    /* 65 */ {KEY_F7, KEY_F7},
    /* 66 */ {KEY_F8, KEY_F8},
    /* 67 */ {KEY_F9, KEY_F9},
    /* 68 */ {KEY_F10, KEY_F10},
    /* 69 */ {KEY_NUMLOCK, KEY_NUMLOCK},
    /* 70 */ {KEY_SCROLLLOCK, KEY_SCROLLLOCK},

    /* 71 */ {KEY_HOME, '7'},
    /* 72 */ {KEY_UP, '8'},
    /* 73 */ {KEY_PAGEUP, '9'},
    /* 74 */ {'-', '-'},
    /* 75 */ {KEY_LEFT, '4'},
    /* 76 */ {KEY_CENTER, '5'},
    /* 77 */ {KEY_RIGHT, '6'},
    /* 78 */ {'+', '+'},
    /* 79 */ {KEY_END, '1'},
    /* 80 */ {KEY_DOWN, '2'},
    /* 81 */ {KEY_PAGEDOWN, '3'},
    /* 82 */ {KEY_INS, '0'},
    /* 83 */ {KEY_DEL, '9'},
    /* 84 */ {KEY_NONE, KEY_NONE},
    /* 85 */ {KEY_NONE, KEY_NONE},
    /* 86 */ {KEY_NONE, KEY_NONE},
    /* 87 */ {KEY_F11, KEY_F11},
    /* 88 */ {KEY_F12, KEY_F12}
};

BOOL kIsAlphabetScanCode(BYTE bScanCode) {
    if(('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) && (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z')) return TRUE;
    return FALSE;
}

// Returns whether it is a numeric or symbolic range(2~53, not English char. -> Number/Symbol)
// Keyboard.h
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode) {
    if((2 <= bScanCode) && (bScanCode <= 53) && (kIsAlphabetScanCode(bScanCode) == FALSE)) return TRUE;
    return FALSE;
}

// Returns whether the number pad range is(71~83)
// Keyboard.h
BOOL kIsNumberPadScanCode(BYTE bScanCode) {
    if((71 <= bScanCode) && (bScanCode <= 83)) return TRUE;
    return FALSE;
}

BOOL kIsUseCombinedCode(BYTE bScanCode){
    BYTE bDownScanCode;
    BOOL bUseCombinedKey = FALSE;

    bDownScanCode = bScanCode & 0x7F;

    if(kIsAlphabetScanCode(bDownScanCode) == TRUE){
        if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn) bUseCombinedKey = TRUE;
        else bUseCombinedKey = FALSE;
    }
    else if(kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE){
        if(gs_stKeyboardManager.bShiftDown == TRUE) bUseCombinedKey = TRUE;
        else bUseCombinedKey = FALSE;
    }
    else if((kIsNumberPadScanCode(bDownScanCode) == TRUE) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE)){
        if(gs_stKeyboardManager.bNumLockOn == TRUE) bUseCombinedKey = TRUE;
        else bUseCombinedKey = FALSE;
    }
    return bUseCombinedKey;
}

void UpdateCombinationKeyStatusAndLED(BYTE bScanCode) {
    BOOL bDown;
    BYTE bDownScanCode;
    BOOL bLEDStatusChanged = FALSE;

    // Determines KeyDown and KeyUp through the value of the highest bit (Bit 7)
    // 1: KeyUp, 0: KeyDown
    if(bScanCode & 0x80) {
        bDown = FALSE;
        bDownScanCode = bScanCode & 0x7F;
    } else {
        bDown = TRUE;
        bDownScanCode = bScanCode;
    }

    // Combination key Search/checkBOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICODE, BO
    // If the scan code of the Shift key is 42 or 54, the status of the Shift key is updated
    if((bDownScanCode == 42) || (bDownScanCode == 54)) gs_stKeyboardManager.bShiftDown = bDown;
    else if((bDownScanCode == 58) && (bDown == TRUE)) { // If the scan code of the Caps Lock key is 58, update the state of Caps Lock and change the state of the LED
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    } else if((bDownScanCode == 69) && (bDown == TRUE)) { // If the scan code of the Num Lock key is (69) then update the Num Lock state and change the LED state
        gs_stKeyboardManager.bNumLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    } else if((bDownScanCode == 70) && (bDown == TRUE)) { // If the scan code of the Scroll Lock key is 70, update the state of the Scroll Lock and change the state of the LED
        gs_stKeyboardManager.bScrollLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    // Actual LED blinking is controlled according to the changed keyboard LED status value.
    // Caps Lock, Num Lock, Scroll Lock
    if(bLEDStatusChanged == TRUE) kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
}

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE *pbASCIICode, BOOL *pbFlags) {
    BOOL bUseCombinedKey;

    // If a Pause key has been previously received, ignore the remaining scan codes in Pause
    if(gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    // Pause Key
    if(bScanCode == 0xE1) {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    } else if(bScanCode == 0xE0) { // When the extended key code comes in, the actual key value comes next, so just set the flag and exit
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }
    
    // A function that returns whether the currently entered scan code and combination key state should be converted to a combination key.
    bUseCombinedKey = kIsUseCombinedCode(bScanCode);
    
    // Set key value
    if(bUseCombinedKey == TRUE) *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode; // 스캔 코드를 ASCII 코드로 변화하는 테이블
    else *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;

    // Extended key existence setting
    if(gs_stKeyboardManager.bExtendedCodeIn == TRUE) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    } else *pbFlags = 0;

    // Setting whether pressed or dropped
    if((bScanCode & 0x80) == 0) *pbFlags |= KEY_FLAGS_DOWN;

    // Update modifier key pressed or dropped state
    UpdateCombinationKeyStatusAndLED(bScanCode);
    return TRUE;
}

BOOL kInitializeKeyboard(void) {
    // Init Queue
    kInitializeQueue( &gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));

    // Activate Keyboard
    return kActivateKeyboard();
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
    KEYDATA stData;
    BOOL bResult = FALSE;
    BOOL bPreviousInterrupt;

    // Insert Scan Code into Key Data
    stData.bScanCode = bScanCode;

    // Convert scan code to ASCII code & key state to insert key data
    if (kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode)&(stData.bFlags)) == TRUE) {
        // Cannot Interrupt
        bPreviousInterrupt = kSetInterruptFlag(FALSE);

        // Insert Key Queue
        bResult = kPutQueue( &gs_stKeyQueue, &stData);

        // Restore previous interrupt flags
        kSetInterruptFlag(bPreviousInterrupt);
    }

    result bResult;
}

// Delete Key Data from Key Queue
BOOL kGetKeyFromKeyQueue(KEYDATA* pstData) {
    BOOL bResult;
    BOOL bPreviousInterrupt;

    if( kIsQueueEmpty(&gs_stKeyQueue) == TRUE) return FALSE;

    // Unable to Interrupt
    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // Insert Key Queue
    bResult = kGetQueue(&gs_stKeyQueue, pstData);

    // Restore previous interrupt flags
    kSetInterruptFlag(bPreviousInterrupt);
    return bResult;
}
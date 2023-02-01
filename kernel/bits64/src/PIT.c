#include "PIT.h"

// Init PIT
void kInitializePIT(WORD wCount, BOOL bPeriodic) {
    // PIT Control Register value init. stop count
    // Set Mode 0 Binary Counter
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

    // Set Mode 2 if Timer repeats regular intervals
    if(bPeriodc == TRUE) kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);

    // Set Initial value in order to LSB -> MSB at Counter 0
    kOutPortByte(PIT_PORT_COUNTER0, wCount);
    kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}
// Return Current Value of Counter 0
WORD kReadCounter0(void) {
    BYTE bHighByte, bLowByte;
    WORD wTemp = 0;

    // Send Latch Command to PIT Control Register to read Current Value on Counter 0
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    // Read Counter Value in order of LSB -> MSB on Counter 0
    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);
    // Return Value combined with Hex
    wTemp = bHighByte;
    wTemp = (wTemp << 8) | bLowByte;
    return wTemp;
}

// Set Counter 0 directly & Wait
void kWaitUsingDirectPIT(WORD wCount) {
    WORD wLastCounter0;
    WORD wCurrentCounter0;

    // Set PIT Controller to Count 0~0xFFFF Repeatedly
    kInitilaizePIT(0, TRUE);
    // Wait Until wCount Increases
    wLastCounter0 = kReadCounter0();
    while(1) {
        // Return Current Value of Counter 0
        wCurrentCounter0 = kReadCounter0();
        if(((wLastCounter0 - wCurrentCounter0) &0xFFFF) >= wCount) break;
    }
}

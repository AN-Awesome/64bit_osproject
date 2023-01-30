#include "PIC.h"

// Init PIC
void kInitializePIC(void) {
    /* =======================
    Init Master PIC Controller
    ======================= */

    // ICW1 (Port 0x20), IC4 (Bit 0) = 1
    kOutPortByte(PIC_MASTER_PORT1, 0x11);

    // ICW2 (Port 0x21), Interrupt Vector(0x20)
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

    // ICW3 (Port 0x21), Slave PIC Controller: Connect Point(Bits)
    kOutPortByte(PIC_MASTER_PORT2, 0x04);   // PIN2: 0x04(BIT 2)

    // ICW4(Port 0x21), uPM(BIT 0) = 1
    kOutPortByte(PIC_MASTER_PORT2, 0x01);

    /* =======================
    Init Slave PIC Controller
    ======================= */

    // ICW1 (Port 0xA0), IC4 (BIT 0) = 1
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);

    // ICW2 (Port 0xA1), Interrupt Vector(0x20 + 8)
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

    // ICW3(Port 0xA1), Connect Point of Master PIC(INT)
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);    // PIN2: 0x02(PIN 2)

    // ICW4(Port 0xA1), uPM (BIT 0) = 1
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

// Process interrupts so that they do not occur by masking interrupts
void kMaskPICInterrupt(WORD wIRQBitMask) {
    // Set IMR to Master PIC Controller
    kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitMask); // OCW1(Port 0x21), IRQ 0 ~ IRQ 7

    // Set IMR to Slave PIC Controller
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitMask >> 8)); // OCW1(Port 0xA1), IRQ 8 ~ IRQ 15
}

// Sent EOI(Complete proceed INTERRUPT)
void kSendEOIToPIC(int iTRQNumber) {
    // OCW2(Port 0x20), EOI (BIT 5) = 1
    kOutPortByte(PIC_MASTER_PORT1, 0x20);
    
    // IF INTERRUPT of Slave PIC -> Send EOI to Slave PIC Controller too.
    if(iTRQNumber >= 8) kOutPortByte(PIC_SLAVE_PORT1, 0x20);    // OCW OCW(Port 0xA0), EOI(BIT 5) = 1
}
#ifndef __PIC_H__
#define __PIC_H__

#include "Types.h"
#include "AssemblyUtility.h"

// DEFINE I/O PORT
#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0x21
#define PIC_SLAVE_PORT1 0xA0
#define PIC_SLAVE_PORT2 0xA1

// ENTRY POINT OF INTERRUPT VECTOR IN IDT TABLE
#define PIC_IRQSTARTVECTOR 0x20

void kInitializePIC(void);
void kMaskPICInterrupt(WORD wIRQBitmask);
void kSendEOIToPIC(int iTRQNumber);

#endif
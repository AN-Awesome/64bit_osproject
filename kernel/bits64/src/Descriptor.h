#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__
#include "Types.h"

// GDT :: MACRO(DEFAULT)
#define GDT_TYPE_CODE 0x0A
#define GDT_TYPE_DATA 0x02
#define GDT_TYPE_TSS 0x09
#define GDT_FLAGS_LOWER_S 0x10
#define GDT_FLAGS_LOWER_DPL0 0x00
#define GDT_FLAGS_LOWER_DPL1 0x20
#define GDT_FLAGS_LOWER_DPL2 0x40
#define GDT_FLAGS_LOWER_DPL3 0x60
#define GDT_FLAGS_LOWER_P 0x80
#define GDT_FLAGS_UPPER_L 0x20
#define GDT_FLAGS_UPPER_DB 0x40
#define GDT_FLAGS_UPPER_G 0x80

// GDT :: MACRO(BE USE)
// Lower Flags_SET - Code/Data/TSS, DPL0, Present
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

// Upper Flags set to Granulaty and code and data added 64 bits
#define GDT_FLAGS_UPPER_CODE (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS (GDT_FLAGS_UPPER_G)

// Segment descriptor offset
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT 0x18

// ANOTHER GDT MACRO
// Starting address of GDTR, page table area from 1 MB to 264 KB
#define GDTR_STARTADDRESS 0x142000

// Number of 8byte entries, null descriptor/kernel code/kernel data
#define GDT_MAXENTRY8COUNT 3

// Number of 16byte entries, TSS
#define GDT_MAXENTRY16COUNT 1

// Size of the GDT table
#define GDT_TABLESIZE ((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))

#define TSS_SEGMENTSIZE (sizeof(TSSSEGMENT))

// IDT :: MACRO(BE USE)
#define IDT_TYPE_INTERRUPT 0x0e
#define IDT_TYPE_TRAP 0x0f
#define IDT_FLAGS_DPL0 0x00
#define IDT_FLAGS_DPL1 0x20
#define IDT_FLAGS_DPL2 0X40
#define IDT_FLAGS_DPL3 0x60
#define IDT_FLAGS_P 0x80
#define IDT_FLAGS_ISTO 0
#define IDT_FLAGS_IST1 1

// IDT :: MACRO(BE USE)
#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// ANOTHER IDT MACRO
// Number of IDT entries
#define IDT_MAXENTRYCOUNT 100
// IDTR start address, located at the end of the TSS segment
#define IDTR_STARTADDRESS (GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)

// Start address of IDT table
#define IDT_STARTADDRESS (IDTR_STARTADDRESS + sizeof(IDTR))

// Total size of IDT table
#define IDT_TABLESIZE (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

// IST start address
#define IST_STARTADDRESS 0x700000

// SIZE OF IST
#define IST_SIZE 0x100000

// STRUCTURE
// ALIGNED BY 1-BYTE
#pragma pack (push, 1)

// Struct(GDTR, IDTR)
typedef struct kGDTRStruct {
    WORD wLimit;
    QWORD qwBaseAddress;
    
    // Addresses are aligned in 16-byte units
    WORD wPading;
    DWORD dwPading;
} GDTR, IDTR;

typedef struct kGDTEntry8Struct { // GDT ENTRY STRUCTURE OF 8-BYTES
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bUpperBaseAddress1;

    // 4bit Type, 1bit S, 2bit DPL, 1bit P
    BYTE bTypeAndLowerFlag;

    // 4bit Segment Limit, 1bit AVL, L, D/B, G
    BYTE bUpperLimitAndUpperFlag;
    BYTE bUpperBaseAddress2;
} GDTENTRY8;

typedef struct kGDTEntry16Struct { // GDT ENTRY STRUCTURE OF 16-BYTES

    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bMiddleBaseAddress1;

    // 4bit Type, 1bit 0, 2bit DPL, 1bit P
    BYTE bTypeAndLowerFlag; 

    // 4bit Segment Limit, 1bit AVL, 0, 0, G
    BYTE bUpperLimitAndUpperFlag;
    BYTE bMiddleBaseAddress2;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} GDTENTRY16;

typedef struct kTSSDataStruct { // STRUCTURE OF TSS DATA
    DWORD dwReserve1;
    QWORD qwRsp[3];
    QWORD qwReserved2;
    QWORD qwIST[7];
    QWORD qwReserved3;
    WORD wReserved;
    WORD wIOMapBaseAddress;
} TSSSEGMENT;

typedef struct kIDTEntryStruct { // STRUCTURE OF IDT GATE DESCRIPTOR
    WORD wLowerBaseAddress;
    WORD wSegmentSelector;

    // 3bit IST, 5bit 0
    BYTE bIST;

    // 4bit TYPE, 1bit 0, 2bit DPL, 1bit P
    BYTE bTypeAndFlags;
    WORD wMiddleBaseAddress;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} IDTENTRY;

#pragma pack(pop)

void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);

void kInitializeIDTTables(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD WSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);

#endif
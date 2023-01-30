#include "Page.h"

// Generating page tables for IA-32e mode kernels
void kInitializePageTables(void)
{
    PML4ENTRY* pstPML4Entry;
    PDPENTRY* pstPDPTEntry;
    PDENTRY* pstPDEntry;
    DWORD dwMappingAddress;
    int i;

    // Create a PML4 table :: Initialize everything except the first entry to 0
    pstPML4Entry = (PML4ENTRY*) 0x100000;
    kSetPageEntryData (&(pstPML4Entry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
    for(i = 1; i < PAGE_MAXENTRYCOUNT; i++) kSetPageEntryData(&(pstPML4Entry[i]), 0, 0, 0, 0);

    // Create page directory pointer table :: Up to 512GB can be mapped with one PDPT, so one is sufficient
    // *Set up 64 entries to map up to 64GB
    pstPDPTEntry = (PDPENTRY*)0x101000;
    for(i = 0; i < 64; i++) kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT, 0);
    for(i = 64; i < PAGE_MAXENTRYCOUNT; i++) kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);

    // Create page directory table(One page directory can map up to 1 GB)
    // *Generates up to 64 GB of page directories, up to 64 GB in total.
    pstPDEntry = (PDENTRY*) 0x102000;
    dwMappingAddress = 0;

    // The upper address cannot be expressed in the 32-bit system.
    // Addresses are calculated in units of MB, and the final result is divided by 4KB(12bit) to calculate addresses in the range of 32 bits or more.
    for (i = 0; i < PAGE_MAXENTRYCOUNT * 64 ; i++) {
        kSetPageEntryData(&(pstPDEntry[i]), (i*(PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

// Set base address and attribute flags on page entry
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags) {
    pstEntry -> dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry -> dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}
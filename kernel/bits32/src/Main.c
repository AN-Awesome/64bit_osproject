#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"
#include "TextColor.h"

void kPrintString(int iX, int iY, const char* pcString, int color);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);
void kCopyKernel64ImageTo2MB(void);

void Main(void){

	DWORD i;
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0,};

	//START CAPTION PRINTF
    // kPrintString(1, 8, "C Object Processing.", YELLOW);
    // kPrintString(35, 8, ">> COMPLETE <<", GREEN);

	// Checking if the current memory size is greater than the minimum requiRED_BR amount.
    // kPrintString(1, 9, "Minimum Memory Size Check.", BROWN);
    if(kIsMemoryEnough() == FALSE) {
        // kPrintString(35, 9, ">> FAIL <<", RED_BR);
        // kPrintString(1, 10, "Not Enough Memory. Require More Over 64mb Space.", RED_BR);
        while(1);   // jmp $
    }
    // else kPrintString(35, 9, ">> COMPLETE <<", GREEN);

	// IA-32e Kernel Area Init. Process
    // kPrintString(1, 10, "IA-32e Kernel Area Initialize.", BROWN);
    if(kInitializeKernel64Area() == FALSE) {
        // kPrintString(35, 10, ">> FAIL <<", RED_BR);
        // kPrintString(1, 11, "Kernel Init Error.", RED_BR);
        while(1);   // jmp $
    }
    // kPrintString(35, 10, ">> COMPLETE <<", GREEN);

	// Create Page Table for IA-32e Mode Kernel. 
    // kPrintString(1, 11, "IA-32e Kernel Page Tables Init.", BROWN);
    kInitializePageTables();
    // kPrintString(35, 11, ">> COMPLETE <<", GREEN);

	/// Check the manufacturer information of the processor.
    kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    *(DWORD*) vcVendorString = dwEBX;
    *((DWORD*) vcVendorString + 1) = dwEDX;
    *((DWORD*) vcVendorString + 2) = dwECX;
    // kPrintString(1, 13, "Processor Vendor String: ", BLUE_BR);
    // kPrintString(26, 13, vcVendorString, RED_BR);

	// Checks if the current processor can use a 64-bit system.
    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    // kPrintString(1, 14, "64Bit System IsSupoort: ", BLUE_BR);
    if (dwEDX & (1 << 29)) NULL; // kPrintString(25, 14, "Support", SKY);
    else {
        // kPrintString(40, 14, "NOT SUPPORT", RED_BR);
        // kPrintString(1, 15, "This Processor Does Not Support 64bit Mode", RED_BR);
        while(1); // jmp $
    }

	// kPrintString(1, 16, "Copy IA-32e Kernel To 2M Address.", BLUE_BR);
    kCopyKernel64ImageTo2MB();
    // kPrintString(35, 16, ">> COMPLETE <<", GREEN);

	// Switch Mode To IA-32e
    // kPrintString(1, 17, "Switch To IA-32e Mode.", BLUE_BR);
    kSwitchAndExecute64bitKernel();

	while(1);
}

void kPrintString(int iX, int iY, const char* pcString, int color) {
	CHARACTER* pstScreen = (CHARACTER*) 0xB8000;
	int i;

	pstScreen += (iY*80) +iX;
	for(i=0; pcString[i] != 0; i++) {
		pstScreen[i].bCharactor = pcString[i];
        pstScreen[i].bAttribute = color;
	}

}

//Initialize Kernel64 Area to 0
BOOL kInitializeKernel64Area(void) {
	DWORD* pdwCurrentAddress;

	// 1MB~6MB AREA
	pdwCurrentAddress = (DWORD*) 0x100000;
	while((DWORD) pdwCurrentAddress < 0x600000) {
		*pdwCurrentAddress = 0x00;
		// if Current Address is not 0x00, problem to read address
		if(*pdwCurrentAddress != 0) return FALSE;

		//Next address
		pdwCurrentAddress++;
	}

	return TRUE;
}

BOOL kIsMemoryEnough(void) {
	DWORD* pdwCurrentAddress;
	//Start 1Mbyte
	pdwCurrentAddress = (DWORD*) 0x100000;

	//Until 64Mbyte
	while((DWORD) pdwCurrentAddress < 0x4000000){
		*pdwCurrentAddress = 0xdeadbeef;

		// If Current Address is not 0xdeadbeef(can't read address), memory less than 64Mbyte
		if (*pdwCurrentAddress != 0xdeadbeef){
			return FALSE;
		}
		// next 1MB
		pdwCurrentAddress += 0x100000;
	}
	return TRUE;
}

void kCopyKernel64ImageTo2MB(void) {
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD* pdwSourceAddress, *pdwDestinationAddress;
	int i;

	wTotalKernelSectorCount = *((WORD*) 0x7c05);	// Total sector count
	wKernel32SectorCount = *((WORD*) 0x7c07);		// 32bit Kernel sector count

	pdwSourceAddress = (DWORD*) (0x10000 + (wKernel32SectorCount * 512));
	pdwDestinationAddress = (DWORD*) 0x200000;

	//64bit Kernel sector count = total - 32bit
	for(i=0; i < 512 * (wTotalKernelSectorCount - wKernel32SectorCount)/4; i++){
		*pdwDestinationAddress = *pdwSourceAddress;
		pdwDestinationAddress++;
		pdwSourceAddress++;
	}
}
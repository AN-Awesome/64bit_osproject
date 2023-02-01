#include "ConsoleEster.h"
#include "Console.h"

// {"PYT", "<< Show Our Team Member >>", KShowTeammember}

void KShowTeamMember(const char* pcCommandBuffer) {
    kSetColor(GREEN);
    kPrintf("\n ===============================\n");
    kPrintf(" === MASTER : KIM DONG HUI   ===\n");
    kPrintf(" === SLAVE1 : PARK YEON TAEK ===\n");
    kPrintf(" === SLAVE2 : JUNG HAE RAM   ===\n");
    kPrintf(" ===============================\n");
}
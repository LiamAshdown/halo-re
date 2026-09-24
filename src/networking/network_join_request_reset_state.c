// network_join_request_reset_state  (Ghidra: FUN_004e0ab0, unnamed)
// address 0x4e0ab0, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Calls two parameterless helper routines,
// likely resetting some channel/session state as part of the join-request handshake."
// register convention: none visible.

#include "tags.h"
#include "memory.h"

extern void FUN_004dd390(void); // other module (UNSURE)
extern void FUN_00575ff0(void); // other module (UNSURE)

void network_join_request_reset_state(void)
{
    FUN_004dd390();
    FUN_00575ff0();
}

#if 0
Original Ghidra decompilation (0x4e0ab0):

void FUN_004e0ab0(void)

{
  FUN_004dd390();
  FUN_00575ff0();
  return;
}
#endif

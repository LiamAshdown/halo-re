// master_server_list_refresh_request  (Ghidra: master_server_list_refresh_request, already named)
// address 0x4b6660, size 83 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("requests a server-list refresh from
// the master server by setting a pending-request flag and a 10-second timeout deadline").
// register convention: __cdecl, no arguments. QueryPerformanceCounter/__allmul/__alldiv folded
// into plain int64_t arithmetic, matching network_update.c's precedent for the same idiom.
// UNSURE: 0x006953fc (the computed deadline) and 0x00719488 (a flag set alongside it) have no
// documented names in networking_types_notes.md; declared here only by address.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint32_t master_server_request_flags; // 0x0071969c
extern int32_t DAT_006953fc; // see UNSURE, a millisecond deadline
extern uint8_t DAT_00719488; // see UNSURE
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module


// blam-cc: __cdecl, no arguments
void master_server_list_refresh_request(void)
{
    large_integer counter;
    int32_t now_ms;

    master_server_request_flags = master_server_request_flags | 8;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    DAT_006953fc = now_ms + 10000;
    DAT_00719488 = 1;
}

#if 0
Original Ghidra decompilation (0x4b6660):

void __cdecl master_server_list_refresh_request(void)

{
  int iVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  DAT_0071969c = DAT_0071969c | 8;
  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  DAT_006953fc = iVar1 + 10000;
  DAT_00719488 = 1;
  return;
}
#endif

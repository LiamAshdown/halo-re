// network_server_status_periodic_print  (Ghidra: FUN_004e1520, unnamed)
// address 0x4e1520, size 120 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Periodically (every ~15s) prints the
// dedicated server status by invoking sv_status when the relevant channel flag is active."
// param_1+6 matches network_server_globals::flags (stats-logging bit).
// register convention: stack = server (network_server_globals *).
// blam-cc: stack -> server
// UNSURE: DAT_0071c2f0 (the "last printed" timestamp) has no established name elsewhere.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int32_t network_server_status_last_print_ms; // 0x0071c2f0 (UNSURE name)
extern void sv_status(void); // 0x4e2e50, this batch

// While server's stats-logging flag is set, prints the dedicated server status roughly every
// 15000ms.
uint32_t network_server_status_periodic_print(network_server_globals *server)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    if ((server->flags >> 2 & 1) != 0) {
        int32_t now_ms;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if ((uint32_t)(now_ms - network_server_status_last_print_ms) > 15000) {
            sv_status();
            network_server_status_last_print_ms = now_ms;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e1520):

undefined4 FUN_004e1520(int param_1)

{
  int iVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  if ((*(byte *)(param_1 + 6) >> 2 & 1) != 0) {
    QueryPerformanceCounter(&local_8);
    uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
    if (15000 < (uint)(iVar1 - DAT_0071c2f0)) {
      sv_status();
      DAT_0071c2f0 = iVar1;
    }
  }
  return 1;
}
#endif

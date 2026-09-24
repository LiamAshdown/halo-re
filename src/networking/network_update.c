// network_update  (Ghidra: network_update, already named)
// address 0x4418d0, size 129 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("per-frame networking service
// routine: updates the high-resolution clock, drives the connection-statistics log, and pumps
// both network channels"); reuses network_game_socket/network_query_socket from
// networking_types_notes.md and performance_frequency from
// src/math/random_seed_generate.c (same QueryPerformanceCounter/__allmul/__alldiv shape,
// folded into plain int64_t arithmetic here for the same reason).
// register convention: __cdecl, no arguments.
// UNSURE: DAT_006869bf is not documented anywhere in networking_types_notes.md; named here
// only from its one-shot clear-if-set shape. FUN_00614540 (channel pump) and FUN_006154f0 are
// foreign GameSpy transport calls; the final `& 0xffff0000` return mask is preserved literally
// without a guess at its meaning.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_high_res_clock_ms;    // 0x006a6144, UNSURE: exact unit/consumer
extern uint8_t network_update_unknown_869bf; // 0x006869bf, UNSURE
extern int32_t network_game_socket;          // 0x006f14c4
extern int32_t network_query_socket;         // 0x006f14c8
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module

extern void network_connection_stats_log_tick(void); // 0x440d80, this module
extern void FUN_00614540(int32_t socket); // foreign GameSpy transport call, "pump" a channel
extern uint32_t FUN_006154f0(void);       // foreign GameSpy transport call
extern int32_t QueryPerformanceCounter(large_integer *counter);

uint32_t network_update(void)
{
    large_integer counter;
    uint32_t result;

    QueryPerformanceCounter(&counter);
    network_high_res_clock_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    if (network_update_unknown_869bf == 1) {
        network_update_unknown_869bf = 0;
    }
    network_connection_stats_log_tick();
    if (network_game_socket != 0) {
        FUN_00614540(network_game_socket);
    }
    if (network_query_socket != 0) {
        FUN_00614540(network_query_socket);
    }
    result = FUN_006154f0();
    return result & 0xffff0000;
}

#if 0
Original Ghidra decompilation (0x4418d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint network_update(void)

{
  uint uVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_006a6144 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  if (DAT_006869bf == '\x01') {
    DAT_006869bf = '\0';
  }
  network_connection_stats_log_tick();
  if (DAT_006f14c4 != 0) {
    FUN_00614540(DAT_006f14c4);
  }
  if (DAT_006f14c8 != 0) {
    FUN_00614540(DAT_006f14c8);
  }
  uVar1 = FUN_006154f0();
  return uVar1 & 0xffff0000;
}
#endif

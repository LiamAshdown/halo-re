// network_game_search_entry_is_fresh  (Ghidra: FUN_004da770; renamed, no prior name)
// address 0x4da770, size 94 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Tests whether a single entry in the
// 9-slot game-search/pending-connection record table is still within its ~6 second freshness
// window"); entry+0x12d and entry+0x18 match types/networking.h's
// network_game_search_entry::in_use and ::received_ms exactly, and 0x1771 (6001 ms) matches
// k_network_game_search_expiry_ms (6000) plus one.
// register convention: the entry pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> entry
// UNSURE: the real return value is `CONCAT31(garbage, result_byte)` in Ghidra (garbage from an
// uninitialized `in_EAX` on the not-in-use path, or from the elapsed-time computation on the
// stale path); simplified to a plain 0/1 return, matching the "callers only read the low byte"
// idiom used throughout this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

// blam-cc: ESI -> entry
int32_t network_game_search_entry_is_fresh(network_game_search_entry *entry)
{
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed_ms;

    if (entry->in_use == 0) {
        return 0;
    }
    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed_ms = now_ms - entry->received_ms;
    if (elapsed_ms < 0x1771) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4da770):

uint FUN_004da770(void)

{
  undefined4 in_EAX;
  int iVar1;
  uint uVar2;
  int unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  uVar2 = CONCAT31((int3)((uint)in_EAX >> 8),*(char *)(unaff_ESI + 0x12d));
  if (*(char *)(unaff_ESI + 0x12d) != '\0') {
    QueryPerformanceCounter(&local_8);
    uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    uVar2 = iVar1 - *(int *)(unaff_ESI + 0x18);
    if ((int)uVar2 < 0x1771) {
      return CONCAT31((int3)(uVar2 >> 8),1);
    }
  }
  return uVar2 & 0xffffff00;
}
#endif

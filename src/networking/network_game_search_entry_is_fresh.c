// network_game_search_entry_is_fresh  (Ghidra: FUN_004da770; renamed, no prior name)
// address 0x4da770, size 94 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Tests whether a single entry in the
// 9-slot game-search/pending-connection record table is still within its ~6 second freshness
// window"); entry+0x12d and entry+0x18 match types/networking.h's
// network_game_search_entry::in_use and ::received_ms exactly, and 0x1771 (6001 ms) matches
// k_network_game_search_expiry_ms (6000) plus one.
// register convention: the entry pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> entry
// Return: only AL is defined (0 or 1; the upper EAX bytes are scratch), so a plain 0/1 uint8_t
// return is equivalent.

// VERIFIED against disassembly 0x4da770..0x4da7ce (2026-09-30): in_use at +0x12d, QPC*1000/freq via _allmul/_alldiv, elapsed (signed) <= 0x1770; AL result only
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

// blam-cc: ESI -> entry
uint8_t network_game_search_entry_is_fresh(network_game_search_entry *entry)
{
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed_ms;

    if (entry->in_use == 0) {
        return 0;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

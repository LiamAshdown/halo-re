// game_engine_ctf_pick_random_flag  (Ghidra: game_engine_ctf_pick_random_flag, already named)
// address 0x46dfe0, size 160 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/game.h ctf_globals::flag_id_mask (0x006b1290, this batch's
//   game_engine_ctf_initialize_flags.c); types/tags.h ScenarioNetgameFlags (type +0x10,
//   usage_id +0x12); random_seed_global LCG already used identically elsewhere in this batch.
// register convention: exclude id is the function's own stack parameter (__cdecl).

// CORRECTED (phase 4 review): the Blam random-index idiom is
//   movsx ecx,<count> ; shr eax,0x10 ; imul eax,ecx ; shr eax,0x10 ; movsx <idx>,ax
// so the seed's high half is used ZERO-extended (shr, no movsx) and the product is shifted
// down logically. Casting (seed >> 16) to int16_t first, as this file did, makes the index
// negative for half of all seeds, which silently disables the pick.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ctf_globals ctf_globals_live; // 0x006b1290
extern Scenario *global_scenario;    // 0x00746f8c
extern random_seed random_seed_global;  // 0x00719cd0

// Randomly selects one active flag usage id (from flag_id_mask), excluding `exclude_flag_index`
// if it is not -1, and returns the matching ScenarioNetgameFlags entry's own usage_id (which is
// normally the same value, since exactly one entry claims each id) -- or -1 if none qualify.
int32_t game_engine_ctf_pick_random_flag(int32_t exclude_flag_index)
{
    int16_t active_count = 0;
    int32_t i;
    int32_t pick;
    int32_t flag_count;
    ScenarioNetgameFlags *flags;

    for (i = 0; i < 0x20; i++) {
        if ((ctf_globals_live.flag_id_mask & (1u << (i & 0x1f))) != 0) {
            active_count++;
        }
    }
    if (exclude_flag_index != -1) {
        active_count--;
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    pick = (int16_t)(((random_seed_global >> 0x10) *
                      (uint32_t)(int32_t)(int16_t)active_count) >> 0x10);

    flag_count = (int32_t)global_scenario->netgame_flags.count;
    if (flag_count < 1) {
        return -1;
    }
    flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

    for (i = 0; i < flag_count; i++) {
        if (flags[i].type == 3 && flags[i].usage_id != exclude_flag_index) {
            if (pick == 0) {
                return flags[i].usage_id;
            }
            pick--;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x46dfe0), from tools/pack.py 0x46dfe0:

int __cdecl game_engine_ctf_pick_random_flag(int exclude_flag_index)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  sVar1 = 0;
  iVar2 = 0;
  do {
    if ((DAT_006b1290 & 1 << ((byte)iVar2 & 0x1f)) != 0) {
      sVar1 = sVar1 + 1;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x20);
  if (exclude_flag_index != -1) {
    sVar1 = sVar1 + -1;
  }
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  iVar4 = (int)(short)((random_seed_global >> 0x10) * (int)sVar1 >> 0x10);
  iVar2 = 0;
  if (*(int *)(global_scenario + 0x378) < 1) {
    return -1;
  }
  iVar3 = *(int *)(global_scenario + 0x37c);
  do {
    if ((*(short *)(iVar3 + 0x10) == 3) && (*(short *)(iVar3 + 0x12) != exclude_flag_index)) {
      if (iVar4 == 0) {
        return (int)*(short *)(iVar3 + 0x12);
      }
      iVar4 = iVar4 + -1;
    }
    iVar2 = iVar2 + 1;
    iVar3 = iVar3 + 0x94;
  } while (iVar2 < *(int *)(global_scenario + 0x378));
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

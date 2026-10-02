// team_pair_override_remove  (Ghidra: FUN_0045bf10; renamed per symbols/review_queue.txt)
// address 0x45bf10, size 162 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED against objdump 0x45bf10..0x45bfab; FIXED: team_pair_set gets BL = 1, stack 1 (break the alliance) -- the draft passed (0, 1), which re-allied the teams)
// evidence: types/game.h team_pair_override / team_pair_globals::override_count; the trailing
//   4-dword-plus-word copy is exactly sizeof(team_pair_override) (0x12 bytes), i.e. compacting
//   the list by moving the last entry into the removed slot.
// register convention: first index in EAX (in_AX); the second index is the recognized stack
//   parameter (param_1).
//   // blam-cc: EAX -> index_a, stack -> index_b
//
// UNSURE: `team_pair_set(entry, 0, 1)` reconstructs FUN_0045c130(1)'s elided EAX/EBX arguments;
// `entry` is the matched override (confident) and `active` = 0 is a best-effort guess
// (deactivating the pair being removed), not independently verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern team_pair_globals *team_pair_data; // 0x006b0b84
extern void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary); // this batch, 0x45c130

// Finds the override matching (index_a, index_b) in either order, clears its bit via
// team_pair_set, then compacts the list by moving the last entry into the freed slot. Returns 0
// if no matching entry was found.
uint32_t team_pair_override_remove(int16_t index_a, int16_t index_b)
    // blam-cc: EAX -> index_a, stack -> index_b
{
    int16_t i;
    team_pair_override *entry;

    if (team_pair_data->override_count < 1) {
        return 0;
    }
    for (i = 0; ; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if ((entry->index_a == index_b && entry->index_b == index_a) ||
            (entry->index_b == index_b && entry->index_a == index_a)) {
            break;
        }
        if (team_pair_data->override_count <= i + 1) {
            return 0;
        }
    }

    team_pair_set(entry, 1, 1); // 0x45bf59..0x45bf5f: BL = 1 (break the alliance), stack 1 (clear the secondary bits)
    team_pair_data->override_count = team_pair_data->override_count - 1;
    if (i < team_pair_data->override_count) {
        *entry = team_pair_data->overrides[team_pair_data->override_count];
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45bf10), from tools/pack.py 0x45bf10:

undefined4 FUN_0045bf10(short param_1)

{
  short *psVar1;
  short in_AX;
  short *psVar2;
  short sVar3;

  psVar1 = DAT_006b0b84;
  psVar2 = DAT_006b0b84 + 1;
  if (*DAT_006b0b84 < 1) {
    return 0;
  }
  sVar3 = 0;
  while( true ) {
    if (((*psVar2 == param_1) && (psVar2[1] == in_AX)) ||
       ((psVar2[1] == param_1 && (*psVar2 == in_AX)))) break;
    sVar3 = sVar3 + 1;
    psVar2 = psVar2 + 9;
    if (*DAT_006b0b84 <= sVar3) {
      return 0;
    }
  }
  FUN_0045c130(1);
  *psVar1 = *psVar1 + -1;
  if (sVar3 < *psVar1) {
    psVar2 = psVar1 + *psVar1 * 9 + 1;
    psVar1 = psVar1 + sVar3 * 9 + 1;
    *(undefined4 *)psVar1 = *(undefined4 *)psVar2;
    *(undefined4 *)(psVar1 + 2) = *(undefined4 *)(psVar2 + 2);
    *(undefined4 *)(psVar1 + 4) = *(undefined4 *)(psVar2 + 4);
    *(undefined4 *)(psVar1 + 6) = *(undefined4 *)(psVar2 + 6);
    psVar1[8] = psVar2[8];
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

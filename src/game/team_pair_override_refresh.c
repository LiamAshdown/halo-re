// team_pair_override_refresh  (Ghidra: FUN_0045c090; renamed per symbols/review_queue.txt)
// address 0x45c090, size 92 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x45c090..0x45c0eb (EBX index_b, EDI index_a).)
// evidence: types/game.h team_pair_override (unknown_09 0x09, unknown_08 0x08, refcount 0x0e,
//   timer_reset 0x06, timer 0x10); same directional-match pattern as
//   team_pair_override_adjust_counter.c (0x45bfc0).
// register convention: both indices are implicit registers (unaff_BX, unaff_DI); per the
//   project's EAX/ECX/EDX/EBX/ESI/EDI ordering, EBX is read first.
//   // blam-cc: EBX -> index_b, EDI -> index_a

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern team_pair_globals *team_pair_data; // 0x006b0b84

// Finds the directional override entry matching (index_a, index_b) (same matching rule as
// team_pair_override_adjust_counter) and, if it still has a positive refcount, resets its
// countdown timer back to timer_reset.
void team_pair_override_refresh(int16_t index_b, int16_t index_a)
    // blam-cc: EBX -> index_b, EDI -> index_a
{
    int16_t i;
    team_pair_override *entry;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b && entry->unknown_09 != 0) ||
                (entry->index_b == index_a && entry->index_a == index_b && entry->unknown_08 != 0)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return;
            }
        }
        if (0 < entry->refcount && entry->timer_reset != -1) {
            entry->timer = entry->timer_reset;
        }
    }
}

#if 0
Original Ghidra decompilation (0x45c090), from tools/pack.py 0x45c090:

void FUN_0045c090(void)

{
  short *psVar1;
  short unaff_BX;
  short sVar2;
  short unaff_DI;

  psVar1 = DAT_006b0b84 + 1;
  sVar2 = 0;
  if (0 < *DAT_006b0b84) {
    while ((((*psVar1 != unaff_DI || (psVar1[1] != unaff_BX)) ||
            (*(char *)((int)psVar1 + 9) == '\0')) &&
           (((psVar1[1] != unaff_DI || (*psVar1 != unaff_BX)) || ((char)psVar1[4] == '\0'))))) {
      sVar2 = sVar2 + 1;
      psVar1 = psVar1 + 9;
      if (*DAT_006b0b84 <= sVar2) {
        return;
      }
    }
    if ((0 < psVar1[7]) && (psVar1[3] != -1)) {
      psVar1[8] = psVar1[3];
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

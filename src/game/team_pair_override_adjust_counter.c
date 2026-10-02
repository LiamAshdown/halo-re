// team_pair_override_adjust_counter  (Ghidra: FUN_0045bfc0; renamed per symbols/review_queue.txt)
// address 0x45bfc0, size 198 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED against objdump 0x45bfc0..0x45c085 (+1/+3/-1 by selector, timer refresh, threshold -> team_pair_set BL 1, out flag = !+0x0c))
// evidence: types/game.h team_pair_override (unknown_09 0x09, unknown_08 0x08, refcount 0x0e,
//   timer_reset 0x06, timer 0x10, threshold 0x04, unknown_0c 0x0c).
// register convention: first index in EAX (in_AX); the rest are the recognized stack
//   parameters.
//   // blam-cc: EAX -> index_a, stack -> index_b, delta_selector, out_flag
//
// UNSURE: `team_pair_set(entry, 1, 0)` reconstructs FUN_0045c130(0)'s elided EAX/EBX arguments,
// following the same reasoning as team_pair_override_add.c (the entry stays active once its
// threshold is reached, so `active` = 1 is a best-effort guess, not independently verified).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern team_pair_globals *team_pair_data; // 0x006b0b84
extern void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary); // this batch, 0x45c130

// Finds the directional override entry matching (index_a, index_b) -- the (a,b) order requires
// unknown_09, the (b,a) order requires unknown_08 -- and adjusts its refcount by +1, +3 or -1
// per `delta_selector` (0, 1, 2). Refreshes the countdown timer from timer_reset, and once
// refcount reaches the entry's threshold, activates it via team_pair_set and optionally reports
// whether unknown_0c was zero through `out_flag`.
uint32_t team_pair_override_adjust_counter(int16_t index_a, int16_t index_b, int16_t delta_selector, uint8_t *out_flag)
    // blam-cc: EAX -> index_a, stack -> index_b, delta_selector, out_flag
{
    int16_t i;
    team_pair_override *entry;
    int16_t delta;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b && entry->unknown_09 != 0) ||
                (entry->index_b == index_a && entry->index_a == index_b && entry->unknown_08 != 0)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return 0;
            }
        }

        delta = 0;
        if (delta_selector == 0) {
            delta = 1;
        } else if (delta_selector == 1) {
            delta = 3;
        } else if (delta_selector == 2) {
            delta = -1;
        }
        entry->refcount = entry->refcount + delta;
        if (entry->timer_reset != -1) {
            entry->timer = entry->timer_reset;
        }
        if (entry->threshold != -1 && entry->threshold <= entry->refcount) {
            team_pair_set(entry, 1, 0); // UNSURE: see header
            if (out_flag != (uint8_t *)0) {
                *out_flag = entry->unknown_0c == 0;
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45bfc0), from tools/pack.py 0x45bfc0:

undefined4 FUN_0045bfc0(short param_1,short param_2,int param_3)

{
  short in_AX;
  undefined4 uVar1;
  short sVar2;
  short *psVar3;

  psVar3 = DAT_006b0b84 + 1;
  uVar1 = 0;
  sVar2 = 0;
  if (0 < *DAT_006b0b84) {
    while ((((*psVar3 != in_AX || (psVar3[1] != param_1)) || (*(char *)((int)psVar3 + 9) == '\0'))
           && (((psVar3[1] != in_AX || (*psVar3 != param_1)) || ((char)psVar3[4] == '\0'))))) {
      sVar2 = sVar2 + 1;
      psVar3 = psVar3 + 9;
      if (*DAT_006b0b84 <= sVar2) {
        return uVar1;
      }
    }
    sVar2 = 0;
    if (param_2 == 0) {
      sVar2 = 1;
    }
    else if (param_2 == 1) {
      sVar2 = 3;
    }
    else if (param_2 == 2) {
      sVar2 = -1;
    }
    psVar3[7] = psVar3[7] + sVar2;
    if (psVar3[3] != -1) {
      psVar3[8] = psVar3[3];
    }
    if ((psVar3[2] != -1) && (psVar3[2] <= psVar3[7])) {
      FUN_0045c130(0);
      uVar1 = 1;
      if (param_3 != 0) {
        *(bool *)param_3 = (char)psVar3[6] == '\0';
      }
    }
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// team_pair_override_clear_flag  (Ghidra: FUN_0045c0f0; renamed per symbols/review_queue.txt)
// address 0x45c0f0, size 62 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: types/game.h team_pair_override::status (0x0b), the same byte
// team_pair_override_get_flag (0x459be00 [sic, 0x45be00]) reads.
// register convention: both indices are implicit registers (unaff_BX, unaff_DI); EBX read
//   first per the project's convention.
//   // blam-cc: EBX -> index_b, EDI -> index_a

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern team_pair_globals *team_pair_data; // 0x006b0b84

// Finds an override matching (index_a, index_b) in either order and clears its status byte.
void team_pair_override_clear_flag(int16_t index_b, int16_t index_a)
    // blam-cc: EBX -> index_b, EDI -> index_a
{
    int16_t i;
    team_pair_override *entry;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b) ||
                (entry->index_b == index_a && entry->index_a == index_b)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return;
            }
        }
        entry->status = 0;
    }
}

#if 0
Original Ghidra decompilation (0x45c0f0), from tools/pack.py 0x45c0f0:

void FUN_0045c0f0(void)

{
  short *psVar1;
  short sVar2;
  short unaff_BX;
  short unaff_DI;

  psVar1 = DAT_006b0b84 + 1;
  sVar2 = 0;
  if (0 < *DAT_006b0b84) {
    while (((*psVar1 != unaff_DI || (psVar1[1] != unaff_BX)) &&
           ((psVar1[1] != unaff_DI || (*psVar1 != unaff_BX))))) {
      sVar2 = sVar2 + 1;
      psVar1 = psVar1 + 9;
      if (*DAT_006b0b84 <= sVar2) {
        return;
      }
    }
    *(undefined1 *)((int)psVar1 + 0xb) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

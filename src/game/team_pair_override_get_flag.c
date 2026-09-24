// team_pair_override_get_flag  (Ghidra: FUN_0045be00; renamed per symbols/review_queue.txt)
// address 0x45be00, size 77 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: types/game.h team_pair_override (index_a 0x00, index_b 0x02, status 0x0b), size 0x12
//   (9 shorts), matching the stride here.
// register convention: first index in EBX (unaff_BX); the second index is the recognized stack
//   parameter (param_1).
//   // blam-cc: EBX -> index_a, stack -> index_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern team_pair_globals *team_pair_data; // 0x006b0b84

// Scans the override list for a pair (either index order) and returns its status byte, or 0 if
// no matching entry exists.
uint8_t team_pair_override_get_flag(int16_t index_a, int16_t index_b)
    // blam-cc: EBX -> index_a, stack -> index_b
{
    int16_t i;

    if (0 < team_pair_data->override_count) {
        for (i = 0; i < team_pair_data->override_count; i = i + 1) {
            team_pair_override *entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b) ||
                (entry->index_b == index_a && entry->index_a == index_b)) {
                return entry->status;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45be00), from tools/pack.py 0x45be00:

undefined1 FUN_0045be00(short param_1)

{
  undefined1 uVar1;
  short *psVar2;
  short unaff_BX;
  short sVar3;

  psVar2 = DAT_006b0b84 + 1;
  uVar1 = 0;
  sVar3 = 0;
  if (0 < *DAT_006b0b84) {
    while (((*psVar2 != unaff_BX || (psVar2[1] != param_1)) &&
           ((psVar2[1] != unaff_BX || (*psVar2 != param_1))))) {
      sVar3 = sVar3 + 1;
      psVar2 = psVar2 + 9;
      if (*DAT_006b0b84 <= sVar3) {
        return uVar1;
      }
    }
    uVar1 = *(undefined1 *)((int)psVar2 + 0xb);
  }
  return uVar1;
}
#endif

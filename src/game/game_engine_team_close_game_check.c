// game_engine_team_close_game_check  (Ghidra: FUN_00470790; renamed, no established name)
// address 0x470790, size 114 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Checks whether the score margin between two sides is
// close enough (within about 20%) to warrant a close-game notification"); the shared
// game_engine_gather_team_score_totals.c out-array layout (out_count[0..1], out_score[0..1]).
// register convention: `side` (0 or 1) arrives in EBX (Ghidra's `unaff_EBX`); the stack parameter
// is forwarded unchanged to game_engine_gather_team_score_totals as `filter_value`. `in_EAX` is
// only ever used to clear its own low byte for the "not 0/1" early-out and is not modeled as a
// real input.
//   // blam-cc: EBX -> side, stack -> filter_value
// UNSURE: this function's return value is not a plain boolean -- Ghidra's CONCAT22 packs
// `fVar1 < fVar2`, an unordered-compare bit and `fVar1 == fVar2` into bits 8/10/14 of the high
// 16 bits (an x87-status-word-shaped result), which no caller in this batch consumes; it is
// transcribed literally rather than simplified to a bool. The `(a < b) == (a == b)` idiom
// elsewhere in this module (see game_engine_tick.c) reduces to `a > b`; applied here that would
// make the guarded `return uVar3` path equivalent to `other_margin > allowed_margin`, but the
// packed return value itself is kept as-is since nothing here proves it is dead.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern void game_engine_gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2],
    int32_t filter_value); // this batch, 0x470690

// blam-cc: EBX -> side, stack -> filter_value
// For `side` 0 or 1, gathers the team totals and, if the other side's score is still behind
// `side`'s, checks whether the other side's match-count margin over `side` exceeds 20% of the
// other side's own count; returns a packed comparison-flags word if so (see UNSURE above),
// otherwise 1.
uint8_t game_engine_team_close_game_check(int32_t side, int32_t filter_value)
{
    uint32_t out_count[2];
    uint32_t out_score[2];
    int32_t other_side = 1 - side;

    if (side != 0 && side != 1) {
        return 0;
    }

    game_engine_gather_team_score_totals(out_count, out_score, filter_value);

    if ((int32_t)out_score[other_side] < (int32_t)out_score[side]) {
        float count_margin = (float)((int32_t)out_count[other_side] - (int32_t)out_count[side]);
        float allowed_margin = (float)(int32_t)out_count[other_side] * 0.2f;

        if (allowed_margin < count_margin) {
            uint16_t flags = (uint16_t)(((allowed_margin < count_margin) << 8) |
                (0 << 10) | ((allowed_margin == count_margin) << 14)); // UNSURE: NAN-bit dropped, see header
            // CORRECTED (phase 4 review): the only caller tests AL alone
            // (objdump 0x47096f "test al,al"), so the packed CONCAT22 word Ghidra shows is
            // dead above the low byte and the return type is a bool.
            return (uint8_t)flags;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x470790), from tools/pack.py 0x470790:

uint FUN_00470790(undefined4 param_1)

{
  float fVar1;
  float fVar2;
  uint in_EAX;
  uint uVar3;
  int unaff_EBX;
  uint local_10 [4];

  uVar3 = in_EAX & 0xffffff00;
  if ((unaff_EBX == 0) || (unaff_EBX == 1)) {
    FUN_00470690(local_10,param_1);
    uVar3 = local_10[-unaff_EBX + 3];
    if ((int)uVar3 < (int)local_10[unaff_EBX + 2]) {
      fVar2 = (float)(int)(local_10[-unaff_EBX + 1] - local_10[unaff_EBX]);
      fVar1 = (float)(int)local_10[-unaff_EBX + 1] * 0.2;
      uVar3 = CONCAT22((short)(local_10[unaff_EBX] >> 0x10),
                       (ushort)(fVar1 < fVar2) << 8 | (ushort)(NAN(fVar1) || NAN(fVar2)) << 10 |
                       (ushort)(fVar1 == fVar2) << 0xe);
      if (fVar1 < fVar2 == (fVar1 == fVar2)) {
        return uVar3;
      }
    }
    uVar3 = CONCAT31((int3)(uVar3 >> 8),1);
  }
  return uVar3;
}
#endif

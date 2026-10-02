// team_pair_flag_test  (Ghidra: FUN_0045bdb0; renamed per symbols/review_queue.txt)
// address 0x45bdb0, size 68 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: types/game.h team_pair_globals::secondary_bits (0x94); mirrors teams_are_enemies
//   (0x45bd50) but reads the OTHER bitmask and does not invert the result, and indexes with the
//   pair reversed (team_b + team_a*10 instead of team_a + team_b*10).
// register convention: both team indices are the recognized register parameters (in_CX, in_DX).
//   // blam-cc: ECX -> team_a, EDX -> team_b
//
// UNSURE: the true return width is a single bit in AL; Ghidra's `uint` return also carries
// leftover high bits of `in_EAX` on both paths (`in_EAX & 0xffffff00` on the out-of-range path,
// a 3-byte shift remnant on the in-range path). Declared uint8_t below, matching how the two
// known callers (0x461230-ish region, outside this batch) most likely consume it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern team_pair_globals *team_pair_data; // 0x006b0b84

// Tests the secondary per-pair flag bit (distinct from the enemy_bits bitmask) for a pair of
// 0-9 team indices.
uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b)
    // blam-cc: ECX -> team_a, EDX -> team_b
{
    int32_t index;

    if (-1 < team_a && team_a < 10 && -1 < team_b && team_b < 10) {
        index = (int32_t)team_b + team_a * 10;
        return (team_pair_data->secondary_bits[index >> 5] & (1u << (index & 0x1f))) != 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45bdb0), from tools/pack.py 0x45bdb0:

uint FUN_0045bdb0(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  short in_CX;
  short in_DX;

  uVar2 = in_EAX & 0xffffff00;
  if ((((-1 < in_CX) && (in_CX < 10)) && (-1 < in_DX)) && (in_DX < 10)) {
    iVar1 = (int)in_DX + in_CX * 10;
    uVar2 = CONCAT31((int3)(iVar1 >> 0xd),
                     (*(uint *)(DAT_006b0b84 + 0x94 + (iVar1 >> 5) * 4) & 1 << ((byte)iVar1 & 0x1f))
                     != 0);
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

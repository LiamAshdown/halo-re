// team_pair_table_init_defaults  (Ghidra: FUN_0045bc80; renamed per symbols/review_queue.txt)
// address 0x45bc80, size 101 bytes
// name confidence: 0.35   rewrite confidence: 0.75
// evidence: types/game.h team_pair_globals (override_count 0x00, secondary_bits 0x94,
//   enemy_bits 0xa4); the two 4-dword zero writes land exactly on secondary_bits and enemy_bits,
//   and the following loop sets the diagonal (index == index) bit of every one of the 10
//   team indices in enemy_bits, matching teams_are_enemies (0x45bd50)'s own indexing formula
//   (a + b*10).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern team_pair_globals *team_pair_data; // 0x006b0b84

// Clears the override list and both relationship bitmasks, then marks every team index as
// related to itself (the reflexive default) in enemy_bits.
void team_pair_table_init_defaults(void)
{
    int32_t i;
    int32_t index;

    team_pair_data->override_count = 0;
    team_pair_data->secondary_bits[0] = 0;
    team_pair_data->secondary_bits[1] = 0;
    team_pair_data->secondary_bits[2] = 0;
    team_pair_data->secondary_bits[3] = 0;
    team_pair_data->enemy_bits[0] = 0;
    team_pair_data->enemy_bits[1] = 0;
    team_pair_data->enemy_bits[2] = 0;
    team_pair_data->enemy_bits[3] = 0;

    index = 0;
    for (i = 10; i != 0; i = i - 1) {
        team_pair_data->enemy_bits[index >> 5] |= 1u << (index & 0x1f);
        index = index + 0xb;
    }
}

#if 0
Original Ghidra decompilation (0x45bc80), from tools/pack.py 0x45bc80:

void FUN_0045bc80(void)

{
  byte bVar1;
  undefined2 *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;

  puVar2 = DAT_006b0b84;
  *DAT_006b0b84 = 0;
  *(undefined4 *)(puVar2 + 0x4a) = 0;
  *(undefined4 *)(puVar2 + 0x4c) = 0;
  *(undefined4 *)(puVar2 + 0x4e) = 0;
  *(undefined4 *)(puVar2 + 0x50) = 0;
  *(undefined4 *)(puVar2 + 0x52) = 0;
  *(undefined4 *)(puVar2 + 0x54) = 0;
  *(undefined4 *)(puVar2 + 0x56) = 0;
  *(undefined4 *)(puVar2 + 0x58) = 0;
  iVar3 = 0;
  iVar5 = 10;
  do {
    iVar4 = iVar3 >> 5;
    bVar1 = (byte)iVar3;
    iVar3 = iVar3 + 0xb;
    iVar5 = iVar5 + -1;
    *(uint *)(puVar2 + iVar4 * 2 + 0x52) =
         *(uint *)(puVar2 + iVar4 * 2 + 0x52) | 1 << (bVar1 & 0x1f);
  } while (iVar5 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

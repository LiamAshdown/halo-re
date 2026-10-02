// game_engine_reset_player_profile_stats  (Ghidra: FUN_00468150; named per this rewrite)
// address 0x468150, size 67 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/game.h player_profile (0x30 bytes, in_use at 0x00, player handle at 0x04,
//   16 of them at 0x006b0b88 through 0x006b0e88); player_data (0x0087a480, stride 0x200). The
//   loop walks the cache with an 0x30 (sizeof(player_profile)) stride testing in_use, then
//   zeroes 15 dwords of the referenced player starting at +0x90 -- the run
//   killing_spree_count(0x96)..unknown_c8(0xc8) plus its immediate neighbours (the tail of
//   unknown_8d, unknown_b4, unknown_b8..bf), i.e. essentially the whole per-round statistics
//   block.
// register convention: no parameters.
// UNSURE: the zeroed range (player + 0x90 .. + 0xcc) does not line up on a single named field's
//   start, so it is written here as a raw dword run rather than through individual field names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;                 // 0x0087a480
extern player_profile player_profile_cache[16]; // 0x006b0b88

// For every player_profile still marked in_use in the 16-slot cache, zeroes 15 dwords
// (player + 0x90 .. + 0xcc) of statistics on that profile's player.
void game_engine_reset_player_profile_stats(void)
{
    int32_t i;

    for (i = 0; i < 16; i++) {
        if (player_profile_cache[i].in_use == 1) {
            uint32_t player_index = (uint32_t)player_profile_cache[i].player & 0xffff;
            uint32_t *stats = (uint32_t *)((uint8_t *)player_data->data + player_index * 0x200 + 0x90);
            int32_t j;
            for (j = 0; j < 15; j++) {
                stats[j] = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x468150), from tools/pack.py 0x468150:

void FUN_00468150(void)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  undefined4 *puVar4;

  iVar1 = DAT_0087a480;
  puVar3 = &DAT_006b0b8c;
  do {
    if ((char)puVar3[-1] == '\x01') {
      puVar4 = (undefined4 *)((*puVar3 & 0xffff) * 0x200 + 0x90 + *(int *)(iVar1 + 0x34));
      for (iVar2 = 0xf; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = 0;
        puVar4 = puVar4 + 1;
      }
    }
    puVar3 = puVar3 + 0xc;
  } while ((int)puVar3 < 0x6b0e8c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

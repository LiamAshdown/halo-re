// game_engine_build_scoreboard_sort_key  (Ghidra: FUN_0045cc30; the exact address
// types/game.h's scoreboard_entry note cites as "the key builder")
// address 0x45cc30, size 87 bytes
// name confidence: 0.55 (still FUN_0045cc30 in Ghidra, but named directly from
//   types/game.h's own citation: "The key builder is 0x45cc30 (score clamped at -1000, biased
//   +1000, plus bit 0x40000000 for 'still has lives' and 0x20000000 for 'not marked for
//   deletion')")
// rewrite confidence: 0.7
// evidence: types/game.h player (deaths 0xae, marked_for_deletion 0xd5),
//   game_variant::lives_per_round (live copy at 0x006f1cd8); data_array player_data
//   (0x0087a480).
// register convention: player index in ECX (in_ECX), raw score in EDX (in_EDX).
//   // blam-cc: ECX -> player_index, EDX -> score

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;              // 0x0087a480
extern game_variant game_engine_variant;    // 0x006f1c88 (::lives_per_round at 0x006f1cd8)

// Builds a packed scoreboard_entry::key_N value: bit 0x40000000 when the player still has
// lives left (deaths < lives_per_round), bit 0x20000000 when it is not marked for deletion,
// and the low bits a score clamped to >= -1000 and biased by +1000.
uint32_t game_engine_build_scoreboard_sort_key(uint32_t player_index, int32_t score)
    // blam-cc: ECX -> player_index, EDX -> score
{
    player *p;
    uint32_t flags;

    flags = 0;
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    if (score < -1000) {
        score = -1000;
    }
    if (p->deaths < game_engine_variant.lives_per_round) {
        flags = 0x40000000;
    }
    if (p->marked_for_deletion == 0) {
        flags = flags | 0x20000000;
    }
    return flags | (uint32_t)(score + 1000);
}

#if 0
Original Ghidra decompilation (0x45cc30), from tools/pack.py 0x45cc30:

uint FUN_0045cc30(void)

{
  uint uVar1;
  uint in_ECX;
  int iVar2;
  int in_EDX;

  uVar1 = 0;
  iVar2 = (in_ECX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if (in_EDX < -1000) {
    in_EDX = -1000;
  }
  if (*(short *)(iVar2 + 0xae) < DAT_006f1cd8) {
    uVar1 = 0x40000000;
  }
  if (*(char *)(iVar2 + 0xd5) == '\0') {
    uVar1 = uVar1 | 0x20000000;
  }
  return uVar1 | in_EDX + 1000U;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

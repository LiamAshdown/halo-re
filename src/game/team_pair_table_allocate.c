// team_pair_table_allocate  (Ghidra: FUN_0045bc30; renamed per symbols/review_queue.txt)
// address 0x45bc30, size 72 bytes
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: types/game.h team_pair_globals (0xb4 bytes, pointer at 0x006b0b84); the bump
//   allocator pattern (game-state cursor + crc32_update tracking) matches every other
//   game-state allocation in this module (e.g. particle_systems_initialize).
// register convention: no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t game_state_cursor;       // 0x006e2dcc
extern uint8_t *game_state_base;         // 0x006e2dc8
extern uint32_t game_state_crc;         // 0x006e2dd4
extern team_pair_globals *team_pair_data; // 0x006b0b84

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module

// Bump-allocates and zeroes the team-pair relationship table from the game-state arena.
void team_pair_table_allocate(void)
{
    int32_t count;
    uint32_t *cursor;
    uint32_t size;

    cursor = (uint32_t *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0xb4;
    size = 0xb4;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    team_pair_data = (team_pair_globals *)cursor;
    for (count = 0x2d; count != 0; count = count - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
}

#if 0
Original Ghidra decompilation (0x45bc30), from tools/pack.py 0x45bc30:

void FUN_0045bc30(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  puVar2 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 0xb4;
  local_4 = 0xb4;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b0b84 = puVar2;
  for (iVar1 = 0x2d; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

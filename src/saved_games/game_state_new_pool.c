// game_state_new_pool  (Ghidra: game_state_new_pool, already named)
// address 0x538150, size 112 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; matches types/memory.h memory_pool layout
// exactly: signature 'pool' (0x706f6f6c little-endian), name[32], base == this+0x38,
// size/free_bytes both set to the requested pool size, first_block/last_block cleared. Same
// arena-carving and crc-folding pattern as game_state_new (0x5380d0).
// register convention: pool size in EBX (unaff_EBX); name is the recognized stack parameter
// (param_1).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern uint8_t *game_state_base; // 0x006e2dc8
extern int32_t game_state_cursor; // 0x006e2dcc
extern uint32_t game_state_crc; // 0x006e2dd4

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0

// blam-cc: pool size in EBX, then the recognized stack parameter (name)
// Carves a new memory_pool header + storage block out of the game-state arena, folding the
// block's total size into the running game-state crc. Returns a pointer to the new pool.
memory_pool *game_state_new_pool(char *name, int32_t pool_size)
{
    memory_pool *pool;
    int32_t block_size;
    uint32_t *zero;
    int32_t i;

    block_size = pool_size + 0x38;
    pool = (memory_pool *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + block_size;
    crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);

    zero = (uint32_t *)pool;
    for (i = 0xe; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }
    pool->signature = k_memory_pool_signature; // 'pool'
    strncpy(pool->name, name, 0x1f);
    pool->base = (uint8_t *)pool + 0x38;
    pool->first_block = 0;
    pool->last_block = 0;
    pool->size = pool_size;
    pool->free_bytes = pool_size;
    return pool;
}

#if 0
Original Ghidra decompilation (0x538150):

undefined4 * game_state_new_pool(char *param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int unaff_EBX;
  undefined4 *puVar3;
  int local_4;

  local_4 = unaff_EBX + 0x38;
  puVar1 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + local_4;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  puVar3 = puVar1;
  for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *puVar1 = 0x706f6f6c;
  _strncpy((char *)(puVar1 + 1),param_1,0x1f);
  puVar1[9] = puVar1 + 0xe;
  puVar1[0xc] = 0;
  puVar1[0xd] = 0;
  puVar1[10] = unaff_EBX;
  puVar1[0xb] = unaff_EBX;
  return puVar1;
}
#endif

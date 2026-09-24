// decals_initialize  (Ghidra: decals_initialize, already named -- cea-pdb hint on the "decals"
// string)
// address 0x44df90, size 99 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: types/effects.h k_maximum_decals (0x800), decal is 0x38 bytes, decal_grid is 0x280c
// bytes; out/phase4/effects_types_notes.md evidence #1 walks this exact sequence (data_array
// valid flag set directly, 0x280c carved from game state, folded into the CRC, decal_grid_block
// pointer stored). src/game/game_initialize.c establishes game_state_cursor/game_state_base/
// game_state_crc and crc32_update's (crc, data, length) signature.
// register convention: no arguments.
// UNSURE: the raw decompile writes byte offset +0x25 of the data_array header, which is
// types/memory.h's pad_25[0] (padding), one byte past the documented `valid` field at +0x24 that
// decal_rehash_object_decals 0x44e000 actually reads. Preserved exactly as the pad byte rather
// than silently "corrected" to `valid`, since decal_rehash_object_decals's own guard would then
// read as permanently false in retail -- which is itself worth flagging, not resolving here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *decal_data;          // 0x0087abe4
extern decal_grid *decal_grid_block;    // 0x006b0ad8
extern uint8_t *game_state_base;        // 0x006e2dc8
extern int32_t game_state_cursor;       // 0x006e2dcc
extern uint32_t game_state_crc;         // 0x006e2dd4

extern void *game_state_new(int16_t element_size, char *name, int16_t maximum_count); // 0x5380d0
    // blam-cc: EBX -> element_size, stack -> (name, maximum_count)
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module
extern void rasterizer_decals_initialize(void); // 0x51a6a0

// Registers the decal datum table, marks it valid immediately, carves decal_grid out of game
// state (folding its size into the game-state CRC), and initializes the rasterizer's decal
// geometry storage.
void decals_initialize(void)
{
    uint32_t block_size = sizeof(decal_grid);

    decal_data = (data_array *)game_state_new(sizeof(decal), "decals", k_maximum_decals);
    ((uint8_t *)decal_data)[0x25] = 1; // see UNSURE above: the pad byte after `valid`, not `valid` itself

    decal_grid_block = (decal_grid *)(game_state_base + game_state_cursor);
    game_state_cursor = game_state_cursor + sizeof(decal_grid);
    crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);

    rasterizer_decals_initialize();
}

#if 0
Original Ghidra decompilation (0x44df90):

void __cdecl decals_initialize(void)

{
  int iVar1;
  undefined4 local_4;

  DAT_0087abe4 = game_state_new("decals",0x800);
  *(undefined1 *)(DAT_0087abe4 + 0x25) = 1;
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x280c;
  local_4 = 0x280c;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b0ad8 = iVar1;
  rasterizer_decals_initialize();
  return;
}
#endif

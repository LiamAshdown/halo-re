// render_initialize  (Ghidra: FUN_00511da0; named from out/phase4/render_types_notes.md: "0x511da0
// is render_initialize: it carves the 0x10 byte tint block (0x0071cfc0) and calls
// rasterizer_initialize_direct3d")
// address 0x511da0, size 66 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/render.h globals list: "0x0071cfc0 ColorARGB *rasterizer_model_ambient_reflection_tint
//   0x10 byte game state block carved by 0x511da0". The crc32_update call folds a fixed 0x10
//   (the block size) into a running checksum kept alongside the game state cursor, matching the
//   allocator idiom used elsewhere for carving fixed blocks out of the game state arena.
// register convention: none observed (void).
// UNSURE: the exact identity/owner of the three 0x006e2dxx globals (the game state cursor,
//   base and running checksum) is not recovered here; named generically and cross-checked only
//   by their use as a simple bump allocator.

#include "tags.h"
#include "memory.h"
#include "fn_rasterizer.h"

extern uint8_t *game_state_base;              // 0x006e2dc8 (matches src/ai/ai_communication_initialize.c)
extern int32_t game_state_cursor;             // 0x006e2dcc
extern uint32_t game_state_crc;               // 0x006e2dd4
extern ColorARGB *rasterizer_model_ambient_reflection_tint; // 0x0071cfc0

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module


// Carves the 0x10 byte model ambient reflection tint block out of the game state arena and
// initializes the Direct3D rasterizer device.
uint8_t render_initialize(void)
{
    int32_t block_size = 0x10;

    rasterizer_model_ambient_reflection_tint =
        (ColorARGB *)(game_state_base + game_state_cursor);
    game_state_cursor = game_state_cursor + 0x10;
    crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);
    return rasterizer_initialize_direct3d();  // the original returns this call's result (EAX) unchanged
}

#if 0
Original Ghidra decompilation (0x511da0):

void FUN_00511da0(void)

{
  int iVar1;
  undefined4 local_4;

  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x10;
  local_4 = 0x10;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_0071cfc0 = iVar1;
  rasterizer_initialize_direct3d();
  return;
}
#endif

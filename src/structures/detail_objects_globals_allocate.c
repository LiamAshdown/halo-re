// detail_objects_globals_allocate  (Ghidra: FUN_00552260; named here)
// address 0x552260, size 93 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/structures.h detail_object_globals (size 0xa430, matches the reservation
//   exactly) and its default_z_reference field at +0xa420 (the {0,0,1.0,0}
//   ScenarioStructureBSPGlobalZReferenceVector this function seeds); global 0x0072277c
//   detail_objects, this module.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"

extern uint8_t *game_state_base;   // 0x006e2dc8, game.h (saved_games)
extern int32_t game_state_cursor;  // 0x006e2dcc, game.h (saved_games)
extern uint32_t game_state_crc;    // 0x006e2dd4, game.h (saved_games)
extern detail_object_globals *detail_objects; // 0x0072277c, this module

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module

// Reserves the one 0xa430-byte detail-object game-state block and seeds its default
// z-reference vector {0, 0, 1.0, 0} (used by detail_objects_update_render_list as the fallback
// z_reference when a cell's tag block has none of its own).
void detail_objects_globals_allocate(void)
{
    uint8_t *region = game_state_base + game_state_cursor;
    int32_t size = sizeof(detail_object_globals);

    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    detail_objects = (detail_object_globals *)region;

    detail_objects->default_z_reference.z_reference_i = 0.0f;
    detail_objects->default_z_reference.z_reference_j = 0.0f;
    detail_objects->default_z_reference.z_reference_k = 1.0f;
    detail_objects->default_z_reference.z_reference_l = 0.0f;
}

#if 0
Original Ghidra decompilation (0x552260):

void FUN_00552260(void)

{
  int iVar1;
  undefined4 local_4;

  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0xa430;
  local_4 = 0xa430;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_0072277c = iVar1;
  *(undefined4 *)(iVar1 + 0xa420) = 0;
  *(undefined4 *)(iVar1 + 0xa424) = 0;
  *(undefined4 *)(iVar1 + 0xa428) = 0x3f800000;
  *(undefined4 *)(iVar1 + 0xa42c) = 0;
  return;
}
#endif

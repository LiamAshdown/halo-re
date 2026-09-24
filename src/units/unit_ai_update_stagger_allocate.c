// unit_ai_update_stagger_allocate  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_561fe0" from the object_type_definition vtable evidence)
// address 0x561fe0, size 61 bytes
// name confidence 0.4, rewrite confidence 0.7
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are 0x561fe0 (+0x14
//   initialize) ... Five of those ... are not in this module's function list even though they
//   are unit methods." types/units.h documents 0x006ef910 as "a pointer to the AI update-stagger
//   record {int16 threshold, int16 highest, uint8 claimed}". This carves 8 bytes from the raw
//   game-state bump allocator (game_state_base/cursor/crc, 0x006e2dc8/cc/d4 -- named exactly this
//   way by ai_communication_initialize.c) the same way every other module's `_initialize`
//   allocates its game-state block, folds the 8-byte size into the running CRC, and stores the
//   carved pointer into ai_update_stagger. Object size is 8 even though the record itself is only
//   5 bytes, matching this allocator's dword-rounding seen elsewhere in the module.
// register convention: no parameters (pure global-state initializer, called once per game state
//   creation like every other module's `_initialize`).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"

extern uint8_t *game_state_base;   // 0x006e2dc8
extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint32_t game_state_crc;    // 0x006e2dd4
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, foreign (memory)
extern struct { int16_t threshold; int16_t highest; uint8_t claimed; } *ai_update_stagger; // 0x006ef910

// object_type_definition "unit" row, +0x14 column. Carves an 8-byte block for ai_update_stagger
// out of the game-state arena and folds its size into the running allocation CRC.
void unit_ai_update_stagger_allocate(void)
{
    uint8_t *block = game_state_base + game_state_cursor;
    int32_t size = 8;

    game_state_cursor = game_state_cursor + 8;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    ai_update_stagger = (void *)block;
}

#if 0
Original Ghidra decompilation (0x561fe0):

void missed_561fe0(void)

{
  int iVar1;
  undefined4 local_4;

  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 8;
  local_4 = 8;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006ef910 = iVar1;
  return;
}
#endif

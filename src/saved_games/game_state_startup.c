// game_state_startup  (Ghidra: game_state_startup, already named)
// address 0x537f90, size 98 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// "0x440000 arena" and "0x14c game-state header" notes: pushes 0x400000 (cpu_size) and passes
// 0x40000 (extra_size) in ECX to game_state_allocate_buffer, then crcs the literal 0x14c
// (k_game_state_header_size) into game_state_crc and reserves it at the front of the cursor.
// register convention: no parameters, no return value.

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
extern game_state_header *game_state_header_ptr; // 0x006e2de0

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0


void game_state_startup(void)
{
    int32_t header_size;
    uint8_t *header_base;

    game_state_crc = 0xffffffff;
    game_state_base = (uint8_t *)game_state_allocate_buffer(k_game_state_cpu_size, k_game_state_extra_size);
    game_state_create_persistent_storage_file();
    header_base = game_state_cursor + game_state_base;
    game_state_cursor = game_state_cursor + k_game_state_header_size;
    header_size = k_game_state_header_size;
    crc32_update(&game_state_crc, (uint8_t *)&header_size, 4);
    game_state_header_ptr = (game_state_header *)header_base;
}

#if 0
Original Ghidra decompilation (0x537f90):

void game_state_startup(void)

{
  int iVar1;
  undefined4 local_4;

  DAT_006e2dd4 = 0xffffffff;
  DAT_006e2dc8 = game_state_allocate_buffer(0x400000);
  chimera__multiple_instance_2();
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x14c;
  local_4 = 0x14c;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006e2de0 = iVar1;
  return;
}
#endif

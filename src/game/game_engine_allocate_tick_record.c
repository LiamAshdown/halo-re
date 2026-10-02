// game_engine_allocate_tick_record  (Ghidra: game_engine_allocate_tick_record, already named)
// address 0x470a80, size 86 bytes, cc=__cdecl
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Allocates and zero-initializes a new per-tick
// game-state record, updates the running CRC over it, and makes it the current tick record");
// types/game.h game_time_globals (0x20 bytes, pointer at 0x006f1d6c); game_engine_tick.c's own
// evidence note for this function; game.h globals list (game_state_base/cursor/crc, "saved_games"
// module ownership -- read here, not owned).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *game_state_base;    // 0x006e2dc8 (saved_games)
extern int32_t game_state_cursor;   // 0x006e2dcc (saved_games)
extern uint32_t game_state_crc;     // 0x006e2dd4 (saved_games)
extern game_time_globals *game_time; // 0x006f1d6c

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0

// Carves a fresh 0x20-byte game_time_globals record off the end of the game-state arena, folds
// its size into the running game-state CRC, zeroes every field, and makes it the current tick
// record.
void game_engine_allocate_tick_record(void)
{
    game_time_globals *record = (game_time_globals *)(game_state_base + game_state_cursor);
    int32_t record_size = 0x20;

    game_state_cursor = game_state_cursor + 0x20;
    crc32_update(&game_state_crc, (uint8_t *)&record_size, 4);

    memset(record, 0, sizeof(*record)); // Ghidra zeroes all 8 dwords (puVar1[0..7]) individually;
                                         // sizeof(game_time_globals) == 0x20 == that same span.
    game_time = record;
}

#if 0
Original Ghidra decompilation (0x470a80), from tools/pack.py 0x470a80:

void __cdecl game_engine_allocate_tick_record(void)

{
  undefined4 *puVar1;
  undefined4 local_4;

  puVar1 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 0x20;
  local_4 = 0x20;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  *puVar1 = 0;
  puVar1[1] = 0;
  puVar1[2] = 0;
  puVar1[3] = 0;
  puVar1[4] = 0;
  puVar1[5] = 0;
  puVar1[6] = 0;
  DAT_006f1d6c = puVar1;
  puVar1[7] = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

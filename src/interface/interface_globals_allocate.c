// interface_globals_allocate  (Ghidra: FUN_00494340, renamed per types/interface.h)
// address 0x494340, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/interface.h first_person_weapon_interface struct comment names this address
// interface_globals_allocate ("reserves it... bumps the game-state cursor by 0x1ea0 and folds
// that constant into the state checksum"); src/game/players_initialize.c's confirmed
// game_state_cursor/game_state_base/game_state_crc globals and crc32_update signature; the same
// header's own text names 0x4a9780 hud_state_allocate.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint8_t *game_state_base;   // 0x006e2dc8
extern uint32_t game_state_crc;    // 0x006e2dd4
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern void terminal_initialize(void);   // 0x4963d0
extern void hud_state_allocate(void);    // 0x4a9780
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0

// Initializes the developer console and HUD state blocks, then reserves one
// first_person_weapon_interface (0x1ea0 bytes) from the game-state bump allocator, folding its
// size into the running game-state checksum.
void interface_globals_allocate(void)
{
    int32_t block;
    int32_t size = 0x1ea0;

    terminal_initialize();
    hud_state_allocate();

    block = game_state_cursor + (int32_t)game_state_base;
    game_state_cursor = game_state_cursor + 0x1ea0;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    first_person_weapon_interfaces = (first_person_weapon_interface *)block;
}

#if 0
Original Ghidra decompilation (0x494340):

void FUN_00494340(void)

{
  int iVar1;
  undefined4 local_4;

  terminal_initialize();
  FUN_004a9780();
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x1ea0;
  local_4 = 0x1ea0;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b2d98 = iVar1;
  return;
}
#endif

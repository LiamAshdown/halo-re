// hud_state_allocate  (Ghidra: FUN_004a9780; named by types/interface.h's HUD runtime state
// note, "hud_state_allocate @0x4a9780")
// address 0x4a9780, size 330 bytes
// name confidence: 0.5 (from types/interface.h)   rewrite confidence: 0.6
// evidence: types/interface.h HUD runtime state block sizes (0x004, 0x488, 0x05c, 0x07c,
// 0x030, 0x570) and their six owning globals, all stated by the header as reserved by this
// function; src/interface/interface_globals_allocate.c's established game_state_cursor/
// game_state_base/game_state_crc globals (that file already calls this one) and
// src/memory/crc32_update.c for the checksum callee.
// The six size/pointer assignments are pipelined by the compiler (each pointer is stored two
// steps after its address is computed), which changes nothing observable -- no branch or read
// depends on an intermediate value, so this rewrite issues them in the straightforward
// sequential order instead of reproducing the stagger.
// register convention: no parameters; operates on the shared game-state bump allocator.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint8_t *game_state_base;   // 0x006e2dc8
extern uint32_t game_state_crc;    // 0x006e2dd4

extern hud_globals_flags *hud_flags;                 // 0x00719420
extern hud_messaging_globals *hud_messaging;          // 0x006b3a40
extern hud_unit_meter_globals *hud_unit_meters;       // 0x0071942c
extern hud_weapon_interface_state *hud_weapon_state;  // 0x00719430
extern hud_waypoint_state *hud_waypoints;             // 0x006b3a44
extern motion_sensor_globals *motion_sensor;          // 0x00719438

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length);

// Reserves the six HUD runtime-state blocks from the game-state bump allocator, in order, and
// folds each of the first five blocks' size (not its contents) into the running state checksum
// (the sixth, motion_sensor, is not checksummed, matching Ghidra exactly).
void hud_state_allocate(void)
{
    int32_t size;

    size = sizeof(hud_globals_flags);
    hud_flags = (hud_globals_flags *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_messaging_globals);
    hud_messaging = (hud_messaging_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_unit_meter_globals); // 0x5c
    hud_unit_meters = (hud_unit_meter_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_weapon_interface_state);
    hud_weapon_state = (hud_weapon_interface_state *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_waypoint_state);
    hud_waypoints = (hud_waypoint_state *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(motion_sensor_globals);
    motion_sensor = (motion_sensor_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
}

#if 0
Original Ghidra decompilation (0x4a9780):

void FUN_004a9780(void)

{
  int iVar1;
  int iVar2;
  undefined4 local_4;

  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 4;
  local_4 = 4;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x488;
  local_4 = 0x488;
  DAT_00719420 = iVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x5c;
  local_4 = 0x5c;
  DAT_006b3a40 = iVar2;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x7c;
  local_4 = 0x7c;
  DAT_0071942c = iVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x30;
  local_4 = 0x30;
  DAT_00719430 = iVar2;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x570;
  local_4 = 0x570;
  DAT_006b3a44 = iVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_00719438 = iVar2;
  return;
}
#endif

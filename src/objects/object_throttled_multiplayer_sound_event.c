// object_throttled_multiplayer_sound_event
// address 0x4ee370, size 67 bytes
// name confidence: 0.25 (still FUN_004ee370 in Ghidra; out/phase4/objects_functions.md's
// inherited summary "Refreshes a cached lighting probe..." does not match this code at all —
// there is no object, light or probe reference anywhere in it — so a neutral name based on the
// actual behaviour is used instead)
// rewrite confidence: 0.4
// evidence: none of this function's globals are documented in types/objects.h; the game-time
// tick field is the one exception, matching the "+0x0c is the current tick" note on 0x006f1d6c.
// UNSURE: DAT_00689481 and DAT_006b8a00 are not owned by this module and their true meaning
// (a mode flag and a last-played-tick counter, guessed from usage) is not established.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"

extern uint8_t g_00689481; // 0x00689481, UNSURE: mode/gametype flag, not owned by this module
extern int32_t object_sound_event_last_tick; // 0x006b8a00, UNSURE: last-played-tick counter, not owned by this module
extern hs_game_time_globals *game_time; // 0x006f1d6c, game time globals; +0x0c is the current tick

extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40

void object_throttled_multiplayer_sound_event(void)
{
    if (g_00689481 == 1 && (uint32_t)(object_sound_event_last_tick + 2) < (uint32_t)game_time->current_tick) {
        game_engine_queue_multiplayer_sound(0);
        object_sound_event_last_tick = game_time->current_tick;
    }
}

#if 0
Original Ghidra decompilation (0x4ee370):

void FUN_004ee370(void)

{
  if ((DAT_00689481 == '\x01') && (DAT_006b8a00 + 2U < *(uint *)(DAT_006f1d6c + 0xc))) {
    game_engine_queue_multiplayer_sound(0);
    DAT_006b8a00 = *(int *)(DAT_006f1d6c + 0xc);
  }
  return;
}
#endif

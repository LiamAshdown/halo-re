// game_engine_ctf_notify_flag_carried_throttled  (Ghidra: FUN_004689e0; named per its summary)
// address 0x4689e0, size 51 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Throttles a periodic flag-related update/notify call
//   to at most once every four seconds"); types/game.h game_time_globals::game_time (0x0c);
//   0x78 ticks == 4.0 s at k_game_ticks_per_second (30).
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_time_globals *game_time;         // 0x006f1d6c
extern int32_t ctf_notify_throttle_tick;     // 0x006b0eb4

extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40

// Queues the "flag carried" announcer sound at most once every 4 seconds (120 ticks).
void game_engine_ctf_notify_flag_carried_throttled(void)
{
    if (ctf_notify_throttle_tick < game_time->game_time) {
        game_engine_queue_multiplayer_sound(1);
        ctf_notify_throttle_tick = game_time->game_time + 0x78;
    }
}

#if 0
Original Ghidra decompilation (0x4689e0), from tools/pack.py 0x4689e0:

void FUN_004689e0(void)

{
  if (DAT_006b0eb4 < *(int *)(DAT_006f1d6c + 0xc)) {
    game_engine_queue_multiplayer_sound(1);
    DAT_006b0eb4 = *(int *)(DAT_006f1d6c + 0xc) + 0x78;
  }
  return;
}
#endif

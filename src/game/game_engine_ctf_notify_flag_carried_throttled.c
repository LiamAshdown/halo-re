// game_engine_ctf_notify_flag_carried_throttled  (Ghidra: FUN_004689e0; named per its summary)
// address 0x4689e0, size 51 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Throttles a periodic flag-related update/notify call
//   to at most once every four seconds"); types/game.h game_time_globals::game_time (0x0c);
//   0x78 ticks == 4.0 s at k_game_ticks_per_second (30).
// register convention: no parameters.
//   // blam-cc: EDI -> target_player
// EDI carries target_player (never written here, so a pass-through input read by game_engine_queue_multiplayer_sound at 0x4689f8);
// it is forwarded to that call below.
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time;         // 0x006f1d6c
extern int32_t ctf_notify_throttle_tick;     // 0x006b0eb4

extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast

// Queues the "flag carried" announcer sound at most once every 4 seconds (120 ticks).
// blam-cc: EDI -> target_player
void game_engine_ctf_notify_flag_carried_throttled(int32_t target_player)
{
    if (ctf_notify_throttle_tick < game_time->game_time) {
        game_engine_queue_multiplayer_sound(0x1c, (datum_index)target_player, 1); // 0x4689f1..0x4689f8
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// game_engine_reset_round_objects  (Ghidra: FUN_00468260; named per its summary)
// address 0x468260, size 82 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: out/phase4/game_functions.md ("Runs the game engine's per-round reset sequence:
//   clears unit flags, garbage-collects stray objects, reloads netgame equipment, and clears
//   cached player stats"); types/game.h game_engine_definition::reset_objects (0xac) names
//   this function (and FUN_00468320, this batch) as its two callers; game_time_globals::
//   game_time (0x006f1d6c + 0xc) and the sibling unnamed global 0x0087aa20.
// register convention: no parameters.
// UNSURE: 0x0087aa20's role beyond "snapshot of the current game_time taken at every round
//   reset" is not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_time_globals *game_time;                // 0x006f1d6c
extern int32_t game_engine_round_reset_tick;        // 0x0087aa20, UNSURE identity

extern void game_engine_reset_all_unit_grenade_counts(void);       // 0x467de0
extern void game_engine_reset_respawns_and_cleanup_bipeds(void);   // 0x467e60
extern void game_engine_cleanup_stray_items(void);                 // 0x468010, this batch
extern void game_engine_cleanup_stray_projectiles(void);           // 0x467f70
extern void game_engine_update_netgame_equipment(char force_respawn); // 0x45f9f0
extern void game_engine_reset_vehicles_or_race_cleanup(void);      // 0x4681a0, this batch
extern void game_engine_reset_player_profile_stats(void);          // 0x468150, this batch

// The engine's per-round reset: clears every unit's grenade counts, respawns/cleans up bipeds,
// deletes stray items, deletes stray projectiles, runs the loaded gametype's own reset_objects
// hook (if any), stamps the current game_time, force-reloads netgame equipment, resets/cleans
// vehicles (or does Race's stray-vehicle sweep), and clears the cached player profile stats.
void game_engine_reset_round_objects(void)
{
    game_engine_reset_all_unit_grenade_counts();
    game_engine_reset_respawns_and_cleanup_bipeds();
    game_engine_cleanup_stray_items();
    game_engine_cleanup_stray_projectiles();
    if (current_game_engine->reset_objects != 0) {
        ((void (*)(void))current_game_engine->reset_objects)();
    }
    game_engine_round_reset_tick = game_time->game_time;
    game_engine_update_netgame_equipment(1);
    game_engine_reset_vehicles_or_race_cleanup();
    game_engine_reset_player_profile_stats();
}

#if 0
Original Ghidra decompilation (0x468260), from tools/pack.py 0x468260:

void FUN_00468260(void)

{
  FUN_00467de0();
  FUN_00467e60();
  FUN_00468010();
  FUN_00467f70();
  if (*(code **)(DAT_006f1d20 + 0xac) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0xac))();
  }
  DAT_0087aa20 = *(undefined4 *)(DAT_006f1d6c + 0xc);
  game_engine_update_netgame_equipment(1);
  FUN_004681a0();
  FUN_00468150();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

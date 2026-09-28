// game_engine_initialize_for_new_game  (Ghidra: FUN_0045c370)
// address 0x45c370, size 207 bytes
// name confidence: 0.5 (still FUN_0045c370 in Ghidra; named from types/game.h's own
//   game_engine_definition comment, which cites this exact address for the +0x0c
//   "initialize_for_new_game" vtable slot this function calls)
// rewrite confidence: 0.4
// evidence: types/game.h current_game_engine (0x006f1d20, initialize_for_new_game at +0x0c),
//   multiplayer_sound_request queue (0x006b10f0, 5 x 0x10, count at 0x006b1140),
//   custom_waypoint array (0x006f1888, 32 x 0x20), game_engine_auto_team_counter (0x0087aa04),
//   game_engine_ctf_reset_ticks (0x0087aa24, "seeded with 0x1e" elsewhere -- here reset to 0),
//   game_engine_dedicated_idle/_timer (0x0087aa18/0x0087aa1c); game_engine_unload (0x45c330,
//   this batch).
// register convention: __cdecl, no arguments.
//
// UNSURE: 0x0068e590 is an unrecovered per-map table (map_list_find_known_map_index's result,
// bounded to 0x13 entries, indexes it with a 0x30-byte stride -- Ghidra's own
// "(&DAT_0068e590)[index * 0xc]" is an undefined4-array index, i.e. byte offset index*0x30);
// only its first dword is read here and the table's meaning is not recovered. 0x006f1d24 and
// 0x006f1d25 are Ghidra-reported as overlapping symbols at the same address (the "WARNING:
// Globals starting with '_' overlap smaller symbols" note): the whole dword is written from
// the map table, and separately byte 1 of that same dword is independently zeroed later; both
// writes are reproduced literally. 0x00722a18, FUN_00463810 and FUN_00466890 are outside this
// batch's evidence and kept as raw/opaque.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine;   // 0x006f1d20
extern int32_t game_engine_map_table_value;            // 0x006f1d24, TYPES-GAP: see UNSURE above
extern uint8_t map_per_map_table[];                     // 0x0068e590, TYPES-GAP: 0x30-byte
                                                        //   stride, 0x13 (19) entries, only the
                                                        //   first dword of an entry is read here
extern uint8_t network_session_host_state;                        // 0x00722a18, UNSURE meaning
extern multiplayer_sound_request multiplayer_sound_queue[5]; // 0x006b10f0
extern int32_t multiplayer_sound_queue_count;           // 0x006b1140
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints];            // 0x006f1888
extern int32_t game_engine_auto_team_counter;           // 0x0087aa04
extern int32_t game_engine_ctf_reset_ticks;             // 0x0087aa24
extern uint8_t game_engine_dedicated_idle;              // 0x0087aa18
extern float game_engine_dedicated_idle_timer;          // 0x0087aa1c

extern void game_engine_unload(void);              // 0x45c330, this batch
extern void game_engine_validate_scenario_placements_noop(void);                    // UNSURE module
extern void game_engine_touch_multiplayer_predicted_resources(void);                    // UNSURE module
extern int32_t map_list_find_known_map_index(void); // 0x494ff0

// Resets the multiplayer sound queue, custom waypoints, auto-team counter and CTF reset timer,
// then calls the loaded engine's initialize_for_new_game vtable slot; if that slot exists and
// reports failure, rolls the engine back via game_engine_unload.
void game_engine_initialize_for_new_game(void)
{
    int32_t map_index;
    uint32_t *dst;
    int32_t i;
    uint8_t initialize_result;

    if (current_game_engine != (game_engine_definition *)0) {
        map_index = map_list_find_known_map_index();
        game_engine_map_table_value = 0;
        if (map_index < 0x13) {
            game_engine_map_table_value = *(int32_t *)(map_per_map_table + map_index * 0x30);
        }
        game_engine_validate_scenario_placements_noop();

        dst = (uint32_t *)multiplayer_sound_queue;
        for (i = 0x14; i != 0; i = i - 1) {
            *dst = 0;
            dst = dst + 1;
        }
        multiplayer_sound_queue[0].player = (datum_index)0xffffffff;
        multiplayer_sound_queue[0].sound_index = -1;

        dst = (uint32_t *)custom_waypoints;
        for (i = 0x100; i != 0; i = i - 1) {
            *dst = 0;
            dst = dst + 1;
        }

        multiplayer_sound_queue[0].remaining_ticks = 0x3c;
        *(uint32_t *)&multiplayer_sound_queue[0].broadcast = 0;
        game_engine_auto_team_counter = 0;
        multiplayer_sound_queue_count = 1;
        game_engine_ctf_reset_ticks = 0;

        if (current_game_engine->initialize_for_new_game != (void *)0) {
            initialize_result =
                ((uint8_t (*)(void))current_game_engine->initialize_for_new_game)();
            if (initialize_result == 0) {
                game_engine_unload();
            }
        }
        game_engine_touch_multiplayer_predicted_resources();
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        ((uint8_t *)&game_engine_map_table_value)[1] = 0;
        if (network_session_host_state != 2) {
            network_session_host_state = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x45c370), from tools/pack.py 0x45c370:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0045c370(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;

  if (DAT_006f1d20 != 0) {
    iVar2 = map_list_find_known_map_index();
    DAT_006f1d24 = 0;
    if (iVar2 < 0x13) {
      DAT_006f1d24 = (&DAT_0068e590)[iVar2 * 0xc];
    }
    FUN_00463810();
    puVar3 = &DAT_006b10f0;
    for (iVar2 = 0x14; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    DAT_006b10f0 = 0xffffffff;
    DAT_006b10f4 = 0xffffffff;
    puVar3 = &DAT_006f1888;
    for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    _DAT_006b10f8 = 0x3c;
    DAT_006b10fc = 0;
    DAT_0087aa04 = 0;
    DAT_006b1140 = 1;
    _DAT_0087aa24 = 0;
    if (*(code **)(DAT_006f1d20 + 0xc) != (code *)0x0) {
      cVar1 = (**(code **)(DAT_006f1d20 + 0xc))();
      if (cVar1 == '\0') {
        game_engine_unload();
      }
    }
    FUN_00466890();
    DAT_0087aa18 = 0;
    _DAT_0087aa1c = 0;
    DAT_006f1d25 = 0;
    if (DAT_00722a18 != 2) {
      DAT_00722a18 = 1;
    }
  }
  return;
}
#endif

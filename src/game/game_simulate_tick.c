// game_simulate_tick  (Ghidra: FUN_0045b780)
// address 0x45b780, size 300 bytes
// name confidence: 0.4 (still FUN_0045b780 in Ghidra; named for what it does: the single
//   per-frame simulation driver called once per game loop iteration)
// rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Main per-tick game update that advances scripts,
//   structure-BSP switching, and network client/server catch-up logic"); network_game_mode
//   (0x00719720); types/units.h's own note that 0x006ef910 (the AI update-stagger record) is
//   "owned and reset" by this address; game_engine_flag_local_player_units (0x45b590),
//   team_pair_overrides_tick (0x45bcf0), game_engine_tick (0x45ff30), hs_runtime_update
//   (0x48a1a0), all already named elsewhere in this module; main_game_globals (0x006b0b80,
//   byte+2 slow-motion flag, same global game_effects_update.c already declares).
// register convention: __cdecl; the recognized stack parameter gates one call
//   (FUN_004768c0) below.
//
// UNSURE: most of this function's ~20 callees are outside this batch (device/hs/main/network/
// objects/players modules not yet rewritten) and are declared here as opaque `void (void)`
// externs. DAT_00699f40/44 and DAT_0071cc20/24 look like two more {buffer, count} network
// message queues flushed via FUN_004e8040 whenever non-empty, by analogy with the
// {ai_update_stagger} record shape already established in types/units.h, but no header
// documents them; kept as raw pairs with TYPES-GAP. The float constant selected for
// effects_update_all (1/30 s normally, 1/60 s in slow motion) is likely a "seconds per simulated
// tick" setter but its real name/role could not be confirmed from this batch's evidence.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_hs.h"
#include "fn_ai.h"
#include "fn_game.h"
#include "fn_objects.h"
#include "fn_networking.h"

extern uint8_t DAT_0087ab18;                 // 0x0087ab18, UNSURE: "simulation in progress" reentrancy flag
typedef struct ai_update_stagger_state { int16_t threshold; int16_t highest; uint8_t claimed; } ai_update_stagger_state;
extern ai_update_stagger_state *ai_update_stagger; // 0x006ef910
extern int16_t network_game_mode;            // 0x00719720
extern game_main_globals *main_game_globals; // 0x006b0b80
extern int32_t player_effect_reentry_count;    // 0x00719ccc, UNSURE name (incremented/decremented around FUN_004923d0)
extern int32_t network_scenario_round_counter_a;             // 0x00699f44, TYPES-GAP: message queue count
extern uint8_t unknown_00699f40[];           // 0x00699f40, TYPES-GAP: message queue buffer
extern int32_t network_scenario_round_counter_b;             // 0x0071cc24, TYPES-GAP: message queue count
extern uint8_t unknown_0071cc20[];           // 0x0071cc20, TYPES-GAP: message queue buffer


extern void recorded_animations_update(void);            // 0x44aa90, UNSURE module
extern void effects_update_all(float seconds_per_tick);  // 0x450aa0, UNSURE module/role
extern void player_effect_clear_dead_players(void);                    // UNSURE module


extern void first_person_weapon_interface_tick(void);                    // UNSURE module (hs-related, guarded by
                                                   //   player_effect_reentry_count)
extern void hud_update_dispatch(void);                    // UNSURE module

extern void network_event_feed_flush(void *queue);             // UNSURE module (flushes a message queue)


// The per-frame simulation driver: resets the AI update-stagger record, sets the FPU control
// word, ticks team-pair overrides and local-player flags, advances the network/host update
// path for the current connection role, sets a tick-length constant (halved in slow motion),
// then runs the main tick sequence (game engine, hs scripts, devices, objects, structure-BSP
// switch) and, while hosting, the outbound network catch-up and message-queue flush.
void game_simulate_tick(uint32_t predict_pass)
{
    DAT_0087ab18 = 1;
    _control87(0x9001f, 0xfffff);
    game_engine_flag_local_player_units();
    team_pair_overrides_tick();

    ai_update_stagger->threshold = ai_update_stagger->highest;
    ai_update_stagger->highest = 0;
    ai_update_stagger->claimed = 0;

    ai_tick_dispatcher();

    if (network_game_mode != 0) {
        if (network_game_mode == 1) {
            game_engine_players_update_client();
            goto after_role_update;
        }
        if (network_game_mode != 2) {
            goto after_role_update;
        }
    }
    game_engine_players_update_server();

after_role_update:
    {
        float seconds_per_tick = (main_game_globals->players_are_double_speed == 0) ? 0.033333335f : 0.016666668f; // UNSURE: see header
        effects_update_all(seconds_per_tick);
    }

    player_effect_reentry_count = player_effect_reentry_count + 1;
    first_person_weapon_interface_tick();
    player_effect_reentry_count = player_effect_reentry_count - 1;

    game_engine_tick();
    hs_runtime_update();
    recorded_animations_update();
    objects_update();
    main_switch_structure_bsp();
    hud_update_dispatch();
    player_effect_clear_dead_players();

    if (network_game_mode == 2) {
        if (predict_pass == 0) {
            players_server_catchup_on_client_updates();
        }
        game_engine_server_update_player_positions();
        network_client_send_local_player_updates();
        network_server_broadcast_object_type_changes();
        if (0 < network_scenario_round_counter_a) {
            network_event_feed_flush(unknown_00699f40);
        }
        if (0 < network_scenario_round_counter_b) {
            network_event_feed_flush(unknown_0071cc20);
        }
    }
    if (network_game_mode == 1) {
        players_client_catchup_on_server_updates();
    }

    DAT_0087ab18 = 0;
}

#if 0
Original Ghidra decompilation (0x45b780), from tools/pack.py 0x45b780:

void FUN_0045b780(int param_1)

{
  undefined2 *puVar1;
  undefined4 local_4;

  DAT_0087ab18 = 1;
  __control87(0x9001f,0xfffff);
  FUN_0045b590();
  FUN_0045bcf0();
  puVar1 = DAT_006ef910;
  *DAT_006ef910 = DAT_006ef910[1];
  puVar1[1] = 0;
  *(undefined1 *)(puVar1 + 2) = 0;
  FUN_0042a900();
  if (DAT_00719720 != 0) {
    if (DAT_00719720 == 1) {
      FUN_00474590();
      goto LAB_0045b7d9;
    }
    if (DAT_00719720 != 2) goto LAB_0045b7d9;
  }
  FUN_004740a0();
LAB_0045b7d9:
  local_4 = 0x3c888889;
  if (*(char *)(DAT_006b0b80 + 2) == '\0') {
    local_4 = 0x3d088889;
  }
  FUN_00450aa0(local_4);
  DAT_00719ccc = DAT_00719ccc + 1;
  FUN_004923d0();
  DAT_00719ccc = DAT_00719ccc + -1;
  game_engine_tick();
  hs_runtime_update();
  device_groups_update();
  objects_update();
  main_switch_structure_bsp();
  FUN_004a9990();
  FUN_00456730();
  if (DAT_00719720 == 2) {
    if (param_1 == 0) {
      FUN_004768c0();
    }
    FUN_00476760();
    network_client_send_local_player_updates();
    FUN_0045b680();
    if (0 < DAT_00699f44) {
      FUN_004e8040(&DAT_00699f40);
    }
    if (0 < DAT_0071cc24) {
      FUN_004e8040(&DAT_0071cc20);
    }
  }
  if (DAT_00719720 == 1) {
    players_client_catchup_on_server_updates();
  }
  DAT_0087ab18 = 0;
  return;
}
#endif

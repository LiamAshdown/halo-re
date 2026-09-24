// game_engine_tick  (Ghidra: game_engine_tick, already named)
// address 0x45ff30, size 706 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("The main per-tick update for the multiplayer game
// engine, driving object cleanup, equipment respawns, teleporters, and the end-of-game state
// machine"); types/game.h game_engine_definition (update +0x38, is this function's own slot;
// time_scale_override +0x88); types/memory.h data_iterator; src/hs/hs_object_runtime_cleanup.c
// for the established data_iterator_next idiom used for the player loop here.
// RESOLVED (phase 4 review): the "player_data->data + 0x1fffe34" access here is the same folded
// constant documented in game_engine_players_ready_for_bsp_switch.c --
// (0xffff << 9) + 0x34 == 0x1fffe34 -- i.e. player_at(iterator.index)->unit. objdump at
// 0x45fff9..0x460007 shows "mov edx,[esi+0x34] / and ecx,0xffff / shl ecx,9 /
// cmp [ecx+edx*1+0x34],-1" against the player handle in EDI. It is not a 32 MB offset.
// Several float comparisons below use Ghidra's `(a < b) == (a == b)` idiom for an FPU flag test
// that reduces to `a > b`; see game_engine_update_end_game_sequence.c for the derivation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;   // 0x00719720
extern data_array *player_data;     // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88
extern game_engine_state game_engine_state_value; // 0x0087aa10
extern float game_engine_end_game_timer; // 0x0087aa08
extern uint32_t unknown_0087aa00;   // 0x0087aa00, UNSURE identity (flag bitfield)

extern void game_engine_multiplayer_sound_queue_tick(void); // 0x46bd80
extern void game_engine_cleanup_dropped_objects(void); // 0x45f320, this batch
extern void game_engine_update_item_scale_and_pickup(void); // 0x45f560, this batch
extern void game_engine_update_netgame_equipment(char force_respawn); // 0x45f9f0, this batch
extern void game_engine_player_profile_cache_sync_all(datum_index player_handle); // 0x466cb0, not in this batch
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void game_engine_clear_unit_shields_when_disabled(datum_index player_handle); // 0x45fd20, this batch
extern void player_kill_streak_set_max(int32_t unknown); // 0x479ca0, not in this batch
extern void game_engine_update_teleporter(datum_index player_handle); // 0x461630
extern char game_engine_announce_time_remaining(void); // 0x45cae0, not in this batch
extern void game_engine_begin_end_game_sequence(void); // 0x45fd90, this batch
extern void sound_class_set_gain_by_name(const char *class_name, float gain, int32_t ticks); // 0x545390
extern void game_engine_end_game_sequence_stage2(void); // 0x4670f0, not in this batch
extern void game_engine_send_end_game_notification(void); // 0x4671d0, not in this batch
extern void network_server_advance_connect_state(void); // 0x4df290, not in this batch

extern char idle_ambient_sound_class_name[]; // 0x0065512c, UNSURE exact contents (a sound class name)

// The multiplayer game engine's per-tick update: advances the announcer queue, item cleanup,
// netgame-equipment respawns and (while hosting) each player's kill-streak/teleporter/per-player
// engine hook, then drives the end-of-game state machine's two timed stages.
void game_engine_tick(void)
{
    if (current_game_engine == 0) {
        return;
    }

    game_engine_multiplayer_sound_queue_tick();
    game_engine_cleanup_dropped_objects();
    game_engine_update_item_scale_and_pickup();

    if (network_game_mode == 2 || network_game_mode == 0) {
        game_engine_update_netgame_equipment(0);
    }
    if (network_game_mode == 2) {
        game_engine_player_profile_cache_sync_all((datum_index)0xffffffff);
    }

    {
        data_iterator player_iter;
        void *player_element;

        player_iter.data = player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)0xffffffff;
        player_element = data_iterator_next(&player_iter);

        while (player_element != 0) {
            game_engine_clear_unit_shields_when_disabled(player_iter.index);

            if (current_game_engine != 0 &&
                ((game_engine_variant.flags & 0x10) != 0 ||
                 (current_game_engine->time_scale_override != 0 &&
                  ((char (*)(datum_index, int32_t))current_game_engine->time_scale_override)(
                      (datum_index)0xffffffff, 1) != 0)) &&
                ((player *)player_element)->unit != k_datum_index_none) {
                player_kill_streak_set_max(0);
            }

            game_engine_update_teleporter((datum_index)0xffffffff);
            if (current_game_engine->update != 0) {
                ((void (*)(datum_index))current_game_engine->update)((datum_index)0xffffffff);
            }

            player_element = data_iterator_next(&player_iter);
        }
    }

    if (current_game_engine->get_score != 0) {
        ((void (*)(void))current_game_engine->get_score)();
    }

    if (game_engine_state_value == _game_engine_state_not_started) {
        if (game_engine_announce_time_remaining() != 0) {
            game_engine_begin_end_game_sequence();
        }
    } else if (game_engine_state_value == _game_engine_state_ending) {
        if (game_engine_end_game_timer <= 2.0f && (unknown_0087aa00 & 0x10) == 0) {
            sound_class_set_gain_by_name(idle_ambient_sound_class_name, 0.0f, 0x1e);
            sound_class_set_gain_by_name("ambient_nature", 0.2f, 0x1e);
            sound_class_set_gain_by_name("ambient_machinery", 0.2f, 0x1e);
            sound_class_set_gain_by_name("ambient_computers", 0.2f, 0x1e);
            unknown_0087aa00 = unknown_0087aa00 | 0x10;
        }

        game_engine_end_game_timer = game_engine_end_game_timer - 0.033333335f;
        if (game_engine_end_game_timer <= 0.0f && network_game_mode == 2) {
            game_engine_end_game_sequence_stage2();
            game_engine_send_end_game_notification();
            network_server_advance_connect_state();
        }
    }
}

#if 0
Original Ghidra decompilation (0x45ff30), from tools/pack.py 0x45ff30:

/* WARNING: Removing unreachable block (ram,0x0046004f) */
/* WARNING: Removing unreachable block (ram,0x00460060) */
/* WARNING: Removing unreachable block (ram,0x0046006a) */
/* WARNING: Removing unreachable block (ram,0x0046007f) */
/* WARNING: Removing unreachable block (ram,0x00460084) */
/* WARNING: Removing unreachable block (ram,0x00460089) */
/* WARNING: Removing unreachable block (ram,0x00460093) */
/* WARNING: Removing unreachable block (ram,0x004600b6) */
/* WARNING: Removing unreachable block (ram,0x004600b8) */
/* WARNING: Removing unreachable block (ram,0x004600c3) */
/* WARNING: Removing unreachable block (ram,0x004600d0) */
/* WARNING: Removing unreachable block (ram,0x004600d6) */
/* WARNING: Removing unreachable block (ram,0x0046009d) */
/* WARNING: Removing unreachable block (ram,0x004600a8) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_tick(void)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int iVar4;

  if (DAT_006f1d20 != 0) {
    game_engine_multiplayer_sound_queue_tick();
    game_engine_cleanup_dropped_objects();
    FUN_0045f560();
    if ((DAT_00719720 == 2) || (DAT_00719720 == 0)) {
      game_engine_update_netgame_equipment(0);
    }
    if (DAT_00719720 == 2) {
      FUN_00466cb0(0xffffffff);
    }
    iVar4 = DAT_0087a480;
    iVar3 = data_iterator_next();
    iVar1 = DAT_006f1d20;
    while (iVar3 != 0) {
      FUN_0045fd20();
      if ((iVar1 != 0) &&
         (((((byte)DAT_006f1cc0 & 0x10) != 0 ||
           ((*(code **)(iVar1 + 0x88) != (code *)0x0 &&
            (cVar2 = (**(code **)(iVar1 + 0x88))(0xffffffff,1), iVar4 = DAT_0087a480, cVar2 != '\0')
            ))) && (*(int *)(*(int *)(iVar4 + 0x34) + 0x1fffe34) != -1)))) {
        FUN_00479ca0(0);
      }
      game_engine_update_teleporter(0xffffffff);
      if (*(code **)(DAT_006f1d20 + 0x38) != (code *)0x0) {
        (**(code **)(DAT_006f1d20 + 0x38))(0xffffffff);
      }
      iVar4 = DAT_0087a480;
      iVar1 = DAT_006f1d20;
      iVar3 = data_iterator_next();
    }
    if (*(code **)(iVar1 + 0x48) != (code *)0x0) {
      (**(code **)(iVar1 + 0x48))();
    }
    if (DAT_0087aa10 == 0) {
      cVar2 = FUN_0045cae0();
      if (cVar2 != '\0') {
        game_engine_begin_end_game_sequence();
      }
    }
    else if (DAT_0087aa10 == 1) {
      if ((_DAT_0087aa08 < 2.0 != (_DAT_0087aa08 == 2.0)) && ((_DAT_0087aa00 & 0x10) == 0)) {
        sound_class_set_gain_by_name(&DAT_0065512c,0,0x1e);
        sound_class_set_gain_by_name("ambient_nature",0x3e4ccccd,0x1e);
        sound_class_set_gain_by_name("ambient_machinery",0x3e4ccccd,0x1e);
        sound_class_set_gain_by_name("ambient_computers",0x3e4ccccd,0x1e);
        _DAT_0087aa00 = _DAT_0087aa00 | 0x10;
      }
      _DAT_0087aa08 = _DAT_0087aa08 - 0.033333335;
      if ((_DAT_0087aa08 < 0.0 != (_DAT_0087aa08 == 0.0)) && (DAT_00719720 == 2)) {
        FUN_004670f0();
        FUN_004671d0();
        FUN_004df290();
        return;
      }
    }
  }
  return;
}
#endif

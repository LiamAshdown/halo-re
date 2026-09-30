// game_engine_check_bucket_scores_and_end_round  (Ghidra: FUN_0046db70; named per its summary)
// address 0x46db70, size 615 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Aggregates per-bucket (team/hill/flag) score values
//   each tick and, once any bucket reaches its target, declares a winner, arms the end-of-round
//   countdown, plays the win announcement, and broadcasts a UI-close/chat-reset message");
//   types/game.h player::team/marked_for_deletion (0x20/0xd5), player_data (0x0087a480);
//   game_variant::ctf_value_80 aliased 0x006f1d08 (aggregation mode: 0 = min, 1 = max, 2 = sum
//   with a second 16-entry table added in), score_limit aliased 0x006f1ce0; network_server's
//   own end-of-game flag at +0xa0f (types/game.h evidence); game_engine_state_value /
//   game_engine_end_game_timer (7.0 s); multiplayer_sound_enabled[1] (0x00688329);
//   GlobalsMultiplayerInformation::sounds[1].tag_id (+0x60 array pointer, +0x1c into it);
//   game_engine_get_multiplayer_sound_duration_ticks / message_delta_encode_message / network_session_broadcast_to_flagged
//   already established. Globals 0x00718f94/0x00718f98/0x00718fa6/0x006953e8/0x00712542.. belong
//   to the interface/network-chat modules, not this one; kept as opaque externs.
// register convention: no parameters.
// UNSURE: the exact identity of the 0x00718f9x/0x006953e8/0x007125xx globals (widget/chat reset,
//   outside this module); player + 0xc6's real field name (also UNSURE in
//   game_engine_ctf_player_flag_tick.c, this batch).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include <stdint.h>
#include "units.h"
#include "networking.h"
#include "fn_game.h"

extern data_array *player_data;   // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_value_80/score_limit aliased
                                          // 0x006f1d08/0x006f1ce0)
extern int32_t game_engine_bucket_scores[16];      // 0x006b1318
extern int32_t game_engine_bucket_scores_extra[16]; // 0x006b1358, added in when mode==2
extern int16_t network_game_mode;         // 0x00719720
extern game_engine_state game_engine_state_value; // 0x0087aa10
extern network_server_globals *network_server;
extern float game_engine_end_game_timer;  // 0x0087aa08
extern uint8_t multiplayer_sound_enabled[]; // 0x00688328 (index 1 used here)
extern Globals *global_globals;           // 0x00746fa0
extern int32_t multiplayer_sound_queue_count; // 0x006b1140
extern multiplayer_sound_request multiplayer_sound_queue[k_maximum_queued_multiplayer_sounds]; // 0x006b10f0
extern uint8_t shared_hud_text_draw_state; // 0x00871de0

extern void *ui_root_widget; // 0x00718f94, UNSURE identity (interface module)
extern void *ui_widget_history;     // 0x00718f98, UNSURE identity
extern uint8_t ui_pause_depth;   // 0x00718fa6, UNSURE identity
extern int32_t controls_capture_row; // 0x006953e8, UNSURE identity (network/chat module)
extern uint8_t controls_input_capture_flags;    // 0x00712542, UNSURE identity
extern uint8_t controls_input_capture_buffer[0xa0 * 4]; // 0x00712544, UNSURE identity (zeroed 0xa0 dwords)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void game_engine_player_profile_cache_sync_all(datum_index player_handle); // 0x466cb0, UNSURE (not the profile-cache-sync
    // function of the same address prefix; distinct call shape here, single int arg)
extern void widget_close(void *widget); // 0x497c00
extern void widget_pool_list_free_all(void); // 0x4994b0
extern void sound_start_unspatialized(float volume); // 0x543dd0

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server

void game_engine_check_bucket_scores_and_end_round(void)
{
    int32_t bucket;

    for (bucket = 0; bucket < 16; bucket++) {
        data_iterator iter;
        player *p;
        int32_t count = 0;
        int32_t aggregate = 0;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        p = (player *)data_iterator_next(&iter);
        while (p != (player *)0) {
            if (p->team == bucket && p->marked_for_deletion == 0) {
                int32_t value = *(int16_t *)((uint8_t *)p + 0xc6); // UNSURE field name
                if (game_engine_variant.ctf_value_80 == 0) {
                    if (count == 0 || value < aggregate) {
                        aggregate = value;
                    }
                } else if (game_engine_variant.ctf_value_80 == 1) {
                    if (count == 0 || aggregate <= value) {
                        aggregate = value;
                    }
                } else {
                    aggregate += value;
                }
                count++;
            }
            p = (player *)data_iterator_next(&iter);
        }

        game_engine_bucket_scores[bucket] = aggregate;
        if (game_engine_variant.ctf_value_80 == 2) {
            game_engine_bucket_scores[bucket] = game_engine_bucket_scores_extra[bucket] + aggregate;
        }
    }

    game_engine_player_profile_cache_sync_all(-1);

    for (bucket = 0; bucket < 16; bucket++) {
        if (game_engine_bucket_scores[bucket] >= game_engine_variant.score_limit &&
            network_game_mode == 2 && game_engine_state_value == 0) {
            network_server->game_over = 1;
            game_engine_state_value = _game_engine_state_ending;
            game_engine_end_game_timer = 7.0f;

            if (multiplayer_sound_enabled[1] == 0) {
                GlobalsMultiplayerInformation *mp_info =
                    (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
                if (mp_info != (GlobalsMultiplayerInformation *)0 && (int32_t)mp_info->sounds.count > 1) {
                    uint8_t *sound1 = (uint8_t *)mp_info->sounds.pointer + 0x10;
                    if ((int32_t)mp_info->sounds.pointer != -0x10 && *(int32_t *)(sound1 + 0xc) != -1) {
                        sound_start_unspatialized(1.0f);
                    }
                }
            } else {
                int32_t duration = game_engine_get_multiplayer_sound_duration_ticks(1);
                if (multiplayer_sound_queue_count < k_maximum_queued_multiplayer_sounds) {
                    multiplayer_sound_request *slot = &multiplayer_sound_queue[multiplayer_sound_queue_count];
                    slot->player = (datum_index)0xffffffff;
                    slot->sound_index = 1;
                    multiplayer_sound_queue_count++;
                    slot->remaining_ticks = duration + 5;
                    slot->broadcast = 0;
                }
                if (multiplayer_sound_queue_count == 1) {
                    GlobalsMultiplayerInformation *mp_info =
                        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
                    if (mp_info != (GlobalsMultiplayerInformation *)0 && (int32_t)mp_info->sounds.count > 1) {
                        uint8_t *sound1 = (uint8_t *)mp_info->sounds.pointer + 0x10;
                        if ((int32_t)mp_info->sounds.pointer != -0x10 && *(int32_t *)(sound1 + 0xc) != -1) {
                            sound_start_unspatialized(1.0f);
                        }
                    }
                }
            }

            if (ui_root_widget != (void *)0) {
                widget_close(ui_root_widget);
            }
            if (ui_widget_history != (void *)0) {
                widget_pool_list_free_all();
            }
            ui_pause_depth = 0;
            if (controls_capture_row != -1) {
                int32_t i;
                controls_input_capture_flags &= 0xf7;
                for (i = 0; i < 0xa0; i++) {
                    ((uint32_t *)controls_input_capture_buffer)[i] = 0;
                }
                controls_capture_row = -1;
            }

            {
                uint8_t payload = 1;
                uint8_t *payload_ptr = &payload;
                int32_t encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x16, 0, (void **)&payload_ptr, 0, 1, 0);
                if (encoded_bits > 0) {
                    network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46db70), from tools/pack.py 0x46db70:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046db70(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  bool bVar7;
  int *local_20;
  undefined4 local_1c;
  undefined4 *local_18;
  undefined4 local_14;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  iVar4 = 0;
  uVar1 = DAT_0087a480 ^ 0x69746572;
  do {
    iVar3 = 0;
    local_10 = DAT_0087a480;
    local_c = 0;
    local_8 = 0xffffffff;
    iVar5 = 0;
    local_4 = uVar1;
    iVar2 = data_iterator_next();
    while (iVar2 != 0) {
      if ((*(int *)(iVar2 + 0x20) == iVar4) && (*(char *)(iVar2 + 0xd5) == '\0')) {
        iVar2 = (int)*(short *)(iVar2 + 0xc6);
        if (DAT_006f1d08 == 0) {
          if ((iVar3 == 0) || (iVar2 < iVar5)) {
LAB_0046dbfa:
            iVar5 = iVar2;
          }
        }
        else if (DAT_006f1d08 == 1) {
          if ((iVar3 == 0) || (iVar5 <= iVar2)) goto LAB_0046dbfa;
        }
        else {
          iVar5 = iVar5 + iVar2;
        }
        iVar3 = iVar3 + 1;
      }
      iVar2 = data_iterator_next();
    }
    bVar7 = DAT_006f1d08 == 2;
    (&DAT_006b1318)[iVar4] = iVar5;
    if (bVar7) {
      (&DAT_006b1318)[iVar4] = (&DAT_006b1358)[iVar4] + iVar5;
    }
    iVar4 = iVar4 + 1;
  } while (iVar4 < 0x10);
  FUN_00466cb0(0xffffffff);
  local_20 = &DAT_006b1318;
  do {
    if (((DAT_006f1ce0 <= *local_20) && (DAT_00719720 == 2)) && (DAT_0087aa10 == 0)) {
      *(undefined1 *)(DAT_0071c2d4 + 0xa0f) = 1;
      DAT_0087aa10 = 1;
      _DAT_0087aa08 = 0x40e00000;
      if (DAT_00688329 == '\0') {
        iVar4 = *(int *)(DAT_00746fa0 + 0x168);
LAB_0046dcf6:
        if (((iVar4 != 0) && (1 < *(int *)(iVar4 + 0x5c))) &&
           ((*(int *)(iVar4 + 0x60) != -0x10 && (*(int *)(*(int *)(iVar4 + 0x60) + 0x1c) != -1)))) {
          FUN_00543dd0(0x3f800000);
        }
      }
      else {
        iVar4 = game_engine_get_multiplayer_sound_duration_ticks();
        if (DAT_006b1140 < 5) {
          iVar3 = DAT_006b1140 * 0x10;
          (&DAT_006b10f0)[DAT_006b1140 * 4] = 0xffffffff;
          (&DAT_006b10f4)[DAT_006b1140 * 4] = 1;
          DAT_006b1140 = DAT_006b1140 + 1;
          *(int *)(&DAT_006b10f8 + iVar3) = iVar4 + 5;
          (&DAT_006b10fc)[iVar3] = 0;
        }
        if (DAT_006b1140 == 1) {
          iVar4 = *(int *)(DAT_00746fa0 + 0x168);
          goto LAB_0046dcf6;
        }
      }
      if (DAT_00718f94 != 0) {
        widget_close(DAT_00718f94);
      }
      if (DAT_00718f98 != 0) {
        FUN_004994b0();
      }
      _DAT_00718fa6 = 0;
      if (DAT_006953e8 != -1) {
        DAT_00712542 = DAT_00712542 & 0xf7;
        puVar6 = &DAT_00712544;
        for (iVar4 = 0xa0; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar6 = 0;
          puVar6 = puVar6 + 1;
        }
        DAT_006953e8 = -1;
      }
      local_18 = &local_1c;
      local_1c = 1;
      local_14 = 0;
      iVar4 = message_delta_encode_message(0,0x16,0,&local_18,0,1,'\0');
      if (0 < iVar4) {
        FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
      }
    }
    local_20 = local_20 + 1;
    if (0x6b1357 < (int)local_20) {
      return;
    }
  } while( true );
}
#endif

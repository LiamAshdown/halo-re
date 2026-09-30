// game_engine_players_update_client  (Ghidra: FUN_00474590; named per this rewrite)
// address 0x474590, size 1034 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Client-side counterpart of
//   game_engine_players_update: applies the locally staged update and processes respawn/action
//   handling without dequeuing the server queue"); mirrors src/game/game_engine_players_update_server.c
//   (this batch, 0x4740a0) closely for the respawn and unit_control_data-build logic (see that
//   file for the corresponding struct-field evidence); update_client_distribute_staged_entry.c
//   (0x473270, an earlier batch) for the per-player player_action array this fills.
//   objdump -d -M intel --start-address=0x474590 --stop-address=0x474710 bin/halo.exe was read
//   by hand for the three-way "which action record for this player" selection and the
//   player_update_queue_pop_current / player_apply_first_position_update call, which Ghidra's own rendering (unusually, for this function)
//   gets right in outline but splits across several oddly-named locals.
// register convention: no arguments.
//
// CORRECTED: as in game_engine_players_update_server.c, Ghidra shows game_engine_resolve_player_team,
// player_respawn and game_engine_apply_player_grenade_counts called with zero arguments; the
// disassembly shows EAX loaded with the player handle before each, matching their established
// convention.
// UNSURE: `carried_weapon_index` / `carried_grenade_or_zoom` are two dwords Ghidra's own decompile
// (local_2c8 / local_2cc) only ever explicitly zeroes on the "no queued or current update at all"
// path; on every other path they keep whatever value they held out of the PREVIOUS player's
// iteration (or, on the very first player, uninitialized stack). That is transcribed as-is
// (persisting across loop iterations, seeded to 0 here rather than left truly uninitialized).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"
#include <string.h>
#include <stdint.h>

// player_update_record is types/game.h's (0x2c: field0, references_remaining,
// reference_count, player_action action).

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0
extern int16_t network_game_mode;            // 0x00719720 (UNSURE: not referenced directly by
                                              //   this function, kept for parity with the server twin)
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t ui_split_screen;              // 0x00718fc9, UNSURE identity
extern uint8_t global_00719750;              // 0x00719750, UNSURE identity
extern int16_t global_00719772;              // 0x00719772, UNSURE identity
extern real_vector3d global_origin3d;        // 0x0065c230


extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

extern void player_apply_first_position_update(uint32_t field0, player *plr); // this batch, 0x476cf0;
    // blam-cc: EDI -> field0, ESI -> plr


extern void player_respawn(datum_index player_handle); // this module's next batch, 0x477ea0
extern void player_apply_pickup_effect(datum_index player_handle, datum_index item_index); // this batch, 0x479930
extern void unit_release_selected_equipment(datum_index unit_handle); // 0x56d300, units module, not in this batch
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
                                                real_vector3d *out_forward); // this batch, 0x473d70
extern void unit_apply_control_block(void *record_or_field, int32_t grenade_value); // 0x5639f0, units module,
    // not in this batch; blam-cc: EDX -> record_or_field, ECX -> grenade_value
extern void game_engine_build_visible_cluster_bitmask(void *out_bitmask, uint32_t flag); // this module's next batch, 0x4782a0

// Client-side per-tick players update: fills a 16-entry player_action array from this machine's
// own staged local input (update_client_distribute_staged_entry); if there is none staged, does
// nothing. For each player, picks its action record for this tick: the freshly staged one if the
// player IS this machine's local player, otherwise whatever player_update_queue_pop_current peeks off its
// update_history queue, or its already-latched "current" record, or an all-zero record if
// neither is available. Handles respawn exactly as game_engine_players_update_server, then --
// for a player whose unit has object flags bit 6 set -- either builds a unit_control_data from
// the selected action record and the unit's freshly computed forward vector (when this machine
// is running the player simulation locally), or one seeded from the unit's own desired vectors
// (when idle and unowned by any actor/swarm), and hands it to unit_apply_control_block.
void game_engine_players_update_client(void)
{
    player_action actions[16];
    int32_t counter;
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    player_action current_action;
    uint32_t carried_weapon_index;
    uint32_t carried_grenade_or_zoom;

    memset(actions, 0, sizeof(actions));
    if (update_client_distribute_staged_entry((uint8_t *)actions) == 0) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    counter = 0;
    carried_weapon_index = 0;
    carried_grenade_or_zoom = 0;

    plr = (player *)data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        if (plr->local_player_index == -1) {
            player_update_record peek;
            if (player_update_queue_pop_current(&peek, &plr->update_history) == 1) {
                current_action = peek.action;
                if (peek.references_remaining == peek.reference_count - 1) {
                    player_apply_first_position_update(peek.field0, plr);
                }
            } else if (*(uint8_t *)&plr->update_history.has_current == 1) {
                current_action = *(player_action *)plr->update_history.current;
            } else {
                current_action.control_flags = 0;
                memset((uint8_t *)&current_action + 4, 0, sizeof(current_action) - 4);
                carried_weapon_index = 0;
                carried_grenade_or_zoom = 0;
            }
        } else {
            current_action = actions[counter];
        }
        counter = counter + 1;
        player_handle = player_iter.index;

        if (plr->unit == (datum_index)-1) {
            if (current_game_engine == 0) {
                if (ui_split_screen == 0) {
                    if (plr->deaths == 0) {
                        player_respawn(player_handle);
                    } else if (local_player_globals->no_player_has_a_unit == 0) {
                        global_00719750 = 1;
                        if (local_player_globals->unknown_16 != 0) {
                            global_00719772 = 0x5b;
                        }
                    }
                }
            } else if (game_engine_player_ready_to_respawn(player_handle) != 0) {
                game_engine_resolve_player_team(player_handle);
                player_respawn(player_handle);
                if (plr->unit == (datum_index)-1) {
                    plr->respawn_timer = 1;
                } else {
                    game_engine_apply_player_grenade_counts(player_handle);
                }
            }
        }

        if (plr->unit != (datum_index)-1) {
            object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

            if ((unit->flags & 0x40) != 0) { // UNSURE: unnamed unit_flags bit 6
                if (local_player_globals->input_disabled == 0) {
                    unit_control_data ctrl;

                    if ((current_action.control_flags & 0x80) != 0 &&
                        unit->equipment_object_index != (datum_index)-1) {
                        player_apply_pickup_effect(player_handle, unit->equipment_object_index);
                        unit_release_selected_equipment(plr->unit);
                    }

                    memset(&ctrl, 0, sizeof(ctrl));
                    ctrl.control_flags = (uint16_t)current_action.control_flags;
                    player_compute_view_forward_vector(player_handle, &current_action.desired_yaw,
                                                        &ctrl.facing_vector);
                    ctrl.throttle.i = current_action.throttle_x;
                    ctrl.throttle.j = current_action.throttle_y;
                    ctrl.throttle.k = 0.0f;
                    ctrl.primary_trigger = current_action.primary_trigger;
                    ctrl.weapon_index = (int16_t)carried_grenade_or_zoom;
                    ctrl.grenade_index = (int16_t)(carried_grenade_or_zoom >> 16);
                    ctrl.zoom_level = (int16_t)carried_weapon_index;
                    ctrl.unknown_0a = 0;
                    ctrl.animation_state = 3;
                    ctrl.aiming_speed = 0;
                    unit_apply_control_block(&ctrl, 0);
                } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                    unit_control_data ctrl;

                    memset(&ctrl, 0, sizeof(ctrl));
                    ctrl.animation_state = 3;
                    ctrl.aiming_speed = 0;
                    ctrl.control_flags = 0;
                    ctrl.weapon_index = -1;
                    ctrl.grenade_index = -1;
                    ctrl.zoom_level = -1;
                    ctrl.throttle = global_origin3d;
                    ctrl.primary_trigger = 0.0f;
                    ctrl.facing_vector = unit->desired_facing_vector;
                    ctrl.aiming_vector = unit->desired_aiming_vector;
                    ctrl.looking_vector = unit->desired_looking_vector;
                    unit_apply_control_block(&ctrl, 0);
                }
            }
        }

        plr = (player *)data_iterator_next(&player_iter);
    }

    game_engine_build_visible_cluster_bitmask((uint8_t *)local_player_globals + 0x58, 1);
    game_engine_build_visible_cluster_bitmask((uint8_t *)local_player_globals + 0x18, 0);
    local_player_globals->local_player_count = (int16_t)(local_player_globals->local_players[0] != (datum_index)-1);
}

#if 0
Original Ghidra decompilation (0x474590), from tools/pack.py 0x474590:

void FUN_00474590(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  undefined4 local_2e4 [6];
  undefined4 local_2cc;
  undefined4 local_2c8;
  uint local_2c4;
  undefined2 local_2c0;
  undefined4 local_2bc;
  uint local_2b8;
  undefined1 local_2b4;
  undefined1 local_2b3;
  undefined2 local_2b2;
  undefined2 local_2b0;
  undefined2 local_2ae;
  undefined2 local_2ac;
  undefined4 local_2a8;
  undefined4 local_2a4;
  undefined4 local_2a0;
  undefined4 local_29c;
  undefined4 local_298;
  undefined4 local_294;
  undefined4 local_290;
  undefined4 local_28c;
  undefined4 local_288;
  undefined4 local_284;
  undefined4 local_280;
  undefined4 local_27c;
  undefined4 local_278;
  undefined1 local_274;
  undefined1 local_273;
  undefined2 local_272;
  undefined2 local_270;
  undefined2 local_26e;
  undefined2 local_26c;
  undefined4 local_268;
  undefined4 local_264;
  undefined4 local_260;
  undefined4 local_25c;
  undefined4 local_258;
  undefined4 local_254;
  undefined4 local_250;
  undefined4 local_24c;
  undefined4 local_248;
  undefined4 local_244;
  undefined4 local_240;
  undefined4 local_23c;
  undefined4 local_238;
  int local_22c;
  int local_228;
  undefined4 local_224 [8];
  undefined4 local_204 [129];

  local_204[0] = 0;
  puVar8 = local_204;
  for (iVar5 = 0x7f; puVar8 = puVar8 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar8 = 0;
  }
  cVar3 = FUN_00473270(local_204);
  if (cVar3 != '\0') {
    local_2c4 = DAT_0087a480;
    local_2b8 = DAT_0087a480 ^ 0x69746572;
    sVar4 = 0;
    local_2c0 = 0;
    local_2bc = 0xffffffff;
    iVar5 = data_iterator_next();
    iVar6 = DAT_0087a478;
    while (DAT_0087a478 = iVar6, iVar5 != 0) {
      if (*(short *)(iVar5 + 2) == -1) {
        cVar3 = FUN_00479fb0();
        iVar6 = local_22c;
        if (cVar3 == '\x01') {
          puVar8 = local_224;
          puVar9 = local_2e4;
          for (iVar7 = 8; iVar7 != 0; iVar7 = iVar7 + -1) {
            *puVar9 = *puVar8;
            puVar8 = puVar8 + 1;
            puVar9 = puVar9 + 1;
          }
          if (iVar6 == local_228 + -1) {
            FUN_00476cf0();
          }
        }
        else {
          if (*(char *)(iVar5 + 0x138) == '\x01') {
            puVar8 = (undefined4 *)(iVar5 + 0x13c);
            goto LAB_00474617;
          }
          local_2c8 = 0;
          local_2e4[0] = 0;
          local_2e4[1] = 0;
          local_2e4[2] = 0;
          local_2e4[3] = 0;
          local_2e4[4] = 0;
          local_2e4[5] = 0;
          local_2cc = 0;
        }
      }
      else {
        puVar8 = local_204 + sVar4 * 8;
LAB_00474617:
        puVar9 = local_2e4;
        for (iVar6 = 8; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar9 = *puVar8;
          puVar8 = puVar8 + 1;
          puVar9 = puVar9 + 1;
        }
      }
      uVar2 = local_2bc;
      uVar1 = local_2c8;
      sVar4 = sVar4 + 1;
      if (*(int *)(iVar5 + 0x34) == -1) {
        if (DAT_006f1d20 == 0) {
          if (DAT_00718fc9 == '\0') {
            if (*(short *)(iVar5 + 0xae) == 0) {
              player_respawn();
            }
            else if ((*(char *)(DAT_0087a478 + 0x10) == '\0') &&
                    (DAT_00719750 = 1, *(char *)(DAT_0087a478 + 0x16) != '\0')) {
              DAT_00719772 = 0x5b;
            }
          }
        }
        else {
          cVar3 = game_engine_player_ready_to_respawn();
          if (cVar3 != '\0') {
            FUN_004611b0();
            player_respawn();
            if (*(int *)(iVar5 + 0x34) == -1) {
              *(undefined4 *)(iVar5 + 0x2c) = 1;
            }
            else {
              game_engine_apply_player_grenade_counts();
            }
          }
        }
      }
      if ((*(uint *)(iVar5 + 0x34) != 0xffffffff) &&
         (iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                          (*(uint *)(iVar5 + 0x34) & 0xffff) * 0xc),
         (*(uint *)(iVar5 + 0x204) >> 6 & 1) != 0)) {
        if (*(char *)(DAT_0087a478 + 0x11) == '\0') {
          if (((char)local_2e4[0] < '\0') && (*(int *)(iVar5 + 0x318) != -1)) {
            FUN_00479930(uVar2,*(int *)(iVar5 + 0x318));
            FUN_0056d300();
          }
          local_2b2 = (undefined2)local_2e4[0];
          FUN_00473d70();
          local_278 = local_284;
          local_290 = local_284;
          local_27c = local_288;
          local_294 = local_288;
          local_280 = local_28c;
          local_298 = local_28c;
          local_2a4 = local_2e4[4];
          local_2a8 = local_2e4[3];
          local_29c = local_2e4[5];
          local_2ae = local_2cc._2_2_;
          local_2a0 = 0;
          local_2b4 = 3;
          local_2b0 = (undefined2)local_2cc;
          local_2ac = (undefined2)uVar1;
          local_2b3 = 0;
        }
        else {
          if ((*(int *)(iVar5 + 0x1f8) != -1) || (*(int *)(iVar5 + 500) != -1)) goto LAB_00474949;
          local_268 = *(undefined4 *)PTR_DAT_00696714;
          local_264 = *(undefined4 *)(PTR_DAT_00696714 + 4);
          local_260 = *(undefined4 *)(PTR_DAT_00696714 + 8);
          local_258 = *(undefined4 *)(iVar5 + 0x224);
          local_254 = *(undefined4 *)(iVar5 + 0x228);
          local_250 = *(undefined4 *)(iVar5 + 0x22c);
          local_24c = *(undefined4 *)(iVar5 + 0x230);
          local_248 = *(undefined4 *)(iVar5 + 0x234);
          local_244 = *(undefined4 *)(iVar5 + 0x238);
          local_240 = *(undefined4 *)(iVar5 + 0x254);
          local_23c = *(undefined4 *)(iVar5 + 600);
          local_238 = *(undefined4 *)(iVar5 + 0x25c);
          local_270 = 0xffff;
          local_26e = 0xffff;
          local_26c = 0xffff;
          local_274 = 3;
          local_273 = 0;
          local_272 = 0;
          local_25c = 0;
        }
        FUN_005639f0(0xffffffff);
      }
LAB_00474949:
      iVar5 = data_iterator_next();
      iVar6 = DAT_0087a478;
    }
    FUN_004782a0(iVar6 + 0x58,1);
    FUN_004782a0(iVar6 + 0x18,0);
    *(ushort *)(iVar6 + 0xc) = (ushort)(*(int *)(iVar6 + 4) != -1);
  }
  return;
}
#endif

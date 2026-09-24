// game_engine_players_update_server  (Ghidra: FUN_004740a0; named per this rewrite)
// address 0x4740a0, size 1262 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Main per-tick players update used on the
//   server/single-player path: applies queued client input, handles respawning, and processes
//   each player's action flags"); types/game.h player_action (0x20 bytes, this batch's
//   update_client_queue_apply_tick.c's "out_a"), player (deaths +0xae, respawn_timer +0x2c,
//   unit +0x34), player_globals (unknown_16 +0x16, unknown_0c +0x0c); types/units.h unit_data
//   (flags +0x204 bit 6, equipment_object_index +0x318, current_weapon_index +0x2f2,
//   weapons[] +0x2f8, desired_facing_vector +0x224, desired_aiming_vector +0x230,
//   desired_looking_vector +0x254), unit_control_data, unit_control_flags (action 0x40,
//   primary/secondary trigger 0x1800, exchange_weapon 0x4000); types/objects.h object (flags
//   +0x04, parent_object +0x11c). objdump -d -M intel --start-address=0x4740a0
//   --stop-address=0x474590 bin/halo.exe was read by hand for this whole function: Ghidra's own
//   C is unusually unreliable here (see the two CORRECTED notes below), because the function
//   never establishes an EBP frame and Ghidra's stack-slot tracker loses track of several
//   ESP-relative locals across the two dozen intervening CALLs, splitting one 16-entry,
//   0x10-byte-per-player array into three differently-typed "local_2fc" / "local_303" /
//   "local_304" pseudo-variables and, separately, mistaking every CALL instruction's own pushed
//   return address for an assignment into a stack variable (shown as e.g.
//   "pcStack_3b8 = (char *)0x4740f1").
//
// CORRECTED: Ghidra shows FUN_004611b0 (game_engine_resolve_player_team), player_respawn and
// game_engine_apply_player_grenade_counts called with zero visible arguments in this function;
// the disassembly shows EAX loaded with the player handle (or player index) immediately before
// every one of those calls, matching each function's own already-established "player index/
// handle in EAX" convention, and is passed explicitly here.
// CORRECTED: Ghidra renders `build_remote_player_transform_update(0xffffffff, uVar3, iVar2)` from its confused view of
// the per-player carry array; the real arguments (read off the disassembly) are the player's own
// handle, and the carry record's dwords at +4 and +8 (not the -1 sentinel Ghidra invented).
//
// UNSURE (pervasive through the back half): the per-player "client_update_carry" record's byte
// and dword fields (only ever produced by update_client_queue_apply_tick.c's own equally-UNSURE
// "out_b" output) are given placeholder names; the two divergent code paths that each build a
// unit_control_data on the stack before calling unit_apply_control_block were reconstructed field-by-field
// from the disassembly, but the second (network-idle) path's exact byte alignment against the
// struct base was not independently re-verified byte-for-byte, only through its own internal
// consistency (animation_state/aiming_speed/control_flags packed into one dword, throttle/
// facing_vector/aiming_vector/looking_vector following in order).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>

// client_update_carry is types/game.h's (0x10; every field still UNSURE, see that header).

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0
extern tag_instance *tag_instances;          // 0x0087bc14
extern int16_t network_game_mode;            // 0x00719720
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t global_00718fc9;              // 0x00718fc9, UNSURE identity
extern uint8_t global_00719750;              // 0x00719750, UNSURE identity, set to 1
extern int16_t global_00719772;              // 0x00719772, UNSURE identity, set to 0x5b
extern real_vector3d global_origin3d;        // 0x0065c230, reached via global_origin3d_pointer
                                              //   @0x00696714 (see src/math/vector3d_rotate_toward_with_acceleration.c)

extern uint32_t update_client_queue_apply_tick(player_action *out_actions,
    client_update_carry *out_carry); // 0x4730d0; only AL is meaningful (see that file)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern uint8_t game_engine_player_ready_to_respawn(uint32_t player_index); // 0x460f70
extern void game_engine_resolve_player_team(uint32_t player_index); // 0x4611b0
extern void game_engine_apply_player_grenade_counts(uint32_t player_index); // 0x4613c0
extern void player_respawn(datum_index player_handle); // this module's next batch, 0x477ea0
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
                                                real_vector3d *out_forward); // this batch, 0x473d70
extern void build_remote_player_transform_update(datum_index player_handle, int32_t field1, int32_t field2); // 0x4e7b50,
    // networking module, not in this batch; UNSURE: also implicitly reads a 32-byte copy of the
    // player's player_action record staged on the caller's stack immediately below these args
extern uint8_t player_execute_pending_interaction(datum_index player_handle); // this batch, 0x4793a0, stack -> player_handle
extern uint8_t player_execute_weapon_drop_interaction(datum_index player_handle); // this batch, 0x4790d0, stack -> player_handle
extern void player_apply_pickup_effect(datum_index player_handle, datum_index item_index); // this batch, 0x479930
extern void unit_release_selected_equipment(datum_index unit_handle); // 0x56d300, units module, not in this batch;
    // blam-cc: EAX -> unit_handle; UNSURE full behavior
extern void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index); // 0x56dcd0,
    // blam-cc: param_1 -> event_byte, ECX -> unit_index
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern void unit_apply_control_block(void *record_or_field, int32_t grenade_value); // 0x5639f0, units module,
    // not in this batch; blam-cc: EDX -> record_or_field, ECX -> grenade_value; the function
    // that block-moves a unit_control_data into the unit (types/units.h)
extern void game_engine_build_visible_cluster_bitmask(void *out_bitmask, uint32_t flag); // this module's next batch, 0x4782a0

// Applies this tick's queued client update into a 16-entry player_action array plus a 16-entry
// carry-record array (update_client_queue_apply_tick); if that fails (no update ready), does
// nothing else. Otherwise walks every player: replays any queued grenade-throw/pickup
// notification from its carry record, respawns it if it has no unit (via the ready-to-respawn
// gate in a running multiplayer engine, or directly in single-player-like modes), and -- for a
// player that does have a unit whose object flags bit 6 is set -- processes its action/exchange-
// weapon/trigger input for this tick and hands a unit_control_data built from its player_action
// record (or, for an idle non-local, non-AI unit, a default-facing one) to unit_apply_control_block. Finally
// rebuilds the two encounter/squad-presence bitmasks and player_globals::unknown_0c.
void game_engine_players_update_server(void)
{
    player_action actions[16];
    client_update_carry carry[16];
    int32_t counter;
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    player_action *action;
    client_update_carry *entry;
    int32_t grenade_value;

    memset(actions, 0, sizeof(actions));
    memset(carry, 0, sizeof(carry));
    carry[0].flag_a = (uint8_t)-1; // matches the single explicit byte Ghidra's own init writes

    if ((uint8_t)update_client_queue_apply_tick(actions, carry) == 0) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    counter = 0;

    plr = (player *)data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        action = &actions[counter];
        entry = &carry[counter];
        grenade_value = -1;
        counter = counter + 1;
        player_handle = player_iter.index;

        if (entry->flag_a == 1) {
            if (network_game_mode == 2 && entry->field2 == entry->field3 + 1) {
                build_remote_player_transform_update(player_handle, entry->field1, entry->field2);
            }
            if (entry->flag_b == 1) {
                grenade_value = entry->field1;
            }
        }

        if (plr->unit == (datum_index)-1) {
            if (current_game_engine == 0) {
                if (global_00718fc9 == 0) {
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
                if (network_game_mode == 0) {
                    if (plr->unit == (datum_index)-1) {
                        plr->respawn_timer = 1;
                    } else {
                        game_engine_apply_player_grenade_counts(player_handle);
                    }
                }
            }
        }

        if (plr->unit != (datum_index)-1) {
            object *unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

            if ((unit->flags & 0x40) != 0) { // UNSURE: unnamed unit_flags bit 6
                if (local_player_globals->unknown_11 == 0) {
                    // Operates on action->control_flags in place, exactly as the disassembly
                    // re-reads it after the first OR (rather than shadowing it in a local).
                    if ((action->control_flags & _unit_control_flag_action) != 0 &&
                        unit_obj->parent_object == (datum_index)-1) {
                        if (player_execute_pending_interaction(player_handle) == 0) { // this batch, 0x4793a0
                            action->control_flags = action->control_flags | 0x400;
                        }
                    }
                    if ((action->control_flags & 0x4000) == 0 || unit_obj->parent_object != (datum_index)-1) {
                        *(uint8_t *)&plr->unknown_3e = 0;
                    } else if (*(uint8_t *)&plr->unknown_3e == 0) {
                        *(uint8_t *)&plr->unknown_3e = player_execute_weapon_drop_interaction(player_handle); // this batch, 0x4790d0
                    }

                    if ((action->control_flags & 0x80) != 0 && unit->equipment_object_index != (datum_index)-1) {
                        player_apply_pickup_effect(player_handle, unit->equipment_object_index);
                        unit_release_selected_equipment(plr->unit);
                    }

                    if (unit->current_weapon_index != -1) {
                        datum_index weapon_handle = unit->weapons[unit->current_weapon_index];
                        if (weapon_handle != (datum_index)-1) {
                            object *weapon_obj = ((object_header *)object_data->data)[weapon_handle & 0xffff].data;
                            Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                            if ((weapon_tag->weapon_flags & 0x08) != 0) { // must_be_readied
                                if ((action->control_flags & 0x1800) != 0) { // primary or secondary trigger
                                    if (unit_obj->network_role == 0) {
                                        unit_dispatch_scripted_event_1b(1, plr->unit);
                                    }
                                    unit_drop_current_weapon(plr->unit, 1);
                                }
                                action->weapon_index = unit->current_weapon_index;
                            }
                        }
                    }

                    {
                        real_vector3d forward;
                        unit_control_data ctrl;

                        player_compute_view_forward_vector(player_handle, &action->desired_yaw, &forward);
                        memset(&ctrl, 0, sizeof(ctrl));
                        ctrl.animation_state = 3;
                        ctrl.aiming_speed = 0;
                        ctrl.control_flags = (uint16_t)action->control_flags;
                        ctrl.weapon_index = action->weapon_index;
                        ctrl.grenade_index = action->grenade_index;
                        ctrl.zoom_level = action->zoom_level;
                        ctrl.throttle.i = action->throttle_x;
                        ctrl.throttle.j = action->throttle_y;
                        ctrl.throttle.k = 0.0f;
                        ctrl.primary_trigger = action->primary_trigger;
                        ctrl.facing_vector = forward;
                        unit_apply_control_block(&ctrl, grenade_value);
                    }
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
                    unit_apply_control_block(&ctrl, grenade_value);
                }
            }
        }

        plr = (player *)data_iterator_next(&player_iter);
    }

    game_engine_build_visible_cluster_bitmask((uint8_t *)local_player_globals + 0x58, 1);
    game_engine_build_visible_cluster_bitmask((uint8_t *)local_player_globals + 0x18, 0);
    local_player_globals->unknown_0c = (int16_t)(local_player_globals->local_players[0] != (datum_index)-1);
}

#if 0
Original Ghidra decompilation (0x4740a0), from tools/pack.py 0x4740a0 -- unreliable, see the
header comment for why (Ghidra's own confused view of the ESP-relative stack, treating pushed
return addresses as assigned locals):

void FUN_004740a0(void)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  char cVar5;
  undefined1 uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  short sVar11;
  uint *puVar12;
  uint *puVar13;
  undefined4 *puVar14;
  uint *puVar15;
  uint auStack_3d4 [5];
  undefined4 uStack_3c0;
  uint *puStack_3bc;
  char *pcStack_3b8;
  undefined4 local_39c;
  char local_304;
  undefined1 local_303 [7];
  int local_2fc [62];
  uint local_204 [6];
  undefined2 local_1ec [246];

  sVar11 = 0;
  local_204[0] = 0;
  puVar13 = local_204;
  for (iVar9 = 0x7f; puVar13 = puVar13 + 1, iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar13 = 0;
  }
  local_304 = -1;
  puVar14 = (undefined4 *)local_303;
  for (iVar9 = 0x3f; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar14 = 0;
    puVar14 = puVar14 + 1;
  }
  *(undefined2 *)puVar14 = 0;
  *(undefined1 *)((int)puVar14 + 2) = 0;
  pcStack_3b8 = &local_304;
  puStack_3bc = local_204;
  uStack_3c0 = 0x4740f1;
  cVar5 = FUN_004730d0();
  if (cVar5 != '\0') {
    /* ... see out/phase4/game_functions.md and the objdump this file's header cites for the
       real control flow; Ghidra's own rendering of the rest of this function mixes up three
       overlapping views of the same per-player array and several bogus "pcStack"/"puStack"
       assignments that are really just CALL's own return-address pushes. */
  }
  return;
}
#endif

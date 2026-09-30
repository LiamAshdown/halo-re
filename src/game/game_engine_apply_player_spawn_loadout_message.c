// game_engine_apply_player_spawn_loadout_message  (Ghidra: FUN_00477c70; renamed -- the network
// handler that applies an incoming "spawn loadout" message to a player, mirroring the local
// spawn path in player_respawn.c)
// address 0x477c70, size 555 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md; the same "envelope" shape as
//   game_engine_apply_player_join_message.c (this batch); `machine_table` (0x00687558+0x28,
//   already named in src/game/game_engine_handle_kill_feed_network_event.c) resolves a machine
//   id to a handle; `object_network_id_table` (0x00687130+0x28, already named across
//   src/objects/*.c and src/items/*.c) resolves a pooled node id to an object index;
//   types/game.h player (team 0x20, team_index 0x66, unit 0x34, local_player_index 0x02, deaths
//   0xae, kill_streak 0x68, interaction_type 0x28, interaction_object 0x24); types/objects.h
//   object (owner_linkage 0xc0, name_index 0xb8, network_role 0x04); types/units.h
//   unit_data::controlling_player (0x218 absolute), current_weapon_index (0x2f2),
//   desired_weapon_index (0x2f4), weapons[4] (0x2f8); unit_apply_starting_profile (0x473c50,
//   already rewritten) reused with the same profile-index selection logic as player_respawn.c;
//   game_engine_init_player_look_state_from_object / game_engine_apply_player_grenade_counts
//   (both already established) and this batch's player_add_kill_streak.
// register convention: an envelope in EAX (in_EAX, `**envelope == 0` gates acceptance exactly
//   like game_engine_apply_player_join_message.c); a player handle for datum_get and
//   player_add_kill_streak is elided by Ghidra at every use here (UNSURE which register carries
//   it -- modeled as a second explicit parameter for clarity).
// UNSURE: player_handle's real register/provenance (never shown by Ghidra in this function, only
//   inferred from datum_get's and player_add_kill_streak's own established shapes); object_try_
//   and_get's object argument here (the newly resolved unit index, `new_unit_object_index`);
//   the weapon-slot loop's exact effect when a slot IS valid (Ghidra shows the unit_pickup_weapon call
//   but never an explicit store into weapons[i] in that branch -- transcribed literally).
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"
#include "fn_units.h"

extern network_id_table *machine_table;
extern network_id_table *object_network_id_table; // 0x00687130
extern Scenario *global_scenario;            // 0x00746f8c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *object_data;           // 0x008603b0

extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680; UNSURE array argument
extern data_array *player_data; // 0x0087a480
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL

extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle,
    uint8_t reset_stats); // 0x473c50, already rewritten

extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack mode, EAX weapon, ECX unit


// Decodes an incoming spawn-loadout message and, once the target player and unit are both
// resolved, applies it: stamps the unit's owner/team fields, resets its look state (or, for a
// non-local player, its state timers), applies a starting profile in single player, applies
// grenade counts, wires up the four weapon inventory slots and desired weapon, optionally seats
// the unit in a vehicle, and applies any queued kill-streak deltas.
// blam-cc: EAX -> envelope
// FIXED 2026-09-28 (networking call audit, from the disassembly 0x477c70..0x477e95): there is no player_handle
// argument -- the player is the message's first field looked up in the player key table (0x687558 +0x28, EBX),
// the same handle the unit's owner fields, the grenade counts (0x4613c0, EAX) and the kill streaks (0x479ba0, EBX)
// get. The key tables are indexed through the pointer at +0x28 (the C added 0x28 to the table's own address), and
// each known weapon is picked up with unit_pickup_weapon(0, weapon, unit) (0x477ded: EAX weapon, ECX unit, stack 0).
void game_engine_apply_player_spawn_loadout_message(void **envelope)
{
    struct {
        int32_t machine_id;
        int32_t unit_pooled_id;
        int32_t team;
        int32_t seat_vehicle_pooled_id;
        int32_t seat_number;
        int32_t weapon_pooled_ids[4];
        int16_t desired_weapon_index;
        int16_t kill_streak_delta[2];
        int16_t pad;
    } message;

    if (*(int32_t *)*envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!message_delta_decode_compound_field(envelope, &message)) {
        return;
    }

    {
        datum_index owner_handle = (datum_index)0xffffffff;
        uint32_t player_handle;
        if (message.machine_id != 0) {
            owner_handle = (datum_index)(*(int32_t **)&machine_table->handles)[message.machine_id];
        }
        player_handle = (uint32_t)owner_handle;

        {
            player *p = (player *)datum_get(player_handle, player_data); // UNSURE: array argument
            if (p != 0 && message.unit_pooled_id != 0) {
                datum_index new_unit = (datum_index)((int32_t *)object_network_id_table->handles)[
                    message.unit_pooled_id];
                if (new_unit != (datum_index)0xffffffff) {
                    object *unit_obj = object_try_and_get(new_unit, 3);
                    if (unit_obj != 0) {
                        p->unit = new_unit;
                        p->team = message.team;
                        p->team_index = (int8_t)message.team;
                        unit_obj->owner_linkage = (uint32_t)owner_handle;
                        unit_obj->owner_team = (int16_t)p->team;
                        ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->controlling_player = owner_handle;
                        unit_refresh_targeting_flag_and_weapons(new_unit, 1); // CL = 1

                        if (p->local_player_index == -1) {
                            // UNSURE: raw player-record offsets, no header names these four
                            // dwords specifically as look-state timers.
                            ((struct player *)p)->position_updates.read_index = 0;
                            ((struct player *)p)->position_updates.write_index = 0;
                            ((struct player *)p)->vehicle_updates.read_index = 0;
                            ((struct player *)p)->vehicle_updates.write_index = 0;
                        } else {
                            unit_obj->network_role = 2;
                            game_engine_init_player_look_state_from_object(new_unit, p->local_player_index);
                        }

                        if (current_game_engine == 0 &&
                            ((global_scenario->player_starting_profile.count > 1 && p->deaths > 0) ||
                             global_scenario->player_starting_profile.count != 0)) {
                            int16_t starting_profile_index =
                                (global_scenario->player_starting_profile.count > 1 && p->deaths > 0) ? 1 : 0;
                            unit_apply_starting_profile(starting_profile_index, new_unit, 1);
                        }

                        // CORRECTED by review: the original clears a full DWORD at player+0x68
                        // ("*(undefined4 *)(iVar3 + 0x68) = 0"), which is BOTH kill_streak
                        // entries, not just slot 0 -- the same pair
                        // game_engine_apply_player_join_message.c already zeroes as two words.
                        p->kill_streak[0] = 0;
                        p->kill_streak[1] = 0;
                        p->interaction_type = 0;
                        p->interaction_object = (datum_index)0xffffffff;
                        game_engine_apply_player_grenade_counts(player_handle); // 0x477db2: EAX = the player handle

                        {
                            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset); // unit_data starts at object +0x1f4
                            int32_t i;
                            for (i = 0; i < 4; i++) {
                                int32_t weapon = message.weapon_pooled_ids[i] != 0
                                    ? ((int32_t *)object_network_id_table->handles)[message.weapon_pooled_ids[i]]
                                    : -1;
                                if (weapon == -1) {
                                    unit->weapons[i] = (datum_index)0xffffffff;
                                } else {
                                    unit_pickup_weapon(0, (uint32_t)weapon, new_unit);
                                }
                            }
                            unit->current_weapon_index = -1;
                            unit->desired_weapon_index = message.desired_weapon_index;
                        }

                        if (message.seat_vehicle_pooled_id != -1 && message.seat_vehicle_pooled_id != 0) {
                            datum_index vehicle = (datum_index)((int32_t *)object_network_id_table->handles)[
                                message.seat_vehicle_pooled_id];
                            if (vehicle != (datum_index)0xffffffff) {
                                unit_enter_vehicle_seat(vehicle, (int16_t)message.seat_number, p->unit); // 0x477e4b: EAX = player +0x34
                            }
                        }

                        if (0 < message.kill_streak_delta[0]) {
                            player_add_kill_streak(0, message.kill_streak_delta[0], player_handle);
                        }
                        if (0 < message.kill_streak_delta[1]) {
                            player_add_kill_streak(1, message.kill_streak_delta[1], player_handle);
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x477c70), from tools/pack.py 0x477c70:

void FUN_00477c70(void)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 *puStack_30;
  int local_2c;
  int local_28;
  undefined4 local_24;
  int iStack_20;
  undefined4 uStack_1c;
  int aiStack_18 [4];
  undefined2 uStack_8;
  short sStack_6;
  short sStack_4;
  undefined2 uStack_2;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (cVar2 != '\0') {
      uVar5 = 0xffffffff;
      if (local_2c != 0) {
        uVar5 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_2c * 4);
      }
      iVar3 = datum_get();
      if ((((iVar3 != 0) && (local_28 != 0)) &&
          (iVar1 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_28 * 4), iVar1 != -1)) &&
         (iVar4 = object_try_and_get(3), iVar4 != 0)) {
        *(int *)(iVar3 + 0x34) = iVar1;
        *(undefined4 *)(iVar3 + 0x20) = local_24;
        *(undefined1 *)(iVar3 + 0x66) = (undefined1)local_24;
        *(undefined4 *)(iVar4 + 0xc0) = uVar5;
        *(undefined2 *)(iVar4 + 0xb8) = *(undefined2 *)(iVar3 + 0x20);
        *(undefined4 *)(iVar4 + 0x218) = uVar5;
        FUN_00569bf0(iVar1);
        if (*(short *)(iVar3 + 2) == -1) {
          *(undefined4 *)(iVar3 + 0x180) = 0;
          *(undefined4 *)(iVar3 + 0x17c) = 0;
          *(undefined4 *)(iVar3 + 0x1e0) = 0;
          *(undefined4 *)(iVar3 + 0x1dc) = 0;
        }
        else {
          *(undefined4 *)(iVar4 + 4) = 2;
          game_engine_init_player_look_state_from_object();
        }
        if ((DAT_006f1d20 == 0) &&
           (((1 < *(int *)(global_scenario + 0x348) && (0 < *(short *)(iVar3 + 0xae))) ||
            (*(int *)(global_scenario + 0x348) != 0)))) {
          FUN_00473c50(1);
        }
        *(undefined4 *)(iVar3 + 0x68) = 0;
        *(undefined2 *)(iVar3 + 0x28) = 0;
        *(undefined4 *)(iVar3 + 0x24) = 0xffffffff;
        game_engine_apply_player_grenade_counts();
        puStack_30 = (undefined4 *)(iVar4 + 0x2f8);
        iVar3 = 0;
        do {
          if ((aiStack_18[iVar3] == 0) ||
             (*(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + aiStack_18[iVar3] * 4) == -1)) {
            *puStack_30 = 0xffffffff;
          }
          else {
            FUN_0056d400(0);
          }
          iVar3 = iVar3 + 1;
          puStack_30 = puStack_30 + 1;
        } while (iVar3 < 4);
        *(undefined2 *)(iVar4 + 0x2f2) = 0xffff;
        *(undefined2 *)(iVar4 + 0x2f4) = uStack_8;
        if (((iStack_20 != -1) && (iStack_20 != 0)) &&
           (iVar3 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + iStack_20 * 4), iVar3 != -1)) {
          unit_enter_vehicle_seat(iVar3,uStack_1c);
        }
        if (0 < sStack_6) {
          FUN_00479ba0(0,CONCAT22(sStack_4,sStack_6));
        }
        if (0 < sStack_4) {
          FUN_00479ba0(1,CONCAT22(uStack_2,sStack_4));
          return;
        }
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif

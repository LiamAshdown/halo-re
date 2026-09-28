// ctf_engine_flag_tick  (Ghidra: ctf_engine_flag_tick, already named)
// address 0x468bf0, size 1415 bytes
// name confidence: 0.55   rewrite confidence: 0.25
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x468bf0
//   --stop-address=0x469177): both parameters are ordinary EBP-relative stack arguments
//   ([ebp+0x8] flag handle, [ebp+0xc] flag object pointer), not register-passed. Confirms the
//   already-committed unit_get_weapon_object_index/unit_dispatch_scripted_event_1b/
//   unit_drop_current_weapon call shapes exactly (unit index and slot loaded through
//   player->unit and unit->current_weapon_index (0x2f2), types/units.h). types/objects.h
//   object.flags (_object_needs_cluster_update_bit 0x800, parent_object 0x11c),
//   item_data.flags/held_game_time (0x1f4/0x204); game_engine_find_player_holding_object
//   (0x468b50, this batch) confirmed called with EBX -> flag handle, matching its own header.
//   The two custom_waypoint_register (0x462260, already committed as `void`) calls are followed
//   by code that reads EAX as if it were a return value and compares it to -1 -- the disassembly
//   shows this is really the icon id that custom_waypoint_register's own internal hud_waypoint_arrow_find
//   lookup leaves live in EAX when it returns (custom_waypoint_register never touches EAX again
//   after that internal call), which the original binary relies on as an incidental side
//   channel. Modeled here by calling hud_waypoint_arrow_find a second time to recover the same value,
//   flagged UNSURE below.
// register convention: flag handle and flag object pointer are both ordinary stack parameters.
// UNSURE: (1) the two game_engine_broadcast_kill_feed_to_team calls near the end need three more
//   forwarded arguments (message_type/subject/broadcast) whose true registers are not resolved
//   past this function's own already-ambiguous local register reuse; modeled as local guesses.
//   (2) the "re-call hud_waypoint_arrow_find to recover the leaked icon id" modeling for the
//   custom_waypoint_register EAX artifact (see evidence). (3) object + 0xc0 (owner_linkage) read
//   here as a player handle, and object + 0xb8 read as a team index (the objects.h name_index /
//   team_index conflict PLAN.md flags).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "game.h"
#include <stdint.h>

extern int16_t network_game_mode;               // 0x00719720
extern game_variant game_engine_variant;        // 0x006f1c88 (ctf_value_80 aliased 0x006f1d08,
    // the per-map configured auto-return duration in ticks)
extern int32_t ctf_flag_auto_return_ticks;       // 0x006b0eb0, the live countdown
extern uint8_t ctf_single_flag_mode;             // 0x006b0ebc
extern data_array *player_data;                  // 0x0087a480
extern data_array *object_data;               // 0x008603b0
extern tag_instance *tag_instances;              // 0x0087bc14
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern player_globals *local_player_globals;       // 0x0087a478
extern uint8_t ctf_active_team;                  // 0x006b0eb8
extern game_time_globals *game_time;             // 0x006f1d6c
extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88
extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4
extern int32_t ctf_team_return_credit_ticks[2];  // 0x006b0ea8
extern datum_index ctf_team_flag_object[2];      // 0x006b0e90
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern void *data_iterator_next(data_iterator *iterator);                 // 0x4d05d0
extern void *datum_get(datum_index handle, data_array *array);            // 0x4d0680
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0
extern void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index); // 0x56dcd0
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force);          // 0x56dec0
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30
extern void object_delete(datum_index object_index);                       // 0x4f5bd0
extern void game_engine_ctf_respawn_team_flag(int32_t team, real_point3d *forwarded_position,
    uint16_t forwarded_name_index);                                        // 0x468430, this batch
extern void game_engine_ctf_notify_both_teams(int32_t team); // 0x468460, blam-cc: EAX team
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast); // 0x460ba0, blam-cc: ESI message_type, BL broadcast, stack team
extern void game_engine_ctf_reset_team_return_credit(uint32_t object_index);          // 0x468840, this batch
extern datum_index game_engine_find_player_holding_object(datum_index target_object);    // 0x468b50, this batch
extern uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position); // 0x4bd740
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position,
    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260
extern int16_t hud_waypoint_arrow_find(void); // 0x4af070, icon-name lookup; UNSURE exact identity

void ctf_engine_flag_tick(uint32_t flag_handle, object *flag_obj)
{
    item_data *item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
    int32_t team; // sVar1 -- object + 0xb8, UNSURE: name_index/team_index conflict
    int32_t other_team;
    datum_index holder_player_index;
    real_point3d item_position;
    uint8_t position_valid;

    if (network_game_mode == 2) {
        if (game_engine_variant.ctf_value_80 > 0) {
            if (ctf_flag_auto_return_ticks > 0) {
                ctf_flag_auto_return_ticks--;
            }
            if (ctf_flag_auto_return_ticks == 0) {
                if ((item->flags & _item_in_inventory_bit) != 0) {
                    if (ctf_single_flag_mode != 0 && *(int32_t *)&((struct object *)flag_obj)->owner_linkage != -1) {
                        player *carrier = (player *)datum_get(
                            (datum_index)((struct object *)flag_obj)->owner_linkage, player_data);
                        if (carrier != (player *)0) {
                            object *unit_obj = object_try_and_get(carrier->unit, _object_mask_unit);
                            if (unit_obj != (object *)0) {
                                unit_data *unit =
                                    (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                                datum_index current_weapon =
                                    unit_get_weapon_object_index((uint32_t)carrier->unit, unit->current_weapon_index);
                                if (current_weapon != (datum_index)flag_handle) {
                                    int32_t slot;
                                    for (slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
                                        if (unit->weapons[slot] == (datum_index)flag_handle) {
                                            unit->current_weapon_index = (int16_t)slot;
                                            unit_ready_desired_weapon((uint32_t)carrier->unit, 1);
                                            break;
                                        }
                                    }
                                }
                                current_weapon = unit_get_weapon_object_index(
                                    (uint32_t)carrier->unit, unit->current_weapon_index);
                                if (current_weapon == (datum_index)flag_handle) {
                                    unit_dispatch_scripted_event_1b(1, (uint32_t)carrier->unit);
                                    unit_drop_current_weapon((uint32_t)carrier->unit, 1);
                                }
                            }
                        }
                    }
                    goto notify_teams; // (item->flags & 1) is still set: skip straight to LAB_00468f37
                }

                {
                    data_iterator iter;
                    void *element;
                    iter.data = object_data; // UNSURE: iterates data_array of dropped-flag records
                    iter.next_index = 0;
                    iter.index = (datum_index)0xffffffff;
                    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                    element = data_iterator_next(&iter);
                    while (element != 0) {
                        chimera__kill_feed((datum_index)0xffffffff, 0x2d, (uint32_t)0xffffffff, 1, 0); // UNSURE arg shapes
                        element = data_iterator_next(&iter);
                    }
                }

                ctf_team_return_credit_active[0] = 0;
                ctf_team_return_credit_active[1] = 0;
                ctf_team_return_credit_ticks[0] = 0;
                ctf_team_return_credit_ticks[1] = 0;
                custom_waypoints[0] = (custom_waypoint){0};
                custom_waypoints[1] = (custom_waypoint){0};
                ctf_team_flag_object[team = ((struct object *)flag_obj)->owner_team] = (datum_index)0xffffffff;

                {
                    uint32_t toggled = (uint32_t)(((struct object *)flag_obj)->owner_team + 1) & 0x80000001;
                    if ((int32_t)toggled < 0) {
                        toggled = (toggled - 1 | 0xfffffffe) + 1;
                    }
                    object_delete((datum_index)flag_handle);
                    game_engine_ctf_respawn_team_flag((int32_t)toggled, (real_point3d *)0, 0); // UNSURE forwarded args
                    ctf_active_team = (uint8_t)toggled;
                    flag_handle = *(uint32_t *)((uint8_t *)&ctf_team_flag_object[0] + (int16_t)toggled * 4);
                    flag_obj = ((object_header *)object_data->data)[flag_handle & 0xffff].data;
                    item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
                    game_engine_queue_multiplayer_sound(0x25 + (((struct object *)flag_obj)->owner_team != 0), 0xffffffff, 1); // 0x468e5d..0x468e79
                    game_engine_ctf_reset_team_return_credit(flag_handle); // FIXED 2026-09-28: 0x468840 takes only EAX
                    custom_waypoints[2] = (custom_waypoint){0};
                    custom_waypoints[3] = (custom_waypoint){0};
                    ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;
                    game_engine_ctf_notify_both_teams((int32_t)toggled);
                }
            }
notify_teams:
            if (local_player_globals->local_players[0] != (datum_index)0xffffffff) {
                player *lp = (player *)((uint8_t *)player_data->data +
                    ((uint32_t)local_player_globals->local_players[0] & 0xffff) * sizeof(player));
                *(int32_t *)((uint8_t *)lp + 0x74) = (ctf_active_team == (uint8_t)lp->team) ? 0x31 : 0x30;
                *(int32_t *)((uint8_t *)lp + 0x78) = 0;
            }
        }
        // else: no auto-return configured; falls straight through to the shared tail below
    } else {
        if (game_engine_variant.ctf_value_80 > 0) {
            if (ctf_flag_auto_return_ticks > 0) {
                ctf_flag_auto_return_ticks--;
            }
            if (ctf_flag_auto_return_ticks == 0 && (item->flags & _item_in_inventory_bit) == 0) {
                ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;
            }
            goto notify_teams;
        }
        goto weapon_coordination;
    }

    // Common tail reached whenever hosting (whether or not the auto-return branch ran).
    if (game_time->game_time - item->held_game_time <= 0x1fe) {
        goto weapon_coordination;
    }
    {
        int16_t obj_type = *(int16_t *)tag_instances[(uint32_t)flag_obj->definition_tag & 0xffff].data;
        object_type_definition *type_def = object_type_definitions[obj_type];
        if ((*(uint32_t *)((uint8_t *)type_def + 0x308) >> 3 & 1) == 0) { // UNSURE: tag-data offset 0x308 bit3
            goto weapon_coordination;
        }
    }
    if ((flag_obj->flags & _object_needs_cluster_update_bit) == 0) {
        goto weapon_coordination;
    }
    if (flag_obj->parent_object != (datum_index)0xffffffff) {
        goto weapon_coordination;
    }
    team = ((struct object *)flag_obj)->owner_team;
    {
        uint32_t toggled = (uint32_t)(team + 1) & 0x80000001;
        if ((int32_t)toggled < 0) {
            toggled = (toggled - 1 | 0xfffffffe) + 1;
        }
        other_team = (int32_t)toggled;
    }
    if ((*(uint8_t *)((uint8_t *)flag_obj + 0x22c) & 0x40) != 0) {
        // UNSURE: broadcast_enabled/message_type/subject/broadcast below are local
        // reconstructions -- see header note (1).
        game_engine_queue_multiplayer_sound(team != 0 ? 9 : 0xc, 0xffffffff, 1); // 0x469026..0x469037
        ctf_team_return_credit_active[team] = 0;
        ctf_team_return_credit_ticks[team] = 0;
        // FIXED 2026-09-28: 0x469049..0x469075 load ESI = 0x2b / 0x2c and BL = 1.
        game_engine_broadcast_kill_feed_to_team(0x2b, team, 1);
        game_engine_broadcast_kill_feed_to_team(0x2c, other_team, 1);
        // FIXED 2026-09-28: 0x46907d..0x469080 then resets the credit for the flag (the first argument).
        game_engine_ctf_reset_team_return_credit(flag_handle);
    }

weapon_coordination:
    holder_player_index = game_engine_find_player_holding_object((datum_index)flag_handle);
    team = ((struct object *)flag_obj)->owner_team;
    {
        uint32_t toggled = (uint32_t)(team + 1) & 0x80000001;
        if ((int32_t)toggled < 0) {
            toggled = (toggled - 1 | 0xfffffffe) + 1;
        }
        other_team = (int32_t)toggled;
    }
    position_valid = item_get_effective_position((datum_index)flag_handle, &item_position);

    if ((game_engine_variant.ctf_value_80 < 1 || ctf_active_team == team) && position_valid == 1) {
        int16_t icon;

        custom_waypoint_register(holder_player_index, (int16_t)team, &item_position, 0.0f,
            (datum_index)0xffffffff, (int16_t)other_team);
        icon = hud_waypoint_arrow_find(); // UNSURE: recovers the value custom_waypoint_register's own
                                // internal call left live in EAX; see header note (2)

        if (icon != -1) {
            real_point3d other_stand = *ctf_team_flag_stand_position[other_team];
            custom_waypoint_register((datum_index)0xffffffff, (int16_t)(team + 2), &other_stand,
                0.3f, (datum_index)(uint16_t)icon, (int16_t)0xffffffff);
        } else {
            custom_waypoints[team + 2] = (custom_waypoint){0};
        }
    }
}

#if 0
Original Ghidra decompilation (0x468bf0), from tools/pack.py 0x468bf0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void ctf_engine_flag_tick(uint param_1,int param_2)

{
  short sVar1;
  short sVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  uint *puVar8;

  if (DAT_00719720 == 2) {
    if (0 < DAT_006f1d08) {
      if (0 < DAT_006b0eb0) {
        DAT_006b0eb0 = DAT_006b0eb0 + -1;
      }
      if (DAT_006b0eb0 == 0) {
        if ((*(byte *)(param_2 + 500) & 1) != 0) {
          if ((((DAT_006b0ebc != '\0') && (*(int *)(param_2 + 0xc0) != -1)) &&
              (iVar4 = datum_get(), iVar4 != 0)) && (iVar5 = object_try_and_get(3), iVar5 != 0)) {
            uVar6 = unit_get_weapon_object_index();
            if (uVar6 != param_1) {
              iVar7 = 0;
              puVar8 = (uint *)(iVar5 + 0x2f8);
              do {
                if (*puVar8 == param_1) {
                  *(short *)(iVar5 + 0x2f4) = (short)iVar7;
                  unit_ready_desired_weapon(*(undefined4 *)(iVar4 + 0x34),1);
                  break;
                }
                iVar7 = iVar7 + 1;
                puVar8 = puVar8 + 1;
              } while (iVar7 < 4);
            }
            uVar6 = unit_get_weapon_object_index();
            if (uVar6 == param_1) {
              FUN_0056dcd0(1);
              unit_drop_current_weapon(*(undefined4 *)(iVar4 + 0x34),1);
            }
          }
          if ((*(byte *)(param_2 + 500) & 1) != 0) goto LAB_00468f37;
        }
        iVar4 = data_iterator_next();
        while (iVar4 != 0) {
          chimera__kill_feed(0xffffffff,0x2d,0xffffffff,1);
          iVar4 = data_iterator_next();
        }
        DAT_006b0ea4 = 0;
        DAT_006b0ea8 = 0;
        DAT_006b0ea5 = 0;
        DAT_006b0eac = 0;
        DAT_006f1888 = 0;
        _DAT_006f18a8 = 0;
        DAT_006f188c = 0;
        _DAT_006f18ac = 0;
        DAT_006f1890 = 0;
        _DAT_006f18b0 = 0;
        DAT_006f1894 = 0;
        _DAT_006f18b4 = 0;
        DAT_006f1898 = 0;
        _DAT_006f18b8 = 0;
        DAT_006f189c = 0;
        _DAT_006f18bc = 0;
        DAT_006f18a0 = 0;
        _DAT_006f18c0 = 0;
        *(undefined4 *)(&DAT_006b0e90 + *(short *)(param_2 + 0xb8) * 4) = 0xffffffff;
        DAT_006f18a4 = 0;
        _DAT_006f18c4 = 0;
        uVar6 = (int)*(short *)(param_2 + 0xb8) + 1U & 0x80000001;
        if ((int)uVar6 < 0) {
          uVar6 = (uVar6 - 1 | 0xfffffffe) + 1;
        }
        FUN_004f5bd0();
        FUN_00468430();
        DAT_006b0eb8 = (byte)uVar6;
        param_1 = *(uint *)(&DAT_006b0e90 + (short)uVar6 * 4);
        param_2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
        game_engine_queue_multiplayer_sound(1);
        FUN_00468840();
        _DAT_006f18c8 = 0;
        _DAT_006f18e8 = 0;
        _DAT_006f18cc = 0;
        _DAT_006f18ec = 0;
        _DAT_006f18d0 = 0;
        _DAT_006f18f0 = 0;
        _DAT_006f18d4 = 0;
        _DAT_006f18f4 = 0;
        _DAT_006f18d8 = 0;
        _DAT_006f18f8 = 0;
        _DAT_006f18dc = 0;
        _DAT_006f18fc = 0;
        _DAT_006f18e0 = 0;
        _DAT_006f1900 = 0;
        _DAT_006f18e4 = 0;
        _DAT_006f1904 = 0;
        DAT_006b0eb0 = DAT_006f1d08;
        FUN_00468460();
      }
LAB_00468f37:
      if (*(uint *)(DAT_0087a478 + 4) != 0xffffffff) {
        iVar4 = (*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200;
        iVar5 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
        *(uint *)(iVar5 + 0x74) =
             0x31 - (uint)((uint)DAT_006b0eb8 !=
                          *(uint *)(iVar4 + 0x20 + *(int *)(DAT_0087a480 + 0x34)));
        *(undefined4 *)(iVar5 + 0x78) = 0;
      }
      goto LAB_00468f7b;
    }
  }
  else {
    if (0 < DAT_006f1d08) {
      if (0 < DAT_006b0eb0) {
        DAT_006b0eb0 = DAT_006b0eb0 + -1;
      }
      if ((DAT_006b0eb0 == 0) && ((*(byte *)(param_2 + 500) & 1) == 0)) {
        DAT_006b0eb0 = DAT_006f1d08;
      }
      goto LAB_00468f37;
    }
LAB_00468f7b:
    if (DAT_00719720 != 2) goto LAB_00469085;
  }
  if (((0x1fe < (uint)(*(int *)(DAT_006f1d6c + 0xc) - *(int *)(param_2 + 0x204))) &&
      ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc
                                       ) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1)
       != 0)) &&
     (((*(uint *)(param_2 + 0x10) >> 0xb & 1) != 0 && (*(int *)(param_2 + 0x11c) == -1)))) {
    sVar1 = *(short *)(param_2 + 0xb8);
    uVar6 = (int)sVar1 + 1U & 0x80000001;
    if ((int)uVar6 < 0) {
      uVar6 = (uVar6 - 1 | 0xfffffffe) + 1;
    }
    if ((*(byte *)(param_2 + 0x22c) & 0x40) != 0) {
      game_engine_queue_multiplayer_sound(1);
      sVar2 = *(short *)(param_2 + 0xb8);
      (&DAT_006b0ea4)[sVar2] = 0;
      (&DAT_006b0ea8)[sVar2] = 0;
      FUN_00460ba0((int)sVar1);
      FUN_00460ba0(uVar6);
    }
    FUN_00468840();
  }
LAB_00469085:
  iVar4 = FUN_00468b50();
  sVar1 = *(short *)(param_2 + 0xb8);
  uVar6 = (int)sVar1 + 1U & 0x80000001;
  if ((int)uVar6 < 0) {
    uVar6 = (uVar6 - 1 | 0xfffffffe) + 1;
  }
  cVar3 = item_get_effective_position();
  if (((DAT_006f1d08 < 1) || ((uint)DAT_006b0eb8 == (int)sVar1)) && (cVar3 == '\x01')) {
    FUN_00462260(0,0xffffffff,uVar6);
    if (iVar4 != -1) {
      FUN_00462260(0x3e99999a,iVar4,0xffffffff);
      return;
    }
    iVar4 = (int)(short)(sVar1 + 2);
    (&DAT_006f1888)[iVar4 * 8] = 0;
    (&DAT_006f188c)[iVar4 * 8] = 0;
    (&DAT_006f1890)[iVar4 * 8] = 0;
    (&DAT_006f1894)[iVar4 * 8] = 0;
    (&DAT_006f1898)[iVar4 * 8] = 0;
    (&DAT_006f189c)[iVar4 * 8] = 0;
    (&DAT_006f18a0)[iVar4 * 8] = 0;
    (&DAT_006f18a4)[iVar4 * 8] = 0;
  }
  return;
}
#endif

// player_respawn  (Ghidra: player_respawn, already named)
// address 0x477ea0, size 1010 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: PARTIALLY VERIFIED against the disassembly (objdump -d -M intel
//   --start-address=0x477ea0 --stop-address=0x478250). The main "spawn a fresh unit" path is
//   confirmed instruction-by-instruction: player_pick_random_starting_location (0x4776d0, this
//   batch) picks a starting-location index; game_get_player_starting_location (0x477640, outside this batch --
//   already identified by out/phase4/game_types_notes.md note 3 as "the function that indexes"
//   Scenario::player_starting_locations) resolves it to {x,y,z,facing}; the spawn tag is
//   GlobalsPlayerInformation[0]::unit (single player) or GlobalsMultiplayerInformation::unit
//   (current_game_engine != NULL), both TagDependency fields read straight out of the Globals
//   tag pointed to by global_globals (0x00746fa0, TagReflexive pointers at +0x168/+0x174 --
//   types/tags.h Globals::multiplayer_information/player_information); the placement's forward
//   vector is (cos(facing), sin(facing), 0) and its up vector defaults from
//   object_placement_default_up (0x00696720, already named in object_placement_data_initialize.c);
//   game_engine_get_player_color (0x463290, already named) supplies the color
//   object_placement_data_set_change_colors
//   (0x477670, outside this batch) then folds into the placement; the new object's
//   owner_linkage/network_role/name_index/controlling_player are stamped from the player exactly
//   like the pattern already established in player_spawn_starting_profile_weapon.c and
//   player_set_pending_interaction_action.c (this batch); unit_apply_starting_profile (0x473c50,
//   already rewritten) is invoked with a profile index chosen from
//   Scenario::player_starting_profile's count and the player's own death count; the closing
//   dedicated-server-only block re-serializes the unit's weapon loadout via
//   game_engine_send_unit_weapon_loadout (this batch) after a unit_build_network_update network encode.
//   The LEADING "despawn an existing local-player unit first" branch (only taken when no
//   multiplayer engine is loaded and this is a local player who somehow already has a unit) is
//   NOT independently re-verified past Ghidra's own decompilation; it is transcribed with raw
//   offsets and flagged UNSURE below rather than guessed at further.
// register convention: a player index in EAX (in_EAX, saved into EBX at entry and used
//   throughout); no stack parameters.
//   // blam-cc: EAX -> player_index
// UNSURE: the entire leading despawn branch (object_header/light-attachment bit twiddling on a
//   player's already-existing unit and its held weapon) is preserved as raw offsets exactly as
//   Ghidra decompiled it -- not independently confirmed by disassembly in this pass; local_player_
//   set_controlled_unit's arguments there (presumably local_player_index and -1, i.e. "clear");
//   game_get_player_starting_location / observer_new's exact signatures beyond what their one call site
//   here requires.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;                     // 0x0087a480
extern data_array *object_headers;                  // 0x008603b0
extern player_globals *local_player_globals;        // 0x0087a478
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;                   // 0x00719720
extern Globals *global_globals;                 // 0x00746fa0, UNSURE: identity as Globals*,
    // see header evidence (TagReflexive pointers at +0x168/+0x174 match multiplayer_information/
    // player_information)
extern Scenario *global_scenario;                   // 0x00746f8c
extern real_vector3d object_placement_default_up;   // 0x00696720
extern uint8_t shared_hud_text_draw_state;           // 0x00871de0
extern tag_instance *tag_instances;                 // 0x0087bc14

extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0
extern void object_delete(uint32_t object_index); // 0x4f5bd0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20
extern void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index); // 0x474fc0
extern int16_t player_pick_random_starting_location(datum_index player_handle); // this batch, 0x4776d0
extern float *game_get_player_starting_location(int32_t location_index); // 0x477640, not in this batch; UNSURE exact
    // signature; returns {x, y, z, facing}
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0
extern real *game_engine_get_player_color(uint32_t player_index, real *out_rgb); // 0x463290;
    // returns out_rgb (mov eax,esi at 0x4632ee); blam-cc: EAX -> player_index, ESI -> out_rgb
extern void object_placement_data_set_change_colors(real *color,
    object_placement_data *placement); // 0x477670, this module (VERIFIED against this very
    // call site, see that file); blam-cc: EAX -> color, ECX -> placement
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index); // 0x569bf0, not in this batch
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index); // 0x470e80, already established (src/game/game_engine_init_player_look_state_from_object.c)
extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN
extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle,
    uint8_t reset_stats); // 0x473c50, already rewritten (src/game/unit_apply_starting_profile.c)
extern void game_engine_apply_player_grenade_counts(uint32_t player_index); // 0x4613c0, established
    // (src/game/game_engine_apply_player_grenade_counts.c)
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, established
    // (src/game/game_engine_update_netgame_equipment.c)
extern int32_t unit_build_network_update(datum_index object_index, void *buffer, uint32_t buffer_size); // 0x55aed0, not in this batch
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2, uint32_t unknown_3,
    uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80
extern void game_engine_send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle,
    int32_t value, int32_t machine_index); // this batch, 0x477a80
extern void observer_new(uint32_t player_index); // 0x447740, not in this batch; UNSURE exact signature

// blam-cc: EAX -> player_index
// Respawns `player_index`'s unit. If this is a local player in single-player who already has a
// unit (UNSURE branch, see header note), despawns it and its held weapon first and returns
// early. Otherwise, in single-player or as the network server, picks a random starting
// location, resolves the correct default unit tag for the current game mode, builds an
// object_placement_data for it (position/facing from the location, color from the player) and
// spawns it, wiring it back into the player and (in single player) applying a starting weapon
// profile. On a dedicated server, additionally serializes the new unit's grenade counts and
// weapon loadout to observers. Always clears the player's pending kill-streak/interaction state
// at the end, and notifies the local-player look state on a local player.
void player_respawn(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (current_game_engine == 0 && p->local_player_index != -1) {
        // UNSURE: this whole branch (despawning an existing local-player unit) is transcribed
        // from Ghidra's own decompilation with raw offsets, not independently re-verified.
        datum_index existing_unit = local_player_globals->local_player_units[p->local_player_index];
        local_player_globals->local_player_units[p->local_player_index] = (datum_index)0xffffffff;

        if (existing_unit != (datum_index)0xffffffff) {
            object_header *existing_header = &((object_header *)object_headers->data)[existing_unit & 0xffff];
            object *unit_obj = existing_header->data;
            if ((*((uint8_t *)&unit_obj->vitality_flags) & 4) == 0) {
                unit_data *unit = (unit_data *)unit_obj;
                datum_index held_weapon = (datum_index)0xffffffff;
                int16_t current_weapon_index = unit->current_weapon_index;

                if (current_weapon_index != -1) {
                    held_weapon = unit->weapons[current_weapon_index];
                }
                object_mark_pending_delete(existing_unit);

                // UNSURE: the check here is against the Object TAG data
                // (tag_instances[unit_obj->definition_tag].data), not the runtime object -- the
                // tag field at +0x34 is not otherwise identified. Kept as a raw offset read
                // exactly as Ghidra decompiled it.
                if (*(int32_t *)((uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data + 0x34) != -1) {
                    if ((existing_header->flags & 1) != 0) {
                        object_for_each_light_attachment(existing_unit, 0, 1);
                    }
                    if (*(int32_t *)((uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data + 0x34) != -1) {
                        existing_header->flags = existing_header->flags & ~1u;
                    }
                }

                local_player_set_controlled_unit((datum_index)0xffffffff, p->local_player_index);

                if (held_weapon != (datum_index)0xffffffff) {
                    object_header *weapon_header = &((object_header *)object_headers->data)[held_weapon & 0xffff];
                    object *weapon_obj = weapon_header->data;
                    if (*(int32_t *)((uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data + 0x34) != -1) {
                        if ((weapon_header->flags & 1) != 0) {
                            object_for_each_light_attachment(held_weapon, 0, 1);
                        }
                        if (*(int32_t *)((uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data + 0x34) != -1) {
                            weapon_header->flags = weapon_header->flags & ~1u;
                        }
                    }
                }
                goto reset_player_state;
            }
            object_delete(existing_unit);
        }
    }

    if (network_game_mode == 2 || network_game_mode == 0) {
        int16_t location_index = player_pick_random_starting_location(player_index);
        if (location_index != -1) {
            GlobalsPlayerInformation *player_info = (GlobalsPlayerInformation *)global_globals->player_information.pointer;
            TagDependency *spawn_unit_tag = &player_info[0].unit;
            if (*(datum_index *)&spawn_unit_tag->tag_id != (datum_index)0xffffffff) {
            float *location = game_get_player_starting_location(location_index);

            if (current_game_engine != 0) {
                GlobalsMultiplayerInformation *mp_info = (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
                spawn_unit_tag = &mp_info->unit;
            }

            {
                object_placement_data placement;
                float color[3];
                datum_index new_unit;

                object_placement_data_initialize(&placement, *(datum_index *)&spawn_unit_tag->tag_id, (datum_index)0xffffffff);
                placement.position.x = location[0];
                placement.position.y = location[1];
                placement.position.z = location[2];
                placement.forward.i = (float)cos((double)location[3]);
                placement.forward.j = (float)sin((double)location[3]);
                placement.forward.k = 0.0f;
                placement.up = object_placement_default_up;

                game_engine_get_player_color(player_index, color);
                object_placement_data_set_change_colors(color, &placement);

                new_unit = object_new_with_datum_role_control(&placement, 3);
                if (new_unit != (datum_index)0xffffffff) {
                    object *unit_obj = object_try_and_get(new_unit, 3);
                    if (unit_obj != 0) {
                        unit_obj->owner_linkage = player_index;
                        unit_obj->name_index = (int16_t)p->team;
                        ((unit_data *)unit_obj)->controlling_player = (datum_index)player_index;
                        p->unit = new_unit;
                        unit_refresh_targeting_flag_and_weapons(new_unit);

                        if (p->local_player_index != -1) {
                            game_engine_init_player_look_state_from_object(new_unit, p->local_player_index);
                        }

                        {
                            int32_t profile_count = (int32_t)global_scenario->player_starting_profile.count;
                            if (profile_count != 0) {
                                int16_t starting_profile_index = 0;
                                if (profile_count > 1 && p->deaths > 0) {
                                    starting_profile_index = 1;
                                }
                                unit_apply_starting_profile(starting_profile_index, new_unit, 1);
                            }
                        }

                        if (network_game_mode == 2) {
                            game_engine_apply_player_grenade_counts(player_index);
                            unit_obj->network_role = 0;
                            object_type_override_call_0x68(new_unit);
                            {
                                int32_t encoded_bits = unit_build_network_update(new_unit, &shared_hud_text_draw_state, 0x7ff8);
                                if (0 < encoded_bits) {
                                    network_session_broadcast_to_flagged(1, &shared_hud_text_draw_state, 1, 0, 0, 3);
                                }
                            }
                            p->kill_streak[0] = 0;
                            game_engine_send_unit_weapon_loadout(new_unit, (datum_index)player_index, 0, -1);
                        }
                    }
                }
            }
            }
        }
    }

reset_player_state:
    p->kill_streak[0] = 0;
    p->interaction_type = 0;
    p->interaction_object = (datum_index)0xffffffff;
    if (p->local_player_index != -1) {
        observer_new(player_index);
    }
}

#if 0
Original Ghidra decompilation (0x477ea0), from tools/pack.py 0x477ea0:

void player_respawn(void)

{
  byte *pbVar1;
  uint uVar2;
  uint *puVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  float10 fVar11;
  uint local_b0;
  undefined1 local_9c [24];
  undefined4 local_84;
  undefined4 local_80;
  undefined4 local_7c;
  float local_68;
  float local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;

  iVar5 = (in_EAX & 0xffff) * 0x200;
  iVar8 = *(int *)(DAT_0087a480 + 0x34) + iVar5;
  if ((DAT_006f1d20 == 0) && (*(short *)(iVar8 + 2) != -1)) {
    uVar2 = *(uint *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4);
    *(undefined4 *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4) = 0xffffffff;
    if (uVar2 != 0xffffffff) {
      iVar9 = (uVar2 & 0xffff) * 0xc;
      if ((*(byte *)(*(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x106) & 4) == 0) {
        iVar7 = *(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
        sVar4 = *(short *)(iVar7 + 0x2f2);
        local_b0 = 0xffffffff;
        if (sVar4 != -1) {
          local_b0 = *(uint *)(iVar7 + 0x2f8 + sVar4 * 4);
        }
        object_mark_pending_delete();
        puVar3 = *(uint **)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
        iVar7 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(int *)(iVar7 + 0x34) != -1) {
          if ((puVar3[4] & 1) != 0) {
            object_for_each_light_attachment(0,1);
          }
          if (*(int *)(iVar7 + 0x34) != -1) {
            iVar7 = *(int *)(DAT_008603b0 + 0x34);
            puVar3[4] = puVar3[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar7 + iVar9 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
        }
        local_player_set_controlled_unit();
        if (local_b0 != 0xffffffff) {
          iVar7 = (local_b0 & 0xffff) * 0xc;
          puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
          iVar9 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar9 + 0x34) != -1) {
            if ((puVar3[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar9 + 0x34) != -1) {
              iVar9 = *(int *)(DAT_008603b0 + 0x34);
              puVar3[4] = puVar3[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar9 + iVar7 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
        }
        goto LAB_0047824b;
      }
      object_delete();
    }
  }
  if ((((DAT_00719720 == 2) || (DAT_00719720 == 0)) && (sVar4 = FUN_004776d0(), sVar4 != -1)) &&
     (iVar9 = *(int *)(*(int *)(DAT_00746fa0 + 0x174) + 0xc), iVar9 != -1)) {
    puVar6 = (undefined4 *)FUN_00477640();
    if (DAT_006f1d20 != 0) {
      iVar9 = *(int *)(*(int *)(DAT_00746fa0 + 0x168) + 0x1c);
    }
    object_placement_data_initialize(iVar9,0xffffffff);
    local_84 = *puVar6;
    local_80 = puVar6[1];
    local_7c = puVar6[2];
    fVar11 = (float10)fcos((float10)(float)puVar6[3]);
    local_60 = 0;
    local_68 = (float)fVar11;
    fVar11 = (float10)fsin((float10)(float)puVar6[3]);
    local_64 = (float)fVar11;
    local_5c = *(undefined4 *)PTR_DAT_00696720;
    local_58 = *(undefined4 *)(PTR_DAT_00696720 + 4);
    local_54 = *(undefined4 *)(PTR_DAT_00696720 + 8);
    game_engine_get_player_color();
    FUN_00477670();
    iVar9 = object_new_with_datum_role_control(local_9c,3);
    if ((iVar9 != -1) && (iVar7 = object_try_and_get(3), iVar7 != 0)) {
      iVar10 = *(int *)(DAT_0087a480 + 0x34) + iVar5;
      *(uint *)(iVar7 + 0xc0) = in_EAX;
      *(undefined2 *)(iVar7 + 0xb8) = *(undefined2 *)(iVar10 + 0x20);
      *(uint *)(iVar7 + 0x218) = in_EAX;
      *(int *)(iVar10 + 0x34) = iVar9;
      FUN_00569bf0(iVar9);
      if (*(short *)(iVar10 + 2) != -1) {
        game_engine_init_player_look_state_from_object();
      }
      if ((DAT_006f1d20 == 0) &&
         (((1 < *(int *)(global_scenario + 0x348) && (0 < *(short *)(iVar10 + 0xae))) ||
          (*(int *)(global_scenario + 0x348) != 0)))) {
        FUN_00473c50(1);
      }
      if (DAT_00719720 == 2) {
        game_engine_apply_player_grenade_counts();
        *(undefined4 *)(iVar7 + 4) = 0;
        object_type_override_call_0x68();
        iVar9 = FUN_0055aed0(iVar9,&DAT_00871de0,0x7ff8);
        if (0 < iVar9) {
          FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
        }
        *(undefined4 *)(iVar10 + 0x68) = 0;
        FUN_00477a80();
      }
    }
  }
LAB_0047824b:
  iVar9 = DAT_0087a480;
  *(undefined4 *)(iVar8 + 0x68) = 0;
  iVar5 = *(int *)(iVar9 + 0x34) + iVar5;
  *(undefined2 *)(iVar5 + 0x28) = 0;
  *(undefined4 *)(iVar5 + 0x24) = 0xffffffff;
  if (*(short *)(iVar8 + 2) != -1) {
    FUN_00447740();
  }
  return;
}
#endif

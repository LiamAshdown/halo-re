// game_engine_update_teleporter  (Ghidra: game_engine_update_teleporter, already named)
// address 0x461630, size 1096 bytes
// name confidence: 0.75   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Handles teleporting a unit through a level
// teleporter, finding and validating a destination and reporting failures"); the CEA/PDB string
// match on "failed to teleport %d"; types/tags.h ScenarioNetgameFlags (position, facing +0xc,
// usage_id +0x12) and Scenario::netgame_flags (the same +0x37c pointer this batch's
// game_engine_find_valid_starting_locations reads); types/game.h player::unknown_70 (cached
// entrance flag index), player::unknown_cc/unknown_d4 (teleported-into counter/flag, both
// UNRESOLVED offsets in the header, kept as raw offsets here since they fall inside the
// player struct's own "unresolved" run); types/objects.h object::forward (+0x74),
// object_set_position_and_orientation's canonical 4-argument form (src/objects/
// object_set_position_and_orientation.c).
// register convention: no register-passed arguments; param_1 is this function's own stack
// parameter (a player index).
// UNSURE (see individual comments below, this is the least certain file in this batch):
//  - The stack slot Ghidra renders as a single `float local_ac70` is genuinely reused for an
//    int32 search-result index (compared bit-for-bit against a `-NaN` sentinel, and multiplied
//    by 0x94 to index ScenarioNetgameFlags) AND, later, in what Ghidra prints as ordinary float
//    arithmetic feeding physics_model_build_from_sphere_query. Both readings cannot be literally true of one C variable;
//    modeled as an int32 `found_index` for the indexing/sentinel uses, and as
//    `(float)found_index` for the arithmetic use, which at least avoids inventing a value.
//  - physics_model_build_from_sphere_query, physics_shape_test_point, unit_get_crouch_height_offset and the constant-filled struct built for
//    player_effect_set_screen_flash_for_player are all far outside this batch's own address range and evidence; they are
//    preserved as literally as Ghidra's own argument lists allow, with placeholder types.
//  - object_set_position_and_orientation's own "up" argument is register-passed and not visible
//    here at all; NULL is substituted. Its "position" argument is likewise not visible, but the
//    refined destination computed a few lines earlier (`destination_position`) is passed
//    explicitly instead of inventing a NULL, since nothing else in this function produces a more
//    plausible candidate.
//  - RESOLVED (phase 4 review): 0x4726b0 is unit_get_local_player_weapon_index (the misattribution
//    note in out/phase4/game_types_notes.md is right; objdump confirms it returns
//    player::local_player_index and takes only EAX). The first pass also passed it the wide
//    string that was actually pushed for the FOLLOWING chimera__hud_message call, and used
//    string-list index 0 where the disassembly uses 0x65. Both are fixed.

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data;      // 0x0087a480
extern Scenario *global_scenario;    // 0x00746f8c
extern data_array *object_headers;   // 0x008603b0
extern int32_t teleport_message_cooldown; // 0x006f1d2c, UNSURE identity
extern wchar_t empty_string;          // 0x00660c34
extern uint8_t *network_client;          // 0x0071c2d8

// UNSURE: the following ten globals feed a constant-filled struct passed to player_effect_set_screen_flash_for_player; their
// true meanings are not established anywhere in this batch's evidence.
extern uint32_t teleport_effect_const_00687af0; // 0x00687af0
extern uint32_t teleport_effect_const_00687af4; // 0x00687af4
extern uint32_t teleport_effect_const_00687af8; // 0x00687af8
extern uint32_t teleport_effect_const_00687afc; // 0x00687afc
extern uint32_t teleport_effect_const_00687b00; // 0x00687b00
extern uint32_t teleport_effect_const_00687b04; // 0x00687b04
extern uint32_t teleport_effect_const_00687b08; // 0x00687b08
extern int16_t teleport_effect_const_006f1d30;  // 0x006f1d30

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern double atan2(double y, double x); // x87 FPATAN
extern double fcos(double radians); // a single x87 FCOS instruction
extern double fsin(double radians); // a single x87 FSIN instruction
extern void player_effect_set_screen_flash_for_player(void *effect_struct, uint32_t one_point_zero); // 0x456980, not in this batch
extern int game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results); // 0x461080, this batch
extern int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team,
    real_point3d *origin, float max_horizontal_dist, float max_height_delta); // 0x461180, this batch
extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40
extern int16_t unit_get_local_player_weapon_index(datum_index unit_index); // 0x4726b0, blam-cc:
    // EAX -> unit_index. RENAMED from symbols/functions.txt's unit_get_local_player_weapon_index:
    // objdump 0x4726b0..0x4726eb resolves unit -> object+0x218 (controlling_player) and returns
    // `mov ax,[player+0x02]`, i.e. player::local_player_index, or -1. There is no weapon field
    // and no stack argument.
extern void chimera__hud_message(int16_t local_player_index, wchar_t *text); // 0x4ae180,
    // blam-cc: EAX -> local_player_index, stack -> text. The EAX it consumes is the value
    // unit_get_local_player_weapon_index just returned (objdump 0x4618dc..0x4618e1: the two calls are
    // back to back with nothing in between, and the text was pushed before both).
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first
extern void player_update_history_free_all(void *queue); // 0x4e6f20
extern void object_set_position_and_orientation(datum_index object_index, real_vector3d *forward,
    real_vector3d *up, real_point3d *position); // 0x4f51c0
extern uint8_t physics_shape_test_point(void *candidates, real_point3d *position, void *out_facing); // 0x504260, not in this batch; UNSURE
extern uint8_t physics_model_build_from_sphere_query(uint32_t tag_group, real_point3d *position, float a, float b,
    float c, uint32_t exclude, void *candidates_out); // 0x506440, not in this batch; UNSURE
extern float unit_get_crouch_height_offset(float *out); // 0x55a2e0, not in this batch; UNSURE signature
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
extern void game_engine_compute_look_angles_from_vector(real_vector3d *facing,
    int16_t local_player_index); // 0x470d80. CORRECTED by review: objdump 0x4619f1..0x4619fb
    // shows "mov cx,WORD [ebp+0x2]" (player::local_player_index) and "lea eax,[esp+0x1c]"
    // (the forward vector built just above) live at the call. blam-cc: EAX -> facing,
    // CX -> local_player_index

void game_engine_update_teleporter(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit = p->unit;
    object *unit_object;
    int32_t found_index;

    if (unit == (datum_index)0xffffffff) {
        return;
    }
    unit_object = ((object_header *)object_headers->data)[unit & 0xffff].data;

    // Cache invalidation: if the cached entrance flag (player::unknown_70) is more than 1 unit
    // away from the unit's current position, forget it.
    if (p->unknown_70 != (datum_index)0xffffffff) {
        ScenarioNetgameFlags *cached = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer
            + (int32_t)p->unknown_70;
        float dx = unit_object->position.x - cached->position.x;
        float dy = unit_object->position.y - cached->position.y;
        float dz = unit_object->position.z - cached->position.z;
        if (1.0f < dx * dx + dy * dy + dz * dz) {
            p->unknown_70 = (datum_index)0xffffffff;
        }
    }

    // Find the entrance netgame_flag (team filter 6, no type filter) nearest the unit.
    found_index = -1; // UNSURE: stands in for Ghidra's bit-pattern "-NaN" sentinel; see file header
    game_engine_find_valid_starting_locations(0, 0.5f, 0.0f, 6, -1, 1, &found_index);

    if (found_index != -1 && found_index != (int32_t)p->unknown_70) {
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
        ScenarioNetgameFlags *entrance = &flags[found_index];
        int16_t entrance_usage_id = (int16_t)entrance->usage_id;

        // Find the matching exit netgame_flag (team filter 7, type = the entrance's usage_id).
        found_index = -1;
        game_engine_find_valid_starting_locations(0, 0.0f, 0.0f, 7, entrance_usage_id, 1, &found_index);

        if (found_index == -1) {
            console_print_error_va(0, "failed to teleport %d", (int32_t)entrance_usage_id);
        } else {
            ScenarioNetgameFlags *exit_flag = &flags[found_index];
            real_vector3d forward;
            real_point3d destination_position;
            float margin;
            void *candidates[44036 / sizeof(void *)]; // matches Ghidra's local_ac08 [44036]
            uint8_t blocked;

            unit_object = ((object_header *)object_headers->data)[unit & 0xffff].data;
            forward = unit_object->forward;
            margin = unit_get_crouch_height_offset(&margin); // UNSURE: real output target and meaning

            destination_position.x = exit_flag->position.x;
            destination_position.y = exit_flag->position.y;
            destination_position.z = exit_flag->position.z;

            blocked = physics_model_build_from_sphere_query(0x200380, &destination_position,
                (float)found_index + (float)found_index + margin, // UNSURE, see file header
                margin, (float)found_index, 0xffffffff, candidates);

            if (blocked != 0) {
                void *obstruction_facing;
                blocked = physics_shape_test_point(candidates, &destination_position, &obstruction_facing);
            }

            if (blocked != 0) {
                // Destination is obstructed: notify whoever is standing there, then throttle the
                // failure message.
                datum_index obstruction = (datum_index)0xffffffff; // UNSURE: really physics_shape_test_point's
                    // own out-parameter (Ghidra's local_ac20), not modeled as a real output above
                if (obstruction != (datum_index)0xffffffff) {
                    object *blocker = ((object_header *)object_headers->data)[obstruction & 0xffff].data;
                    if (((1 << blocker->type) & _object_mask_unit) != 0) {
                        datum_index controller =
                            ((unit_data *)((uint8_t *)blocker +
                                           k_unit_data_offset))->controlling_player;
                        if (controller != (datum_index)0xffffffff) {
                            player *other = (player *)((uint8_t *)player_data->data +
                                (controller & 0xffff) * sizeof(player));
                            *(uint8_t *)((uint8_t *)other + 0xd4) = 1;      // UNSURE offset
                            *(int32_t *)((uint8_t *)other + 0xcc) =
                                *(int32_t *)((uint8_t *)other + 0xcc) + 1;   // UNSURE offset
                        }
                    }
                }

                if (teleport_message_cooldown < 1) {
                    wchar_t *text;
                    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text");

                    teleport_message_cooldown = 0x78;
                    // objdump 0x4618c1: the string-list index is 0x65, not 0.
                    text = (tag_id == k_datum_index_none) ? &empty_string
                        : text_string_list_get_string(tag_id, 0x65);
                    chimera__hud_message(unit_get_local_player_weapon_index(p->unit), text);
                    return;
                }
                teleport_message_cooldown = teleport_message_cooldown - 1;
                return;
            }

            // Not obstructed: play the teleport cue for a local player and queue its effect.
            if (p->local_player_index != -1) {
                game_engine_queue_multiplayer_sound(0);
                if (p->local_player_index != -1) {
                    // UNSURE: this ~14-dword struct and its ten DAT_ constants are outside this
                    // batch's evidence; preserved as a literal field-for-field fill.
                    uint32_t effect[14];
                    int32_t i;
                    for (i = 0; i < 13; i++) {
                        effect[i] = 0;
                    }
                    *(int16_t *)&effect[13] = 0;
                    effect[10] = teleport_effect_const_00687b08;
                    *(int16_t *)((uint8_t *)effect + 0x2c) = teleport_effect_const_006f1d30; // UNSURE offset
                    *(int16_t *)effect = 0; // local_ac40 low word, overwritten below
                    effect[1] = teleport_effect_const_00687afc; // UNSURE offset mapping
                    effect[2] = teleport_effect_const_00687af4;
                    effect[3] = teleport_effect_const_00687af8;
                    *(int16_t *)&effect[0] = 2;
                    effect[4] = 0;
                    effect[5] = teleport_effect_const_00687b00;
                    effect[6] = teleport_effect_const_00687b04;
                    player_effect_set_screen_flash_for_player(effect, 1.0f); // 1.0f
                }
            }

            // Compute the destination facing (entrance forward direction, corrected by the
            // difference between the exit's and entrance's authored facings) and teleport.
            {
                float yaw = (float)atan2(forward.j, forward.i);
                yaw = (yaw + exit_flag->facing) - entrance->facing;
                forward.i = (float)fcos(yaw);
                forward.j = (float)fsin(yaw);
                vector3d_normalize_with_length(&forward);

                object_set_position_and_orientation(unit, &forward, 0, &destination_position);

                if (p->local_player_index != -1) {
                    game_engine_compute_look_angles_from_vector(&forward,
                        p->local_player_index);
                }

                p->unknown_70 = (datum_index)game_engine_find_one_valid_starting_location(0, -1,
                    0, 1.0f, 0.0f); // UNSURE: original call is FUN_00461180(0x3f800000,0); argument
                                    // order/identity guessed from that wrapper's own signature

                if ((unit_object->network_role == 1 || unit_object->network_role == 2) &&
                    p->local_player_index != -1 && network_client != 0) {
                    player_update_history_free_all(*(void **)((uint8_t *)network_client + 0xf48));
                    return;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x461630), from tools/pack.py 0x461630:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void game_engine_update_teleporter(uint param_1)

{
  uint uVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  float *pfVar7;
  undefined *puVar8;
  undefined4 uVar9;
  int iVar10;
  int iVar11;
  undefined4 *puVar12;
  int iVar13;
  undefined4 *puVar14;
  float10 fVar15;
  float10 fVar16;
  float local_ac70;
  undefined4 *local_ac6c;
  int local_ac68;
  float local_ac64;
  float local_ac60;
  undefined4 local_ac5c;
  float local_ac58;
  int local_ac54;
  undefined4 local_ac50;
  undefined4 local_ac4c;
  undefined4 local_ac48;
  int local_ac44;
  undefined2 local_ac40;
  undefined4 local_ac3e [3];
  undefined4 local_ac30;
  undefined2 local_ac2c;
  uint local_ac20;
  undefined4 local_ac1c;
  undefined4 local_ac18;
  undefined4 local_ac14;
  undefined4 local_ac10;
  undefined4 local_ac0c;
  undefined1 local_ac08 [44036];
  undefined4 uStack_4;

  uStack_4 = 0x46163a;
  iVar11 = (param_1 & 0xffff) * 0x200;
  uVar1 = *(uint *)(*(int *)(DAT_0087a480 + 0x34) + 0x34 + iVar11);
  iVar10 = *(int *)(DAT_0087a480 + 0x34) + iVar11;
  if (uVar1 != 0xffffffff) {
    iVar13 = (uVar1 & 0xffff) * 0xc;
    iVar2 = *(int *)(iVar13 + 8 + *(int *)(DAT_008603b0 + 0x34));
    if ((*(int *)(iVar10 + 0x70) != -1) &&
       (pfVar7 = (float *)(*(int *)(iVar10 + 0x70) * 0x94 + *(int *)(global_scenario + 0x37c)),
       fVar3 = *(float *)(iVar2 + 0x5c) - *pfVar7, fVar4 = *(float *)(iVar2 + 0x60) - pfVar7[1],
       fVar5 = *(float *)(iVar2 + 100) - pfVar7[2],
       1.0 < fVar4 * fVar4 + fVar3 * fVar3 + fVar5 * fVar5)) {
      *(undefined4 *)(iVar10 + 0x70) = 0xffffffff;
    }
    local_ac70 = -NAN;
    game_engine_find_valid_starting_locations(0.5,0.0,6,-1,1,(int *)&local_ac70);
    if ((local_ac70 != -NAN) && (local_ac70 != *(float *)(iVar10 + 0x70))) {
      local_ac6c = *(undefined4 **)(global_scenario + 0x37c);
      local_ac54 = (int)local_ac70 * 0x94 + (int)local_ac6c;
      local_ac68 = (int)*(short *)(local_ac54 + 0x12);
      local_ac70 = -NAN;
      game_engine_find_valid_starting_locations
                (0.0,0.0,7,*(short *)(local_ac54 + 0x12),1,(int *)&local_ac70);
      iVar2 = DAT_0087a480;
      if (local_ac70 == -NAN) {
        console_print_error_va("failed to teleport %d",(int)(short)local_ac68);
      }
      else {
        local_ac6c = (undefined4 *)((int)local_ac70 * 0x94 + (int)local_ac6c);
        local_ac68 = *(int *)(iVar13 + 8 + *(int *)(DAT_008603b0 + 0x34));
        local_ac64 = *(float *)(local_ac68 + 0x74);
        local_ac60 = *(float *)(local_ac68 + 0x78);
        local_ac5c = *(undefined4 *)(local_ac68 + 0x7c);
        local_ac44 = *(int *)(DAT_0087a480 + 0x34) + iVar11;
        FUN_0055a2e0(&local_ac58);
        puVar12 = local_ac6c;
        local_ac50 = *local_ac6c;
        local_ac4c = local_ac6c[1];
        local_ac48 = local_ac6c[2];
        cVar6 = FUN_00506440(0x200380,&local_ac50,local_ac70 + local_ac70 + local_ac58,local_ac58,
                             local_ac70,0xffffffff,local_ac08);
        if ((cVar6 != '\0') &&
           (cVar6 = FUN_00504260(local_ac08,&local_ac50,&local_ac40), cVar6 != '\0')) {
          if ((local_ac20 != 0xffffffff) &&
             ((iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_ac20 & 0xffff) * 0xc),
              (1 << (*(byte *)(iVar10 + 0xb4) & 0x1f) & 3U) != 0 &&
              (uVar1 = *(uint *)(iVar10 + 0x218), uVar1 != 0xffffffff)))) {
            iVar11 = (uVar1 & 0xffff) * 0x200;
            iVar10 = *(int *)(iVar11 + 0xcc + *(int *)(iVar2 + 0x34));
            iVar11 = iVar11 + *(int *)(iVar2 + 0x34);
            *(undefined1 *)(iVar11 + 0xd4) = 1;
            *(int *)(iVar11 + 0xcc) = iVar10 + 1;
          }
          if (DAT_006f1d2c < 1) {
            DAT_006f1d2c = 0x78;
            iVar10 = tag_lookup("ui\\multiplayer_game_text");
            if (iVar10 == -1) {
              puVar8 = &DAT_00660c34;
            }
            else {
              puVar8 = (undefined *)text_string_list_get_string();
            }
            unit_get_local_player_weapon_index(puVar8);
            chimera__hud_message();
            return;
          }
          DAT_006f1d2c = DAT_006f1d2c + -1;
          return;
        }
        if ((*(short *)(iVar10 + 2) != -1) &&
           (game_engine_queue_multiplayer_sound(0), puVar12 = local_ac6c,
           *(short *)(iVar10 + 2) != -1)) {
          puVar14 = local_ac3e;
          for (iVar11 = 0xd; iVar11 != 0; iVar11 = iVar11 + -1) {
            *puVar14 = 0;
            puVar14 = puVar14 + 1;
          }
          *(undefined2 *)puVar14 = 0;
          local_ac30 = DAT_00687b08;
          local_ac2c = DAT_006f1d30;
          local_ac40 = DAT_00687af0;
          local_ac14 = DAT_00687afc;
          local_ac20 = DAT_00687af4;
          local_ac18 = DAT_00687af8;
          local_ac3e[0]._0_2_ = 2;
          local_ac1c = 0;
          local_ac10 = DAT_00687b00;
          local_ac0c = DAT_00687b04;
          FUN_00456980(&local_ac40,0x3f800000);
        }
        fVar16 = (float10)fpatan((float10)local_ac60,(float10)local_ac64);
        fVar16 = (fVar16 + (float10)(float)puVar12[3]) - (float10)*(float *)(local_ac54 + 0xc);
        fVar15 = (float10)fcos(fVar16);
        local_ac64 = (float)fVar15;
        fVar16 = (float10)fsin(fVar16);
        local_ac60 = (float)fVar16;
        vector3d_normalize_with_length();
        object_set_position_and_orientation(*(undefined4 *)(iVar10 + 0x34),&local_ac64,0);
        if (*(short *)(iVar10 + 2) != -1) {
          game_engine_compute_look_angles_from_vector();
        }
        iVar11 = local_ac68;
        uVar9 = FUN_00461180(0x3f800000,0);
        *(undefined4 *)(iVar10 + 0x70) = uVar9;
        if ((((*(int *)(iVar11 + 4) == 1) || (*(int *)(iVar11 + 4) == 2)) &&
            (*(short *)(iVar10 + 2) != -1)) && (DAT_0071c2d8 != 0)) {
          player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
          return;
        }
      }
    }
  }
  return;
}
#endif

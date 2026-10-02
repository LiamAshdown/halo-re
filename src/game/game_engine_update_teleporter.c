// game_engine_update_teleporter  (Ghidra: game_engine_update_teleporter, already named)
// address 0x461630, size 1096 bytes
// VERIFIED against disassembly 0x461630..0x461a78 (2026-09-30)
// name confidence: 0.75   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Handles teleporting a unit through a level
// teleporter, finding and validating a destination and reporting failures"); the CEA/PDB string
// match on "failed to teleport %d"; types/tags.h ScenarioNetgameFlags (position, facing +0xc,
// usage_id +0x12) and Scenario::netgame_flags (the same +0x37c pointer this batch's
// game_engine_find_valid_starting_locations reads); types/game.h player::teleporter_flag_index (+0x70, the cached
// entrance flag), player +0xcc / +0xd4 (telefrag counter / danger flag).
// register convention: no register-passed arguments; param_1 is this function's own stack parameter (a player index).
//
// FIXED 2026-09-30 (full instruction-level comparison; the earlier draft was "the least certain file in the batch"):
//  - game_engine_find_valid_starting_locations' first call takes the unit's POSITION (object +0x5c) in EBX; the draft passed NULL. (The second,
//    exit-flag search really does pass NULL: `xor ebx,ebx` at 0x461733.)
//  - The stack slot the draft modelled as a "float found_index" is the pill-radius OUT parameter of unit_get_crouch_height_offset
//    (EAX = position buffer, ECX = unit, EBX = &pill_radius, stack = &pill_height). The sphere query is
//    physics_model_build_from_sphere_query(0x200380, &destination, 2*radius + height, height, radius, -1, model), with the destination
//    written over the position buffer with the exit flag's position; the obstruction object is out_contact.object_index (+0x20) of
//    physics_shape_test_point (a physics_model_contact), NOT a constant -1 (so the draft never flagged the telefrag victim).
//  - The screen flash is a player_screen_flash (0x38 bytes): type = word[0x687af0] (6), priority 2, duration = [0x687b08] (1.0),
//    fade function = word[0x6f1d30], maximum intensity = [0x687af4] (1.0), intensity 0, colour ARGB = ([0x687af8], [0x687afc],
//    [0x687b00], [0x687b04]) = (0.5, 0.35, 1.0, 0.35); it is played for the player index (EAX) with falloff 1.0.
//  - game_engine_find_one_valid_starting_location is called with type -1 (ECX), team 6 (EDX), origin = the unit position (EBX),
//    stack (1.0, 0.0); the draft passed (0, -1, NULL, ...).
//  - 0x4726b0 is unit_get_local_player_weapon_index (really: the controlling player's local player index, EAX = unit).

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
#include "effects.h"
#include "physics.h"
#include <wchar.h>
#include "networking.h"

extern data_array *player_data;      // 0x0087a480
extern Scenario *global_scenario;    // 0x00746f8c
extern data_array *object_data;   // 0x008603b0
extern int32_t teleport_message_cooldown; // 0x006f1d2c, ticks until the "cannot teleport" message may show again
extern wchar_t empty_string;          // 0x00660c34
extern network_client_globals *network_client; // 0x0071c2d8

// The teleport screen flash's constants (read at 0x461920..0x461977): all live in the exe's data.
extern int16_t teleport_flash_type;               // 0x00687af0 (word, value 6)
extern uint32_t teleport_flash_maximum_intensity; // 0x00687af4 (1.0f)
extern uint32_t teleport_flash_alpha;             // 0x00687af8 (0.5f)
extern uint32_t teleport_flash_red;               // 0x00687afc (0.35f)
extern uint32_t teleport_flash_green;             // 0x00687b00 (1.0f)
extern uint32_t teleport_flash_blue;              // 0x00687b04 (0.35f)
extern uint32_t teleport_flash_duration;          // 0x00687b08 (1.0f)
extern int16_t teleport_flash_fade_function;      // 0x006f1d30 (word)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern double atan2(double y, double x); // x87 FPATAN
extern double fcos(double radians); // a single x87 FCOS instruction
extern double fsin(double radians); // a single x87 FSIN instruction
extern void player_effect_set_screen_flash_for_player(datum_index player_index, player_screen_flash *descriptor,
    float intensity_falloff); // 0x456980, EAX player, stack (descriptor, intensity_falloff)
extern int game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results); // 0x461080, blam-cc: EBX origin (optional), stack rest
extern int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team,
    real_point3d *origin, float max_horizontal_dist, float max_height_delta); // 0x461180, ECX type, EDX team, EBX origin, stack
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern int16_t unit_get_local_player_weapon_index(datum_index unit_index); // 0x4726b0, blam-cc:
    // EAX -> unit_index. Returns player::local_player_index of the unit's controlling player, or -1.
extern void chimera__hud_message(int16_t local_player_index, wchar_t *text); // 0x4ae180,
    // blam-cc: EAX -> local_player_index, stack -> text.
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first
extern void player_update_history_free_all(void *queue); // 0x4e6f20
extern void object_set_position_and_orientation(datum_index object_index, real_vector3d *forward,
    real_vector3d *up, real_point3d *position); // 0x4f51c0, EDI position
extern uint8_t physics_shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact); // 0x504260, stack
extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius,
    float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model); // 0x506440, stack
extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, blam-cc: EAX object_position, ECX object_index, EBX pill_radius_out, stack pill_height
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
extern void game_engine_compute_look_angles_from_vector(real_vector3d *facing,
    int16_t local_player_index); // 0x470d80. blam-cc: EAX -> facing, CX -> local_player_index

void game_engine_update_teleporter(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit = p->unit;
    object *unit_object;
    int32_t found_index;

    if (unit == (datum_index)0xffffffff) {
        return;
    }
    unit_object = ((object_header *)object_data->data)[unit & 0xffff].data;

    // Cache invalidation: if the cached entrance flag (player::teleporter_flag_index) is more than 1 unit
    // away from the unit's current position, forget it.
    if (p->teleporter_flag_index != (datum_index)0xffffffff) {
        ScenarioNetgameFlags *cached = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer
            + (int32_t)p->teleporter_flag_index;
        float dx = unit_object->position.x - cached->position.x;
        float dy = unit_object->position.y - cached->position.y;
        float dz = unit_object->position.z - cached->position.z;
        if (1.0f < dx * dx + dy * dy + dz * dz) {
            p->teleporter_flag_index = (datum_index)0xffffffff;
        }
    }

    // Find the entrance netgame_flag (team filter 6, no type filter) within 0.5 of the unit's position.
    found_index = -1;
    game_engine_find_valid_starting_locations(&unit_object->position, 0.5f, 0.0f, 6, -1, 1, &found_index);

    if (found_index != -1 && found_index != (int32_t)p->teleporter_flag_index) {
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
            float pill_height;
            float pill_radius;
            physics_model candidates;
            physics_model_contact contact;
            uint8_t blocked;

            unit_object = ((object_header *)object_data->data)[unit & 0xffff].data;
            forward = unit_object->forward;
            p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
            unit_get_crouch_height_offset(&destination_position, p->unit, &pill_height, &pill_radius);

            // the buffer the callee filled with the unit's position now takes the exit flag's position
            destination_position.x = exit_flag->position.x;
            destination_position.y = exit_flag->position.y;
            destination_position.z = exit_flag->position.z;

            blocked = physics_model_build_from_sphere_query(0x200380, &destination_position,
                pill_radius + pill_radius + pill_height, pill_height, pill_radius, 0xffffffff, &candidates);

            if (blocked != 0) {
                blocked = physics_shape_test_point(&candidates, &destination_position, &contact);
            }

            if (blocked != 0) {
                // Destination is obstructed: mark the controlling player of whatever is standing there, then throttle
                // the failure message.
                datum_index obstruction = contact.object_index;

                if (obstruction != (datum_index)0xffffffff) {
                    object *blocker = ((object_header *)object_data->data)[obstruction & 0xffff].data;
                    if (((1 << blocker->type) & _object_mask_unit) != 0) {
                        datum_index controller =
                            ((unit_data *)((uint8_t *)blocker +
                                           k_unit_data_offset))->controlling_player;
                        if (controller != (datum_index)0xffffffff) {
                            player *other = (player *)((uint8_t *)player_data->data +
                                (controller & 0xffff) * sizeof(player));
                            other->telefrag_danger = 1;
                            *(int32_t *)((uint8_t *)other + 0xcc) =
                                *(int32_t *)((uint8_t *)other + 0xcc) + 1;
                        }
                    }
                }

                if (teleport_message_cooldown < 1) {
                    wchar_t *text;
                    datum_index tag_id = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");

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

            // Not obstructed: play the teleport cue for a local player and queue its flash.
            if (p->local_player_index != -1) {
                game_engine_queue_multiplayer_sound(0x1b, 0xffffffff, 0); // 0x461901..0x461909
                if (p->local_player_index != -1) {
                    player_screen_flash flash;
                    uint8_t *flash_bytes = (uint8_t *)&flash;
                    int32_t i;

                    for (i = 0; i < (int32_t)sizeof(flash); i++) {
                        flash_bytes[i] = 0;
                    }
                    flash.type = teleport_flash_type;
                    flash.priority = 2;
                    flash.duration = *(float *)&teleport_flash_duration;
                    flash.fade_function = (uint16_t)teleport_flash_fade_function;
                    flash.maximum_intensity = teleport_flash_maximum_intensity;
                    flash.intensity = 0.0f;
                    flash.color.alpha = *(float *)&teleport_flash_alpha;
                    flash.color.red = *(float *)&teleport_flash_red;
                    flash.color.green = *(float *)&teleport_flash_green;
                    flash.color.blue = *(float *)&teleport_flash_blue;
                    player_effect_set_screen_flash_for_player(player_index, &flash, 1.0f);
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

                object_set_position_and_orientation(unit, &forward, 0, (real_point3d *)(&exit_flag->position));

                if (p->local_player_index != -1) {
                    game_engine_compute_look_angles_from_vector(&forward,
                        p->local_player_index);
                }

                // 0x461a00..0x461a1b: type -1 (ECX), team 6 (EDX), origin = the unit position (EBX), stack (1.0, 0.0)
                p->teleporter_flag_index = (datum_index)game_engine_find_one_valid_starting_location(-1, 6,
                    &unit_object->position, 1.0f, 0.0f);

                if ((unit_object->network_role == 1 || unit_object->network_role == 2) &&
                    p->local_player_index != -1 && network_client != 0) {
                    player_update_history_free_all(*(void **)&network_client->update_history);
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

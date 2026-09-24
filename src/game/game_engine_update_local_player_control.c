// game_engine_update_local_player_control  (Ghidra: FUN_00471ae0; renamed, no established name)
// address 0x471ae0, size 1108 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/game.h names this exact function in the header comments of player_action ("The
// per-tick control record the local machine stages for the network layer.
// game_engine_update_local_player_control (0x471ae0) assembles one on its stack and rep movs-es
// eight dwords to 0x006f7ea4 + update_client_staged_count * 0x20"), player_control_input,
// local_player_control::autolevelling_ticks / autolevelling_active and
// game_time_globals::paused. out/phase4/game_functions.md lists it only as an unnamed 1108-byte
// function; the name here is the one types/game.h already commits to.
// register convention: all three parameters are ordinary stack parameters (cdecl). The callees
// are all register-passing: ESI for object_find_next_untargeted, EDI for object_find_nearest_biped,
// EBX for unit_sample_camera_shake_from_velocity, EDI+ESI for local_player_set_controlled_unit,
// EAX for unit_find_next_zone_permitted_weapon_slot, ECX for unit_find_weapon_index_with_fixed_flag,
// EAX+ECX for weapon_get_next_zoom_level, AX for game_engine_update_local_player_look and EDX for
// 0x495a60.
//   // blam-cc: stack -> local_player_index, delta_time, ticks_this_frame
//
// Reconstructed against the disassembly (objdump -M intel --start-address=0x471ae0
// --stop-address=0x471f40 bin/halo.exe). Ghidra elides every register argument listed above, and
// mangles the grenade-cycling loop into a shape whose break condition reads backwards; the loop
// below is the disassembly at 0x471d47..0x471d96 read directly.
// The third parameter is the tick count for this frame: both callers compute it as
// game_engine_accumulate_simulation_ticks(delta_time), zero 0x006f7ea4..0x006f7ec0 and
// 0x006f7ecc, store it in 0x006f7ec4 themselves and then pass it here, and
// update_client_distribute_staged_entry decrements 0x006f7ec4 as it consumes ticks.
//
// 0x006ac5b1 and 0x006ac5b2 are camera.h director.suppress_look_update (+0x51) and
// director.look_input_consumed (+0x52) of directors[] (0x006ac560, stride 0xf8) (R03). They are
// kept here as two byte arrays at the exact addresses the code indexes (camera.h is not included);
// the first skips the look update only, the second blanks the whole input record.
// UNSURE: 0x495a60 takes the local-player index in EDX and returns a bool in AL. Its identity is
// not established; from its use here it answers "is this local player free to auto-level", i.e.
// most likely "the player is not aiming with a zoomed or scoped weapon".
// UNSURE: 0x006f187c is a pointer whose byte +9 blocks zoom cycling; the same pointer's byte +10
// is tested by game_engine_digitize_control_input, which is where this declaration's name and
// type come from.
// UNSURE: the staging globals keep the raw names the three update_client_* files in this module
// already use (update_client_staged / update_client_unknown_ec4), so that the externs agree; the
// record written into update_client_staged is typed here as types/game.h player_action, which is
// what the eight-dword rep movs actually builds. player_action::pad_1e is deliberately left
// uninitialized, exactly as the original does.
// reconciled: R03 0x006ac5b1/0x006ac5b2 identified as camera.h director +0x51/+0x52 (comments only)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern player_globals *local_player_globals;                 // 0x0087a478
extern data_array *object_headers;                           // 0x008603b0
extern player_control_globals *player_control_globals_ptr;   // 0x006b145c
extern game_time_globals *game_time;                         // 0x006f1d6c
extern Globals *global_globals;                              // 0x00746fa0
extern int16_t network_game_mode;                            // 0x00719720 (tested as a word here)
extern uint8_t *unknown_006f187c;                            // 0x006f187c, UNSURE (see header)
extern uint8_t local_player_input_frozen[];                  // 0x006ac5b2 = directors[i].look_input_consumed, stride 0xf8
extern uint8_t local_player_look_frozen[];                   // 0x006ac5b1 = directors[i].suppress_look_update, stride 0xf8

extern uint32_t update_client_staged[8];       // 0x006f7ea4, see update_client_stage_entry.c
extern int32_t update_client_staged_count;     // 0x006f7ecc, zeroed by both callers each frame
extern int32_t update_client_unknown_ec4;      // 0x006f7ec4, see update_client_distribute_staged_entry.c

extern void game_engine_build_local_player_control_input(int16_t local_player_index, real delta_time,
    player_control_input *out);                                                // this batch, 0x4710b0
extern void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta,
    real pitch_delta);                                                         // this batch, 0x472160
extern void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index);
                                    // 0x474fc0; blam-cc: EDI -> local_player_index, ESI -> unit_index
extern int32_t object_find_next_untargeted(int32_t starting_object_index);      // 0x56bdc0, units
                                                                               // blam-cc: ESI
extern int32_t object_find_nearest_biped(int32_t reference_object_index);       // 0x56bee0, units
                                                                               // blam-cc: EDI
extern void unit_sample_camera_shake_from_velocity(uint32_t unit_index);       // 0x56bfc0, units
                                                                               // blam-cc: EBX
extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot,
    int16_t direction);                                        // 0x56dba0, units; blam-cc: EAX unit
extern uint16_t unit_find_weapon_index_with_fixed_flag(uint32_t unit_index);    // 0x570460, units
                                                                               // blam-cc: ECX
extern int32_t weapon_get_next_zoom_level(int32_t current_level, datum_index item_index);
                                        // 0x4c2cf0, items; blam-cc: EAX -> current_level, ECX -> item
extern uint8_t player_profile_get_flag_by_id(int16_t local_player_index); // 0x495a60; blam-cc: EDX; UNSURE identity

// The MSVC uninitialized-stack fill the retail build still emits for this record. Every field is
// overwritten by game_engine_build_local_player_control_input before it is read, but the stores
// are real instructions (objdump 0x471b08..0x471b36), so they are kept.
#define k_uninitialized_fill 0xfafafafau

// blam-cc: stack -> local_player_index, delta_time, ticks_this_frame
// One local player's per-tick control pass: builds this tick's player_control_input, honours the
// two director freeze flags, applies the debug "possess another unit" buttons in single player,
// validates and cycles the desired weapon / grenade / zoom level, drives the look update and the
// autolevelling counter, copies the input back into local_player_control, and stages a
// player_action for the network layer.
void game_engine_update_local_player_control(int16_t local_player_index, real delta_time,
                                             int32_t ticks_this_frame)
{
    local_player_control *control =
        &player_control_globals_ptr->local_players[local_player_index];
    GlobalsPlayerControl *player_control =
        (GlobalsPlayerControl *)global_globals->player_control.pointer;
    player_control_input input;
    uint32_t *input_dwords = (uint32_t *)&input;
    uint32_t button_flags;
    datum_index current_weapon;
    object *unit_object;
    unit_data *unit;
    int32_t i;

    for (i = 0; i < 8; i += 1) {
        input_dwords[i] = k_uninitialized_fill;
    }
    game_engine_build_local_player_control_input(local_player_index, delta_time, &input);

    if (local_player_input_frozen[local_player_index * 0xf8] != 0) {
        for (i = 0; i < 8; i += 1) {
            input_dwords[i] = 0;
        }
    }
    button_flags = input.button_flags;

    if (network_game_mode == 0) {
        // Single player only: the debug "possess the next / nearest unit" buttons.
        if ((button_flags & 0x18) != 0) {
            int32_t new_unit;

            if ((button_flags & 0x10) != 0) {
                new_unit = object_find_next_untargeted((int32_t)control->unit); // blam-cc: ESI
            } else {
                new_unit = object_find_nearest_biped((int32_t)control->unit);   // blam-cc: EDI
            }
            if (new_unit != -1) {
                // blam-cc: EDI -> local_player_index, ESI -> new_unit
                local_player_set_controlled_unit((datum_index)new_unit, local_player_index);
            }
        }
        if ((button_flags & 0x20) != 0) {
            if (control->unit == (datum_index)-1) {
                goto store_input;
            }
            unit_sample_camera_shake_from_velocity(control->unit); // blam-cc: EBX
            button_flags = input.button_flags;
        }
    }

    if (control->unit == (datum_index)-1) {
        goto store_input;
    }

    unit_object = ((object_header *)object_headers->data)[control->unit & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    current_weapon = (datum_index)-1;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    // The desired weapon slot has to still hold a weapon; otherwise fall back to the unit's own.
    if (control->desired_weapon_index == -1 ||
        unit->weapons[control->desired_weapon_index] == (datum_index)-1) {
        control->desired_weapon_index = unit->desired_weapon_index;
    }

    // Cycle the weapon on the button edge, or whenever the current choice became invalid.
    if ((button_flags & 1) != 0 || control->desired_weapon_index == -1 ||
        unit->weapons[control->desired_weapon_index] == (datum_index)-1) {
        // blam-cc: EAX -> control->unit
        control->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(control->unit,
            control->desired_weapon_index, (int16_t)(button_flags & 1));
        control->desired_zoom_level = -1;
        button_flags = input.button_flags;
    }

    // A weapon the unit is forced to hold overrides the choice outright.
    {
        int16_t forced = (int16_t)unit_find_weapon_index_with_fixed_flag(control->unit); // ECX
        if (forced != -1 && control->desired_weapon_index != forced) {
            control->desired_weapon_index = forced;
            control->desired_zoom_level = -1;
        }
    }

    // Same two steps for grenades, against the two-entry grenade_counts array.
    if (control->desired_grenade_index == -1 ||
        unit->grenade_counts[control->desired_grenade_index] == 0) {
        control->desired_grenade_index = (int16_t)unit->desired_grenade_index;
    }
    if ((button_flags & 2) != 0 || control->desired_grenade_index == -1 ||
        unit->grenade_counts[control->desired_grenade_index] == 0) {
        int16_t start = control->desired_grenade_index;
        int16_t best = -1;
        int16_t index;

        if (start == -1) {
            start = 0;
        }
        index = start;
        for (;;) {
            // Signed compare: an empty or negative count is skipped.
            if (unit->grenade_counts[index] > 0) {
                best = index;
                if (index != start) {
                    break;
                }
            }
            index = index == 1 ? (int16_t)0 : (int16_t)(index + 1);
            if (index == start) {
                break;
            }
        }
        control->desired_grenade_index = best;
        button_flags = input.button_flags;
    }

    // Zoom cycles only while the engine is actually running and the player holds a weapon.
    if ((button_flags & 4) != 0 && (player_control_globals_ptr->flags & 1) == 0 &&
        game_time->paused == 0 && current_weapon != (datum_index)-1 &&
        unknown_006f187c[9] == 0) {
        // blam-cc: EAX -> control->desired_zoom_level, ECX -> current_weapon
        control->desired_zoom_level =
            (int16_t)weapon_get_next_zoom_level(control->desired_zoom_level, current_weapon);
    }

    if (local_player_look_frozen[local_player_index * 0xf8] == 0) {
        // blam-cc: AX -> local_player_index
        game_engine_update_local_player_look(local_player_index, input.yaw_delta, input.pitch_delta);
    }

    // Autolevelling: count up while the player is on foot, running hard, not pitching the camera
    // and without an aim-assist target. NOTE the throttle tested here is the one stored in
    // local_player_control last tick -- the write-back below has not happened yet.
    if (unit_object->parent_object == (datum_index)-1) {
        real absolute_throttle_x = control->input_throttle_x < 0.0f
                                       ? -control->input_throttle_x
                                       : control->input_throttle_x;

        if (player_profile_get_flag_by_id(local_player_index) != 0 && absolute_throttle_x > 0.5 && // blam-cc: EDX
            input.pitch_delta < 0.0001f && control->aim_assist_weight < 0.0001f) {
            int32_t ticks = (int32_t)control->autolevelling_ticks + 1;

            if (ticks < 0) {
                ticks = 0;
            } else if (ticks > 0x7f) {
                ticks = 0x7f;
            }
            control->autolevelling_ticks = (int8_t)ticks;
            control->autolevelling_active =
                (uint8_t)((int16_t)(int8_t)ticks > player_control->minimum_autolevelling_ticks);
            goto store_input;
        }
        control->autolevelling_ticks = 0;
    }
    control->autolevelling_active = 0;

store_input:
    control->input_primary_trigger = input.primary_trigger;
    control->input_control_flags = input.control_flags;
    control->input_throttle_x = input.throttle_x;
    control->input_throttle_y = input.throttle_y;

    if (local_player_index != -1 && local_player_index < k_maximum_local_players &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        player_action action;

        action.control_flags = input.control_flags;
        action.desired_yaw = control->yaw;
        action.desired_pitch = control->pitch;
        action.throttle_x = input.throttle_x;
        action.throttle_y = input.throttle_y;
        action.primary_trigger = input.primary_trigger;
        action.weapon_index = control->desired_weapon_index;
        action.grenade_index = control->desired_grenade_index;
        action.zoom_level = control->desired_zoom_level;
        // action.pad_1e is deliberately not written; the eight-dword copy below carries whatever
        // was on the stack, exactly as the original does.

        ((player_action *)update_client_staged)[update_client_staged_count] = action;
        update_client_staged_count = update_client_staged_count + 1;
        update_client_unknown_ec4 = ticks_this_frame;
    }
}

#if 0
Original Ghidra decompilation (0x471ae0), from tools/pack.py 0x471ae0.
Kept verbatim; see the header for the register arguments it drops and for the grenade loop.

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00471ae0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  uint uVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  uint *puVar14;
  short sVar15;
  uint uVar16;
  uint *puVar17;
  int local_50;
  uint local_40;
  uint local_3c;
  uint local_38;
  undefined4 local_34;
  float local_30;
  undefined4 local_2c;
  uint local_28;
  uint local_24;
  uint local_20 [4];
  uint local_10;
  uint local_c;
  undefined2 local_8;
  undefined2 local_6;
  undefined2 local_4;

  sVar4 = (short)param_1;
  iVar13 = (int)sVar4;
  puVar14 = (uint *)(iVar13 * 0x40 + 0x10 + DAT_006b145c);
  iVar11 = *(int *)(DAT_00746fa0 + 0x114);
  local_40 = 0xfafafafa;
  local_3c = 0xfafafafa;
  local_38 = 0xfafafafa;
  local_34 = 0xfafafafa;
  local_30 = -6.515823e+35;
  local_2c = 0xfafafafa;
  local_28 = 0xfafafafa;
  local_24 = 0xfafafafa;
  FUN_004710b0(param_1,param_2,&local_40);
  if ((&DAT_006ac5b2)[iVar13 * 0xf8] != '\0') {
    local_24 = 0;
    local_40 = 0;
    local_3c = 0;
    local_38 = 0;
    local_34 = 0;
    local_30 = 0.0;
    local_2c = 0;
    local_28 = 0;
  }
  uVar12 = local_24;
  if (DAT_00719720 == 0) {
    if ((local_24 & 0x18) != 0) {
      if ((local_24 & 0x10) == 0) {
        iVar7 = FUN_0056bee0();
      }
      else {
        iVar7 = FUN_0056bdc0();
      }
      if (iVar7 != -1) {
        local_player_set_controlled_unit();
      }
    }
    if ((uVar12 & 0x20) != 0) {
      if (*puVar14 == 0xffffffff) goto LAB_00471e85;
      FUN_0056bfc0();
      uVar12 = local_24;
    }
  }
  iVar7 = DAT_008603b0;
  if (*puVar14 != 0xffffffff) {
    iVar8 = (*puVar14 & 0xffff) * 0xc;
    iVar1 = *(int *)(iVar8 + 8 + *(int *)(DAT_008603b0 + 0x34));
    local_50 = -1;
    if (*(short *)(iVar1 + 0x2f2) != -1) {
      local_50 = *(int *)(iVar1 + 0x2f8 + *(short *)(iVar1 + 0x2f2) * 4);
    }
    if (((short)puVar14[8] == -1) ||
       (*(int *)(*(int *)(iVar8 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x2f8 + (short)puVar14[8] * 4
                ) == -1)) {
      *(undefined2 *)(puVar14 + 8) = *(undefined2 *)(iVar1 + 0x2f4);
    }
    if (((((uVar12 & 1) != 0) || (sVar6 = (short)puVar14[8], sVar6 == -1)) ||
        (uVar12 = local_24,
        *(int *)(*(int *)(iVar8 + 8 + *(int *)(iVar7 + 0x34)) + 0x2f8 + sVar6 * 4) == -1)) ||
       (sVar6 == -1)) {
      uVar5 = FUN_0056dba0((short)puVar14[8],uVar12 & 1);
      iVar7 = DAT_008603b0;
      *(undefined2 *)(puVar14 + 8) = uVar5;
      *(undefined2 *)(puVar14 + 9) = 0xffff;
    }
    uVar2 = *puVar14;
    sVar6 = FUN_00570460();
    if ((sVar6 != -1) && ((short)puVar14[8] != sVar6)) {
      *(short *)(puVar14 + 8) = sVar6;
      *(undefined2 *)(puVar14 + 9) = 0xffff;
    }
    if ((*(short *)((int)puVar14 + 0x22) == -1) ||
       (*(char *)(*(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x31e +
                 (int)*(short *)((int)puVar14 + 0x22)) == '\0')) {
      *(short *)((int)puVar14 + 0x22) = (short)*(char *)(iVar1 + 0x31d);
    }
    if (((((uVar12 & 2) != 0) || (sVar6 = *(short *)((int)puVar14 + 0x22), sVar6 == -1)) ||
        (uVar12 = local_24,
        *(char *)(*(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x31e + (int)sVar6
                 ) == '\0')) || (sVar6 == -1)) {
      uVar9 = (uint)*(ushort *)((int)puVar14 + 0x22);
      uVar16 = 0xffffffff;
      uVar10 = uVar9;
      if (*(ushort *)((int)puVar14 + 0x22) == 0xffff) {
        uVar9 = 0;
        uVar10 = uVar9;
      }
      do {
        sVar6 = (short)uVar9;
        if (('\0' < *(char *)(sVar6 + 0x31e +
                             *(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc))) &&
           (uVar16 = uVar9, sVar15 = sVar6, sVar6 != (short)uVar10)) break;
        sVar15 = (short)uVar16;
        if (sVar6 == 1) {
          uVar9 = 0;
        }
        else {
          uVar9 = (int)sVar6 + 1;
        }
      } while ((short)uVar9 != (short)uVar10);
      *(short *)((int)puVar14 + 0x22) = sVar15;
    }
    if ((((uVar12 & 4) != 0) && ((*(byte *)(DAT_006b145c + 0xc) & 1) == 0)) &&
       ((*(char *)(DAT_006f1d6c + 2) == '\0' &&
        ((local_50 != -1 && (*(char *)(DAT_006f187c + 9) == '\0')))))) {
      uVar5 = FUN_004c2cf0();
      *(undefined2 *)(puVar14 + 9) = uVar5;
    }
    if ((&DAT_006ac5b1)[iVar13 * 0xf8] == '\0') {
      FUN_00472160(local_34,local_30);
    }
    if (*(int *)(iVar1 + 0x11c) == -1) {
      cVar3 = FUN_00495a60();
      if ((((cVar3 != '\0') && (0.5 < ABS((float)puVar14[5]))) && (local_30 < 0.0001)) &&
         ((float)puVar14[0xc] < 0.0001)) {
        iVar7 = *(char *)((int)puVar14 + 0x27) + 1;
        if (iVar7 < 0) {
          iVar7 = 0;
        }
        else if (0x7f < iVar7) {
          iVar7 = 0x7f;
        }
        *(char *)((int)puVar14 + 0x27) = (char)iVar7;
        *(bool *)((int)puVar14 + 0x26) = *(short *)(iVar11 + 0x6e) < (short)(char)iVar7;
        goto LAB_00471e85;
      }
      *(undefined1 *)((int)puVar14 + 0x27) = 0;
    }
    *(undefined1 *)((int)puVar14 + 0x26) = 0;
  }
LAB_00471e85:
  puVar14[7] = local_38;
  puVar14[1] = local_28;
  puVar14[5] = local_40;
  puVar14[6] = local_3c;
  if (((sVar4 != -1) && (sVar4 < 1)) && (*(int *)(DAT_0087a478 + 4 + iVar13 * 4) != -1)) {
    local_20[0] = local_28;
    local_8 = (short)puVar14[8];
    local_20[1] = puVar14[3];
    local_10 = local_3c;
    local_6 = *(undefined2 *)((int)puVar14 + 0x22);
    local_20[3] = local_40;
    local_20[2] = puVar14[4];
    local_c = local_38;
    local_4 = (short)puVar14[9];
    puVar14 = local_20;
    puVar17 = &DAT_006f7ea4 + DAT_006f7ecc * 8;
    for (iVar11 = 8; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar17 = *puVar14;
      puVar14 = puVar14 + 1;
      puVar17 = puVar17 + 1;
    }
    DAT_006f7ecc = DAT_006f7ecc + 1;
    _DAT_006f7ec4 = param_3;
  }
  return;
}
#endif

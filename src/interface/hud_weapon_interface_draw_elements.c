// hud_weapon_interface_draw_elements  (Ghidra: FUN_004b1ff0; the first rewrite called it
// hud_weapon_crosshair_state_update; renamed in the phase-4 review)
// address 0x4b1ff0, size 2747 bytes (to 0x4b2ab4)
// name confidence: 0.6 (chosen)   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b1ff0..0x4b2ab4 in the phase-4 review. The first rewrite
// was a mechanical copy of the decompile with the tag id in EAX (it is the first of seven cdecl
// arguments) and unresolved callee argument lists. Draws the static, meter, number and overlay
// elements of one WeaponHUDInterface (types/tags.h) and recurses into its child_hud with the
// three state arrays it computed:
//   state_flags[8]  per WeaponHUDInterfaceStateAttachedTo: bit 0 flashing (below the cutoff),
//                   bit 1 disabled (empty), bit 2 split screen; the draw flags of the statics
//                   meters and numbers, and the flash start times at hud_weapon_state +
//                   index * 0x28 follow bit 0.
//   overlay_types[8] bit 0 flashing, bit 1 empty, bit 2 reloading/overheated, bit 3 default
//                   (none of the others), bit 4 always: the WeaponHUDInterfaceOverlayType mask
//                   of the overlays.
//   numbers[8]      rounds, heat * 255, (1 - age) * 100 as shorts; meter alpha and number value.
// A child hud whose flags bit 0 (use parent hud flashing parameters) is set takes the parent
// arrays instead of computing its own (the float values then stay 0). Distance and elevation
// to the aim assist target (local_player_control +0x28/+0x2c, weight 1.0) are in meters
// (world units * 3.048), a quiet NaN (0xffc00000) when there is no target.
// Behaviour kept from the binary: the secondary magazine overlay types overwrite entries 0 and
// 1 (entries 4 and 5 stay 0); overlay type 2 of the heat entry takes its "empty" bit from the
// age; static elements with a flash period or length stored as the integer 0x3f80 are patched
// to 1.0f in the tag; element byte +0x02 bit 0 hides an element (types/tags.h calls it
// padding); the meter fraction argument is the flash start time converted to float.
// register convention: plain cdecl, seven stack arguments.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "items.h"
#include "interface.h"
#include "fn_interface.h"

extern tag_instance *tag_instances;                  // 0x0087bc14
extern data_array *object_data; // 0x008603b0
extern data_array *player_data;                      // 0x0087a480
extern player_globals *local_player_globals;         // 0x0087a478
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern game_time_globals *game_time;                 // 0x006f1d6c
extern Scenario *global_scenario; // 0x00746f8c
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430

extern float sqrtf(float x);
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation

extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80, blam-cc: ECX unit_index, EDI out
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, blam-cc: EAX out, ECX object


// trunc(value * 100) clamped to 0..100 (evaluated up to three times by the binary)
static int32_t hud_percent(float value)
{
    int32_t percent = ui_real_to_int_truncate(value * 100.0f);
    if (percent < 0) {
        return 0;
    }
    if (percent > 100) {
        return 100;
    }
    return percent;
}

// bits 0..2 as computed, bit 3 when none of them is set, bit 4 always
static uint16_t hud_overlay_type_bits(uint16_t bits)
{
    if (bits == 0) {
        bits = 8;
    } else {
        bits &= 0xfff7;
    }
    return (uint16_t)(bits | 0x10);
}

void hud_weapon_interface_draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag,
                                        const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags,
                                        const uint16_t *parent_overlay_types, const int16_t *parent_numbers)
{
    WeaponHUDInterface *hud = (WeaponHUDInterface *)tag_instances[hud_tag & 0xffff].data;
    int32_t *flash_start_times = (int32_t *)((uint8_t *)hud_weapon_state + local_player_index * 0x28);
    uint16_t state_flags[8];
    uint16_t overlay_types[8];
    int16_t numbers[8];
    float values[8];
    uint16_t split = local_player_globals->local_player_count > 1 ? 4 : 0;
    uint32_t view_mask;
    int16_t i;

    memset(state_flags, 0, sizeof(state_flags));
    memset(overlay_types, 0, sizeof(overlay_types));
    memset(numbers, 0, sizeof(numbers));
    memset(values, 0, sizeof(values));

    if ((*(uint8_t *)&hud->flags & 1) != 0 && parent_state_flags != 0 && parent_overlay_types != 0 &&
        parent_numbers != 0) {
        memcpy(state_flags, parent_state_flags, sizeof(state_flags));
        memcpy(overlay_types, parent_overlay_types, sizeof(overlay_types));
        memcpy(numbers, parent_numbers, sizeof(numbers));
    } else {
        const weapon_hud_magazine_state *primary = &ammo->magazines[0];
        const weapon_hud_magazine_state *secondary = &ammo->magazines[1];
        int16_t loaded;
        int16_t total;

        // state flags
        state_flags[0] = (uint16_t)((primary->rounds_unloaded <= hud->total_ammo_cutoff ? 1 : 0) |
                                    (primary->rounds_unloaded == 0 ? 2 : 0) | split);
        state_flags[1] = (uint16_t)((primary->rounds_loaded <= hud->loaded_ammo_cutoff && primary->reloading == 0 ? 1 : 0) |
                                    split);
        state_flags[2] = (uint16_t)(((float)hud->heat_cutoff <= ammo->heat * 100.0f ? 1 : 0) | split);
        state_flags[3] = (uint16_t)((!((float)hud->age_cutoff < (1.0f - ammo->age) * 100.0f) ? 1 : 0) |
                                    (100 - hud_percent(ammo->age) == 0 ? 2 : 0) | split);
        state_flags[4] = (uint16_t)((secondary->rounds_unloaded <= hud->total_ammo_cutoff ? 1 : 0) |
                                    (secondary->rounds_unloaded == 0 ? 2 : 0) | split);
        state_flags[5] = (uint16_t)((secondary->rounds_loaded <= hud->loaded_ammo_cutoff && secondary->reloading == 0 ? 1 : 0) |
                                    split);
        for (i = 0; i < 8; i++) {
            if ((state_flags[i] & 1) != 0) {
                if (flash_start_times[i] == -1) {
                    flash_start_times[i] = game_time->game_time;
                }
            } else {
                flash_start_times[i] = -1;
            }
        }

        // overlay types
        total = primary->rounds_unloaded;
        overlay_types[0] = hud_overlay_type_bits((uint16_t)(
            (total <= hud->total_ammo_cutoff && primary->reloading == 0 ? 1 : 0) | (primary->reloading != 0 ? 4 : 0) |
            (total == 0 ? 2 : 0)));
        loaded = primary->rounds_loaded;
        overlay_types[1] = hud_overlay_type_bits((uint16_t)(
            (loaded <= hud->loaded_ammo_cutoff ? 1 : 0) | (primary->reloading != 0 ? 4 : 0) | (loaded == 0 ? 2 : 0)));
        overlay_types[2] = hud_overlay_type_bits((uint16_t)(
            ((float)hud->heat_cutoff <= ammo->heat * 100.0f ? 1 : 0) | (ammo->overheated != 0 ? 4 : 0) |
            (100 - hud_percent(ammo->age) == 0 ? 2 : 0)));
        overlay_types[3] = hud_overlay_type_bits((uint16_t)(
            (!((float)hud->age_cutoff < (1.0f - ammo->age) * 100.0f) ? 1 : 0) | (ammo->overheated != 0 ? 4 : 0) |
            (100 - hud_percent(ammo->age) == 0 ? 2 : 0)));
        // the secondary magazine lands in entries 0 and 1 again (see the header)
        overlay_types[0] = hud_overlay_type_bits((uint16_t)(
            (secondary->rounds_unloaded <= hud->total_ammo_cutoff && secondary->reloading == 0 ? 1 : 0) |
            (secondary->reloading != 0 ? 4 : 0) | (secondary->rounds_unloaded == 0 ? 2 : 0)));
        overlay_types[1] = hud_overlay_type_bits((uint16_t)(
            (secondary->rounds_loaded <= hud->loaded_ammo_cutoff ? 1 : 0) | (secondary->reloading != 0 ? 4 : 0) |
            (secondary->rounds_loaded == 0 ? 2 : 0)));

        // numbers
        numbers[0] = primary->rounds_unloaded;
        numbers[1] = primary->rounds_loaded;
        numbers[2] = (int16_t)__ftol((double)(ammo->heat * 255.0f));
        numbers[3] = (int16_t)__ftol((double)((1.0f - ammo->age) * 100.0f));
        numbers[4] = secondary->rounds_unloaded;
        numbers[5] = secondary->rounds_loaded;

        // distance and elevation to the aim assist target
        {
            local_player_control *control = &player_control_globals_ptr->local_players[local_player_index];
            datum_index target = control->nameplate_target;
            datum_index valid_target = (datum_index)-1;

            if (target != (datum_index)-1 && (int16_t)target >= 0 && (int16_t)target < object_data->maximum_count) {
                uint8_t *header = (uint8_t *)object_data->data + (int16_t)target * object_data->size;
                int16_t salt = (int16_t)((uint32_t)target >> 16);

                if (*(int16_t *)header != 0 && (salt == 0 || *(int16_t *)header == salt) &&
                    (1u << (header[3] & 0x1f)) != 0 && *(void **)(header + 8) != 0) {
                    valid_target = target;
                }
            }
            if (control->nameplate_weight == 1.0f && valid_target != (datum_index)-1) {
                datum_index unit_index = (datum_index)-1;
                real_point3d camera;
                real_point3d position;

                if (local_player_index != -1 && local_player_index < 1 &&
                    local_player_globals->local_players[local_player_index] != (datum_index)-1) {
                    unit_index = ((player *)((uint8_t *)player_data->data +
                                             (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
                }
                unit_get_camera_position(unit_index, &camera);
                object_get_position(&position, valid_target);
                {
                    float dx = camera.x - position.x;
                    float dy = camera.y - position.y;
                    float dz = camera.z - position.z;
                    values[6] = sqrtf(dz * dz + dy * dy + dx * dx) * 3.048f;
                }
                values[7] = (position.z - camera.z) * 3.048f;
            } else {
                *(uint32_t *)&values[6] = 0xffc00000;
                *(uint32_t *)&values[7] = 0xffc00000;
            }
        }
    }

    if (*(datum_index *)&hud->child_hud.tag_id != (datum_index)-1) {
        hud_weapon_interface_draw_elements(*(datum_index *)&hud->child_hud.tag_id, local_player_index, weapon_tag, ammo,
                                           state_flags, overlay_types, numbers);
    }

    view_mask = (*(int16_t *)((uint8_t *)global_scenario + 0x3c) != 2 ? 1 : 0) |
                (local_player_globals->local_player_count == 1 ? 2 : 0) | (local_player_globals->local_player_count > 1 ? 4 : 0);

    for (i = 0; (int32_t)i < (int32_t)hud->static_elements.count; i++) {
        WeaponHUDInterfaceStaticElement *element = (WeaponHUDInterfaceStaticElement *)hud->static_elements.pointer + i;
        int16_t state = element->state_attached_to;

        if (*(int32_t *)&element->flash_period == 0x3f80) {
            element->flash_period = 1.0f;
        }
        if (*(int32_t *)&element->flash_length == 0x3f80) {
            element->flash_length = 1.0f;
        }
        if ((((uint8_t *)element)[2] & 1) != 0 || (view_mask & (1u << *(uint8_t *)&element->allowed_view_type)) == 0) {
            continue;
        }
        hud_draw_static_element(local_player_index, (uint16_t *)&hud->anchor,
                                (const hud_static_element_placement *)&element->anchor_offset, state_flags[state],
                                flash_start_times[state]);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->meter_elements.count; i++) {
        WeaponHUDInterfaceMeter *element = (WeaponHUDInterfaceMeter *)hud->meter_elements.pointer + i;
        int16_t state = element->state_attached_to;
        uint8_t value;

        if ((((uint8_t *)element)[2] & 1) != 0 || (view_mask & (1u << *(uint8_t *)&element->allowed_view_type)) == 0) {
            continue;
        }
        value = (uint8_t)numbers[state];
        hud_meter_draw_fill(&hud->anchor, value, value, (uint32_t)(int16_t)state_flags[state],
                            (float)flash_start_times[state], 0.0f,
                            (const hud_meter_placement *)&element->anchor_offset);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->number_elements.count; i++) {
        WeaponHUDInterfaceNumber *element = (WeaponHUDInterfaceNumber *)hud->number_elements.pointer + i;
        int16_t state = element->state_attached_to;
        int16_t divisor;
        int16_t value;
        int16_t fraction;

        if ((((uint8_t *)element)[2] & 1) != 0 || (view_mask & (1u << *(uint8_t *)&element->allowed_view_type)) == 0) {
            continue;
        }
        divisor = 1;
        if ((*(uint8_t *)&element->weapon_specific_flags & 1) != 0) { // divide number by clip size
            divisor = ((WeaponMagazine *)weapon_tag->magazines.pointer)->rounds_loaded_maximum;
        }
        if (element->number_of_fractional_digits != 0) {
            float power;
            float scaled;

            if (*(uint32_t *)&values[state] == 0xffc00000) {
                continue;
            }
            power = (float)pow(10.0, 4.0);
            scaled = power * values[state];
            fraction = (int16_t)lrint(fmod((double)(scaled < 0.0f ? -scaled : scaled), (double)power));
            value = (int16_t)ui_real_to_int_truncate(values[state] / (float)divisor);
        } else {
            fraction = -1;
            value = (int16_t)(numbers[state] / divisor);
        }
        hud_draw_number((void *)(int32_t)local_player_index, (uint16_t *)&hud->anchor,
                        (const hud_number_placement *)&element->anchor_offset, value, fraction, state_flags[state],
                        flash_start_times[state], 0.0f);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->overlay_elements.count; i++) {
        WeaponHUDInterfaceOverlayElement *element = (WeaponHUDInterfaceOverlayElement *)hud->overlay_elements.pointer + i;
        int16_t state = element->state_attached_to;

        if ((((uint8_t *)element)[2] & 1) != 0 || (view_mask & (1u << *(uint8_t *)&element->allowed_view_type)) == 0) {
            continue;
        }
        hud_draw_overlays((uint16_t *)&hud->anchor, (const hud_overlay_list *)&element->overlay_bitmap,
                          (uint32_t)(int16_t)overlay_types[state], flash_start_times[state], state_flags[state],
                          local_player_globals->local_player_count > 1);
    }
}

#if 0
Original Ghidra decompilation (0x4b1ff0) -- the rewrite above is a close, mechanical
translation of this; see the header comment for exactly which parts are and are not
independently verified:


void FUN_004b1ff0(uint param_1,undefined4 param_2,int param_3,float *param_4,undefined4 *param_5,
                 undefined4 *param_6,undefined4 *param_7)

{
  int *piVar1;
  short sVar2;
  int iVar3;
  ushort uVar4;
  int *piVar5;
  int iVar6;
  short *psVar7;
  int iVar8;
  ushort *puVar9;
  uint uVar10;
  short sVar11;
  int iVar12;
  short sVar13;
  float10 fVar14;
  undefined4 uVar15;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  ushort local_50 [3];
  byte bStack_4a;
  undefined1 uStack_49;
  byte bStack_48;
  undefined1 uStack_47;
  byte local_46;
  undefined1 uStack_45;
  undefined2 uStack_44;
  undefined2 local_42;
  ushort local_40 [2];
  byte bStack_3c;
  undefined1 uStack_3b;
  byte bStack_3a;
  undefined1 uStack_39;
  undefined2 uStack_38;
  undefined2 local_36;
  undefined2 uStack_34;
  undefined2 local_32;
  short local_30 [8];
  float local_20 [8];
  
  iVar3 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  piVar1 = (int *)(DAT_00719430 + (short)param_2 * 0x28);
  local_40[1] = 0;
  bStack_3c = 0;
  uStack_3b = 0;
  bStack_3a = 0;
  uStack_39 = 0;
  uStack_38 = 0;
  local_36 = 0;
  uStack_34 = 0;
  bStack_48 = 0;
  uStack_47 = 0;
  local_32 = 0;
  local_20[0] = 0.0;
  local_46 = 0;
  uStack_45 = 0;
  uStack_44 = 0;
  local_20[1] = 0.0;
  local_42 = 0;
  local_20[2] = 0.0;
  local_30[1] = 0;
  local_30[2] = 0;
  local_20[3] = 0.0;
  local_30[3] = 0;
  local_30[4] = 0;
  local_30[5] = 0;
  local_30[6] = 0;
  local_20[4] = 0.0;
  local_40[0] = 0;
  local_30[0] = 0;
  local_30[7] = 0;
  local_20[5] = 0.0;
  local_20[6] = 0.0;
  local_20[7] = 0.0;
  if (((((*(byte *)(iVar3 + 0x10) & 1) != 0) && (param_5 != (undefined4 *)0x0)) &&
      (param_6 != (undefined4 *)0x0)) && (param_7 != (undefined4 *)0x0)) {
    local_50[0] = (ushort)*param_5;
    local_50[1] = (ushort)((uint)*param_5 >> 0x10);
    uVar15 = param_5[1];
    local_50[2] = (ushort)uVar15;
    bStack_4a = (byte)((uint)uVar15 >> 0x10);
    uStack_49 = (undefined1)((uint)uVar15 >> 0x18);
    uVar15 = param_5[2];
    bStack_48 = (byte)uVar15;
    uStack_47 = (undefined1)((uint)uVar15 >> 8);
    local_46 = (byte)((uint)uVar15 >> 0x10);
    uStack_45 = (undefined1)((uint)uVar15 >> 0x18);
    uStack_44 = (undefined2)param_5[3];
    local_42 = (undefined2)((uint)param_5[3] >> 0x10);
    local_40[0] = (ushort)*param_6;
    local_40[1] = (ushort)((uint)*param_6 >> 0x10);
    uVar15 = param_6[1];
    bStack_3c = (byte)uVar15;
    uStack_3b = (undefined1)((uint)uVar15 >> 8);
    bStack_3a = (byte)((uint)uVar15 >> 0x10);
    uStack_39 = (undefined1)((uint)uVar15 >> 0x18);
    uStack_38 = (undefined2)param_6[2];
    local_36 = (undefined2)((uint)param_6[2] >> 0x10);
    uStack_34 = (undefined2)param_6[3];
    local_32 = (undefined2)((uint)param_6[3] >> 0x10);
    local_30[0] = (short)*param_7;
    local_30[1] = (short)((uint)*param_7 >> 0x10);
    local_30[2] = (short)param_7[1];
    local_30[3] = (short)((uint)param_7[1] >> 0x10);
    local_30[4] = (short)param_7[2];
    local_30[5] = (short)((uint)param_7[2] >> 0x10);
    local_30[6] = (short)param_7[3];
    local_30[7] = (short)((uint)param_7[3] >> 0x10);
    goto LAB_004b2796;
  }
  local_50[0] = (ushort)(*(short *)((int)param_4 + 0x12) <= *(short *)(iVar3 + 0x14));
  if (*(short *)((int)param_4 + 0x12) == 0) {
    local_50[0] = local_50[0] | 2;
  }
  sVar11 = *(short *)(DAT_0087a478 + 0xc);
  if (1 < sVar11) {
    local_50[0] = local_50[0] | 4;
  }
  if ((*(short *)(iVar3 + 0x16) < *(short *)((int)param_4 + 0xe)) ||
     (*(char *)(param_4 + 3) != '\0')) {
    local_50[1] = 0;
  }
  else {
    local_50[1] = 1;
  }
  if (1 < sVar11) {
    local_50[1] = local_50[1] | 4;
  }
  local_50[2] = (ushort)((float)(int)*(short *)(iVar3 + 0x18) < *param_4 * 100.0 !=
                        ((float)(int)*(short *)(iVar3 + 0x18) == *param_4 * 100.0));
  if (1 < sVar11) {
    local_50[2] = local_50[2] | 4;
  }
  bStack_4a = (1.0 - param_4[1]) * 100.0 <= (float)(int)*(short *)(iVar3 + 0x1a);
  uStack_49 = 0;
  iVar6 = FUN_004ab590(param_4[1] * 100.0);
  if (iVar6 < 0) {
    iVar6 = 0;
  }
  else {
    iVar6 = FUN_004ab590(param_4[1] * 100.0);
    if (iVar6 < 0x65) {
      iVar6 = FUN_004ab590(param_4[1] * 100.0);
    }
    else {
      iVar6 = 100;
    }
  }
  iVar12 = DAT_006f1d6c;
  if (iVar6 == 100) {
    bStack_4a = bStack_4a | 2;
  }
  else {
    bStack_4a = bStack_4a & 0xfd;
  }
  if (*(short *)(DAT_0087a478 + 0xc) < 2) {
    bStack_4a = bStack_4a & 0xfb;
  }
  else {
    bStack_4a = bStack_4a | 4;
  }
  if (*(short *)(iVar3 + 0x14) < *(short *)(param_4 + 7)) {
    bStack_48 = bStack_48 & 0xfe;
  }
  else {
    bStack_48 = bStack_48 | 1;
  }
  if (*(short *)(param_4 + 7) == 0) {
    bStack_48 = bStack_48 | 2;
  }
  else {
    bStack_48 = bStack_48 & 0xfd;
  }
  if (*(short *)(DAT_0087a478 + 0xc) < 2) {
    bStack_48 = bStack_48 & 0xfb;
  }
  else {
    bStack_48 = bStack_48 | 4;
  }
  if ((*(short *)(iVar3 + 0x16) < *(short *)(param_4 + 6)) ||
     (*(char *)((int)param_4 + 0x16) != '\0')) {
    local_46 = local_46 & 0xfe;
  }
  else {
    local_46 = local_46 | 1;
  }
  if (*(short *)(DAT_0087a478 + 0xc) < 2) {
    local_46 = local_46 & 0xf9;
  }
  else {
    local_46 = local_46 & 0xfd | 4;
  }
  puVar9 = local_50;
  iVar6 = 8;
  piVar5 = piVar1;
  do {
    if ((*puVar9 & 1) == 0) {
      *piVar5 = -1;
    }
    else if (*piVar5 == -1) {
      *piVar5 = *(int *)(iVar12 + 0xc);
    }
    puVar9 = puVar9 + 1;
    piVar5 = piVar5 + 1;
    iVar6 = iVar6 + -1;
  } while (iVar6 != 0);
  if ((*(short *)(iVar3 + 0x14) < *(short *)((int)param_4 + 0x12)) ||
     (*(char *)(param_4 + 3) != '\0')) {
    local_40[0] = local_40[0] & 0xfffe;
  }
  else {
    local_40[0] = local_40[0] | 1;
  }
  if (*(char *)(param_4 + 3) == '\0') {
    local_40[0] = local_40[0] & 0xfffb;
  }
  else {
    local_40[0] = local_40[0] | 4;
  }
  if (*(short *)((int)param_4 + 0x12) == 0) {
    local_40[0] = local_40[0] | 2;
  }
  else {
    local_40[0] = local_40[0] & 0xfffd;
  }
  if (local_40[0] == 0) {
    local_40[0] = 8;
  }
  else {
    local_40[0] = local_40[0] & 0xfff7;
  }
  local_40[0] = local_40[0] | 0x10;
  if (*(short *)(iVar3 + 0x16) < *(short *)((int)param_4 + 0xe)) {
    local_40[1] = local_40[1] & 0xfffe;
  }
  else {
    local_40[1] = local_40[1] | 1;
  }
  if (*(char *)(param_4 + 3) == '\0') {
    local_40[1] = local_40[1] & 0xfffb;
  }
  else {
    local_40[1] = local_40[1] | 4;
  }
  if (*(short *)((int)param_4 + 0xe) == 0) {
    local_40[1] = local_40[1] | 2;
  }
  else {
    local_40[1] = local_40[1] & 0xfffd;
  }
  if (local_40[1] == 0) {
    local_40[1] = 8;
  }
  else {
    local_40[1] = local_40[1] & 0xfff7;
  }
  local_40[1] = local_40[1] | 0x10;
  if ((float)(int)*(short *)(iVar3 + 0x18) < *param_4 * 100.0 ==
      ((float)(int)*(short *)(iVar3 + 0x18) == *param_4 * 100.0)) {
    bStack_3c = bStack_3c & 0xfe;
  }
  else {
    bStack_3c = bStack_3c | 1;
  }
  if (*(char *)(param_4 + 2) == '\0') {
    bStack_3c = bStack_3c & 0xfb;
  }
  else {
    bStack_3c = bStack_3c | 4;
  }
  iVar6 = FUN_004ab590(param_4[1] * 100.0);
  if (iVar6 < 0) {
    iVar6 = 0;
  }
  else {
    iVar6 = FUN_004ab590(param_4[1] * 100.0);
    if (iVar6 < 0x65) {
      iVar6 = FUN_004ab590(param_4[1] * 100.0);
    }
    else {
      iVar6 = 100;
    }
  }
  if (iVar6 == 100) {
    uVar4 = CONCAT11(uStack_3b,bStack_3c) | 2;
  }
  else {
    uVar4 = CONCAT11(uStack_3b,bStack_3c) & 0xfffd;
  }
  if (uVar4 == 0) {
    uVar4 = 8;
  }
  else {
    uVar4 = uVar4 & 0xfff7;
  }
  bStack_3c = (byte)uVar4 | 0x10;
  uStack_3b = (undefined1)(uVar4 >> 8);
  if ((float)(int)*(short *)(iVar3 + 0x1a) < (1.0 - param_4[1]) * 100.0) {
    bStack_3a = bStack_3a & 0xfe;
  }
  else {
    bStack_3a = bStack_3a | 1;
  }
  if (*(char *)(param_4 + 2) == '\0') {
    bStack_3a = bStack_3a & 0xfb;
  }
  else {
    bStack_3a = bStack_3a | 4;
  }
  iVar6 = FUN_004ab590(param_4[1] * 100.0);
  if (iVar6 < 0) {
    iVar6 = 0;
  }
  else {
    iVar6 = FUN_004ab590(param_4[1] * 100.0);
    if (iVar6 < 0x65) {
      iVar6 = FUN_004ab590(param_4[1] * 100.0);
    }
    else {
      iVar6 = 100;
    }
  }
  if (iVar6 == 100) {
    uVar4 = CONCAT11(uStack_39,bStack_3a) | 2;
  }
  else {
    uVar4 = CONCAT11(uStack_39,bStack_3a) & 0xfffd;
  }
  if (uVar4 == 0) {
    uVar4 = 8;
  }
  else {
    uVar4 = uVar4 & 0xfff7;
  }
  sVar11 = *(short *)(param_4 + 7);
  bStack_3a = (byte)uVar4 | 0x10;
  uStack_39 = (undefined1)(uVar4 >> 8);
  if ((*(short *)(iVar3 + 0x14) < sVar11) || (*(char *)((int)param_4 + 0x16) != '\0')) {
    local_40[0] = local_40[0] & 0xfffe;
  }
  else {
    local_40[0] = local_40[0] | 1;
  }
  if (*(char *)((int)param_4 + 0x16) == '\0') {
    local_40[0] = local_40[0] & 0xfffb;
  }
  else {
    local_40[0] = local_40[0] | 4;
  }
  if (sVar11 == 0) {
    local_40[0] = local_40[0] | 2;
  }
  else {
    local_40[0] = local_40[0] & 0xfffd;
  }
  if (local_40[0] == 0) {
    local_40[0] = 8;
  }
  else {
    local_40[0] = local_40[0] & 0xfff7;
  }
  sVar13 = *(short *)(param_4 + 6);
  local_40[0] = local_40[0] | 0x10;
  if (*(short *)(iVar3 + 0x16) < sVar13) {
    local_40[1] = local_40[1] & 0xfffe;
  }
  else {
    local_40[1] = local_40[1] | 1;
  }
  if (*(char *)((int)param_4 + 0x16) == '\0') {
    local_40[1] = local_40[1] & 0xfffb;
  }
  else {
    local_40[1] = local_40[1] | 4;
  }
  if (sVar13 == 0) {
    local_40[1] = local_40[1] | 2;
  }
  else {
    local_40[1] = local_40[1] & 0xfffd;
  }
  if (local_40[1] == 0) {
    local_40[1] = 8;
  }
  else {
    local_40[1] = local_40[1] & 0xfff7;
  }
  local_30[0] = *(short *)((int)param_4 + 0x12);
  local_40[1] = local_40[1] | 0x10;
  local_30[1] = *(undefined2 *)((int)param_4 + 0xe);
  local_30[2] = __ftol();
  local_30[3] = __ftol();
  local_30[5] = sVar13;
  iVar6 = (short)param_2 * 0x40 + DAT_006b145c;
  local_30[4] = sVar11;
  iVar12 = *(int *)(iVar6 + 0x38);
  if (((iVar12 == -1) || (sVar11 = (short)iVar12, sVar11 < 0)) ||
     (*(short *)(DAT_008603b0 + 0x20) <= sVar11)) {
LAB_004b26c1:
    iVar12 = -1;
  }
  else {
    iVar8 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar11;
    sVar11 = *(short *)(iVar8 + *(int *)(DAT_008603b0 + 0x34));
    iVar8 = iVar8 + *(int *)(DAT_008603b0 + 0x34);
    if ((((sVar11 == 0) ||
         ((sVar13 = (short)((uint)iVar12 >> 0x10), sVar13 != 0 && (sVar11 != sVar13)))) ||
        (1 << (*(byte *)(iVar8 + 3) & 0x1f) == 0)) || (*(int *)(iVar8 + 8) == 0)) goto LAB_004b26c1;
  }
  if ((*(int *)(iVar6 + 0x3c) == 0x3f800000) && (iVar12 != -1)) {
    unit_get_camera_position();
    object_get_position();
    local_20[6] = SQRT((local_5c - local_68) * (local_5c - local_68) +
                       (local_58 - local_64) * (local_58 - local_64) +
                       (local_54 - local_60) * (local_54 - local_60)) * 3.048;
    local_20[7] = (local_60 - local_54) * 3.048;
  }
  else {
    local_20[6] = -NAN;
    local_20[7] = -NAN;
  }
LAB_004b2796:
  if (*(int *)(iVar3 + 0xc) != -1) {
    FUN_004b1ff0(*(int *)(iVar3 + 0xc),param_2,param_3,param_4,local_50,local_40,local_30);
  }
  uVar4 = (ushort)(*(short *)(global_scenario + 0x3c) != 2);
  if (*(short *)(DAT_0087a478 + 0xc) == 1) {
    uVar4 = uVar4 | 2;
  }
  if (1 < *(short *)(DAT_0087a478 + 0xc)) {
    uVar4 = uVar4 | 4;
  }
  sVar11 = 0;
  if (0 < *(int *)(iVar3 + 0x60)) {
    iVar6 = 0;
    do {
      psVar7 = (short *)(iVar6 * 0xb4 + *(int *)(iVar3 + 100));
      if (*(int *)(iVar6 * 0xb4 + 0x60 + *(int *)(iVar3 + 100)) == 0x3f80) {
        psVar7[0x30] = 0;
        psVar7[0x31] = 0x3f80;
      }
      if (*(int *)(psVar7 + 0x36) == 0x3f80) {
        psVar7[0x36] = 0;
        psVar7[0x37] = 0x3f80;
      }
      if (((*(byte *)(psVar7 + 1) & 1) == 0) &&
         (((int)(short)uVar4 & 1 << (*(byte *)(psVar7 + 2) & 0x1f)) != 0)) {
        FUN_004ac6f0(param_2,iVar3 + 0x3c,psVar7 + 0x12,(int)(short)local_50[*psVar7],
                     piVar1[*psVar7]);
      }
      sVar11 = sVar11 + 1;
      iVar6 = (int)sVar11;
    } while (iVar6 < *(int *)(iVar3 + 0x60));
  }
  sVar11 = 0;
  if (0 < *(int *)(iVar3 + 0x6c)) {
    iVar6 = 0;
    do {
      psVar7 = (short *)(iVar6 * 0xb4 + *(int *)(iVar3 + 0x70));
      if (((*(byte *)(psVar7 + 1) & 1) == 0) &&
         (((int)(short)uVar4 & 1 << (*(byte *)(psVar7 + 2) & 0x1f)) != 0)) {
        sVar13 = *psVar7;
        FUN_004abbc0(iVar3 + 0x3c,(char)local_30[sVar13],(char)local_30[sVar13],
                     (int)(short)local_50[sVar13],(float)piVar1[sVar13],0);
      }
      sVar11 = sVar11 + 1;
      iVar6 = (int)sVar11;
    } while (iVar6 < *(int *)(iVar3 + 0x6c));
  }
  sVar11 = 0;
  if (0 < *(int *)(iVar3 + 0x78)) {
    iVar6 = 0;
    do {
      psVar7 = (short *)(iVar6 * 0xa0 + *(int *)(iVar3 + 0x7c));
      if (((*(byte *)(iVar6 * 0xa0 + 2 + *(int *)(iVar3 + 0x7c)) & 1) == 0) &&
         (((int)(short)uVar4 & 1 << (*(byte *)(psVar7 + 2) & 0x1f)) != 0)) {
        sVar13 = 1;
        if ((*(byte *)(psVar7 + 0x3c) & 1) != 0) {
          sVar13 = *(short *)(*(int *)(param_3 + 0x4f4) + 10);
        }
        if ((char)psVar7[0x35] == '\0') {
          sVar2 = *psVar7;
          iVar12 = piVar1[sVar2];
          uVar10 = (uint)local_50[sVar2];
          uVar15 = 0;
          iVar6 = -1;
          iVar8 = (int)local_30[sVar2] / (int)sVar13;
        }
        else {
          if (local_20[*psVar7] == -NAN) goto LAB_004b2a1b;
          FUN_006283c0();
          fVar14 = (float10)FUN_00628cca();
          iVar6 = (int)ROUND((float)fVar14);
          sVar2 = *psVar7;
          iVar12 = piVar1[sVar2];
          uVar10 = (uint)local_50[sVar2];
          uVar15 = 0;
          iVar8 = FUN_004ab590(local_20[sVar2] / (float)(int)sVar13,iVar6,uVar10,iVar12,0);
        }
        FUN_004ac0b0(param_2,iVar3 + 0x3c,psVar7 + 0x12,iVar8,iVar6,uVar10,iVar12,uVar15);
      }
LAB_004b2a1b:
      sVar11 = sVar11 + 1;
      iVar6 = (int)sVar11;
    } while (iVar6 < *(int *)(iVar3 + 0x78));
  }
  sVar11 = 0;
  if (0 < *(int *)(iVar3 + 0x90)) {
    iVar6 = 0;
    do {
      psVar7 = (short *)(iVar6 * 0x68 + *(int *)(iVar3 + 0x94));
      if (((*(byte *)(iVar6 * 0x68 + 2 + *(int *)(iVar3 + 0x94)) & 1) == 0) &&
         (((int)(short)uVar4 & 1 << (*(byte *)(psVar7 + 2) & 0x1f)) != 0)) {
        sVar13 = *psVar7;
        FUN_004ac950(iVar3 + 0x3c,psVar7 + 0x12,(int)(short)local_40[sVar13],piVar1[sVar13],
                     local_50[sVar13],1 < *(short *)(DAT_0087a478 + 0xc));
      }
      sVar11 = sVar11 + 1;
      iVar6 = (int)sVar11;
    } while (iVar6 < *(int *)(iVar3 + 0x90));
  }
  return;
}

#endif

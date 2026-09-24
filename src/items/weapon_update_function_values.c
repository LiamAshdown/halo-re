// weapon_update_function_values  (Ghidra: missed_4c2110, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the weapon object_type_definition
// row)
// address 0x4c2110, size 886 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: the weapon row (0x0069b748) carries this address at +0x38, the same column that
//   holds projectile_update_function_values (0x4c0250, this module's already-written analog) on
//   the projectile row; that function's own evidence and naming precedent apply here directly.
//   types/tags.h WeaponFunctionIn (the numeric switch cases below are exactly its 18 values,
//   1..0x12 -- none/heat/primary_ammunition/secondary_ammunition/primary_rate_of_fire/
//   secondary_rate_of_fire/ready/primary_ejection_port/secondary_ejection_port/overheated/
//   primary_charged/secondary_charged/illumination/age/integrated_light/primary_firing/
//   secondary_firing/primary_firing_on/secondary_firing_on), Weapon.weapon_a_in..d_in (tag
//   0x330, four contiguous WeaponFunctionIn_t), Weapon.magazines (0x4f0 TagReflexive),
//   Weapon.triggers (0x4fc TagReflexive), WeaponMagazine.rounds_loaded_maximum (0x0a),
//   WeaponTrigger.charging_time (0x48) / .charged_illumination (0x54), Weapon
//   .heat_recovery_threshold (0x34c) / .heat_illumination (0x360); types/objects.h
//   object.parent_object (0x11c), object.function_in_values (0x124); types/items.h weapon_data
//   (flags 0x22c -- _weapon_overheated_bit tested, not the weapon_type-3 bit; heat 0x23c, age
//   0x240, charged_fraction 0x244, ready_timer 0x248, last_fire_game_time 0x2d0,
//   magazines[].rounds_loaded 0x08 relative), weapon_trigger_state (effect_state 0x01,
//   firing_rate 0x10, ejection_port_recovery 0x14, illumination_recovery 0x18),
//   weapon_trigger_effect_state (_weapon_trigger_effect_charged 3). Callees
//   weapon_trigger_get_charge_fraction (0x4c3100, this module) and weapon_is_reloading
//   (0x4c2ad0, this module), both confirmed against `objdump -d -M intel
//   --start-address=0x4c2110 --stop-address=0x4c2470 bin/halo.exe`: every call reloads EAX from
//   the live weapon_object_index (esi) first, matching their own declared EAX -> item_index
//   convention, with the trigger index pushed on the stack immediately before each call.
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family (`mov esi,[esp+0x24]` at entry, the extra
//   offset coming from this function's own register saves).
// blam-cc: stack -> object_index
// UNSURE: the walk up object.parent_object is gated on the CURRENT object's own
//   _object_no_collision_bit (object+0x10, re-tested after each step against the new object).
//   Only the destination of the four values moves to the outermost object of that chain (ecx,
//   0x4c2149..0x4c216d); every weapon_data read still uses the weapon itself (ebx).
// Cleanup-pass review (objdump 0x4c2110..0x4c2485, jump table 0x4c2488): the draft read
//   weapon_data from the walked parent (the binary reads it from the weapon, ebx) and made
//   primary/secondary_firing_on always 0 (the binary loads the trigger's firing_rate at 0x4c2426
//   and zeroes it only when the magazine is empty, the weapon is overheated or reloading). The
//   illumination maxima now follow the x87 test (fcomp / test ah,0x41): the candidate replaces the
//   running peak unless the peak is strictly greater, so an unordered compare takes the candidate.
// UNSURE: case weaponfunctionin_ready (6) is a bare 1.0 literal in the binary, never reading any
//   state; reproduced as-is rather than guessed into something state-dependent.
// UNSURE: case weaponfunctionin_integrated_light (14) reads weapon_data.ready_timer (0x248), the
//   same field types/items.h's own header already calls UNSURE about its true meaning; this may
//   be evidence the field is closer to a generic "progress" timer than specifically a ready-
//   animation countdown, but that rename is not made here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern void *game_time;     // 0x006f1d6c, +0x0c the game tick

extern real weapon_trigger_get_charge_fraction(datum_index item_index, int16_t trigger_index); // 0x4c3100, this module
extern int32_t weapon_is_reloading(datum_index item_index); // 0x4c2ad0, this module

void weapon_update_function_values(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Weapon *tag = (Weapon *)tag_instances[(uint16_t)obj->definition_tag].data;
    object *destination = obj;

    // Walk up the parent chain, exactly as the binary does: while the current object's own
    // _object_no_collision_bit is set and it has a parent, follow parent_object. Only the
    // destination of the values moves; see UNSURE note above.
    while ((destination->flags & _object_no_collision_bit) != 0 &&
           destination->parent_object != (datum_index)k_datum_index_none) {
        destination = ((object_header *)object_data->data)[destination->parent_object & 0xffff].data;
    }

    {
        weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
        WeaponFunctionIn_t *sources = &tag->weapon_a_in; // four contiguous int16 fields
        float *function_in = destination->function_in_values;
        WeaponMagazine *tag_magazine = (WeaponMagazine *)tag->magazines.pointer;
        WeaponTrigger *tag_trigger = (WeaponTrigger *)tag->triggers.pointer;
        int32_t k;

        for (k = 0; k < 4; k++) {
            int16_t source = sources[k];
            real value = 0.0f;

            if (source != weaponfunctionin_none) {
                int16_t idx;

                switch (source) {
                case weaponfunctionin_heat:
                    value = wd->heat;
                    break;

                case weaponfunctionin_primary_ammunition:
                case weaponfunctionin_secondary_ammunition:
                    idx = (int16_t)(source - weaponfunctionin_primary_ammunition);
                    if (idx < (int32_t)tag->magazines.count && tag_magazine[idx].rounds_loaded_maximum != 0) {
                        value = (real)wd->magazines[idx].rounds_loaded / (real)tag_magazine[idx].rounds_loaded_maximum;
                    }
                    break;

                case weaponfunctionin_primary_rate_of_fire:
                case weaponfunctionin_secondary_rate_of_fire:
                    idx = (int16_t)(source - weaponfunctionin_primary_rate_of_fire);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                    }
                    break;

                case weaponfunctionin_ready:
                    value = 1.0f;
                    break;

                case weaponfunctionin_primary_ejection_port:
                case weaponfunctionin_secondary_ejection_port:
                    idx = (int16_t)(source - weaponfunctionin_primary_ejection_port);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].ejection_port_recovery;
                    }
                    break;

                case weaponfunctionin_overheated:
                    if ((wd->flags & _weapon_overheated_bit) != 0 && tag->heat_recovery_threshold != 1.0f) {
                        value = (wd->heat - tag->heat_recovery_threshold) / (1.0f - tag->heat_recovery_threshold);
                    }
                    break;

                case weaponfunctionin_primary_charged:
                case weaponfunctionin_secondary_charged:
                    idx = (int16_t)(source - weaponfunctionin_primary_charged);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = weapon_trigger_get_charge_fraction(object_index, idx);
                    }
                    break;

                case weaponfunctionin_illumination: {
                    real peak = 0.0f;
                    int16_t i;

                    for (i = 0; i < (int32_t)tag->triggers.count; i++) {
                        real charge_illum = 0.0f;

                        if (tag_trigger[i].charging_time > 0.0f) {
                            charge_illum = weapon_trigger_get_charge_fraction(object_index, i) *
                                           tag_trigger[i].charged_illumination;
                            if (!(peak > charge_illum)) {
                                peak = charge_illum;
                            }
                        }
                        if (wd->triggers[i].effect_state == _weapon_trigger_effect_charged) {
                            real charged_blend = (1.0f - tag_trigger[i].charged_illumination) * wd->charged_fraction +
                                                  tag_trigger[i].charged_illumination;
                            if (!(peak > charged_blend)) {
                                peak = charged_blend;
                            }
                        }
                        if (!(peak > wd->triggers[i].illumination_recovery)) {
                            peak = wd->triggers[i].illumination_recovery;
                        }
                        wd->triggers[i].illumination_recovery = peak;
                    }
                    value = peak;
                    {
                        real heat_illum = tag->heat_illumination * wd->heat;
                        if (!(value > heat_illum)) {
                            value = heat_illum;
                        }
                    }
                    break;
                }

                case weaponfunctionin_age:
                    value = wd->age;
                    break;

                case weaponfunctionin_integrated_light:
                    value = wd->ready_timer; // UNSURE, see file header
                    break;

                case weaponfunctionin_primary_firing:
                case weaponfunctionin_secondary_firing:
                    idx = (int16_t)(source - weaponfunctionin_primary_firing);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                        if (*(int32_t *)((uint8_t *)game_time + 0xc) - wd->last_fire_game_time > 1) {
                            value = 0.0f;
                        }
                    }
                    break;

                case weaponfunctionin_primary_firing_on:
                case weaponfunctionin_secondary_firing_on:
                    idx = (int16_t)(source - weaponfunctionin_primary_firing_on);
                    if (idx < (int32_t)tag->triggers.count) {
                        value = wd->triggers[idx].firing_rate;
                        if (wd->magazines[idx].rounds_loaded == 0 ||
                            (wd->flags & _weapon_overheated_bit) != 0 ||
                            (uint8_t)weapon_is_reloading(object_index) != 0) { // test al,al
                            value = 0.0f;
                        }
                    }
                    break;
                }

                function_in[k] = value;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c2110):

void missed_4c2110(uint param_1)

{
  byte bVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint *puVar5;
  float fVar6;
  char cVar7;
  int iVar8;
  float *pfVar9;
  int iVar10;
  float10 fVar11;
  float10 extraout_ST0;
  float local_14;
  short *local_10;
  int local_c;
  int local_4;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  bVar1 = (byte)puVar3[4];
  puVar5 = puVar3;
  while (((bVar1 & 1) != 0 && (puVar5[0x47] != 0xffffffff))) {
    puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar5[0x47] & 0xffff) * 0xc);
    bVar1 = (byte)puVar5[4];
  }
  pfVar9 = (float *)(puVar5 + 0x49);
  local_10 = (short *)(iVar4 + 0x330);
  local_4 = 4;
  do {
    sVar2 = *local_10;
    if (sVar2 != 0) {
      fVar11 = (float10)0.0;
      local_14 = (float)fVar11;
      switch(sVar2) {
      case 1:
        fVar11 = (float10)(float)puVar3[0x8f];
        break;
      case 2:
      case 3:
        iVar8 = (int)(short)(sVar2 + -2);
        if ((iVar8 < *(int *)(iVar4 + 0x4f0)) &&
           (sVar2 = *(short *)(iVar8 * 0x70 + *(int *)(iVar4 + 0x4f4) + 10), sVar2 != 0)) {
          fVar11 = (float10)(int)(short)puVar3[iVar8 * 3 + 0xae] / (float10)(int)sVar2;
        }
        break;
      case 4:
      case 5:
        if ((int)(short)(sVar2 + -4) < *(int *)(iVar4 + 0x4fc)) {
          fVar11 = (float10)(float)puVar3[(short)(sVar2 + -4) * 10 + 0x9c];
        }
        break;
      case 6:
        fVar11 = (float10)1.0;
        break;
      case 7:
      case 8:
        if ((int)(short)(sVar2 + -7) < *(int *)(iVar4 + 0x4fc)) {
          fVar11 = (float10)(float)puVar3[(short)(sVar2 + -7) * 10 + 0x9d];
        }
        break;
      case 9:
        if (((puVar3[0x8b] & 1) != 0) && (*(int *)(iVar4 + 0x34c) != 0x3f800000)) {
          fVar11 = ((float10)(float)puVar3[0x8f] - (float10)*(float *)(iVar4 + 0x34c)) /
                   ((float10)1.0 - (float10)*(float *)(iVar4 + 0x34c));
        }
        break;
      case 10:
      case 0xb:
        iVar8 = CONCAT22((short)((uint)local_10 >> 0x10),sVar2) + -10;
        if ((int)(short)iVar8 < *(int *)(iVar4 + 0x4fc)) {
          fVar11 = (float10)weapon_trigger_get_charge_fraction(iVar8);
        }
        break;
      case 0xc:
        local_c = 0;
        if (0 < *(int *)(iVar4 + 0x4fc)) {
          iVar8 = 0;
          do {
            iVar10 = iVar8 * 0x114 + *(int *)(iVar4 + 0x500);
            if (0.0 < *(float *)(iVar8 * 0x114 + 0x48 + *(int *)(iVar4 + 0x500))) {
              fVar11 = (float10)weapon_trigger_get_charge_fraction(local_c);
              fVar11 = fVar11 * (float10)*(float *)(iVar10 + 0x54);
              if ((float10)local_14 <= fVar11) {
                local_14 = (float)fVar11;
              }
            }
            if ((*(char *)((int)puVar3 + iVar8 * 0x28 + 0x261) == '\x03') &&
               (fVar6 = (1.0 - *(float *)(iVar10 + 0x54)) * (float)puVar3[0x91] +
                        *(float *)(iVar10 + 0x54), local_14 <= fVar6)) {
              local_14 = fVar6;
            }
            fVar11 = (float10)local_14;
            if (fVar11 <= (float10)(float)puVar3[iVar8 * 10 + 0x9e]) {
              local_14 = (float)puVar3[iVar8 * 10 + 0x9e];
              fVar11 = (float10)local_14;
            }
            puVar3[iVar8 * 10 + 0x9e] = (uint)(float)fVar11;
            local_c = local_c + 1;
            iVar8 = (int)(short)local_c;
          } while (iVar8 < *(int *)(iVar4 + 0x4fc));
        }
        fVar6 = *(float *)(iVar4 + 0x360) * (float)puVar3[0x8f];
        if (fVar11 <= (float10)fVar6) {
          fVar11 = (float10)fVar6;
        }
        break;
      case 0xd:
        fVar11 = (float10)(float)puVar3[0x90];
        break;
      case 0xe:
        fVar11 = (float10)(float)puVar3[0x92];
        break;
      case 0xf:
      case 0x10:
        if (((int)(short)(sVar2 + -0xf) < *(int *)(iVar4 + 0x4fc)) &&
           (fVar11 = (float10)(float)puVar3[(short)(sVar2 + -0xf) * 10 + 0x9c],
           1 < (int)(*(int *)(DAT_006f1d6c + 0xc) - puVar3[0xb4]))) {
LAB_004c244f:
          fVar11 = (float10)0.0;
        }
        break;
      case 0x11:
      case 0x12:
        if (((int)(short)(sVar2 + -0x11) < *(int *)(iVar4 + 0x4fc)) &&
           ((((short)puVar3[(short)(sVar2 + -0x11) * 3 + 0xae] == 0 || ((puVar3[0x8b] & 1) != 0)) ||
            (cVar7 = weapon_is_reloading(), fVar11 = extraout_ST0, cVar7 != '\0'))))
        goto LAB_004c244f;
      }
      *pfVar9 = (float)fVar11;
    }
    local_10 = local_10 + 1;
    pfVar9 = pfVar9 + 1;
    local_4 = local_4 + -1;
    if (local_4 == 0) {
      return;
    }
  } while( true );
}
#endif

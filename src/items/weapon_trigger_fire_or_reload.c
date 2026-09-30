// weapon_trigger_fire_or_reload  (Ghidra: FUN_004c3280; named from
// out/phase4/items_functions.md, "Per-tick decision routine for a weapon trigger that fires it,
// forces a reload, or plays an idle/overheat cue depending on ammo and heat state")
// address 0x4c3280, size 485 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4c3280..0x4c3464; the effect-state durations now scale seconds to ticks)
// evidence: types/items.h weapon_data.flags (_weapon_overheated_bit), weapon_trigger_state
//   .flags (_weapon_trigger_charge_effect_bit 0x20), weapon_data.age (0x240); types/tags.h
//   Weapon.weapon_flags (bit 11 = 0x800, UNSURE which flag), WeaponTrigger.charging_time (0x48),
//   .overload_time (0xc4).
// register convention: item index in EAX; trigger index and force flag are Ghidra-recognized
// stack parameters.
// blam-cc: EAX -> item_index, stack -> (trigger_index, force)
// UNSURE: scenario_location_get_water_and_weather is outside this module; its address argument is always the base of
// triggers[0] regardless of trigger_index, which is preserved literally rather than
// "corrected" to triggers[trigger_index].

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack
extern void weapon_fire_trigger(datum_index item_index, int16_t trigger_index); // 0x4c3f10


// Decides what a pulled (or forced) weapon trigger should do this tick: refuse while reloading
// or overheated, otherwise fire immediately, or enter the charging/overload effect state.
void weapon_trigger_fire_or_reload(datum_index item_index, int16_t trigger_index, int8_t force)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    uint8_t ready;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    ready = 1;
    if (tag_trigger->magazine != (uint16_t)-1 && wd->magazines[tag_trigger->magazine].state != 0) {
        ready = 0;
    }
    if ((wd->flags & _weapon_overheated_bit) != 0) {
        ready = 0;
    }

    // 0x4c3307: EBX = the weapon's position (+0x5c), stack: its location (+0x98), no weather output -- a weapon under
    // water does not fire
    if (scenario_location_get_water_and_weather((real_point3d *)((uint8_t *)item_obj + 0x5c),
            (bsp_leaf_reference *)((uint8_t *)item_obj + 0x98), 0) == 0 && ready) {
        if (force == 0) {
            if (tag_trigger->charging_time > 0.0f) {
                if ((weapon_tag->weapon_flags & 0x800) != 0 && wd->age >= 1.0f) {
                    weapon_fire_trigger(item_index, trigger_index);
                    return;
                }
                if (weapon_tag->triggers.count < 2) {
                    if (wd->triggers[trigger_index].firing_rate <= 0.0f) {
                        wd->triggers[trigger_index].flags &= ~0x20; // _weapon_trigger_charge_effect_bit
                    } else {
                        wd->triggers[trigger_index].flags |= 0x20;
                        weapon_fire_trigger(item_index, trigger_index);
                    }
                } else {
                    wd->triggers[trigger_index].effect_handle =
                        weapon_play_trigger_tag_effect(item_index, *(datum_index *)&tag_trigger->charging_effect.tag_id, 0, 0); // UNSURE:
                            // tag_id (EDI) placeholder, see weapon_play_trigger_tag_effect.c
                }
                // 0x4c33e1..0x4c33ea: seconds * 30.0 (0x672ac8) -> ticks. FIXED 2026-09-27: the draft passed the raw
                // seconds, so a charge / overload lasted 1/30 as long.
                weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_charging,
                    (int16_t)(int32_t)(tag_trigger->charging_time * 30.0f));
                return;
            }
            if (tag_trigger->overload_time > 0.0f) {
                weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_overloading,
                    (int16_t)(int32_t)(tag_trigger->overload_time * 30.0f)); // 0x4c341e..0x4c342a
                return;
            }
        }
        weapon_fire_trigger(item_index, trigger_index);
    }
}

#if 0
Original Ghidra decompilation (0x4c3280):

void FUN_004c3280(uint param_1,undefined4 param_2,char param_3)

{
  uint *puVar1;
  int iVar2;
  bool bVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  uint uVar8;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar6 = (int)(short)param_2;
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar7 = iVar6 * 0x114 + *(int *)(iVar2 + 0x500);
  bVar3 = true;
  if ((*(short *)(iVar7 + 0x20) != -1) && ((short)puVar1[*(short *)(iVar7 + 0x20) * 3 + 0xac] != 0))
  {
    bVar3 = false;
  }
  if ((puVar1[0x8b] & 1) != 0) {
    bVar3 = false;
  }
  cVar4 = FUN_0053ed60(puVar1 + 0x26,0);
  if ((cVar4 == '\0') && (bVar3)) {
    if (param_3 == '\0') {
      if (0.0 < *(float *)(iVar7 + 0x48)) {
        if (((*(uint *)(iVar2 + 0x308) & 0x800) != 0) && (1.0 <= (float)puVar1[0x90])) {
          weapon_fire_trigger(param_1,param_2);
          return;
        }
        if (*(int *)(iVar2 + 0x4fc) < 2) {
          if ((float)puVar1[iVar6 * 10 + 0x9c] <= 0.0) {
            puVar1[iVar6 * 10 + 0x99] = puVar1[iVar6 * 10 + 0x99] & 0xffffffdf;
          }
          else {
            puVar1[iVar6 * 10 + 0x99] = puVar1[iVar6 * 10 + 0x99] | 0x20;
            weapon_fire_trigger(param_1,param_2);
          }
        }
        else {
          uVar8 = weapon_play_trigger_tag_effect(0,0);
          puVar1[iVar6 * 10 + 0xa0] = uVar8;
        }
        sVar5 = __ftol();
        weapon_trigger_effect_set_state((short)param_2,'\x02',sVar5);
        return;
      }
      if (0.0 < *(float *)(iVar7 + 0xc4)) {
        sVar5 = __ftol();
        weapon_trigger_effect_set_state((short)param_2,'\x01',sVar5);
        return;
      }
    }
    weapon_fire_trigger(param_1,param_2);
  }
  return;
}
#endif

// weapon_trigger_ready_to_fire  (Ghidra: FUN_004c3190; named per types/items.h,
// "weapon_trigger_ready_to_fire (0x4c3190) compares idle_ticks + 1.0 against
// 30.0 / lerp(maximum_rate_of_fire[0], [1], firing_rate)")
// address 0x4c3190, size 233 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4c3190..0x4c3278 (rate lerp, 30 / rate, age penalty, idle + 1 >= ticks, the not-repeating gate).)
// evidence: types/items.h weapon_trigger_state.idle_ticks/.firing_rate/.flags,
//   item_data.flags (_item_held_by_player_bit); types/tags.h WeaponTrigger
//   .maximum_rate_of_fire[2] (0x04), Weapon.age_rate_of_fire_penalty (0x444),
//   weapon_data.primary_trigger (0x234), weapon_data.age (0x240); WeaponTriggerFlags bit 0x200 =
//   analog_rate_of_fire, bit 0x8 = does_not_repeat_automatically.
// register convention: item index in EAX; trigger index is a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> trigger_index
// UNSURE: the original packs an x87 compare-flags byte (NaN/LT/EQ) into the unused upper 24
// return bits; every caller in this module truncates the result to a char, so only the low-byte
// boolean is preserved here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Evaluates whether a weapon trigger has aged long enough, given its (possibly analog) rate of
// fire and the weapon's age penalty, to fire again. A does_not_repeat_automatically trigger that
// is still held down and pulled is never ready.
int32_t weapon_trigger_ready_to_fire(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    weapon_trigger_state *trigger;
    WeaponTrigger *tag_trigger;
    real rate;
    real ticks_per_shot;
    real idle_plus_one;
    int32_t ready;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    trigger = &wd->triggers[trigger_index];
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    rate = (tag_trigger->flags & 0x200) == 0 ? trigger->firing_rate : wd->primary_trigger;
    rate = (tag_trigger->maximum_rate_of_fire[1] - tag_trigger->maximum_rate_of_fire[0]) * rate +
           tag_trigger->maximum_rate_of_fire[0];

    ticks_per_shot = (rate <= 0.0001f) ? 0.0f : 30.0f / rate;
    if (weapon_tag->age_rate_of_fire_penalty > 0.0f) {
        ticks_per_shot = (wd->age * weapon_tag->age_rate_of_fire_penalty + 1.0f) * ticks_per_shot;
    }

    idle_plus_one = (real)trigger->idle_ticks + 1.0f;
    ready = idle_plus_one >= ticks_per_shot;

    if ((tag_trigger->flags & 8) != 0 && (id->flags & _item_held_by_player_bit) != 0 &&
        (trigger->flags & _weapon_trigger_not_pulled_bit) == 0) {
        ready = 0;
    }
    return ready;
}

#if 0
Original Ghidra decompilation (0x4c3190):

int FUN_004c3190(short param_1)

{
  uint *puVar1;
  float fVar2;
  uint *puVar3;
  int iVar4;
  float fVar5;
  uint in_EAX;
  uint *puVar6;
  uint3 uVar7;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  puVar1 = puVar3 + param_1 * 10 + 0x98;
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar6 = (uint *)(param_1 * 0x114 + *(int *)(iVar4 + 0x500));
  if ((*puVar6 & 0x200) == 0) {
    fVar2 = (float)puVar1[4];
  }
  else {
    fVar2 = (float)puVar3[0x8d];
  }
  fVar2 = ((float)puVar6[2] - (float)puVar6[1]) * fVar2 + (float)puVar6[1];
  if (fVar2 <= 0.0001) {
    fVar2 = 0.0;
  }
  else {
    fVar2 = 30.0 / fVar2;
  }
  if (0.0 < *(float *)(iVar4 + 0x444)) {
    fVar2 = ((float)puVar3[0x90] * *(float *)(iVar4 + 0x444) + 1.0) * fVar2;
  }
  fVar5 = (float)(int)(char)*puVar1 + 1.0;
  uVar7 = (uint3)(CONCAT22((char)*puVar1 >> 7,
                           (ushort)(fVar5 < fVar2) << 8 | (ushort)(NAN(fVar5) || NAN(fVar2)) << 10 |
                           (ushort)(fVar5 == fVar2) << 0xe) >> 8);
  if ((((*puVar6 & 8) != 0) && ((puVar3[0x7d] & 2) != 0)) && ((puVar1[1] & 1) == 0)) {
    return (uint)uVar7 << 8;
  }
  return CONCAT31(uVar7,fVar5 >= fVar2);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

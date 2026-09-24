// weapon_set_loaded_ammo_fraction  (Ghidra: weapon_set_loaded_ammo_fraction, already named)
// address 0x4c58c0, size 284 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h weapon_data.age (0x240) as the battery-level complement, per its own
//   comment block ("weapon_set_loaded_ammo_fraction writes 1.0 - fraction into 0x240 for a
//   weapon with no magazines but a trigger that has age_generated_per_round, which is what
//   identifies it as the battery meter"); types/tags.h WeaponTrigger.age_generated_per_round
//   (0xbc), WeaponMagazine.rounds_loaded_maximum (0x0a).
// register convention: item index in EAX; fraction is a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> fraction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

// floor is a single x87/SSE instruction sequence in the original code (Ghidra's ROUND());
// declared locally instead of via <math.h> because -I types shadows that header name with
// types/math.h.
extern double floor(double x);

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Sets a weapon's ammo or battery level from a 0..1 fraction. A weapon with no magazines, or
// whose first trigger has a positive age_generated_per_round, is treated as a battery weapon and
// gets its age set to the complement of the fraction; otherwise the first magazine's loaded
// count is set to round(rounds_loaded_maximum * fraction), and the reserve is adjusted by the
// same delta so the total ammo held is unchanged.
void weapon_set_loaded_ammo_fraction(datum_index item_index, real fraction)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int32_t is_battery;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (weapon_tag->magazines.count == 0) {
        is_battery = 1;
    } else {
        is_battery = 0;
        {
            int16_t i;
            WeaponTrigger *triggers = (WeaponTrigger *)weapon_tag->triggers.pointer;
            for (i = 0; i < weapon_tag->triggers.count; i++) {
                if (triggers[i].age_generated_per_round > 0.0f) {
                    is_battery = 1;
                    break;
                }
            }
        }
    }

    if (fraction >= 0.0f) {
        if (fraction > 1.0f) fraction = 1.0f;
    } else {
        fraction = 0.0f;
    }

    if (!is_battery) {
        if (weapon_tag->magazines.count > 0) {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
            int16_t new_loaded = (int16_t)(int32_t)floor((double)((real)magazine_tag->rounds_loaded_maximum * fraction) + 0.5);
            int16_t old_loaded = wd->magazines[0].rounds_loaded;

            wd->magazines[0].rounds_loaded = new_loaded;
            wd->magazines[0].rounds_unloaded = wd->magazines[0].rounds_unloaded + (new_loaded - old_loaded);
        }
        return;
    }
    wd->age = 1.0f - fraction;
}

#if 0
Original Ghidra decompilation (0x4c58c0):

void weapon_set_loaded_ammo_fraction(float param_1)

{
  uint *puVar1;
  int iVar2;
  bool bVar3;
  uint uVar4;
  short sVar5;
  uint in_EAX;
  int iVar6;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  bVar3 = false;
  if (*(int *)(iVar2 + 0x4f0) == 0) {
    bVar3 = true;
  }
  else {
    sVar5 = 0;
    if (0 < *(int *)(iVar2 + 0x4fc)) {
      iVar6 = 0;
      do {
        if (0.0 < *(float *)(iVar6 * 0x114 + 0xbc + *(int *)(iVar2 + 0x500))) {
          bVar3 = true;
          goto LAB_004c594c;
        }
        sVar5 = sVar5 + 1;
        iVar6 = (int)sVar5;
      } while (iVar6 < *(int *)(iVar2 + 0x4fc));
      bVar3 = false;
    }
  }
LAB_004c594c:
  if (0.0 <= param_1) {
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
  }
  else {
    param_1 = 0.0;
  }
  if (!bVar3) {
    if (0 < *(int *)(iVar2 + 0x4f0)) {
      sVar5 = (short)(int)ROUND((float)(int)*(short *)(*(int *)(iVar2 + 0x4f4) + 10) * param_1);
      uVar4 = puVar1[0xae];
      *(short *)(puVar1 + 0xae) = sVar5;
      *(short *)((int)puVar1 + 0x2b6) = *(short *)((int)puVar1 + 0x2b6) + (sVar5 - (short)uVar4);
    }
    return;
  }
  puVar1[0x90] = (uint)(1.0 - param_1);
  return;
}
#endif

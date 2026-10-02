// weapon_new_from_placement  (Ghidra: FUN_004c1350; named in
// out/phase4/items_types_notes.md, weapon_data section: "weapon_new_from_placement (0x4c1350)")
// address 0x4c1350, size 208 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/objects.h object_header/object_data (stride 0x0c element lookup),
//   object.flags (0x010, _object_at_rest_bit 0x20, _object_unknown_20000_bit 0x20000),
//   object.position (0x05c); types/tags.h ScenarioWeapon (rounds_reserved 0x48,
//   rounds_loaded 0x4a, flags 0x4c = initially_at_rest/obsolete/does_accelerate),
//   Weapon.magazines (0x4f0 TagReflexive), WeaponMagazine (rounds_reserved_maximum 0x08,
//   rounds_loaded_maximum 0x0a); types/items.h weapon_data.magazines[0], item_data.flags
//   (_item_does_not_accelerate_bit).
// register convention: both arguments are Ghidra-recognized stack parameters already.
// blam-cc: stack -> (weapon_object_index, scenario_weapon_placement)
// UNSURE: the float add of 0.05 to object->position.z decompiles as an int<->float round-trip
// because Ghidra typed the object pointer as uint*; rendered here as the plain float add the
// domain logic (lift a not-initially-resting weapon slightly off the placement point) implies.

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

// Initializes a newly-created weapon object's starting magazine counts, at-rest/acceleration
// flags and resting height from its scenario placement record. Returns the same object index it
// was given.
datum_index weapon_new_from_placement(datum_index weapon_object_index, ScenarioWeapon *placement)
{
    object *weapon_obj;
    Weapon *weapon_tag;
    weapon_data *wd;
    item_data *id;
    int16_t rounds;

    weapon_obj = ((object_header *)object_data->data)[(uint16_t)weapon_object_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)weapon_obj->definition_tag].data;
    wd = (weapon_data *)((uint8_t *)weapon_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)weapon_obj + k_item_data_offset);

    if (weapon_tag->magazines.count > 0) {
        WeaponMagazine *magazine = (WeaponMagazine *)weapon_tag->magazines.pointer;

        rounds = placement->rounds_reserved;
        if (magazine->rounds_reserved_maximum < rounds) {
            rounds = magazine->rounds_reserved_maximum;
        }
        wd->magazines[0].rounds_unloaded = rounds;

        rounds = magazine->rounds_loaded_maximum;
        if (placement->rounds_loaded <= rounds) {
            rounds = placement->rounds_loaded;
        }
        wd->magazines[0].rounds_loaded = rounds;
    }

    if ((placement->flags & 1) == 0) {
        weapon_obj->flags = weapon_obj->flags & ~(uint32_t)_object_at_rest_bit;
    } else {
        weapon_obj->flags = weapon_obj->flags | _object_at_rest_bit;
    }
    weapon_obj->flags = weapon_obj->flags | 0x20000; // _object_unknown_20000_bit

    if ((placement->flags & 4) == 0) {
        id->flags = id->flags | _item_does_not_accelerate_bit;
    } else {
        id->flags = id->flags & ~(uint32_t)_item_does_not_accelerate_bit;
    }

    if ((placement->flags & 1) == 0) {
        weapon_obj->position.z = weapon_obj->position.z + 0.05f;
    }

    return weapon_object_index;
}

#if 0
Original Ghidra decompilation (0x4c1350):

uint FUN_004c1350(uint param_1,int param_2)

{
  short sVar1;
  uint *puVar2;
  int iVar3;
  short sVar4;
  uint uVar5;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (0 < *(int *)(iVar3 + 0x4f0)) {
    iVar3 = *(int *)(iVar3 + 0x4f4);
    sVar4 = *(short *)(param_2 + 0x48);
    sVar1 = *(short *)(iVar3 + 8);
    if (sVar1 < sVar4) {
      sVar4 = sVar1;
    }
    *(short *)((int)puVar2 + 0x2b6) = sVar4;
    sVar4 = *(short *)(iVar3 + 10);
    if (*(short *)(param_2 + 0x4a) <= sVar4) {
      sVar4 = *(short *)(param_2 + 0x4a);
    }
    *(short *)(puVar2 + 0xae) = sVar4;
  }
  if ((*(byte *)(param_2 + 0x4c) & 1) == 0) {
    uVar5 = puVar2[4] & 0xffffffdf;
  }
  else {
    uVar5 = puVar2[4] | 0x20;
  }
  puVar2[4] = uVar5;
  puVar2[4] = uVar5 | 0x20000;
  if ((*(byte *)(param_2 + 0x4c) & 4) == 0) {
    puVar2[0x7d] = puVar2[0x7d] | 0x20;
  }
  else {
    puVar2[0x7d] = puVar2[0x7d] & 0xffffffdf;
  }
  if ((*(byte *)(param_2 + 0x4c) & 1) == 0) {
    puVar2[0x19] = (uint)((float)puVar2[0x19] + 0.05);
  }
  return param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

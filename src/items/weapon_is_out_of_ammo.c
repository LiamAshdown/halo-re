// weapon_is_out_of_ammo  (Ghidra: FUN_004c2c70; renamed per items_types_notes.md:
// "age < 1.0 plus both magazine counters")
// address 0x4c2c70, size 118 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4c2c70..0x4c2ce5 (tests current_game_engine 0x6f1d20; AL result).)
// evidence: types/items.h weapon_data.age (0x240), .magazines[0]; types/tags.h Weapon.magazines
//   (0x4f0), WeaponMagazine.rounds_loaded_maximum (0x0a); global 0x006f1d20 network_game_mode.
// register convention: item index in EAX.
// blam-cc: EAX -> item_index
// UNSURE: the boolean combination below is preserved exactly as decompiled; despite the name,
// it returns true when age < 1.0 AND (there is no network game, or no magazines are defined, or
// the first magazine's loaded capacity is under 1, or the first magazine already holds rounds)
// -- the original packs unrelated bits into the unused upper return bytes, dropped here since no
// caller reads them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "units.h"
#include "game.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine;

// Reports whether an item's weapon (per the odd combination above) should be treated as out of
// ammo/charge.
uint8_t weapon_is_out_of_ammo(datum_index item_index) // blam-cc: EAX -> item_index; AL result
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (wd->age < 1.0f) {
        if (current_game_engine == 0) return 1; // 0x4c2caf
        if (weapon_tag->magazines.count < 1) return 1;
        {
            WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
            if (magazine_tag->rounds_loaded_maximum < 1) return 1;
        }
        if (wd->magazines[0].rounds_loaded != 0 || wd->magazines[0].rounds_unloaded != 0) return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c2c70):

uint FUN_004c2c70(void)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  uint uVar5;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar1 = (float)puVar2[0x90];
  iVar4 = (*puVar2 & 0xffff) * 0x20;
  iVar3 = *(int *)(iVar4 + 0x14 + DAT_0087bc14);
  uVar5 = CONCAT22((short)((uint)iVar4 >> 0x10),
                   (ushort)(fVar1 < 1.0) << 8 | (ushort)NAN(fVar1) << 10 |
                   (ushort)(fVar1 == 1.0) << 0xe);
  if ((fVar1 < 1.0) &&
     ((((uVar5 = 0, DAT_006f1d20 == 0 || (uVar5 = *(uint *)(iVar3 + 0x4f0), (int)uVar5 < 1)) ||
       (uVar5 = *(uint *)(iVar3 + 0x4f4), *(short *)(uVar5 + 10) < 1)) ||
      (((short)puVar2[0xae] != 0 || (*(short *)((int)puVar2 + 0x2b6) != 0)))))) {
    return CONCAT31((int3)(uVar5 >> 8),1);
  }
  return uVar5 & 0xffffff00;
}
#endif

// weapon_build_hud_ammo_state  (Ghidra: FUN_004c29d0; named per types/items.h
// weapon_hud_ammo_state comment block)
// address 0x4c29d0, size 250 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: types/items.h weapon_hud_ammo_state / weapon_hud_magazine_state (exact field-by-
//   field match); types/tags.h Weapon.magazines (0x4f0), WeaponMagazine.rounds_reserved_maximum
//   (0x08) / rounds_loaded_maximum (0x0a).
// register convention: item index in EAX; output buffer is a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> out

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

// Builds a HUD-facing summary of a weapon's heat/age and per-magazine reload state.
void weapon_build_hud_ammo_state(datum_index item_index, weapon_hud_ammo_state *out)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    out->heat = wd->heat;
    out->age = wd->age;
    out->overheated = (uint8_t)(wd->flags & _weapon_overheated_bit);
    out->magazine_count = (int16_t)weapon_tag->magazines.count;

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        weapon_magazine_state *magazine = &wd->magazines[i];
        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
        weapon_hud_magazine_state *out_magazine = &out->magazines[i];

        out_magazine->reloading = (magazine->state == _weapon_magazine_reloading ||
                                    magazine->state == _weapon_magazine_chambering) ? 1 : 0;
        out_magazine->idle = (magazine->state == _weapon_magazine_idle) ? 1 : 0;
        out_magazine->rounds_loaded = magazine->rounds_loaded;
        out_magazine->rounds_loaded_maximum = magazine_tag->rounds_loaded_maximum;
        out_magazine->rounds_unloaded = magazine->rounds_unloaded;
        out_magazine->rounds_reserved_maximum = magazine_tag->rounds_reserved_maximum;
    }
}

#if 0
Original Ghidra decompilation (0x4c29d0):

void FUN_004c29d0(uint *param_1)

{
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  undefined1 uVar6;
  int iVar7;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *param_1 = puVar2[0x8f];
  param_1[1] = puVar2[0x90];
  *(byte *)(param_1 + 2) = (byte)puVar2[0x8b] & 1;
  *(undefined2 *)((int)param_1 + 10) = *(undefined2 *)(iVar3 + 0x4f0);
  iVar5 = 0;
  sVar4 = 0;
  if (0 < *(int *)(iVar3 + 0x4f0)) {
    do {
      puVar1 = puVar2 + iVar5 * 3 + 0xac;
      iVar7 = iVar5 * 0x70 + *(int *)(iVar3 + 0x4f4);
      if (((short)*puVar1 == 1) || ((short)*puVar1 == 3)) {
        uVar6 = 1;
      }
      else {
        uVar6 = 0;
      }
      *(undefined1 *)((int)param_1 + iVar5 * 10 + 0xc) = uVar6;
      *(bool *)((int)param_1 + iVar5 * 10 + 0xd) = (short)*puVar1 == 0;
      *(short *)((int)param_1 + iVar5 * 10 + 0xe) = (short)puVar1[2];
      *(undefined2 *)((int)param_1 + iVar5 * 10 + 0x10) = *(undefined2 *)(iVar7 + 10);
      *(short *)((int)param_1 + iVar5 * 10 + 0x12) = *(short *)((int)puVar1 + 6);
      *(undefined2 *)((int)param_1 + (iVar5 * 5 + 10) * 2) = *(undefined2 *)(iVar7 + 8);
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(iVar3 + 0x4f0));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

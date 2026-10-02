// weapon_set_ammo_counts  (Ghidra: weapon_set_ammo_counts, already named)
// address 0x4c5820, size 147 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h weapon_data.magazines[].rounds_unloaded/.rounds_loaded;
//   types/tags.h WeaponMagazine.rounds_reserved_maximum (0x08).
// register convention: item index in EAX; the caller-supplied per-magazine array is a
// Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> reserve_counts
// UNSURE: despite the name, this sets rounds_unloaded (reserve ammo) from the caller array,
// clamped to rounds_reserved_maximum, and then clamps rounds_loaded down to that new reserve
// value as a sanity check -- it does not set rounds_loaded from the caller array.

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

// Sets each magazine's reserve ammo count from a caller-supplied array, clamped to each
// magazine's rounds_reserved_maximum, and re-clamps rounds_loaded so it never exceeds the new
// reserve figure.
void weapon_set_ammo_counts(datum_index item_index, int16_t *reserve_counts)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
        weapon_magazine_state *magazine = &wd->magazines[i];
        int16_t reserve = magazine_tag->rounds_reserved_maximum;

        if (reserve_counts[i] < reserve) {
            reserve = reserve_counts[i];
        }
        magazine->rounds_unloaded = reserve;
        if (magazine->rounds_loaded <= reserve) {
            reserve = magazine->rounds_loaded;
        }
        magazine->rounds_loaded = reserve;
    }
}

#if 0
Original Ghidra decompilation (0x4c5820):

void weapon_set_ammo_counts(int param_1)

{
  short sVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  short sVar5;
  uint in_EAX;
  int iVar6;
  short sVar7;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar7 = 0;
  if (0 < *(int *)(iVar3 + 0x4f0)) {
    iVar6 = 0;
    do {
      sVar5 = *(short *)(iVar6 * 0x70 + *(int *)(iVar3 + 0x4f4) + 8);
      sVar1 = *(short *)(param_1 + iVar6 * 2);
      if (sVar1 < sVar5) {
        sVar5 = sVar1;
      }
      uVar4 = puVar2[iVar6 * 3 + 0xae];
      *(short *)((int)puVar2 + iVar6 * 0xc + 0x2b6) = sVar5;
      if ((short)uVar4 <= sVar5) {
        sVar5 = (short)uVar4;
      }
      sVar7 = sVar7 + 1;
      *(short *)(puVar2 + iVar6 * 3 + 0xae) = sVar5;
      iVar6 = (int)sVar7;
    } while ((int)sVar7 < *(int *)(iVar3 + 0x4f0));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

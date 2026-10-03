// weapon_magazine_reload_tick_predicted  (Ghidra: FUN_004c3a20; named from
// out/phase4/items_functions.md, "Client-predicted counterpart of the reload tick: advances the
// chambered round count locally without posting network events")
// address 0x4c3a20, size 212 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: mirrors weapon_magazine_reload_tick.c; types/tags.h WeaponMagazine
//   .rounds_reloaded/.rounds_loaded_maximum/.flags; global weapon_bottomless_clip.
// register convention: item index is a Ghidra-recognized parameter; magazine index in BX
// (unaff_BX).
// blam-cc: stack -> item_index, BX -> magazine_index

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
extern uint8_t weapon_bottomless_clip; // 0x0087abc2

extern void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted); // 0x4c35b0

// Client-side prediction of one reload step: moves rounds_reloaded worth of ammunition from
// reserve into the magazine (unless bottomless_clip is set) and, if there is still room and the
// weapon isn't inhibited, chains into another reload step locally.
void weapon_magazine_reload_tick_predicted(datum_index item_index, int16_t magazine_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;
    int16_t old_unloaded;
    int16_t new_loaded;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if (magazine_tag->flags & 1) {
        magazine->rounds_loaded = 0;
    }

    old_unloaded = magazine->rounds_unloaded;
    new_loaded = (old_unloaded <= magazine_tag->rounds_reloaded) ? old_unloaded : magazine_tag->rounds_reloaded;
    new_loaded = magazine->rounds_loaded + new_loaded;
    if (new_loaded > magazine_tag->rounds_loaded_maximum) {
        new_loaded = magazine_tag->rounds_loaded_maximum;
    }

    if (weapon_bottomless_clip == 0) {
        magazine->rounds_unloaded = (magazine->rounds_loaded - new_loaded) + old_unloaded;
    }
    magazine->rounds_loaded = new_loaded;

    if (magazine->rounds_unloaded > 0 && new_loaded < magazine_tag->rounds_loaded_maximum &&
        (magazine_tag->flags & 1) == 0 && (wd->control_flags & 0x26) == 0) {
        magazine->state = _weapon_magazine_chamber_pending;
        magazine->state_ticks = 0;
        weapon_trigger_begin_reload(item_index, magazine_index, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4c3a20):

void FUN_004c3a20(uint param_1)

{
  uint *puVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  byte *pbVar6;
  short sVar7;
  short unaff_BX;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = unaff_BX * 0x70;
  iVar4 = *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4f4);
  pbVar6 = (byte *)(iVar5 + iVar4);
  puVar1 = puVar3 + unaff_BX * 3 + 0xac;
  if ((*(byte *)(iVar5 + iVar4) & 1) != 0) {
    *(undefined2 *)(puVar1 + 2) = 0;
  }
  sVar2 = *(short *)((int)puVar1 + 6);
  sVar7 = sVar2;
  if (*(short *)(pbVar6 + 0x18) <= sVar2) {
    sVar7 = *(short *)(pbVar6 + 0x18);
  }
  sVar7 = (short)puVar1[2] + sVar7;
  if (*(short *)(pbVar6 + 10) < sVar7) {
    sVar7 = *(short *)(pbVar6 + 10);
  }
  if (DAT_0087abc2 == '\0') {
    *(short *)((int)puVar1 + 6) = ((short)puVar1[2] - sVar7) + sVar2;
  }
  *(short *)(puVar1 + 2) = sVar7;
  if ((((0 < *(short *)((int)puVar1 + 6)) && (sVar7 < *(short *)(pbVar6 + 10))) &&
      ((*pbVar6 & 1) == 0)) && ((puVar3[0x8c] & 0x26) == 0)) {
    *(undefined2 *)puVar1 = 2;
    *(undefined2 *)((int)puVar1 + 2) = 0;
    weapon_trigger_begin_reload(param_1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

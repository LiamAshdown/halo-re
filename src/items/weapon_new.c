// weapon_new  (Ghidra: missed_4c1420, created by hand this pass -- Ghidra never recovered it as
// a function; only reachable through the weapon object_type_definition row)
// address 0x4c1420, size 258 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the weapon row (0x0069b748) carries this address at +0x28 (query_create), the same
//   column family as equipment_new/item_new/garbage_new in this batch and
//   projectile_new (0x4bd7c0, the fullest sibling); it is "the single best source" for
//   weapon_data initialization, matching out/phase4/items_types_notes.md's own citation:
//   "weapon_new_from_placement (0x4c1350) sets it" refers to the *placement* sibling (0x4c1350,
//   this batch's equipment_new_from_placement's twin), not this one. types/items.h weapon_data
//   (state 0x238, overheat_effect_handle 0x2cc, magazines[].rounds_loaded/.rounds_unloaded,
//   network_state_valid 0x2e0, network_baseline_index 0x2e1, network_sequence 0x2e2),
//   weapon_trigger_state (idle_ticks 0x00, effect_handle 0x20, empty_ticks 0x24); types/tags.h
//   Weapon.magazines (0x4f0 TagReflexive) and .triggers (0x4fc TagReflexive), WeaponMagazine
//   (rounds_total_initial 0x06, rounds_loaded_maximum 0x0a). global 0x0087bc14 tag_instances,
//   0x00719720 network_game_mode.
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index
// reconciled: R26 object +0x009 raw byte store -> network_state_009 (object.unknown_008 is now split into uint8 unknown_008 / network_state_009 / unknown_00a[2])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode;   // 0x00719720, 0 local, 1 client, 2 host

// The weapon row's query_create hook (object_type_definition +0x28). Zeroes weapon_data.state,
// resets overheat_effect_handle to -1, splits each tag magazine's rounds_total_initial between
// loaded (up to rounds_loaded_maximum) and unloaded/reserved, resets every trigger's idle_ticks,
// effect_handle and empty_ticks to their idle values, and clears the network replication bytes
// when the game is networked. Always reports success.
uint8_t weapon_new(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Weapon *tag = (Weapon *)tag_instances[(uint16_t)obj->definition_tag].data;
    weapon_data *wd = (weapon_data *)((uint8_t *)obj + k_item_extension_offset);
    int16_t i;

    wd->state = 0;
    wd->overheat_effect_handle = (datum_index)k_datum_index_none;

    if (tag->magazines.count > 0) {
        WeaponMagazine *tag_magazine = (WeaponMagazine *)tag->magazines.pointer;

        for (i = 0; i < tag->magazines.count; i++) {
            int16_t loaded = tag_magazine[i].rounds_loaded_maximum;
            if (tag_magazine[i].rounds_total_initial < loaded) {
                loaded = tag_magazine[i].rounds_total_initial;
            }
            wd->magazines[i].rounds_loaded = loaded;
            wd->magazines[i].rounds_unloaded = tag_magazine[i].rounds_total_initial - loaded;
        }
    }

    if (tag->triggers.count > 0) {
        for (i = 0; i < tag->triggers.count; i++) {
            wd->triggers[i].effect_handle = (datum_index)k_datum_index_none;
            wd->triggers[i].idle_ticks = 0x7f;
            wd->triggers[i].empty_ticks = 0;
        }
    }

    if (network_game_mode == 1 || network_game_mode == 2) {
        wd->network_state_valid = 0;
        wd->network_baseline_index = 0;
        wd->network_sequence = 0;
        obj->network_state_009 = 0; // same store as projectile_new 0x4bda48
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x4c1420):

undefined4 missed_4c1420(uint param_1)

{
  uint *puVar1;
  short sVar2;
  short sVar3;
  uint *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(undefined1 *)(puVar4 + 0x8e) = 0;
  puVar4[0xb3] = 0xffffffff;
  sVar8 = 0;
  if (0 < *(int *)(iVar5 + 0x4f0)) {
    iVar6 = 0;
    do {
      sVar2 = *(short *)(iVar6 * 0x70 + 10 + *(int *)(iVar5 + 0x4f4));
      iVar7 = iVar6 * 0x70 + *(int *)(iVar5 + 0x4f4);
      sVar3 = *(short *)(iVar7 + 6);
      if (sVar2 < sVar3) {
        sVar3 = sVar2;
      }
      *(short *)(puVar4 + iVar6 * 3 + 0xae) = sVar3;
      sVar8 = sVar8 + 1;
      *(short *)((int)puVar4 + iVar6 * 0xc + 0x2b6) = *(short *)(iVar7 + 6) - sVar3;
      iVar6 = (int)sVar8;
    } while (iVar6 < *(int *)(iVar5 + 0x4f0));
  }
  sVar8 = 0;
  if (0 < *(int *)(iVar5 + 0x4fc)) {
    iVar6 = 0;
    do {
      puVar1 = puVar4 + iVar6 * 10 + 0x98;
      sVar8 = sVar8 + 1;
      puVar1[8] = 0xffffffff;
      *(undefined1 *)puVar1 = 0x7f;
      *(undefined1 *)(puVar1 + 9) = 0;
      iVar6 = (int)sVar8;
    } while (iVar6 < *(int *)(iVar5 + 0x4fc));
  }
  sVar8 = DAT_00719720;
  if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
    *(undefined1 *)(puVar4 + 0xb8) = 0;
    *(undefined1 *)((int)puVar4 + 0x2e1) = 0;
    *(undefined1 *)((int)puVar4 + 0x2e2) = 0;
    *(undefined1 *)((int)puVar4 + 9) = 0;
  }
  return CONCAT31((int3)(char)((ushort)sVar8 >> 8),1);
}
#endif

// weapon_trigger_enter_recovery  (Ghidra: FUN_004c3d00; named from
// out/phase4/items_functions.md, "Updates a trigger's effect state for heat-related recovery,
// entering an 'overheat' state or clearing it and possibly continuing fire")
// address 0x4c3d00, size 223 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: types/items.h weapon_trigger_effect_state (_weapon_trigger_effect_spewing=6,
//   "0x4c3d00 when the tag trigger has spew_time"), weapon_trigger_state.idle_ticks/.firing_rate;
//   types/tags.h WeaponTrigger.spew_time (0x58), Weapon.triggers (0x4fc).
// register convention: item index and trigger index are both Ghidra-recognized parameters.
// blam-cc: stack -> (item_index, trigger_index)
// UNSURE: the "fire trigger 1" call is hardcoded to trigger index 1 regardless of
// trigger_index, preserved literally. The value truncated by __ftol() is rendered as
// spew_time * 30 ticks/second by analogy with this module's other tag-duration timers.

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

extern void weapon_fire_trigger(datum_index item_index, int16_t trigger_index); // 0x4c3f10

void weapon_trigger_enter_recovery(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    if (tag_trigger->spew_time > 0.0f) {
        wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_spewing;
        wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->spew_time * 30.0f);
        wd->triggers[trigger_index].firing_rate = 0.0f;
        return;
    }

    if (weapon_tag->triggers.count > 1) {
        weapon_fire_trigger(item_index, 1);
    }
    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
    wd->triggers[trigger_index].firing_rate = 0.0f;
}

#if 0
Original Ghidra decompilation (0x4c3d00):

void FUN_004c3d00(uint param_1,short param_2)

{
  uint *puVar1;
  int iVar2;
  undefined2 uVar3;
  int iVar4;
  int iVar5;
  int iVar6;

  iVar6 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  iVar4 = (int)param_2;
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = iVar4 * 0x28;
  if (0.0 < *(float *)(iVar4 * 0x114 + *(int *)(iVar2 + 0x500) + 0x58)) {
    uVar3 = __ftol();
    *(undefined1 *)(iVar5 + 0x261 + (int)puVar1) = 6;
    *(undefined2 *)(iVar5 + 0x262 + (int)puVar1) = uVar3;
    puVar1[iVar4 * 10 + 0x9c] = 0;
    return;
  }
  if (1 < *(int *)(iVar2 + 0x4fc)) {
    weapon_fire_trigger(param_1,1);
  }
  iVar2 = DAT_008603b0;
  *(undefined1 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6) + 0x260 + iVar5) = 0;
  iVar2 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + iVar6);
  *(undefined1 *)(iVar5 + 0x261 + iVar2) = 0;
  *(undefined2 *)(iVar5 + 0x262 + iVar2) = 0;
  puVar1[iVar4 * 10 + 0x9c] = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

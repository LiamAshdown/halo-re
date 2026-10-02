// weapon_trigger_get_charge_fraction  (Ghidra: FUN_004c3100; named from
// out/phase4/items_functions.md, "Computes a 0..1 recovery/charge fraction for one weapon
// trigger's timed effect state, used for HUD/animation blending")
// address 0x4c3100, size 132 bytes
// name confidence: 0.35   rewrite confidence: 0.65
// evidence: types/items.h weapon_trigger_effect_state (charging=2, charged=3),
//   weapon_trigger_state.effect_state/.effect_state_ticks; types/tags.h
//   WeaponTrigger.charging_time (0x48).
// register convention: item index in EAX; trigger index is a Ghidra-recognized stack parameter.
// blam-cc: EAX -> item_index, stack -> trigger_index

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

// Returns 1.0 once a trigger is fully charged, an increasing 0..1 fraction while it is
// charging, or 0.0 for every other effect state.
real weapon_trigger_get_charge_fraction(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    weapon_trigger_state *trigger;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    trigger = &wd->triggers[trigger_index];

    if (trigger->effect_state == _weapon_trigger_effect_charging) {
        tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        return 1.0f - ((real)trigger->effect_state_ticks * 0.033333335f) / tag_trigger->charging_time;
    }
    if (trigger->effect_state != _weapon_trigger_effect_charged) {
        return 0.0f;
    }
    return 1.0f;
}

#if 0
Original Ghidra decompilation (0x4c3100):

float10 FUN_004c3100(short param_1)

{
  char cVar1;
  uint *puVar2;
  uint in_EAX;
  int iVar3;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar3 = (int)param_1;
  cVar1 = *(char *)((int)puVar2 + iVar3 * 0x28 + 0x261);
  if (cVar1 == '\x02') {
    return (float10)1.0 -
           ((float10)(int)*(short *)((int)puVar2 + iVar3 * 0x28 + 0x262) * (float10)0.033333335) /
           (float10)*(float *)(*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                       0x500) + 0x48 + iVar3 * 0x114);
  }
  if (cVar1 != '\x03') {
    return (float10)0.0;
  }
  return (float10)1.0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

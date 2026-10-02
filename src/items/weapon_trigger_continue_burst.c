// weapon_trigger_continue_burst  (Ghidra: FUN_004c3c60; named from
// out/phase4/items_functions.md, "Continues a multi-round burst by firing again if rounds
// remain and marking the trigger's effect state as 'in burst'")
// address 0x4c3c60, size 147 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x4c3c60..0x4c3cf2; fires trigger_index + 1 (FIXED))
// evidence: types/items.h weapon_trigger_effect_state (_weapon_trigger_effect_overloading=1,
//   "re-entered by the burst continuation at 0x4c3c60"), Weapon.triggers (0x4fc).
// register convention: item index in ECX; trigger index in EBX (unaff_EBX).
// blam-cc: ECX -> item_index, EBX -> trigger_index
// UNSURE: the guard compares trigger_index+1 against the trigger count before firing THIS same
// trigger again -- preserved literally, though its purpose (why the next index matters) is not
// established. The value truncated by __ftol() into effect_state_ticks is not shown in the
// decompilation; rendered as WeaponTrigger.overload_time * 30 ticks/second by analogy with the
// other charge/overload timers in this module.

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

void weapon_trigger_continue_burst(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index + 1 < weapon_tag->triggers.count) {
        weapon_fire_trigger(item_index, (int16_t)(trigger_index + 1)); // FIXED (0x4c3cad): fires the NEXT trigger
    }

    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_overloading;
    wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->overload_time * 30.0f);
}

#if 0
Original Ghidra decompilation (0x4c3c60):

void FUN_004c3c60(void)

{
  undefined2 uVar1;
  uint in_ECX;
  int unaff_EBX;
  int iVar2;

  iVar2 = (in_ECX & 0xffff) * 0xc;
  if (unaff_EBX + 1 <
      *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) & 0xffff) * 0x20 +
                        0x14 + DAT_0087bc14) + 0x4fc)) {
    weapon_fire_trigger();
  }
  uVar1 = __ftol();
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) + (short)unaff_EBX * 0x28;
  *(undefined1 *)(iVar2 + 0x261) = 1;
  *(undefined2 *)(iVar2 + 0x262) = uVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

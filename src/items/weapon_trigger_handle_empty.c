// weapon_trigger_handle_empty  (Ghidra: FUN_004c3de0; named from
// out/phase4/items_functions.md, "Dispatches trigger-empty handling to either the
// reload-recovery or heat-recovery path based on the trigger's tag-defined recovery type")
// address 0x4c3de0, size 89 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: types/tags.h WeaponTrigger.overcharged_action (0x50, WeaponOverchargedAction_t).
// register convention: item index in EAX; trigger index in CX.
// blam-cc: EAX -> item_index, CX -> trigger_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void weapon_reload_recovery_finish(datum_index item_index); // 0x4c4940, EBX item_index
extern void weapon_trigger_enter_recovery(datum_index item_index, int16_t trigger_index); // 0x4c3d00

void weapon_trigger_handle_empty(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    if (tag_trigger->overcharged_action == 1) {
        weapon_reload_recovery_finish(item_index);
    } else if (tag_trigger->overcharged_action == 2) {
        weapon_trigger_enter_recovery(item_index, trigger_index);
    }
}

#if 0
Original Ghidra decompilation (0x4c3de0):

void FUN_004c3de0(void)

{
  short sVar1;
  uint in_EAX;
  short in_CX;

  sVar1 = *(short *)(*(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                   (in_EAX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                                      + DAT_0087bc14) + 0x500) + 0x50 + in_CX * 0x114);
  if (sVar1 == 1) {
    FUN_004c4940();
  }
  else if (sVar1 == 2) {
    FUN_004c3d00();
    return;
  }
  return;
}
#endif

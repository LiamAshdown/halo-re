// weapon_has_active_state  (Ghidra: item_has_active_state; renamed per items_types_notes.md:
// "reads both triggers' effect_state, both magazines' state and weapon_data.state")
// address 0x4c3070, size 80 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/items.h weapon_trigger_state.effect_state (triggers[0] 0x261, triggers[1]
//   0x289), weapon_magazine_state.state (magazines[0] 0x2b0, magazines[1] 0x2bc),
//   weapon_data.state (0x238).
// register convention: item index in EAX.
// blam-cc: EAX -> item_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

// Reports whether an item currently has any active trigger effect state, in-progress magazine
// reload, or non-idle weapon state.
int32_t weapon_has_active_state(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    if (wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 &&
        wd->magazines[0].state == 0 && wd->magazines[1].state == 0 && wd->state == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c3070):

undefined4 item_has_active_state(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((((*(char *)(iVar1 + 0x261) == '\0') && (*(char *)(iVar1 + 0x289) == '\0')) &&
      (*(short *)(iVar1 + 0x2b0) == 0)) &&
     ((*(short *)(iVar1 + 700) == 0 && (*(char *)(iVar1 + 0x238) == '\0')))) {
    return 0;
  }
  return 1;
}
#endif

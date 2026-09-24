// weapon_trigger_reset_tracking  (Ghidra: FUN_004c3eb0; named per items_types_notes.md,
// weapon_data comment block: "0x4c3eb0 clears it" (tracked_object_index))
// address 0x4c3eb0, size 88 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/items.h weapon_data.tracked_object_index (0x250), weapon_trigger_state
//   .idle_ticks/.effect_state/.effect_state_ticks.
// register convention: item index in EAX; trigger index is a Ghidra-recognized parameter.
// blam-cc: EAX -> item_index, stack -> trigger_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

// Fully resets a weapon's tracked-object reference and one trigger's effect state.
void weapon_trigger_reset_tracking(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->tracked_object_index = (datum_index)0xffffffff;
    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

#if 0
Original Ghidra decompilation (0x4c3eb0):

void FUN_004c3eb0(short param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;

  iVar1 = DAT_008603b0;
  iVar2 = (in_EAX & 0xffff) * 0xc;
  *(undefined4 *)(*(int *)(iVar2 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x250) = 0xffffffff;
  iVar3 = param_1 * 0x28;
  *(undefined1 *)(*(int *)(iVar2 + 8 + *(int *)(iVar1 + 0x34)) + 0x260 + iVar3) = 0;
  iVar1 = *(int *)(iVar2 + 8 + *(int *)(iVar1 + 0x34));
  *(undefined1 *)(iVar3 + 0x261 + iVar1) = 0;
  *(undefined2 *)(iVar3 + 0x262 + iVar1) = 0;
  return;
}
#endif

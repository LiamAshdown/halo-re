// weapon_trigger_finish_shot  (Ghidra: FUN_004c48f0; named from
// out/phase4/items_functions.md, "Clears a trigger's effect state to idle after resolving a
// fired shot")
// address 0x4c48f0, size 71 bytes
// name confidence: 0.35   rewrite confidence: 0.75
// evidence: types/items.h weapon_trigger_state.idle_ticks/.effect_state/.effect_state_ticks.
// register convention: item index in EAX; trigger index is a Ghidra-recognized parameter.
// blam-cc: EAX -> item_index, stack -> trigger_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

void weapon_trigger_finish_shot(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

#if 0
Original Ghidra decompilation (0x4c48f0):

void FUN_004c48f0(short param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;

  iVar1 = DAT_008603b0;
  iVar3 = (in_EAX & 0xffff) * 0xc;
  iVar2 = param_1 * 0x28;
  *(undefined1 *)(*(int *)(iVar3 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x260 + iVar2) = 0;
  iVar1 = *(int *)(iVar3 + 8 + *(int *)(iVar1 + 0x34));
  *(undefined1 *)(iVar2 + 0x261 + iVar1) = 0;
  *(undefined2 *)(iVar2 + 0x262 + iVar1) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// weapon_trigger_effect_clear  (Ghidra: FUN_004c3e40; named per types/items.h
// weapon_trigger_effect_state comment block: "_weapon_trigger_effect_idle = 0, //
// weapon_trigger_effect_clear (0x4c3e40 / 0x4c48f0)")
// address 0x4c3e40, size 48 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: types/items.h weapon_trigger_state.effect_state/.effect_state_ticks.
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

// Clears a weapon trigger's effect state back to idle.
void weapon_trigger_effect_clear(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

#if 0
Original Ghidra decompilation (0x4c3e40):

void FUN_004c3e40(short param_1)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + param_1 * 0x28;
  *(undefined1 *)(iVar1 + 0x261) = 0;
  *(undefined2 *)(iVar1 + 0x262) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

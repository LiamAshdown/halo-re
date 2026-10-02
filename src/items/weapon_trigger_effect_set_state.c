// weapon_trigger_effect_set_state  (Ghidra: weapon_trigger_effect_set_state, already named)
// address 0x4c49c0, size 55 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: types/items.h weapon_trigger_state.effect_state (0x260+0x01) / .effect_state_ticks
//   (0x260+0x02); item element lookup matches every other object_data access in this module.
// register convention: item index in EAX; trigger index, new state and tick counter are
//   Ghidra-recognized __cdecl stack parameters.
// blam-cc: EAX -> item_index, stack -> (trigger_index, state, counter)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

// Sets one weapon trigger's effect-state byte and its tick counter to caller-supplied values.
void weapon_trigger_effect_set_state(datum_index item_index, int16_t trigger_index, int8_t state, int16_t counter)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = state;
    wd->triggers[trigger_index].effect_state_ticks = counter;
}

#if 0
Original Ghidra decompilation (0x4c49c0):

void __cdecl weapon_trigger_effect_set_state(short trigger_index,char state,short counter)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) +
          trigger_index * 0x28;
  *(char *)(iVar1 + 0x261) = state;
  *(short *)(iVar1 + 0x262) = counter;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// weapon_trigger_effect_set_out_of_ammo  (Ghidra: FUN_004c3e70; named per types/items.h
// weapon_trigger_effect_state comment block: "_weapon_trigger_effect_out_of_ammo = 7, //
// 0x4c3e70, counter -1")
// address 0x4c3e70, size 49 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: types/items.h weapon_trigger_state.effect_state/.effect_state_ticks,
//   k_weapon_trigger_effect_ticks_infinite.
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

// Marks a trigger's effect state as "out of ammo", parked forever.
void weapon_trigger_effect_set_out_of_ammo(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_out_of_ammo;
    wd->triggers[trigger_index].effect_state_ticks = k_weapon_trigger_effect_ticks_infinite;
}

#if 0
Original Ghidra decompilation (0x4c3e70):

void FUN_004c3e70(short param_1)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + param_1 * 0x28;
  *(undefined1 *)(iVar1 + 0x261) = 7;
  *(undefined2 *)(iVar1 + 0x262) = 0xffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

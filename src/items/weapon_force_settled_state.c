// weapon_force_settled_state  (Ghidra: FUN_004c5630; named per types/items.h weapon_state
// comment block: "weapon_force_settled_state (0x4c5630) treats 7, 8 and 10 as the states that
// may persist and forces anything else back to idle")
// address 0x4c5630, size 58 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/items.h weapon_data.state.
// register convention: item index in EAX.
// blam-cc: EAX -> item_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670

// Forces a weapon back to idle unless its current state is one of the three that may persist
// (7 = charged_primary, 8 = charged_secondary, 10 = put_away).
void weapon_force_settled_state(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    int8_t state;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    state = wd->state;

    if (state < 7 || (state > 8 && state != 10)) {
        weapon_set_state(item_index, _weapon_state_idle, 1);
    }
}

#if 0
Original Ghidra decompilation (0x4c5630):

void FUN_004c5630(void)

{
  char cVar1;
  uint in_EAX;

  cVar1 = *(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x238);
  if ((cVar1 < '\a') || (('\b' < cVar1 && (cVar1 != '\n')))) {
    weapon_set_state(0,1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

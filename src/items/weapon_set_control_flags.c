// weapon_set_control_flags  (Ghidra: item_set_permutation; renamed per items_types_notes.md:
// "writes control_flags+primary_trigger; only caller is unit_update, which builds the word from
// unit_control_flags. Nothing about permutations.")
// address 0x4c2990, size 60 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h weapon_data.control_flags (0x230) / .primary_trigger (0x234);
//   weapon_control_flags comment block ("unit_update ... passes unit + 0x284 as the float").
// register convention: item index in EAX; control flags word and analog trigger value are
//   Ghidra-recognized stack parameters.
// blam-cc: EAX -> item_index, stack -> (control_flags, primary_trigger)
// UNSURE: transition_function_evaluate's type argument is not visible at this call site;
// rendered as the identity/linear curve (0) since the value passed through is already the raw
// analog trigger fraction (unit + 0x284) per the header note above.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern real transition_function_evaluate(transition_function_t type, real phase); // 0x4ccac0, math module

// Refreshes a weapon's control_flags/primary_trigger pair, which unit_update rebuilds every
// tick from its own unit_control_flags and the analog trigger.
void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger)
{
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->control_flags = control_flags;
    wd->primary_trigger = transition_function_evaluate(0, primary_trigger);
}

#if 0
Original Ghidra decompilation (0x4c2990):

void item_set_permutation(undefined2 param_1,undefined4 param_2)

{
  int iVar1;
  uint in_EAX;
  float10 fVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  *(undefined2 *)(iVar1 + 0x230) = param_1;
  fVar2 = (float10)transition_function_evaluate(param_2);
  *(float *)(iVar1 + 0x234) = (float)fVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

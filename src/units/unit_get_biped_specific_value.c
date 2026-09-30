// unit_get_biped_specific_value  (Ghidra: FUN_00570ad0; renamed from the phase2 proposal)
// address 0x570ad0, size 46 bytes
// name confidence: 0.25 (phase2 proposal at 0.25, matches functions.md summary)
// rewrite confidence: 0.5
// evidence: types/objects.h object.type (0x0b4); callee biped_is_idle_eligible (0x55e8e0,
//   already rewritten in src/units/biped_is_idle_eligible.c).
// register convention: unit object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_units.h"

extern data_array *object_data; // 0x008603b0

extern uint32_t biped_is_idle_eligible(uint32_t object_index); // 0x55e8e0

// Returns whether a biped is eligible for idle behaviors (delegated to biped_is_idle_eligible)
// only when the unit is a biped; returns 0 for other unit types.
uint32_t unit_get_biped_specific_value(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->type == 0) {
        return biped_is_idle_eligible(object_index);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x570ad0):

uint FUN_00570ad0(void)

{
  uint in_EAX;
  uint uVar1;

  if (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0xb4) == 0)
  {
    uVar1 = FUN_0055e8e0();
    return uVar1;
  }
  return in_EAX & 0xffffff00;
}
#endif

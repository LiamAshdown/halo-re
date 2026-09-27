// actor_forward_target_object_reference  (Ghidra: actor_forward_target_object_reference, renamed)
// address 0x428420, size 75 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: types/ai.h actor.target_unit_index (0x270, reused as a prop index here exactly
//   as actor_get_target_prop_object_index @0x4283d0 does); prop.object_index (0x18). Calls
//   actor_target_data_acquire (outside this rewrite's range, UNSURE signature).
// register convention: ECX -> actor_index, stack -> param (forwarded unchanged).
//   // blam-cc: ECX -> actor_index, stack -> param

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern uint8_t actor_target_data_acquire(datum_index actor_index, datum_index object_index,
    datum_index owner_reference, datum_index pair_reference); // 0x41f7d0

// blam-cc: ECX -> actor_index, stack -> param
// If the actor has a target (read as a prop index; see actor_get_target_prop_object_index),
// forwards its tracked object index, together with the caller-supplied param, to
// actor_target_data_acquire.
void actor_forward_target_object_reference(datum_index actor_index, uint32_t param)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
        // 0x428442: (param, the target's object, this actor, its target prop)
        actor_target_data_acquire(param, p->object_index, actor_index, self->target_unit_index);
    }
}

#if 0
Original Ghidra decompilation (0x428420):

void FUN_00428420(undefined4 param_1)

{
  uint uVar1;
  uint in_ECX;

  uVar1 = *(uint *)((in_ECX & 0xffff) * 0x724 + 0x270 + *(int *)(DAT_00880360 + 0x34));
  if (uVar1 != 0xffffffff) {
    FUN_0041f7d0(param_1,*(undefined4 *)
                          ((uVar1 & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34)));
  }
  return;
}
#endif

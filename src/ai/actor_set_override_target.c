// actor_set_override_target  (Ghidra: actor_set_override_target, renamed)
// address 0x42a5e0, size 82 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: types/ai.h actor.flags (0x6d0) bit 0x800 "override_target", actor.override_target
//   (0x720, "0x42a5e0 writes it together with flags bit 0x800").
// register convention: EAX -> actor_index, stack -> enable, override_target.
//   // blam-cc: EAX -> actor_index, stack -> enable, override_target

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> enable, override_target
// Enables or disables the override-target flag (bit 0x800 of actor.flags) and records the
// supplied value into actor.override_target either way.
void actor_set_override_target(datum_index actor_index, uint8_t enable, datum_index override_target)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (enable != 0) {
        self->control_flags |= _actor_flag_override_target;
    } else {
        self->control_flags &= ~_actor_flag_override_target;
    }
    self->override_target = override_target;
}

#if 0
Original Ghidra decompilation (0x42a5e0):

void FUN_0042a5e0(char param_1,undefined4 param_2)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (param_1 != '\0') {
    *(uint *)(iVar1 + 0x6d0) = *(uint *)(iVar1 + 0x6d0) | 0x800;
    *(undefined4 *)(iVar1 + 0x720) = param_2;
    return;
  }
  *(uint *)(iVar1 + 0x6d0) = *(uint *)(iVar1 + 0x6d0) & 0xfffff7ff;
  *(undefined4 *)(iVar1 + 0x720) = param_2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

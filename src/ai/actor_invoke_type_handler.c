// actor_invoke_type_handler  (Ghidra: actor_invoke_type_handler, already named)
// address 0x409e70, size 44 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/ai.h actor.mode (0x6c); actor_mode_definition.update_proc (+0x14) and the
//   global actor_mode_definitions[16] table at 0x00655254, both already documented in
//   types/ai.h as established by this exact function.
// register convention: actor index in ECX.
//   // blam-cc: ECX -> actor_index
// UNSURE: the mode's update_proc is called with no visible arguments in the decompilation;
// it almost certainly still receives the actor index in a register the call site doesn't
// need to reload (ECX), but Ghidra shows a bare indirect call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

// Invokes the type-specific AI handler function for the actor's current mode, if one is
// registered.
void actor_invoke_type_handler(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    void (*update_proc)(uint32_t) = (void (*)(uint32_t))actor_mode_definitions[a->mode].update_proc;

    if (update_proc != 0) {
        update_proc(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x409e70):

void actor_invoke_type_handler(void)

{
  uint in_ECX;

  if (*(code **)(&DAT_00655268 +
                *(short *)((in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x6c) * 0x38)
      != (code *)0x0) {
    (**(code **)(&DAT_00655268 +
                *(short *)((in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x6c) * 0x38)
    )();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

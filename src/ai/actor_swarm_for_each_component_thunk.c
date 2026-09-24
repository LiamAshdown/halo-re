// actor_swarm_for_each_component_thunk  (Ghidra: actor_swarm_for_each_component_thunk, renamed)
// address 0x407240, size 51 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: phase-4 summary "convenience wrapper that forwards to
//   actor_swarm_for_each_component using the caller's register arguments" (phase-4 calls the
//   callee actor_squad_for_each_member; renamed here to match this session's rewrite of
//   0x407040).
// register convention: every argument is register-inherited and forwarded unchanged; Ghidra
//   shows no parameters at all because it never needs to touch them.
//   // blam-cc: EAX/stack -> forwarded unchanged to actor_swarm_for_each_component

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record); // 0x407040, this session

void actor_swarm_for_each_component_thunk(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record)
{
    actor_swarm_for_each_component(actor_index, reset_first, callback, callback_extra, caller_record);
}

#if 0
Original Ghidra decompilation (0x407240):

void FUN_00407240(void)

{
  FUN_00407040();
  return;
}
#endif

// actor_swarm_for_each_component_thunk  (Ghidra: actor_swarm_for_each_component_thunk, renamed)
// address 0x407240, size 51 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: phase-4 summary "convenience wrapper that forwards to
//   actor_swarm_for_each_component using the caller's register arguments" (phase-4 calls the
//   callee actor_squad_for_each_member; renamed here to match this session's rewrite of
//   0x407040). Matches the pattern already confirmed in
//   src/ai/ai_reference_invoke_squad_callback_406f80.c (this module): registers the same fixed
//   callback (LAB_00406f80) with no reset and no extra argument.
// register convention: EAX -> actor_index; everything else is computed here, not forwarded.
//   // blam-cc: EAX -> actor_index
// FIXED (register inputs, objdump): EAX carries actor_index (read at 0x407249, mov ecx,eax); it
// was missing, and the whole premise of this rewrite ("every argument is register-inherited and
// forwarded unchanged") does not match the disassembly. There is only one real input: objdump
// 0x407240..0x407272 shows reset_first/callback/callback_extra are the constants 0, 0x406f80, 0
// (not forwarded from any caller), and the fifth argument (caller_record, EDI) is computed here
// as &actor_data[actor_index] + 0x9c, not received from a caller -- the "push edi ... pop edi"
// bracket saves and restores the caller's OWN edi, unrelated to this value. The `add esp,0x10`
// after the call (4 popped dwords) also confirms actor_swarm_for_each_component itself takes
// actor_index as its first STACK argument, not via EAX as that file's own note currently claims
// (out of scope here: not in this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record); // 0x407040, this session
extern void LAB_00406f80(void); // 0x406f80, outside this rewrite's range, UNSURE signature; same
    // fixed callback used by ai_reference_invoke_squad_callback_406f80.c (this module)

// blam-cc: EAX -> actor_index
void actor_swarm_for_each_component_thunk(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint16_t *caller_record = (uint16_t *)((uint8_t *)a + 0x9c);

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)LAB_00406f80, 0, caller_record);
}

#if 0
Original Ghidra decompilation (0x407240):

void FUN_00407240(void)

{
  FUN_00407040();
  return;
}
#endif

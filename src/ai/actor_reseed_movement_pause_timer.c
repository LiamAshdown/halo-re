// actor_reseed_movement_pause_timer  (Ghidra: actor_reseed_movement_pause_timer, renamed)
// address 0x4104e0, size 213 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: phase-4 summary "reseeds the actor's short movement-pause timer with a new
// randomized value"; writes the result to actor+0x5f4.
// UNSURE: this function's entire dataflow between actor_get_actor_definition(),
// actor_select_stance_offset_pair() and FUN_0046fe70() is invisible in the decompiled C -- all three are
// called with zero visible arguments/results feeding each other, which normally means a
// chain of hidden register values (a tag float feeding a random-range helper). The
// implementation below preserves the call order and the single visible write
// (actor.unknown_5f4 = truncated result) but does not claim to know the intermediate
// values; needs the disassembly review pass to reconstruct properly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern uint32_t random_seed_global; // 0x00719cd0

extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this module
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0, this module
extern float FUN_0046fe70(float param_1); // UNSURE: no visible argument at the call site

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
void actor_reseed_movement_pause_timer(datum_index actor_index)
{
    actor *self;
    void *definition;
    uint8_t *offset_a, *offset_b;
    float stance_value;
    float randomized;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    definition = actor_get_actor_definition(actor_index);
    // UNSURE: real argument(s)/return of the original calls are unknown; `definition` is the
    // most plausible base pointer and only offset_a's resulting float (via a hidden read) is used.
    actor_select_stance_offset_pair(actor_index, (uint8_t *)definition, &offset_a, &offset_b);
    stance_value = offset_a != (uint8_t *)0 ? *(float *)offset_a : 0.0f;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    randomized = FUN_0046fe70(stance_value);

    self->unknown_5f4 = (int16_t)randomized; // __ftol truncation
}

#if 0
Original Ghidra decompilation (0x4104e0):

void FUN_004104e0(uint param_1)

{
  int iVar1;
  undefined2 uVar2;

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  actor_get_actor_definition();
  FUN_004106b0();
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  FUN_0046fe70();
  uVar2 = __ftol();
  *(undefined2 *)((param_1 & 0xffff) * 0x724 + iVar1 + 0x5f4) = uVar2;
  return;
}
#endif

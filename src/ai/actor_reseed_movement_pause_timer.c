// actor_reseed_movement_pause_timer  (Ghidra: actor_reseed_movement_pause_timer, renamed)
// address 0x4104e0, size 213 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (REWRITTEN from objdump; stance-pair outputs checked)
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
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern uint32_t random_seed_global; // 0x00719cd0

extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this module
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0, this module
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70: difficulty scale, ECX table, AX team

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
// REWRITTEN from objdump 0x4104e0..0x4105b4: a uniform random pause between the first stance entry's bounds
// (+0x1c, +0x20), times difficulty scale 0xe for the actor's team, times the second entry's +4 when non-zero,
// times 1.7 when actor+0x1ca is set, converted to ticks (x30, __ftol) into actor+0x5f4.
void actor_reseed_movement_pause_timer(datum_index actor_index)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    void *definition = actor_get_actor_definition(actor_index);
    uint8_t *entry_a = 0;           // EDI output: the pause bounds
    uint8_t *entry_b = 0;           // ESI output: the optional multiplier
    float lower, upper, fraction, pause;

    actor_select_stance_offset_pair(actor_index, (uint8_t *)definition, &entry_a, &entry_b);
    upper = *(float *)(entry_a + 0x20);
    lower = *(float *)(entry_a + 0x1c);
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fraction = (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f;
    pause = fraction * (upper - lower) + lower;
    pause = weapon_get_zoom_fov_resolved(0xe, ((struct actor *)self)->team) * pause;
    if (entry_b != 0 && *(float *)(entry_b + 4) != 0.0f) {
        pause = pause * *(float *)(entry_b + 4);
    }
    if (self->unknown_1ca != 0) {
        pause = pause * 1.7f;
    }
    self->unknown_5f4 = (int16_t)(int32_t)(pause * 30.0f); // __ftol
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

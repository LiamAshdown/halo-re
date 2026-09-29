// swarm_add_component  (Ghidra: swarm_add_component, already named)
// address 0x4279a0, size 78 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/ai.h swarm.unit_index[16]/component_index[16] (0x18/0x58) and
//   swarm.component_count (0x02); swarm_component.unknown_14 (0x14, "swarm_add_component
//   sets -1"). Calls actor_update_swarm_component_position (0x428130, in this rewrite range but not yet written when
//   this file was authored), with no visible arguments in Ghidra's decompile; passed through
//   as the same two register values this function itself received, since nothing reassigns
//   them first.
// register convention: EAX -> component_index, ECX -> unit_index (raw, salt included),
//   EDX -> swarm_index.
//   // blam-cc: EAX -> component_index, ECX -> unit_index, EDX -> swarm_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358


// blam-cc: EAX -> component_index, ECX -> unit_index, EDX -> swarm_index
// Adds a new actor as a member of a swarm's component list and triggers the swarm's
// member-renumbering step.
void swarm_add_component(datum_index component_index, uint32_t unit_index, datum_index swarm_index)
{
    swarm *s = &((swarm *)swarm_data->data)[swarm_index & 0xffff];
    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & 0xffff];

    component->unknown_14 = 0xffffffff;
    s->unit_index[s->component_count] = unit_index;
    s->component_index[s->component_count] = component_index;
    s->component_count = s->component_count + 1;

    actor_update_swarm_component_position(component_index, unit_index);
}

#if 0
Original Ghidra decompilation (0x4279a0):

void swarm_add_component(void)

{
  uint in_EAX;
  undefined4 in_ECX;
  uint in_EDX;
  int iVar1;

  iVar1 = (in_EDX & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
  *(undefined4 *)((in_EAX & 0xffff) * 0x40 + 0x14 + *(int *)(DAT_00880358 + 0x34)) = 0xffffffff;
  *(undefined4 *)(iVar1 + 0x18 + *(short *)(iVar1 + 2) * 4) = in_ECX;
  *(uint *)(iVar1 + 0x58 + *(short *)(iVar1 + 2) * 4) = in_EAX;
  *(short *)(iVar1 + 2) = *(short *)(iVar1 + 2) + 1;
  FUN_00428130();
  return;
}
#endif

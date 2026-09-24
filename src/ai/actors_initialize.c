// actors_initialize  (Ghidra: actors_initialize, already named)
// address 0x426710, size 78 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: types/ai.h header note (the three data arrays this creates, by literal name and
//   capacity, fix actor/swarm/swarm_component's element strides); cea-pdb name match via the
//   "actor"/"swarm"/"swarm component" strings. game_state_new's real signature (element size
//   in EBX, name and maximum_count on the stack) is already established at
//   src/effects/contrails_initialize.c; Ghidra elides the EBX argument here the same way.
// register convention: no arguments (game_state_new's element-size argument is supplied at
//   each call site via EBX, elided from Ghidra's own rendering of the calls below).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358

extern void *game_state_new(int16_t element_size, char *name, int16_t maximum_count); // 0x5380d0
    // blam-cc: EBX -> element_size, stack -> (name, maximum_count)

// Creates the actor, swarm, and swarm-component data arrays used by the rest of the AI actor
// module.
void actors_initialize(void)
{
    actor_data = (data_array *)game_state_new(k_actor_size, "actor", k_actor_data_maximum_count);
    swarm_data = (data_array *)game_state_new(k_swarm_size, "swarm", k_swarm_data_maximum_count);
    swarm_component_data = (data_array *)game_state_new(k_swarm_component_size, "swarm component",
        k_swarm_component_data_maximum_count);
}

#if 0
Original Ghidra decompilation (0x426710):

void __cdecl actors_initialize(void)

{
  DAT_00880360 = game_state_new("actor",0x100);
  DAT_0088035c = game_state_new("swarm",0x20);
  DAT_00880358 = game_state_new("swarm component",0x100);
  return;
}
#endif

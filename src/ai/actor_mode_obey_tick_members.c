// actor_mode_obey_tick_members  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x407300, size 55 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[11] ("obey", 0x006554b0) +0x10 slot.
// objdump 0x407300: actor_swarm_for_each_component(actor, 0, actor_obey_member_tick, 0) with EDI = the actor's mode data (+0x9c).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360


void actor_mode_obey_tick_members(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_obey_member_tick, 0, (uint16_t *)(actor + 0x9c));
}

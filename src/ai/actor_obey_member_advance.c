// actor_obey_member_advance  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x406f80, size 17 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: pushed as the actor_swarm_for_each_component callback by 0x407240 and 0x434e60 (the
//   ai_command_list_advance script function).
// objdump 0x406f80..0x406f90: component record byte +4: clears bit 0x10, sets bit 0x08.
// blam-cc: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

void actor_obey_member_advance(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *record = (uint8_t *)component_record;

    (void)actor_index;
    (void)unit_index;
    (void)command_list_index;
    (void)secondary_record;
    (void)callback_extra;
    record[4] = (uint8_t)((record[4] & 0xef) | 8);
}

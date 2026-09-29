// actor_obey_member_enter  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x406f30, size 66 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: the per-member callback actor_mode_obey_enter 0x407280 hands actor_swarm_for_each_component.
// objdump 0x406f30..0x406f71: when the command list (scenario +0x43c, 0x60 each, index = extra) has flag 0x10, the
//   member unit's +0x204 gets bit 0x1000.
// blam-cc: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *object_data;  // 0x008603b0
extern Scenario *global_scenario;

void actor_obey_member_enter(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *list = (uint8_t *)global_scenario->command_lists.pointer + (int16_t)command_list_index * 0x60;

    (void)actor_index;
    (void)component_record;
    (void)secondary_record;
    (void)callback_extra;
    if (list[0x20] & 0x10) {
        uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

        ((unit_object *)unit)->unit.flags |= 0x1000;
    }
}

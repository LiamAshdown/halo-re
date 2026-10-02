// actor_obey_member_exit  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x406fa0, size 79 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: the per-member callback actor_mode_obey_exit 0x4072c0 hands actor_swarm_for_each_component.
// objdump 0x406fa0..0x406fee: unless the record is finished (+4 bit 2), actor_squad_action_reset_entry 0x406c50
//   (EAX actor, ECX unit, EBX record, stack: command list, secondary record, and the unit argument slot reused as
//   an out byte) resets it; the unit's +0x204 loses bit 0x1000.
// blam-cc: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;  // 0x008603b0
extern void actor_squad_action_reset_entry(uint32_t actor_index, uint32_t check_object_index, uint8_t *state,
    int16_t command_list_index, uint8_t *aim_state, uint8_t *next_action_index_out); // 0x406c50, EAX, ECX, EBX, stack

void actor_obey_member_exit(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *record = (uint8_t *)component_record;

    (void)callback_extra;
    if ((record[4] & 2) == 0) {
        uint8_t next_action = 0;

        actor_squad_action_reset_entry(actor_index, unit_index, record, (int16_t)command_list_index,
            (uint8_t *)secondary_record, &next_action);
    }
    ((unit_object *)unit)->unit.flags &= ~0x1000u;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

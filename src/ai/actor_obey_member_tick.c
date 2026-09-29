// actor_obey_member_tick  (not a Ghidra function; AI "obey" mode, the command-list mode ai_command_list switches actors to)
// address 0x406ff0, size 68 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: the per-member callback actor_mode_obey_tick_members 0x407300 hands actor_swarm_for_each_component.
// objdump 0x406ff0..0x407033: the record's word +2 counts down to 0, and word +8 too while byte +5 has bit 2;
//   with byte +5 bits 0 and 1 both set it tail-jumps to actor_get_body_axis_vector 0x405390 (EAX actor,
//   EDX unit, ECX the record).
// blam-cc: stack -> actor, unit, command_list_index, component_record, secondary_record, callback_extra

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_ai.h"


void actor_obey_member_tick(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *record = (uint8_t *)component_record;
    uint8_t flags;

    (void)command_list_index;
    (void)secondary_record;
    (void)callback_extra;
    if (*(int16_t *)(record + 2) > 0) {
        *(int16_t *)(record + 2) = (int16_t)(*(int16_t *)(record + 2) - 1);
    }
    flags = record[5];
    if ((flags & 4) && *(int16_t *)(record + 8) > 0) {
        *(int16_t *)(record + 8) = (int16_t)(*(int16_t *)(record + 8) - 1);
    }
    if ((flags & 1) && (flags & 2)) {
        actor_get_body_axis_vector(actor_index, unit_index, (actor_axis_request *)record);
    }
}

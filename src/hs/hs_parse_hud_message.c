// hs_parse_hud_message  (not a Ghidra function; an hs primitive parser)
// address 0x4873c0, size 62 bytes
// name confidence: 0.7  rewrite confidence: 0.85
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 22 (hud_message) = 0x4873c0; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x4873c0..0x4873fd: with a hud_messages tag on the scenario (+0x5a0 not -1), hs_parse_scenario_datum
//   over that tag's +0x20 block (EBX = 0, stride 0x40); otherwise 0.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char hs_parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array, int32_t stride);
    // 0x486f90, blam-cc: EAX -> node_index, EBX -> name_offset, ESI -> array, stack -> stride
#include "cache.h"
extern Scenario *global_scenario; // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14

char hs_parse_hud_message(datum_index node_index)
{
    datum_index hud_messages = *(datum_index *)&global_scenario->hud_messages.tag_id;

    if (hud_messages == k_datum_index_none) {
        return 0;
    }
    return hs_parse_scenario_datum(node_index, 0,
        (TagReflexive *)((uint8_t *)tag_instances[hud_messages & 0xffff].data + 0x20), 0x40);
}

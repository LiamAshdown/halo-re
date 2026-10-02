// hs_parse_ai  (not a Ghidra function; an hs primitive parser)
// address 0x487150, size 79 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 17 (ai) = 0x487150; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x487150..0x48719e: ai_reference_parse(EAX = token, ECX = global_scenario, stack = &value); on failure
//   "this is not a valid ai encounter or squad." at the node's offset. Returns ai_reference_parse's AL.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_syntax_data;       // 0x0087a474
extern char *hs_compiled_source;         // 0x006b14c0
extern char *hs_compile_error;           // 0x006b14d4
extern int32_t hs_compile_error_offset;  // 0x006b14d8
extern Scenario *global_scenario; // 0x00746f8c
extern uint8_t ai_reference_parse(char *reference_string, Scenario *scenario, uint32_t *out_packed_reference);
    // 0x432320, blam-cc: EAX -> reference_string, ECX -> scenario, stack -> out_packed_reference

char hs_parse_ai(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    uint8_t found = ai_reference_parse(hs_compiled_source + node->source_offset, global_scenario,
        (uint32_t *)&node->data);

    if (!found) {
        hs_compile_error = (char *)"this is not a valid ai encounter or squad.";
        hs_compile_error_offset = node->source_offset;
    }
    return found;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

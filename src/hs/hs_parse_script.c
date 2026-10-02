// hs_parse_script  (not a Ghidra function; an hs primitive parser)
// address 0x486c80, size 82 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 10 (script) = 0x486c80; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x486c80..0x486cd1: hs_script_find_by_name(token); found, the index goes in the value word and it
//   returns 1, else "this is not a valid script name." at the node's offset and 0.
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
extern int16_t hs_script_find_by_name(char *name); // 0x4833a0

char hs_parse_script(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    int16_t script_index = hs_script_find_by_name(hs_compiled_source + node->source_offset);

    if (script_index != -1) {
        node->data.short_value = script_index;
        return 1;
    }
    hs_compile_error = (char *)"this is not a valid script name.";
    hs_compile_error_offset = node->source_offset;
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

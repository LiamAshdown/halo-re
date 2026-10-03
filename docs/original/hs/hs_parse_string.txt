// hs_parse_string  (not a Ghidra function; an hs primitive parser)
// address 0x486c50, size 40 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 9 (string) = 0x486c50; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x486c50..0x486c77: the value is the token's address (hs_compiled_source + source offset); returns 1.
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

char hs_parse_string(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    node->data.string_value = (int32_t)(hs_compiled_source + node->source_offset);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// hs_parse_boolean  (not a Ghidra function; an hs primitive parser)
// address 0x486a10, size 206 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 5 (boolean) = 0x486a10; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x486a10..0x486add: the token (hs_compiled_source + source offset) "false", "off" or "0" gives 0 and
//   "true", "on" or "1" gives 1 (exact, case-sensitive compares including the terminator), both returning 1.
//   Anything else records "i expected \"true\" or \"false\"." at the node's offset, returns 0 and, as the
//   original does, stores the low byte of its own node_index argument as the value.
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
#include <string.h>

char hs_parse_boolean(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    char *token = hs_compiled_source + node->source_offset;

    if (strcmp(token, "false") == 0 || strcmp(token, "off") == 0 || strcmp(token, "0") == 0) {
        node->data.boolean_value = 0;
        return 1;
    }
    if (strcmp(token, "true") == 0 || strcmp(token, "on") == 0 || strcmp(token, "1") == 0) {
        node->data.boolean_value = 1;
        return 1;
    }
    hs_compile_error = (char *)"i expected \"true\" or \"false\".";
    hs_compile_error_offset = node->source_offset;
    node->data.boolean_value = (uint8_t)node_index; // 0x486aaa: cl = the argument's low byte
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

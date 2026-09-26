// hs_parse_object_list  (not a Ghidra function; an hs primitive parser)
// address 0x487400, size 57 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 23 (object_list) = 0x487400; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x487400..0x487438: the node is parsed as an object_name (type and +0x02 set to 0x2b) by
//   hs_parse_object_name, then its type goes back to 0x17 (object_list); returns hs_parse_object_name's AL.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern data_array *hs_syntax_data;       // 0x0087a474
extern char *hs_compiled_source;         // 0x006b14c0
extern char *hs_compile_error;           // 0x006b14d4
extern int32_t hs_compile_error_offset;  // 0x006b14d8
extern char hs_parse_object_name(datum_index node_index); // 0x487230

char hs_parse_object_list(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    char result;

    node->index_union = 0x2b;
    node->type = 0x2b;
    result = hs_parse_object_name(node_index);
    node->type = 0x17;
    return result;
}

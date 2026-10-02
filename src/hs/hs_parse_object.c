// hs_parse_object  (not a Ghidra function; an hs primitive parser)
// address 0x4872f0, size 104 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 37..42 (object, unit, vehicle, weapon, device, scenery) = 0x4872f0; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x4872f0..0x487357: the token "none" gives -1 and returns 1. Otherwise the node's type is moved up by 6
//   to the matching *_name type (also copied to +0x02), hs_parse_object_name parses it, the type goes back down
//   by 6 (+0x02 keeps the name type) and its AL is returned.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern data_array *hs_syntax_data;       // 0x0087a474
extern char *hs_compiled_source;         // 0x006b14c0
extern char *hs_compile_error;           // 0x006b14d4
extern int32_t hs_compile_error_offset;  // 0x006b14d8
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char hs_parse_object_name(datum_index node_index); // 0x487230

char hs_parse_object(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    char result;

    if (strcmp(hs_compiled_source + node->source_offset, "none") == 0) {
        node->data.long_value = -1;
        return 1;
    }
    node->type = node->type + 6;
    node->index_union = node->type;
    result = hs_parse_object_name(node_index);
    node->type = node->type - 6;
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

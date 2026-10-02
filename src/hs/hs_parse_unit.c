// hs_parse_unit  (not a Ghidra function; the hs parse procedure of unit, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_4854f0)
// address 0x4854f0, size 69 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4854f0..0x485534: one argument parsed as hs type 0x25 (object list).
// blam-cc: stack -> function_index, node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420
extern char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index,
    datum_index *out_indices); // 0x00484fb0, ECX node, EBX out

static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

char hs_parse_unit(int16_t function_index, datum_index node_index)
{
    datum_index argument;

    if (!hs_get_parameter_indices(hs_function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    return hs_parse(argument, 0x25);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

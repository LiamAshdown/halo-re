// hs_parse_logical  (not a Ghidra function; the hs parse procedure of and and or, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_484db0)
// address 0x484db0, size 225 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x484db0..0x484e90: parses every argument as boolean (primitives through
//   hs_parse_primitive, lists through hs_parse_nonprimitive; already typed ones are taken as parsed), stopping at the
//   first failure; fewer than two arguments gives "the %s call requires at least 2 arguments." (0x00665aa4) at the
//   call's source offset.
// blam-cc: stack -> function_index, node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420
extern char hs_parse_primitive(datum_index node_index); // 0x00486480, blam-cc: EDI node
extern char hs_parse_nonprimitive(datum_index node_index); // 0x00486710
extern char hs_compile_error_buffer[0x100]; // 0x006b14dc

static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

// Parses one argument as `type` unless it already has a type: primitives get the type in index_union too and go
// through hs_parse_primitive, lists through hs_parse_nonprimitive (the inlined hs_parse body).
static char parse_typed_argument(datum_index argument, hs_type_t type)
{
    hs_syntax_node *node = syntax_node(argument);

    if (node->type != 0) {
        return 1;
    }
    node->type = type;
    if ((node->flags & 1) != 0) {
        node->index_union = type;
        return hs_parse_primitive(argument);
    }
    return hs_parse_nonprimitive(argument);
}

char hs_parse_logical(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index argument = syntax_node(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;

    for (; argument != 0xffffffff; argument = syntax_node(argument)->next_node) {
        char ok = parse_typed_argument(argument, 5);

        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 2) {
        sprintf(hs_compile_error_buffer, "the %s call requires at least 2 arguments.", hs_function_definitions[function_index]->name);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = syntax_node(node_index)->source_offset;
        return 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

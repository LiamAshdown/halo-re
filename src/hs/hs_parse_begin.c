// hs_parse_begin  (not a Ghidra function; the hs parse procedure of begin and begin_random, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_484600)
// address 0x484600, size 367 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x484600..0x48476e: every argument is parsed in order and the first failure
//   returns 0. For begin (function index 0) every argument but the last is parsed as void and the last as the block's
//   own type; begin_random parses all with the block's type. An untyped block takes the type of the argument that
//   decided it (begin: the last, begin_random: each). No arguments gives "a statement block must contain at least one
//   argument." (formatted into the error buffer with the function name, which the format does not use); begin_random
//   with more than 32 gives its own error. Both at the call's source offset.
// blam-cc: stack -> function_index, node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420
extern char hs_compile_error_buffer[0x100]; // 0x006b14dc

static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

char hs_parse_begin(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index argument = syntax_node(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;
    char ok = 1;

    while (argument != 0xffffffff) {
        hs_syntax_node *node = syntax_node(argument);
        datum_index next = node->next_node;

        if (function_index == 0) {
            ok = hs_parse(argument, next == 0xffffffff ? call->type : 4);
            if (next == 0xffffffff && call->type == 0 && ok) {
                call->type = syntax_node(argument)->type;
            }
        } else {
            ok = hs_parse(argument, call->type);
            if (call->type == 0 && ok) {
                call->type = syntax_node(argument)->type;
            }
        }
        argument = next;
        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 1) {
        sprintf(hs_compile_error_buffer, "a statement block must contain at least one argument.", // 0x00665cc4
            hs_function_definitions[function_index]->name);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    if (count > 0x20 && function_index == 1) {
        hs_compile_error = "begin_random can take a maximum of 32 arguments (matt can increase this.)"; // 0x00665c78
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    return 1;
}

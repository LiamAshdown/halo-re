// hs_parse_cond  (not a Ghidra function; the hs parse procedure of cond, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_484b40)
// address 0x484b40, size 149 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x484b40..0x484bd4: rewrites the cond into a nested if chain
//   (hs_parse_cond_recursive 0x4848f0 from the first clause); on success copies the new node over the call node,
//   keeping the call's identifier and next link, and parses it with the call's original type. -1 from the rewrite
//   returns 0.
// blam-cc: stack -> function_index, node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8


static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

char hs_parse_cond(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index rewritten = hs_parse_cond_recursive(node_index, syntax_node(*(datum_index *)&call->data)->next_node);
    hs_syntax_node *replacement;
    datum_index next;
    hs_type_t type;
    int16_t identifier;

    if (rewritten == 0xffffffff) {
        return 0;
    }
    call = syntax_node(node_index);
    next = call->next_node;
    type = call->type;
    identifier = call->identifier;
    replacement = syntax_node(rewritten);
    replacement->next_node = next;
    *call = *replacement;
    call->identifier = identifier;
    return hs_parse(node_index, type);
}

// hs_parse_sleep  (not a Ghidra function; the hs parse procedure of sleep, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_485280)
// address 0x485280, size 141 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x485280..0x48530c: the first argument (required: else "the sleep call requires a
//   time and, optionally, a script name." at the call's source offset) parses as short, an optional second as script.
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

static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

char hs_parse_sleep(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index time = syntax_node(*(datum_index *)&call->data)->next_node;
    datum_index script;

    if (time == 0xffffffff) {
        hs_compile_error = (char *)"the sleep call requires a time and, optionally, a script name."; // 0x00665a10
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    if (!hs_parse(time, 7)) {
        return 0;
    }
    script = syntax_node(time)->next_node;
    if (script != 0xffffffff && !hs_parse(script, 0xa)) {
        return 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// hs_parse_sleep_until  (not a Ghidra function; the hs parse procedure of sleep_until, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_485310)
// address 0x485310, size 170 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x485310..0x4853b9: the condition (required: else "the sleep_until call requires
//   a condition and, optionally, a period." at the call's source offset) parses as boolean, an optional period as
//   short and an optional third argument as long; returns the last parse result.
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

char hs_parse_sleep_until(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index condition = syntax_node(*(datum_index *)&call->data)->next_node;
    datum_index period;
    datum_index timeout;
    char ok;

    if (condition == 0xffffffff) {
        hs_compile_error = (char *)"the sleep_until call requires a condition and, optionally, a period."; // 0x006659c8
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    period = syntax_node(condition)->next_node;
    ok = hs_parse(condition, 5);
    if (!ok || period == 0xffffffff) {
        return ok;
    }
    timeout = syntax_node(period)->next_node;
    ok = hs_parse(period, 7);
    if (!ok || timeout == 0xffffffff) {
        return ok;
    }
    return hs_parse(timeout, 8);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

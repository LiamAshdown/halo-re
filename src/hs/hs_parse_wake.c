// hs_parse_wake  (not a Ghidra function; the hs parse procedure of wake, reached through the function records' +0x08
//   slot; no C existed, so console input using it trapped as unlisted_4853c0)
// address 0x4853c0, size 160 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4853c0..0x48545f: one argument parsed as a script; a static or stub script
//   (scenario script type 3 or 4) gives "this static script cannot be awakened." at the argument's source offset.
// blam-cc: stack -> function_index, node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420
extern char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index,
    datum_index *out_indices); // 0x00484fb0, ECX node, EBX out
extern Scenario *global_scenario; // 0x00746f8c

static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}

char hs_parse_wake(int16_t function_index, datum_index node_index)
{
    datum_index argument;
    hs_syntax_node *node;
    ScenarioScript *script;

    if (!hs_get_parameter_indices(hs_function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    node = syntax_node(argument);
    if (!hs_parse(argument, 0xa)) {
        return 0;
    }
    script = (ScenarioScript *)global_scenario->scripts.pointer + node->data.short_value;
    if (script->script_type == 3 || script->script_type == 4) {
        hs_compile_error = (char *)"this static script cannot be awakened."; // 0x006659a0
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    return 1;
}

// hs_parse_inspect  (not a Ghidra function; the parse procedure of hs function "inspect" (record 0x6578c8, name
//   0x6646b8); the console wraps a bare expression in (inspect ...), so no C here meant the console trapped as
//   unlisted_485460)
// address 0x485460, size 133 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x485460..0x4854e4. Requires exactly one argument (hs_get_parameter_indices,
//   the index written back over the function_index slot), then parses it with no expected type. When that parse
//   fails without an error of its own, reports "this is not a global variable reference, function call, or
//   script call." (0x665958) at the argument's source offset. Returns 1 only on a successful parse.
// blam-cc: stack -> function_index, node_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8


static char hs_inspect_not_a_reference[] = "this is not a global variable reference, function call, or script call.";

char hs_parse_inspect(int16_t function_index, datum_index node_index)
{
    datum_index argument;

    if (!hs_get_parameter_indices(hs_function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    if (hs_parse(argument, 0)) {
        return 1;
    }
    if (hs_compile_error == 0) {
        hs_compile_error = hs_inspect_not_a_reference; // the original points at its .rdata copy 0x665958
        hs_compile_error_offset = *(int32_t *)((uint8_t *)hs_syntax_data->data + (argument & 0xffff) * 0x14 + 0xc);
    }
    return 0;
}

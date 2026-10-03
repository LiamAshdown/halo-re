// hs_parse_function_arguments  (not a Ghidra function; the default hs_function_definition parse procedure --
//   480 of the 522 script functions point at it. No C existed, so every console command trapped as
//   unlisted_487440; map scripts are precompiled and never needed it)
// address 0x487440, size 240 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x487440..0x48752f. Parses the call node's arguments (the siblings after the
//   function-name node, node +0x10 -> +0x08) against definition->parameters[i] with hs_parse, stopping at the
//   first failure (returns 0 with hs_parse's own error). When every parse succeeded but the argument count differs
//   from definition->parameter_count, formats 'the "%s" call requires exactly %d arguments.' (0x665054) into the
//   compile error buffer and points the error offset at the call node's source offset.
// blam-cc: stack -> function_index, node_index (cdecl)

#include "crt.h"
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
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420

#define HS_NODE(index) ((uint8_t *)hs_syntax_data->data + ((index) & 0xffff) * 0x14)

char hs_parse_function_arguments(int16_t function_index, datum_index node_index)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    uint8_t *node = HS_NODE(node_index);
    datum_index argument = *(datum_index *)(HS_NODE(*(datum_index *)(node + 0x10)) + 0x8);
    char ok = 1;
    int16_t i = 0;

    while (i < definition->parameter_count && argument != k_datum_index_none) {
        if (hs_parse(argument, (hs_type_t)(uint16_t)definition->parameters[i])) {
            argument = *(datum_index *)(HS_NODE(argument) + 0x8);
        } else {
            ok = 0;
        }
        i++;
        if (!ok) {
            return 0;
        }
    }
    if (!ok) {
        return ok;
    }
    if (i != definition->parameter_count || argument != k_datum_index_none) {
        sprintf(hs_compile_error_buffer, "the \"%s\" call requires exactly %d arguments.", definition->name,
            (int32_t)definition->parameter_count);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = *(int32_t *)(HS_NODE(node_index) + 0xc);
        return 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

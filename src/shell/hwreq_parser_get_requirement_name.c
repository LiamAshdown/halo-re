// hwreq_parser_get_requirement_name  (not a Ghidra function; hwreq_parser vtable slot 0x20)
// address 0x5787c0, size 76 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x20; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x5787c0..0x57880c: vector<pair>::operator[] of the requirements property set (+0x1c) with the range check: when the vector was never allocated or index >= count, hwreq_vector_throw_out_of_range 0x57b9e0 (never returns); then c_str of the pair's first string (+0x00 within the 0x38-byte pair). ret 0x4: one stack argument.
// blam-cc: ECX -> parser, stack -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hwreq_vector_throw_out_of_range(void); // 0x57b9e0, throws out_of_range; never returns

static char *string_c_str(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

char *hwreq_parser_get_requirement_name(hwreq_parser *parser, uint32_t index)
{
    hwreq_property_set *set = (hwreq_property_set *)parser->requirements;
    hwreq_string_pair *pairs = (hwreq_string_pair *)set->flags.first;

    if (pairs == 0 || (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair)) <= index) {
        hwreq_vector_throw_out_of_range();
    }
    return string_c_str(&pairs[index].first);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

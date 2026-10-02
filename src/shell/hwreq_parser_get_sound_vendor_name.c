// hwreq_parser_get_sound_vendor_name  (not a Ghidra function; hwreq_parser vtable slot 0x34)
// address 0x5788b0, size 23 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x34; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x5788b0..0x5788c7: c_str of sound_vendor_name (+0x94): capacity at +0xac, buffer at +0x98.
// blam-cc: ECX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

static char *string_c_str(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

char *hwreq_parser_get_sound_vendor_name(hwreq_parser *parser)
{
    return string_c_str(&parser->sound_vendor_name);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

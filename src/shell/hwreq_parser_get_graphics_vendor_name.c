// hwreq_parser_get_graphics_vendor_name  (not a Ghidra function; hwreq_parser vtable slot 0x2c)
// address 0x578880, size 14 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x2c; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x578880..0x57888e: c_str of graphics_vendor_name (+0x5c): capacity at +0x74 >= 0x10 -> heap pointer at +0x60, else the inline buffer at +0x60.
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

char *hwreq_parser_get_graphics_vendor_name(hwreq_parser *parser)
{
    return string_c_str(&parser->graphics_vendor_name);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

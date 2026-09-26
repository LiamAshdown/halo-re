// hwreq_parser_get_sound_device_name  (not a Ghidra function; hwreq_parser vtable slot 0x30)
// address 0x578890, size 17 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x30; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x578890..0x5788a1: c_str of sound_device_name (+0x78): capacity at +0x90, buffer at +0x7c.
// blam-cc: ECX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

static char *string_c_str(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

char *hwreq_parser_get_sound_device_name(hwreq_parser *parser)
{
    return string_c_str(&parser->sound_device_name);
}

// hwreq_parser_get_flags  (not a Ghidra function; hwreq_parser vtable slot 0x08)
// address 0x578860, size 4 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x08; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x578860..0x578864: mov eax,[ecx+0x18]; ret: the flags property set (+0x18).
// blam-cc: ECX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

hwreq_property_set *hwreq_parser_get_flags(hwreq_parser *parser)
{
    return (hwreq_property_set *)parser->flags;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

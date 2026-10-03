// hwreq_parser_has_error  (not a Ghidra function; hwreq_parser vtable slot 0x38)
// address 0x5788d0, size 4 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x38; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x5788d0..0x5788d4: mov al,[ecx+0x20]; ret: the error_reported latch.
// blam-cc: ECX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t hwreq_parser_has_error(hwreq_parser *parser)
{
    return parser->error_reported;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

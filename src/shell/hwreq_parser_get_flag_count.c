// hwreq_parser_get_flag_count  (not a Ghidra function; hwreq_parser vtable slot 0x10)
// address 0x5786c0, size 38 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hwreq_parser_vtable (types/shell.h, vtable 0x006721e8) slot 0x10; shell_parse_config_txt 0x57d410 calls it
//   through the vtable. Not a Ghidra function (only reachable through the vtable). First-boot track: the
//   standalone exe redirects the vtable slot to this C, so every slot needs a rewrite.
// Rewritten from objdump 0x5786c0..0x5786e6: the pair count of the flags property set (+0x18): 0 while its vector was never allocated, else (last - first) / 0x38 as a signed division (imul 0x92492493, sar 5).
// blam-cc: ECX -> parser

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

uint32_t hwreq_parser_get_flag_count(hwreq_parser *parser)
{
    hwreq_property_set *set = (hwreq_property_set *)parser->flags;

    if (set->flags.first == 0) {
        return 0;
    }
    return (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair));
}

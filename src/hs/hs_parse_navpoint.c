// hs_parse_navpoint  (not a Ghidra function; an hs primitive parser)
// address 0x487360, size 96 bytes
// name confidence: 0.7  rewrite confidence: 0.85
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 21 (navpoint) = 0x487360; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x487360..0x4873bf: with a hud_globals tag on global_globals->interface_bitmaps element 0 (+0x6c not -1;
//   like the original, the first element is read without checking the count), hs_parse_scenario_datum over that
//   tag's +0x160 block (EBX = 0, stride 0x68); otherwise 0.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char hs_parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array, int32_t stride);
    // 0x486f90, blam-cc: EAX -> node_index, EBX -> name_offset, ESI -> array, stack -> stride
#include "cache.h"
extern Globals *global_globals; // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

char hs_parse_navpoint(datum_index node_index)
{
    uint8_t *interface_bitmaps = global_globals->interface_bitmaps.count != 0 ?
        (uint8_t *)global_globals->interface_bitmaps.pointer : 0;
    datum_index hud_globals = *(datum_index *)(interface_bitmaps + 0x6c);

    if (hud_globals == k_datum_index_none) {
        return 0;
    }
    return hs_parse_scenario_datum(node_index, 0,
        (TagReflexive *)((uint8_t *)tag_instances[hud_globals & 0xffff].data + 0x160), 0x68);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

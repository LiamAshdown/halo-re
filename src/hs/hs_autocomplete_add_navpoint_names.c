// hs_autocomplete_add_navpoint_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483bf0, size 87 bytes
// name confidence: 0.6  rewrite confidence: 0.80
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 16 = 0x483bf0; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483bf0..0x483c46: the navpoint (waypoint arrow) names: global_globals->interface_bitmaps element 0's
//   hud_globals tag id (+0x6c); when it is not -1, that tag's +0x160 block (name at +0, 0x68 bytes) through
//   hs_autocomplete_scan_globals. Like the original, the first element is read without checking the count
//   (a NULL block pointer is read at +0x6c).
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride); // 0x483770
extern Globals *global_globals; // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

void hs_autocomplete_add_navpoint_names(void)
{
    uint8_t *interface_bitmaps = global_globals->interface_bitmaps.count != 0 ?
        (uint8_t *)global_globals->interface_bitmaps.pointer : 0;
    datum_index hud_globals = *(datum_index *)(interface_bitmaps + 0x6c);

    if (hud_globals != k_datum_index_none) {
        hs_autocomplete_scan_globals((TagReflexive *)((uint8_t *)tag_instances[hud_globals & 0xffff].data + 0x160),
            0, 0x68);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

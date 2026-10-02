// hs_autocomplete_add_hud_message_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483c50, size 51 bytes
// name confidence: 0.6  rewrite confidence: 0.80
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 17 = 0x483c50; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483c50..0x483c82: with a hud_messages tag on the scenario (tag id at +0x5a0 not -1), that tag's
//   +0x20 block (name at +0, 0x40 bytes) through hs_autocomplete_scan_globals.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride); // 0x483770
extern Scenario *global_scenario; // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14

void hs_autocomplete_add_hud_message_names(void)
{
    datum_index hud_messages = *(datum_index *)&global_scenario->hud_messages.tag_id;

    if (hud_messages != k_datum_index_none) {
        hs_autocomplete_scan_globals((TagReflexive *)((uint8_t *)tag_instances[hud_messages & 0xffff].data + 0x20),
            0, 0x40);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

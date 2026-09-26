// hs_autocomplete_add_encounter_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483a10, size 36 bytes
// name confidence: 0.6  rewrite confidence: 0.70
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 6 = 0x483a10; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483a10: with a scenario loaded (global_scenario_index != -1), offers the encounter names: the
//   scenario's encounters block (+0x42c), name at +0 of each 0xb0-byte element, through hs_autocomplete_scan_globals.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride); // 0x483770
extern datum_index global_scenario_index; // 0x0069e8d4
extern Scenario *global_scenario; // 0x00746f8c

void hs_autocomplete_add_encounter_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->encounters, 0, 0xb0);
    }
}

// hs_autocomplete_add_recorded_animation_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483bc0, size 33 bytes
// name confidence: 0.6  rewrite confidence: 0.70
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 15 = 0x483bc0; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483bc0: with a scenario loaded (global_scenario_index != -1), offers the recorded animation names: the
//   scenario's recorded_animations block (+0x36c), name at +0 of each 0x40-byte element, through hs_autocomplete_scan_globals.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern datum_index global_scenario_index; // 0x0069e8d4
extern Scenario *global_scenario; // 0x00746f8c

void hs_autocomplete_add_recorded_animation_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->recorded_animations, 0, 0x40);
    }
}

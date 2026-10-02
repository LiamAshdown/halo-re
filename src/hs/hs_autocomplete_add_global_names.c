// hs_autocomplete_add_global_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483960, size 166 bytes
// name confidence: 0.6  rewrite confidence: 0.90
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 5 = 0x483960; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483960..0x483a05: the 0x1eb builtin hs_global_definitions whose console-context flags (low byte
//   of +0xc, BL) pass hs_gametype_flags_applicable are offered like function names; then, with a scenario
//   loaded, the scenario's globals block (+0x4a8, name at +0, 0x5c bytes) through hs_autocomplete_scan_globals.
// blam-cc: (no arguments)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t hs_autocomplete_maximum_count; // 0x006b14a0
extern char *hs_autocomplete_prefix;          // 0x006b14a4
extern int16_t hs_autocomplete_count;         // 0x006b14b0
extern char **hs_autocomplete_results;        // 0x006b14b4
extern uint8_t hs_gametype_flags_applicable(uint8_t flags); // 0x483600, blam-cc: BL -> flags
extern void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride); // 0x483770
extern datum_index global_scenario_index; // 0x0069e8d4
extern Scenario *global_scenario; // 0x00746f8c
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398

static void autocomplete_offer(char *candidate)
{
    if (hs_autocomplete_count < hs_autocomplete_maximum_count &&
        _strnicmp(candidate, hs_autocomplete_prefix, (int32_t)strlen(hs_autocomplete_prefix)) == 0) {
        hs_autocomplete_results[hs_autocomplete_count] = candidate;
        hs_autocomplete_count = hs_autocomplete_count + 1;
    }
}

void hs_autocomplete_add_global_names(void)
{
    int32_t i;

    for (i = 0; i < k_hs_builtin_global_count; i++) {
        hs_global_definition *definition = hs_global_definitions[i];

        if (hs_gametype_flags_applicable((uint8_t)definition->gametype_flags)) {
            autocomplete_offer(definition->name);
        }
    }
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->globals, 0, 0x5c);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

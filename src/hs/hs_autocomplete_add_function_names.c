// hs_autocomplete_add_function_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x4838a0, size 134 bytes
// name confidence: 0.6  rewrite confidence: 0.90
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 3 = 0x4838a0; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x4838a0..0x483925: every one of the 0x20a hs_function_definitions whose console-context flags
//   (low byte of +0x18, BL) pass hs_gametype_flags_applicable has its name offered: added while the result
//   list has room and the name starts with the prefix (_strnicmp over the prefix length).
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
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

static void autocomplete_offer(char *candidate)
{
    if (hs_autocomplete_count < hs_autocomplete_maximum_count &&
        _strnicmp(candidate, hs_autocomplete_prefix, (int32_t)strlen(hs_autocomplete_prefix)) == 0) {
        hs_autocomplete_results[hs_autocomplete_count] = candidate;
        hs_autocomplete_count = hs_autocomplete_count + 1;
    }
}

void hs_autocomplete_add_function_names(void)
{
    int32_t i;

    for (i = 0; i < 0x20a; i++) {
        hs_function_definition *definition = hs_function_definitions[i];

        if (hs_gametype_flags_applicable((uint8_t)definition->gametype_flags)) {
            autocomplete_offer(definition->name);
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

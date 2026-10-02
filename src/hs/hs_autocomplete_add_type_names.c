// hs_autocomplete_add_type_names  (not a Ghidra function; an hs autocomplete collector)
// address 0x483880, size 22 bytes
// name confidence: 0.6  rewrite confidence: 0.70
// evidence: hs_autocomplete_procedures 0x00689380 (18 entries) slot 2 = 0x483880; hs_autocomplete_gather
//   0x483c90 calls entry i when bit i of its mask is set (console_process_command passes 0x28, so slots 3
//   and 5 run for every console command). Only reachable through that table. First-boot track: the
//   startup exec script's console command gathers the first word through it.
// objdump 0x483880..0x483895: hs_autocomplete_scan_candidates(hs_type_names, AX = 0x31, CX = 4): the type
//   names from index 4 (after the four internal ones) up to 0x31.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_autocomplete_scan_candidates(char **table, int16_t end, int16_t start); // 0x4836f0, blam-cc: AX end, CX start
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78

void hs_autocomplete_add_type_names(void)
{
    hs_autocomplete_scan_candidates(hs_type_names, 0x31, 4);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

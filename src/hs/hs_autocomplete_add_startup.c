// hs_autocomplete_add_startup  (Ghidra: chimera__autocomplete_add_startup; the chimera__
// prefix is a Chimera symbol, not a Bungie one, per out/phase4/hs_types_notes.md's note on the
// sibling functions in this family, so it is renamed to drop it)
// address 0x483860, size 19 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: forwards hs_script_type_names to hs_autocomplete_scan_candidates (0x4836f0); the
// end/start indices are register immediates (AX/CX) that Ghidra could not attribute to this
// call site at all (the callee shows them as in_AX/in_CX, but the caller shows zero visible
// arguments beyond the table pointer). hs_functions.md's summary, "Adds the HS script-type
// keywords (startup/dormant/continuous/static) ... if they match", picks out indices 0..3 of
// the 5-entry table (types/hs.h: startup, dormant, continuous, static, stub) -- i.e. every
// entry except "stub" -- which is used here as start=0, end=4.
// register convention: `table` is recognized directly by Ghidra as the sole visible argument.
// UNSURE: the literal start/end values (0, 4) are inferred from the function's documented
// behavior, not read directly out of this call site's instructions.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_autocomplete_scan_candidates(char **table, int16_t end, int16_t start); // 0x004836f0, this batch

extern char *hs_script_type_names[k_hs_script_type_count]; // 0x00688b3c

// Adds the HS script-type keywords startup/dormant/continuous/static (every
// hs_script_type_names entry except "stub") to the autocomplete results.
void hs_autocomplete_add_startup(void)
{
    hs_autocomplete_scan_candidates(hs_script_type_names, 4, 0);
}

#if 0
Original Ghidra decompilation (0x483860):

void chimera__autocomplete_add_startup(void)

{
  FUN_004836f0(&PTR_s_startup_00688b3c);
  return;
}
#endif

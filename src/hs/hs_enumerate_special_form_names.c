// hs_enumerate_special_form_names  (Ghidra: FUN_00483840; renamed per the CEA-PDB hint
// "hs_enumerate_special_form_names", which out/phase4/hs_functions.md's own string evidence
// ('script', 'global') corroborates)
// address 0x483840, size 23 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: tests the "script" and "global" special-form keyword literals against the
// autocomplete prefix via hs_autocomplete_test_candidate (0x483690, called twice here with no
// visible arguments in Ghidra because each string literal was loaded straight into EDI).
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_autocomplete_test_candidate(char *candidate); // 0x00483690, this batch

// Adds the "script" and "global" special-form keywords to the autocomplete results if they
// match the current prefix.
void hs_enumerate_special_form_names(void)
{
    hs_autocomplete_test_candidate((char *)"script");
    hs_autocomplete_test_candidate((char *)"global");
}

#if 0
Original Ghidra decompilation (0x483840):

void FUN_00483840(void)

{
  FUN_00483690();
  FUN_00483690();
  return;
}
#endif

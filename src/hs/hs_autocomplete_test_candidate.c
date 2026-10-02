// hs_autocomplete_test_candidate  (Ghidra: FUN_00483690, renamed)
// address 0x483690, size 81 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: tests `candidate` case-insensitively against hs_autocomplete_prefix
// (_strnicmp over strlen(prefix) bytes) and, on a match, appends it to hs_autocomplete_results
// if there is room -- the innermost building block of the chimera__autocomplete_* family
// documented in out/phase4/hs_types_notes.md.
// register convention: the candidate string is unrecognized by Ghidra (unaff_EDI); by the
// blam-cc convention this is the sixth register slot, EDI.

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

// blam-cc: candidate string in EDI
// Appends `candidate` to hs_autocomplete_results if it starts with hs_autocomplete_prefix
// (case-insensitively) and the results buffer is not already full.
void hs_autocomplete_test_candidate(char *candidate)
{
    int32_t prefix_length;

    if (hs_autocomplete_count < hs_autocomplete_maximum_count) {
        prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
        if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
            hs_autocomplete_results[hs_autocomplete_count] = candidate;
            hs_autocomplete_count = hs_autocomplete_count + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x483690):

void FUN_00483690(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  char *unaff_EDI;

  if (DAT_006b14b0 < DAT_006b14a0) {
    pcVar2 = DAT_006b14a4;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    iVar3 = __strnicmp(unaff_EDI,DAT_006b14a4,(int)pcVar2 - (int)(DAT_006b14a4 + 1));
    if (iVar3 == 0) {
      iVar3 = (int)DAT_006b14b0;
      DAT_006b14b0 = DAT_006b14b0 + 1;
      *(char **)(DAT_006b14b4 + iVar3 * 4) = unaff_EDI;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

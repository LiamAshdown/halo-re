// hs_string_is_empty  (Ghidra: hs_string_is_single_char -- misattributed per
// out/phase4/hs_types_notes.md: "actually returns 'the string is empty' (strlen(s+1) - ... == 0
// reduces to s[0] == 0 after the first character is skipped)")
// address 0x48aaf0, size 31 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/hs_types_notes.md misattribution note.
// register convention: none (void); string pointer is the recognized stack parameter (param_1).
// note: the decompiled loop computes strlen(s) by scanning to the NUL and comparing it to 0,
// which is bit-for-bit equivalent to testing s[0] == '\0' for every input; written directly here
// rather than as a hand-inlined strlen, matching this codebase's convention of recognizing
// inlined library idioms (see e.g. src/hs/hs_report_expected_enum_values.c's strcat/strcpy).

#include "tags.h"

// Returns nonzero if `s` is the empty string.
char hs_string_is_empty(char *s)
{
    return s[0] == '\0';
}

#if 0
Original Ghidra decompilation (0x48aaf0):

undefined4 hs_string_is_single_char(char *param_1)

{
  char *pcVar1;
  char cVar2;
  undefined4 local_4;

  pcVar1 = param_1 + 1;
  do {
    cVar2 = *param_1;
    param_1 = param_1 + 1;
  } while (cVar2 != '\0');
  local_4 = CONCAT31((int3)((uint)((int)param_1 - (int)pcVar1) >> 8),(int)param_1 - (int)pcVar1 == 0
                    );
  return local_4;
}
#endif

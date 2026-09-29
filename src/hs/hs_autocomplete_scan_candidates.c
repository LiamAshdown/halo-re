// hs_autocomplete_scan_candidates  (Ghidra: FUN_004836f0, renamed)
// address 0x4836f0, size 119 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: iterates a char* table from index `start` to `end`, testing each entry against
// hs_autocomplete_prefix and appending matches to hs_autocomplete_results -- the same
// test-and-append logic as hs_autocomplete_test_candidate (0x483690), duplicated inline here
// (its callee list carries only _strnicmp, not that function) rather than calling it; kept
// inline in this rewrite to match the compiled shape exactly.
// register convention: `table` is recognized directly by Ghidra; `end` and `start` are
// unrecognized (in_AX / in_CX), which by the blam-cc convention are the first and second
// register slots, EAX/AX and ECX/CX.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <string.h>


extern int16_t hs_autocomplete_maximum_count; // 0x006b14a0
extern char *hs_autocomplete_prefix;          // 0x006b14a4
extern int16_t hs_autocomplete_count;         // 0x006b14b0
extern char **hs_autocomplete_results;        // 0x006b14b4

// blam-cc: end index in AX (EAX), start index in CX (ECX)
// Appends every table[start..end) entry that starts with hs_autocomplete_prefix
// (case-insensitively) to hs_autocomplete_results, while there is room.
void hs_autocomplete_scan_candidates(char **table, int16_t end, int16_t start)
{
    char **entry;
    uint16_t remaining;
    char *candidate;
    int32_t prefix_length;

    if (start < end) {
        entry = table + start;
        remaining = (uint16_t)(end - start);
        do {
            candidate = *entry;
            if (hs_autocomplete_count < hs_autocomplete_maximum_count) {
                prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
                if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
                    hs_autocomplete_results[hs_autocomplete_count] = candidate;
                    hs_autocomplete_count = hs_autocomplete_count + 1;
                }
            }
            entry = entry + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
}

#if 0
Original Ghidra decompilation (0x4836f0):

void FUN_004836f0(int param_1)

{
  char cVar1;
  char *_Str1;
  short in_AX;
  char *pcVar2;
  int iVar3;
  short in_CX;
  undefined4 *puVar4;
  uint uVar5;

  if (in_CX < in_AX) {
    puVar4 = (undefined4 *)(param_1 + in_CX * 4);
    uVar5 = (uint)(ushort)(in_AX - in_CX);
    do {
      _Str1 = (char *)*puVar4;
      if (DAT_006b14b0 < DAT_006b14a0) {
        pcVar2 = DAT_006b14a4;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        iVar3 = __strnicmp(_Str1,DAT_006b14a4,(int)pcVar2 - (int)(DAT_006b14a4 + 1));
        if (iVar3 == 0) {
          iVar3 = (int)DAT_006b14b0;
          DAT_006b14b0 = DAT_006b14b0 + 1;
          *(char **)(DAT_006b14b4 + iVar3 * 4) = _Str1;
        }
      }
      puVar4 = puVar4 + 1;
      uVar5 = uVar5 - 1;
    } while (uVar5 != 0);
  }
  return;
}
#endif

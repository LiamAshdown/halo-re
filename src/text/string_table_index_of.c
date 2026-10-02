// string_table_index_of  (orphan pass 4: FUN_004875c0, no Ghidra name)
// address 0x4875c0, size 102 bytes
// name confidence: 0.4 (src/hs/README.md, "Misattributed functions" #3: "a generic 'index of a
//   string in a char** table' helper: (count, table) on the stack, the search string in EAX,
//   returns the index in AX or 0xffff. Used by hs_add_global, hs_add_script and the
//   autocomplete walkers, but it is cseries/text code, not hs.")
// rewrite confidence: 0.5 (a plain linear search with an inlined strcmp; confirmed against the
//   decompilation)
// evidence: src/hs/README.md (quoted above). Placed in src/text alongside this module's other
//   plain string-comparison helpers (text_char_is_double_byte.c and friends).
// register convention: EAX = const char *search, stack arguments = int16_t count,
//   const char **table.
// blam-cc: EAX -> search, stack -> count, table
// FIXED (register inputs, objdump): note phrasing only -- rewritten from the call-style
// "f(x /*EAX*/)" comment the checker cannot parse into "EAX -> search, stack -> ...".

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int16_t string_table_index_of(const char *search, int16_t count, const char **table)
{
    int16_t index;

    for (index = 0; index < count; index++) {
        const unsigned char *a = (const unsigned char *)table[index];
        const unsigned char *b = (const unsigned char *)search;

        for (;;) {
            if (*a != *b) {
                break;
            }
            if (*a == 0) {
                return index; // exact match
            }
            a++;
            b++;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4875c0):

short FUN_004875c0(short param_1,int param_2)

{
  byte bVar1;
  byte *in_EAX;
  byte *pbVar2;
  int iVar3;
  byte *pbVar4;
  short sVar5;
  bool bVar6;

  sVar5 = 0;
  if (0 < param_1) {
    do {
      pbVar4 = *(byte **)(param_2 + sVar5 * 4);
      pbVar2 = in_EAX;
      do {
        bVar1 = *pbVar2;
        bVar6 = bVar1 < *pbVar4;
        if (bVar1 != *pbVar4) {
LAB_00487604:
          iVar3 = (1 - (uint)bVar6) - (uint)(bVar6 != 0);
          goto LAB_00487609;
        }
        if (bVar1 == 0) break;
        bVar1 = pbVar2[1];
        bVar6 = bVar1 < pbVar4[1];
        if (bVar1 != pbVar4[1]) goto LAB_00487604;
        pbVar2 = pbVar2 + 2;
        pbVar4 = pbVar4 + 2;
      } while (bVar1 != 0);
      iVar3 = 0;
LAB_00487609:
      if (iVar3 == 0) {
        return sVar5;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < param_1);
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

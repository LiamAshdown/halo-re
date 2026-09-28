// hs_autocomplete_gather  (Ghidra: chimera__autocomplete_gather; renamed per
// out/phase4/hs_types_notes.md -- "the chimera__ prefix is a Chimera symbol name, not a Bungie
// one")
// address 0x483c90, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: sets up all six autocomplete-state globals, then calls each of the 18
// hs_autocomplete_procedures entries whose bit is set in `category_mask` (the 0x12 loop bound
// matches k_hs_autocomplete_procedures's documented size), then sorts and returns the results.
// The comparator (0x617340) is third-party gamespy SDK code (module=lib:gamespy in
// modules.json), not part of this module, and is left as KeyValCompareKeyA.
// register convention: category_mask and results are recognized directly by Ghidra; prefix,
// maximum_count and gametype_mask are unrecognized (in_EAX/in_CX/in_DX), which by the blam-cc
// convention are the first three register slots, EAX, ECX (CX), EDX (DX).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdlib.h>

extern void KeyValCompareKeyA(const void *a, const void *b); // 0x00617340, lib:gamespy, not this module
extern void hs_enumerate_special_form_names(void); // 0x00483840, this batch (table entry only)
extern void hs_autocomplete_add_startup(void); // 0x00483860, this batch (table entry only)

extern void *hs_autocomplete_procedures[0x12]; // 0x00689380
extern char k_empty_string[1]; // 0x0065512c
extern int16_t hs_autocomplete_maximum_count; // 0x006b14a0
extern char *hs_autocomplete_prefix;          // 0x006b14a4
extern uint16_t hs_autocomplete_gametype_mask; // 0x006b14ac
extern int16_t hs_autocomplete_count;         // 0x006b14b0
extern char **hs_autocomplete_results;        // 0x006b14b4

// blam-cc: prefix in EAX, maximum_count in CX (ECX), gametype_mask in DX (EDX)
// Gathers autocomplete candidates for `prefix` across every category selected by
// `category_mask` into `results` (capped at maximum_count entries), sorts them, and returns
// the number found.
int16_t hs_autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count, uint16_t gametype_mask)
{
    void **procedure;
    int32_t remaining;
    uint8_t bit;

    hs_autocomplete_count = 0;
    hs_autocomplete_results = results;
    hs_autocomplete_prefix = prefix;
    if (prefix == 0) {
        hs_autocomplete_prefix = k_empty_string;
    }
    procedure = hs_autocomplete_procedures;
    remaining = 0x12;
    hs_autocomplete_maximum_count = maximum_count;
    hs_autocomplete_gametype_mask = gametype_mask;
    bit = 0;
    do {
        if ((category_mask & (1u << (bit & 0x1f))) != 0) {
            ((void (*)(void)) * procedure)();
        }
        bit = bit + 1;
        procedure = procedure + 1;
        remaining = remaining - 1;
    } while (remaining != 0);
    qsort(results, (size_t)hs_autocomplete_count, 4, (int (*)(const void *, const void *))KeyValCompareKeyA);
    hs_autocomplete_results = 0;
    return hs_autocomplete_count;
}

#if 0
Original Ghidra decompilation (0x483c90):

short chimera__autocomplete_gather(uint param_1,void *param_2)

{
  undefined1 *in_EAX;
  undefined2 in_CX;
  undefined2 in_DX;
  int iVar1;
  byte bVar2;
  undefined **ppuVar3;

  bVar2 = 0;
  DAT_006b14b0 = 0;
  DAT_006b14b4 = param_2;
  DAT_006b14a4 = in_EAX;
  if (in_EAX == (undefined1 *)0x0) {
    DAT_006b14a4 = &DAT_0065512c;
  }
  ppuVar3 = &PTR_FUN_00689380;
  iVar1 = 0x12;
  DAT_006b14a0 = in_CX;
  DAT_006b14ac._0_2_ = in_DX;
  do {
    if ((param_1 & 1 << (bVar2 & 0x1f)) != 0) {
      (*(code *)*ppuVar3)();
    }
    bVar2 = bVar2 + 1;
    ppuVar3 = ppuVar3 + 1;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  _qsort(param_2,(int)DAT_006b14b0,4,FUN_00617340);
  DAT_006b14b4 = (void *)0x0;
  return DAT_006b14b0;
}
#endif

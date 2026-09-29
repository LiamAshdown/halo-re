// hs_autocomplete_scan_globals  (Ghidra: chimera__autocomplete_scan_globals; renamed per
// out/phase4/hs_types_notes.md -- "the chimera__ prefix is a Chimera symbol name, not a Bungie
// one", the function itself is an engine builtin of the hs_enumerate_* autocomplete family)
// address 0x483770, size 197 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: walks a TagReflexive-shaped (count, pointer) table with a caller-supplied element
// stride and name-field offset -- the generic shape used for both hs_global_definitions and
// Scenario::globals -- calling hs_find_global_by_name on each entry's name and, for a builtin
// hit, gating it on hs_gametype_flags_applicable before testing it as an autocomplete
// candidate (inlined here, same as hs_autocomplete_scan_candidates: its callee list has no
// call to hs_autocomplete_test_candidate).
// register convention: __cdecl (param_1, param_2, param_3 recognized directly); the gametype
// flags byte hs_gametype_flags_applicable reads is unrecognized by Ghidra in this function's
// own body (no local sets BL before that call), so it is modeled as a fourth parameter
// forwarded unchanged from this function's own caller, by the blam-cc convention the fourth
// register slot, EBX.
// UNSURE: `global_index < 0 && hs_global_definitions[global_index] != 0` indexes the table
// with the packed, sign-bit-set hs_global_reference itself (not `global_index &
// k_hs_global_index_mask`), which reads far out of bounds of hs_global_definitions for every
// builtin match. This looks like a decompiler/source artifact rather than intended behavior,
// but it is preserved exactly rather than "fixed", per the no-invented-behaviour rule. Given
// this function also has zero call sites Ghidra could resolve (only reachable through the
// hs_autocomplete_procedures table), the practical effect of this branch is unverified.
// FIXED (verified against 0x4837b2..0x4837c3): the flags byte hs_gametype_flags_applicable reads (BL) is the
//   builtin definition's own +0xc (`mov bl,[eax+0xc]`), not a fourth argument; every caller (the collectors at
//   0x483930..0x483c50) pushes exactly three.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <string.h>


extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern int16_t hs_autocomplete_maximum_count; // 0x006b14a0
extern char *hs_autocomplete_prefix;          // 0x006b14a4
extern int16_t hs_autocomplete_count;         // 0x006b14b0
extern char **hs_autocomplete_results;        // 0x006b14b4

// blam-cc: stack -> table, name_offset, stride (cdecl)
// Scans a (count, pointer) table of stride-`stride` records, testing the name field at
// `name_offset` within each record as an autocomplete candidate: unconditionally if it is not
// a known global at all, or if it is a builtin whose gametype flags allow the current gametype.
void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride)
{
    int32_t count;
    int32_t i;
    char *candidate;
    int16_t global_index;
    char applicable;
    int32_t prefix_length;

    count = (int32_t)table->count;
    if (0 < count) {
        for (i = 0; i < count; i = i + 1) {
            candidate = (char *)((int32_t)name_offset + i * stride + (int32_t)table->pointer);
            global_index = (int16_t)hs_find_global_by_name(candidate);
            if (((global_index == -1) ||
                 ((global_index < 0) &&
                  (hs_global_definitions[global_index] != 0) &&
                  (applicable = (char)hs_gametype_flags_applicable(
                      (uint8_t)hs_global_definitions[global_index]->gametype_flags), applicable != 0))) &&
                (hs_autocomplete_count < hs_autocomplete_maximum_count)) {
                prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
                if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
                    hs_autocomplete_results[hs_autocomplete_count] = candidate;
                    hs_autocomplete_count = hs_autocomplete_count + 1;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x483770):

void chimera__autocomplete_scan_globals(int *param_1,short param_2,int param_3)

{
  char cVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  char *pcVar5;
  char *_Str1;

  iVar4 = 0;
  sVar3 = 0;
  if (0 < *param_1) {
    do {
      _Str1 = (char *)((int)param_2 + iVar4 * param_3 + param_1[1]);
      sVar2 = chimera__get_global_index();
      if (((sVar2 == -1) ||
          (((sVar2 < 0 && ((&PTR_PTR_0068b398)[sVar2] != (undefined *)0x0)) &&
           (cVar1 = FUN_00483600(), cVar1 != '\0')))) && (DAT_006b14b0 < DAT_006b14a0)) {
        pcVar5 = DAT_006b14a4;
        do {
          cVar1 = *pcVar5;
          pcVar5 = pcVar5 + 1;
        } while (cVar1 != '\0');
        iVar4 = __strnicmp(_Str1,DAT_006b14a4,(int)pcVar5 - (int)(DAT_006b14a4 + 1));
        if (iVar4 == 0) {
          iVar4 = (int)DAT_006b14b0;
          DAT_006b14b0 = DAT_006b14b0 + 1;
          *(char **)(DAT_006b14b4 + iVar4 * 4) = _Str1;
        }
      }
      sVar3 = sVar3 + 1;
      iVar4 = (int)sVar3;
    } while (iVar4 < *param_1);
  }
  return;
}
#endif

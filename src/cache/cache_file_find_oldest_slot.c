// cache_file_find_oldest_slot  (Ghidra: cache_file_find_oldest_slot, already named)
// address 0x4437b0, size 275 bytes
// name confidence: 0.65   rewrite confidence: 0.80
// evidence: types/cache.h cache_file_slot_category enum and out/phase4/cache_types_notes.md
// cache_file_slot section (the branchless size-limit expression and its per-category values
// are transcribed verbatim from the decompile in that note).
// register convention: slot_category in EAX (in_AX, the Ghidra-labelled __cdecl parameter
// named "category" is in fact the __cdecl stack argument, renamed required_size below since it
// is compared against each slot's size limit, not used to select the scan range).
//
// UNSURE: when slot_category is anything other than 0/1/2, the original leaves the scan range
// (start/end) uninitialized (whatever was already in the register aliased as in_ECX on entry).
// That is undefined behaviour in the original and is preserved as-is rather than guarded.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern int16_t cache_file_index;                                  // 0x006ac494, -1 when none open

extern int32_t __stdcall CompareFileTime(file_time *a, file_time *b); // 0x0063a2e0 IAT

// Per-slot size limit used by cache_file_find_oldest_slot: 0x18000000 for slots 0-1,
// 0x02300000 for slot 2, 0x08000000 for slots 3-5 (see types/cache.h
// cache_file_slot_category and the branchless original expression preserved in the #if 0 block).
static int32_t cache_file_slot_size_limit(int16_t slot_index)
{
    if (slot_index < 2) {
        return 0x18000000;
    }
    return (((2 < slot_index) - 1) & 0xfa300000) + 0x8000000;
}

// blam-cc: slot_category in EAX (in_AX); required_size is the true __cdecl stack parameter
// Chooses the least-recently-used cache-file slot within slot_category (single_player: slots
// 0-1, multiplayer: slots 3-5, ui: slot 2) whose size limit can hold required_size, skipping the
// currently active slot. Ties are broken by last-write time (CompareFileTime), preferring the
// older file. Returns -1 if no eligible slot exists.
int16_t cache_file_find_oldest_slot(cache_file_slot_category slot_category, int32_t required_size)
{
    int16_t start;
    int16_t end;
    int16_t i;
    int16_t best_slot;
    cache_file_slot *current;
    cache_file_slot *best;

    best_slot = -1;

    if (slot_category == _cache_file_slot_category_single_player) {
        start = 0;
        end = 1;
    } else if (slot_category == _cache_file_slot_category_multiplayer) {
        start = 3;
        end = 5;
    } else if (slot_category == _cache_file_slot_category_ui) {
        start = 2;
        end = 2;
    }
    /* else: start/end left uninitialized, matching the original's undefined behaviour */

    if (start <= end) {
        i = start;
        current = &cache_file_slots[start];
        best = current; // dead until the first candidate is accepted (best_slot still -1 then)
        do {
            if (cache_file_index != i) {
                int32_t limit = cache_file_slot_size_limit(i);
                if (required_size < limit) {
                    if (best_slot != -1) {
                        int32_t best_limit = cache_file_slot_size_limit(best_slot);
                        if (best_limit <= limit &&
                            CompareFileTime(&best->last_write_time, &current->last_write_time) < 1) {
                            goto next;
                        }
                    }
                    best_slot = i;
                    best = current;
                }
            }
next:
            i = i + 1;
            current = current + 1;
        } while (i <= end);
    }

    return best_slot;
}

#if 0
Original Ghidra decompilation (0x4437b0):

short __cdecl cache_file_find_oldest_slot(int category)

{
  short in_AX;
  int iVar1;
  int iVar2;
  LONG LVar3;
  undefined *in_ECX;
  short sVar4;
  undefined *puVar5;
  undefined *puVar6;
  short sVar7;
  undefined *puVar8;
  undefined *local_4;

  puVar5 = (undefined *)0xffffffff;
  sVar7 = -1;
  if (in_AX == 0) {
    in_ECX = (undefined *)0x0;
    local_4 = (undefined *)0x1;
  }
  else if (in_AX == 1) {
    in_ECX = (undefined *)0x3;
    local_4 = (undefined *)0x5;
  }
  else {
    local_4 = in_ECX;
    if (in_AX == 2) {
      in_ECX = (undefined *)0x2;
      local_4 = (undefined *)0x2;
    }
  }
  if ((short)in_ECX <= (short)local_4) {
    puVar8 = &DAT_006a9428 + (short)in_ECX * 0x80c;
    puVar6 = local_4;
    do {
      sVar7 = (short)in_ECX;
      if (DAT_006ac494 != sVar7) {
        if (sVar7 < 2) {
          iVar1 = 0x18000000;
        }
        else {
          iVar1 = ((2 < sVar7) - 1 & 0xfa300000) + 0x8000000;
        }
        if (category < iVar1) {
          sVar4 = (short)puVar5;
          if (sVar4 != -1) {
            if (sVar7 < 2) {
              iVar1 = 0x18000000;
            }
            else {
              iVar1 = ((2 < sVar7) - 1 & 0xfa300000) + 0x8000000;
            }
            if (sVar4 < 2) {
              iVar2 = 0x18000000;
            }
            else {
              iVar2 = ((2 < sVar4) - 1 & 0xfa300000) + 0x8000000;
            }
            if ((iVar2 <= iVar1) &&
               (LVar3 = CompareFileTime((FILETIME *)(puVar6 + 4),(FILETIME *)(puVar8 + 4)),
               LVar3 < 1)) goto LAB_004438a8;
          }
          puVar5 = in_ECX;
          puVar6 = puVar8;
        }
      }
LAB_004438a8:
      sVar7 = (short)puVar5;
      in_ECX = in_ECX + 1;
      puVar8 = puVar8 + 0x80c;
    } while ((short)in_ECX <= (short)local_4);
  }
  return sVar7;
}
#endif

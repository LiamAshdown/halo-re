// object_permutation_find_matching_group  (Ghidra: FUN_004f8d80; renamed, Blam-style, not
// previously named)
// address 0x4f8d80, size 71 bytes (Ghidra split the loop tail 0x4f8dc0..0x4f8dc6 off as "object_new_with_role"; included here)
// name confidence: 0.4 (matches functions.md's summary: "Collects the indices of a region's
//   permutations that belong to a requested probability group"; cited by out/phase4/
//   objects_types_notes.md as evidence for ModelRegionPermutation's flags/permutation_number
//   layout)
// rewrite confidence: 0.6
// evidence: types/tags.h ModelRegion.permutations, ModelRegionPermutation (flags 0x20 bit 0,
//   permutation_number 0x24).
// register convention: region pointer in ESI, probability-group filter in DI, output array as
//   the sole stack parameter. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8d80
//   mov ecx,[esi+0x40] at entry, no other input read before the stack fetch at 0x4f8d8a.
//   // blam-cc: ESI -> region, DI -> group, stack -> out
// UNSURE: the exact semantics of "group == -1 with permutation_number < 100" as an additional
//   match condition are preserved literally rather than named.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

// VERIFIED against disassembly 0x4f8d80..0x4f8dc6 (2026-09-30); fixed: signed 16-bit permutation number compares and the 32-bit
//   count bound (see the loop).
int16_t object_permutation_find_matching_group(ModelRegion *region, int16_t group, int16_t *out)
    // blam-cc: ESI -> region, DI -> group, stack -> out
{
    int16_t count = 0;
    int16_t i;

    for (i = 0; (int32_t)i < (int32_t)region->permutations.count; i++) { // 0x4f8dbd: movsx ecx, dx; cmp ecx, [esi+0x40]
        ModelRegionPermutation *perm = (ModelRegionPermutation *)region->permutations.pointer + i;
        if ((perm->flags & 1) == 0) {
            // 0x4f8d9e..0x4f8db1: 16-bit compares, and `< 100` is SIGNED (jge): the tag field is declared uint16_t, so without the
            // casts 0xffff never equalled group -1 and any value >= 0x8000 failed `< 100`.
            if (((int16_t)perm->permutation_number == group) ||
                ((group == -1) && ((int16_t)perm->permutation_number < 100))) {
                out[count] = i;
                count++;
            }
        }
    }

    return count;
}

#if 0
Original Ghidra decompilation (0x4f8d80):

void FUN_004f8d80(int param_1)

{
  short sVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  int unaff_ESI;
  short unaff_DI;

  sVar2 = 0;
  sVar4 = 0;
  if (0 < *(int *)(unaff_ESI + 0x40)) {
    iVar3 = 0;
    do {
      iVar3 = iVar3 * 0x58 + *(int *)(unaff_ESI + 0x44);
      if (((*(byte *)(iVar3 + 0x20) & 1) == 0) &&
         ((sVar1 = *(short *)(iVar3 + 0x24), sVar1 == unaff_DI ||
          ((unaff_DI == -1 && (sVar1 < 100)))))) {
        *(short *)(param_1 + sVar2 * 2) = sVar4;
        sVar2 = sVar2 + 1;
      }
      sVar4 = sVar4 + 1;
      iVar3 = (int)sVar4;
    } while (iVar3 < *(int *)(unaff_ESI + 0x40));
  }
  return;
}
#endif

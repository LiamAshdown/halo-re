// ai_weighted_random_index  (Ghidra: ai_weighted_random_index; named for this rewrite)
// address 0x432100, size 246 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: generic over its caller (a byte offset, a stride and a count, plus an exclusion
// bitmask), matching the phase-4 summary ("weighted random selection over an array of float
// weights, excluding indices flagged in a bitmask"); not tied to any ai.h struct. Uses the
// same LCG (0x19660d / 0x3c6ef35f) and 1.5259022e-05 (1/65536) scale already established
// elsewhere in this module for random_seed_global.
// VERIFIED against disassembly 0x432100..0x4321f5 (2026-09-30).
// register convention: Ghidra resolved param_1..param_4 as ordinary stack parameters and
// left only the byte offset in DX unresolved.
//   // blam-cc: EDX -> weight_offset, stack -> base, stride, count, exclude_mask

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t random_seed_global; // 0x00719cd0

// blam-cc: EDX -> weight_offset, stack -> base, stride, count, exclude_mask
// Sums the float weight at weight_offset within every non-excluded element of a `count`
// element array (stride bytes apart, base + weight_offset the first weight), then rolls a
// random value in [0, total) and returns the index of the element whose cumulative weight
// range contains it. Returns -1 if count is not positive or every weight is excluded /
// non-positive.
int32_t ai_weighted_random_index(int16_t weight_offset, void *base, int16_t stride, uint16_t count,
                                  uint32_t *exclude_mask)
{
    float total;
    uint8_t *cursor;
    int32_t i;

    if ((int16_t)count < 1) {
        return -1;
    }

    total = 0.0f;
    cursor = (uint8_t *)base + weight_offset;
    for (i = 0; i < (uint16_t)count; i++) {
        if ((exclude_mask[i >> 5] & (1u << (i & 0x1f))) == 0) {
            total += *(float *)cursor;
        }
        cursor += stride;
    }

    if (total > 0.0f) {
        float roll;
        float running = 0.0f;
        int16_t chosen = 0;

        cursor = (uint8_t *)base + weight_offset;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        roll = (float)(random_seed_global >> 0x10) * 1.5259022e-05f * total;

        while ((exclude_mask[chosen >> 5] & (1u << (chosen & 0x1f))) != 0 ||
               (running = running + *(float *)cursor, (roll < running) == (roll == running))) {
            // the comparison above is Ghidra's literal `fVar1 < fVar2 == (fVar1 == fVar2)`,
            // which for non-NaN operands is equivalent to `roll > running`: kept as-is
            // rather than simplified, to preserve the original NaN behaviour exactly.
            chosen = chosen + 1;
            cursor += stride;
            if ((int16_t)count <= chosen) {
                return -1;
            }
        }
        return (int32_t)chosen;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x432100):

int FUN_00432100(int param_1,short param_2,ushort param_3,int param_4)

{
  float fVar1;
  float fVar2;
  int iVar3;
  short in_DX;
  short sVar4;
  float *pfVar5;
  float *pfVar6;
  uint uVar7;

  fVar1 = 0.0;
  pfVar6 = (float *)(param_1 + in_DX);
  if ((short)param_3 < 1) {
    return -1;
  }
  iVar3 = 0;
  uVar7 = (uint)param_3;
  pfVar5 = pfVar6;
  do {
    if ((*(uint *)(param_4 + (iVar3 >> 5) * 4) & 1 << ((byte)iVar3 & 0x1f)) == 0) {
      fVar1 = fVar1 + *pfVar5;
    }
    iVar3 = iVar3 + 1;
    pfVar5 = (float *)((int)pfVar5 + (int)param_2);
    uVar7 = uVar7 - 1;
  } while (uVar7 != 0);
  if (0.0 < fVar1) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    sVar4 = 0;
    fVar1 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * fVar1;
    fVar2 = 0.0;
    while (((*(uint *)(param_4 + ((int)sVar4 >> 5) * 4) & 1 << ((byte)sVar4 & 0x1f)) != 0 ||
           (fVar2 = fVar2 + *pfVar6, fVar1 < fVar2 == (fVar1 == fVar2)))) {
      sVar4 = sVar4 + 1;
      pfVar6 = (float *)((int)pfVar6 + (int)param_2);
      if ((short)param_3 <= sVar4) {
        return -1;
      }
    }
    return (int)sVar4;
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

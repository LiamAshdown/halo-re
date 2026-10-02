// actor_avoidance_interpolate_sample  (Ghidra: actor_avoidance_interpolate_sample, renamed)
// address 0x419240, size 396 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: its only caller is actor_movement_choose_avoidance_direction. It walks a
// circular list of sample directions (stride 0xc, i.e. real_vector3d) looking for the
// consecutive pair whose cross-product-x against the query direction changes sign while the
// dot product stays positive -- that is the pair the query direction lies between -- and
// linearly interpolates both the sample index and the matching entry of a parallel float
// array at that crossing. Returns 1 when a crossing was found, 0 otherwise.
// register convention: the query direction is in ECX and the sample array base in EBX;
// count, the parallel value array and the two out-parameters are genuine stack parameters
// (objdump: mov ebp,[esp+0x28] and the [esp+0x34] / [esp+0x38] loads at 0x41937f).
// blam-cc: ECX -> direction, EBX -> samples, stack -> count, values, out_index, out_value
//
// Facts recovered from the disassembly that Ghidra hides:
//  - the body computes all three components of the cross product into [esp+0x14/0x18/0x1c]
//    but only the x component is ever read; the other two stores are dead. Kept out of this
//    rewrite because nothing can observe them.
//  - the running "previous cross" value lives in the incoming param_1 stack slot, which MSVC
//    reuses once the count has been copied into BP; that is Ghidra's `param_1 = fVar3`.
//  - both non-matching exits return 0 (xor al,al at 0x419354 and mov al,dl at 0x4193c5 with
//    DL zeroed at entry), so Ghidra's CONCAT return expressions are noise.
// UNSURE: the sign test is `cur_cross * prev_cross <= 0`, transcribed from
// `test ah,0x41 / jp` after the fcomp against 0.0 -- a product of exactly zero counts as a
// crossing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> direction, EBX -> samples, stack -> count, values, out_index, out_value
uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction,
                                           const real_vector3d *samples, int16_t count,
                                           const float *values, float *out_index,
                                           float *out_value)
{
    const real_vector3d *sample;
    float previous_cross;
    float cross;
    float dot;
    int16_t previous_index;
    int16_t wrapped_index;
    int16_t i;

    previous_index = (int16_t)(count - 1);
    sample = &samples[previous_index];
    previous_cross = direction->k * sample->j - sample->k * direction->j;

    if (count < 1) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        sample = &samples[i];
        cross = sample->j * direction->k - sample->k * direction->j;
        if (cross * previous_cross <= 0.0f) {
            dot = direction->i * sample->i + sample->j * direction->j + sample->k * direction->k;
            if (dot > 0.0f) {
                wrapped_index = i;
                if (i == 0) {
                    wrapped_index = count;
                }
                *out_index = ((float)previous_index * cross - (float)wrapped_index * previous_cross) /
                             (cross - previous_cross);
                *out_value = (cross * values[previous_index] - previous_cross * values[i]) /
                             (cross - previous_cross);
                return 1;
            }
        }
        previous_index = i;
        previous_cross = cross;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x419240):

uint FUN_00419240(float param_1,int param_2,float *param_3,float *param_4)

{
  int iVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  float *in_ECX;
  int unaff_EBX;
  short sVar6;
  short sVar7;
  int iVar9;
  ushort uVar10;
  int local_20;
  int iVar8;

  iVar1 = unaff_EBX + (short)((int)param_1 + -1) * 0xc;
  sVar6 = SUB42(param_1,0);
  fVar3 = in_ECX[2] * *(float *)(iVar1 + 4) - *(float *)(iVar1 + 8) * in_ECX[1];
  iVar1 = 0;
  iVar9 = (int)param_1 + -1;
  param_1 = fVar3;
  if (sVar6 < 1) {
    return (uint)fVar3 & 0xffffff00;
  }
  do {
    iVar8 = iVar1;
    sVar7 = (short)iVar8;
    pfVar2 = (float *)(unaff_EBX + sVar7 * 0xc);
    fVar3 = pfVar2[1] * in_ECX[2] - pfVar2[2] * in_ECX[1];
    fVar4 = fVar3 * param_1;
    uVar10 = (ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 | (ushort)(fVar4 == 0.0) << 0xe;
    if (fVar4 < 0.0 != (fVar4 == 0.0)) {
      fVar4 = *in_ECX * *pfVar2 + pfVar2[1] * in_ECX[1] + pfVar2[2] * in_ECX[2];
      uVar10 = (ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 | (ushort)(fVar4 == 0.0) << 0xe
      ;
      if (fVar4 >= 0.0 && (fVar4 == 0.0) == 0) {
        sVar5 = sVar7;
        if (sVar7 == 0) {
          sVar5 = sVar6;
        }
        local_20 = (int)sVar5;
        *param_3 = ((float)(int)(short)iVar9 * fVar3 - (float)local_20 * param_1) /
                   (fVar3 - param_1);
        *param_4 = (fVar3 * *(float *)(param_2 + (short)iVar9 * 4) -
                   param_1 * *(float *)(param_2 + sVar7 * 4)) / (fVar3 - param_1);
        return CONCAT31((int3)(char)((uint)iVar8 >> 8),1);
      }
    }
    iVar1 = iVar8 + 1;
    iVar9 = iVar8;
    param_1 = fVar3;
  } while ((short)(iVar8 + 1) < sVar6);
  return CONCAT22((short)((uint)(*pfVar2 * in_ECX[1] - *in_ECX * pfVar2[1]) >> 0x10),uVar10);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

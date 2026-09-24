// model_nodes_blend_transforms  (Ghidra: model_nodes_blend_transforms, already named)
// address 0x4d69e0, size 203 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (review pass: checked against objdump)
// evidence: out/phase4/models_functions.md summary ("blends two arrays of node SQT transforms
//   by a weight fraction, slerping rotations and lerping translation/scale"). VERIFIED against
//   objdump -d -M intel bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d69e0..0x4d6aaa): the
//   quaternion_lerp call drops its arguments in Ghidra's decompilation, but the disassembly
//   shows ECX = EDX = &in_out[node] (both the 'a' operand and the 'out' pointer, i.e. an
//   in-place blend), EDX = &other[node] ('b'), matching the weight order of the (fully
//   inline, unambiguous) scale/translation blend right next to it: weight on in_out, 1-weight
//   on other.
// register convention: blended-in-place orientation array in EAX (in_EAX), node count in CX
//   (in_CX, low 16 bits); the other orientation array, step and steps as the recognized stack
//   parameters (param_1, param_2, param_3).
//   // blam-cc: EAX -> in_out, CX -> node_count, stack -> other, step, steps

#include "tags.h"
#include "math.h"
#include "models.h"

extern void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t); // 0x4cdcc0
extern void quaternion_normalize(real_quaternion *q); // 0x4cdb20

// Blends `other` into `in_out` in place, node by node, by weight = (step+1)/steps: rotation is
// slerped via quaternion_lerp + normalize, translation and scale are plain lerps. Weight
// applies to in_out's own value, (1-weight) to other's.
void model_nodes_blend_transforms(real_orientation *in_out, int16_t node_count,
                                   real_orientation *other, int16_t step, int16_t steps)
{
    real weight, one_minus_weight;
    int16_t node;

    weight = (real)(step + 1) / (real)steps;
    one_minus_weight = 1.0f - weight;

    for (node = 0; node < node_count; node++) {
        in_out[node].scale = weight * in_out[node].scale + one_minus_weight * other[node].scale;

        quaternion_lerp(&in_out[node].rotation, &other[node].rotation, &in_out[node].rotation, weight);
        quaternion_normalize(&in_out[node].rotation);

        in_out[node].translation.x = weight * in_out[node].translation.x +
                                      one_minus_weight * other[node].translation.x;
        in_out[node].translation.y = weight * in_out[node].translation.y +
                                      one_minus_weight * other[node].translation.y;
        in_out[node].translation.z = weight * in_out[node].translation.z +
                                      one_minus_weight * other[node].translation.z;
    }
}

#if 0
Original Ghidra decompilation (0x4d69e0):

void model_nodes_blend_transforms(int param_1,short param_2,short param_3)

{
  float fVar1;
  float fVar2;
  int in_EAX;
  ushort in_CX;
  float *pfVar3;
  float *pfVar4;
  uint local_4;

  fVar1 = (float)(param_2 + 1) / (float)(int)param_3;
  fVar2 = 1.0 - fVar1;
  if (0 < (short)in_CX) {
    local_4 = (uint)in_CX;
    pfVar3 = (float *)(param_1 + 0x14);
    pfVar4 = (float *)(in_EAX + 0x1c);
    do {
      *pfVar4 = fVar1 * *pfVar4 + fVar2 * *(float *)((int)pfVar4 + (param_1 - in_EAX));
      quaternion_lerp(fVar1);
      quaternion_normalize();
      local_4 = local_4 - 1;
      pfVar4[-3] = fVar1 * pfVar4[-3] + fVar2 * pfVar3[-1];
      pfVar4[-2] = fVar1 * pfVar4[-2] + fVar2 * *pfVar3;
      pfVar4[-1] = fVar1 * pfVar4[-1] + fVar2 * pfVar3[1];
      pfVar3 = pfVar3 + 8;
      pfVar4 = pfVar4 + 8;
    } while (local_4 != 0);
  }
  return;
}
#endif

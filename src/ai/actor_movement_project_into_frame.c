// actor_movement_project_into_frame  (Ghidra: actor_movement_project_into_frame, renamed)
// address 0x418c20, size 189 bytes
// name confidence: 0.45  rewrite confidence: 0.85
// evidence: expresses one vector in the orthonormal frame whose first axis is the vector in
// ECX, then normalizes the result. The 3D branch asks 0x55eed0 for the two perpendicular
// axes (it is handed EBX and EDI, two 12-byte stack slots, plus the same ECX) and dots all
// three against the input; the 2D branch uses the implicit perpendicular (-j, i) and leaves
// z at zero. Its only caller is 0x4180c0, which uses the result as a steering direction in
// the actor's own frame.
// register convention: the 3D/2D selector arrives in AL, the frame axis in ECX; the input
// vector and the output vector are genuine stack parameters (objdump: mov ebp,[esp+0x20]
// and mov ecx,[esp+0x30] / [esp+0x28]).
// blam-cc: AL -> use_3d, ECX -> frame_axis, stack -> v, out
// UNSURE: 0x55eed0 is outside this module and not rewritten; the two 12-byte out-parameters
// it fills are read off this call site only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX -> v
extern void FUN_0055eed0(const real_vector3d *axis, real_vector3d *out_axis2,
                         real_vector3d *out_axis3); // 0x55eed0, ECX -> axis, EBX -> out_axis2, EDI -> out_axis3

// blam-cc: AL -> use_3d, ECX -> frame_axis, stack -> v, out
void actor_movement_project_into_frame(uint8_t use_3d, const real_vector3d *frame_axis,
                                       const real_vector3d *v, real_vector3d *out)
{
    real_vector3d axis2;
    real_vector3d axis3;

    if (use_3d != 0) {
        FUN_0055eed0(frame_axis, &axis2, &axis3);
        out->i = frame_axis->i * v->i + frame_axis->j * v->j + frame_axis->k * v->k;
        out->j = axis2.i * v->i + axis2.j * v->j + axis2.k * v->k;
        out->k = axis3.i * v->i + axis3.j * v->j + axis3.k * v->k;
        vector3d_normalize_with_length(out);
        return;
    }

    out->i = frame_axis->i * v->i + frame_axis->j * v->j;
    out->k = 0.0f;
    out->j = frame_axis->i * v->j - frame_axis->j * v->i;
    vector3d_normalize_with_length(out);
}

#if 0
Original Ghidra decompilation (0x418c20):

void FUN_00418c20(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char in_AL;
  float *in_ECX;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (in_AL != '\0') {
    FUN_0055eed0();
    *param_2 = *param_1 * *in_ECX + in_ECX[1] * param_1[1] + in_ECX[2] * param_1[2];
    param_2[1] = local_14 * param_1[1] + local_10 * param_1[2] + local_18 * *param_1;
    param_2[2] = local_8 * param_1[1] + local_4 * param_1[2] + local_c * *param_1;
    vector3d_normalize_with_length();
    return;
  }
  fVar1 = in_ECX[1];
  fVar2 = *in_ECX;
  *param_2 = in_ECX[1] * param_1[1] + *param_1 * *in_ECX;
  fVar3 = *param_1;
  fVar4 = param_1[1];
  param_2[2] = 0.0;
  param_2[1] = fVar2 * fVar4 + -fVar1 * fVar3;
  vector3d_normalize_with_length();
  return;
}
#endif

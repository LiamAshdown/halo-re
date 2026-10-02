// observer_compute_remaining_offset  (Ghidra: FUN_00448710; renamed for this rewrite)
// address 0x448710, size 364 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md's proposed name and its own citation of this
// address ("+0x260 remaining_offset: 448710"). Confirmed against objdump: EAX/ECX/EDX are plain
// parallel walks over observer_command.parameters (target), observer.parameters (current) and
// observer.remaining_offset (output); position/focus_offset/distance/field_of_view (8 floats)
// are a straight subtraction, and orientation is the relative rotation between the target and
// current forward/up bases (quaternion angle*axis, matching observer_parameter_derivatives'
// rotation comment). The matrix4x3_from_forward_up call sites both match the established
// "up in EAX, forward in ECX, out on stack" convention (src/game/player_compute_view_forward_vector.c);
// matrix4x3_inverse matches its established "out in EAX, in in ECX" convention.
// register convention: target in EAX (in_EAX), current in ECX (in_ECX), output in EDX (in_EDX);
// no stack parameters.
//
// review (phase 4 gate, line by line against objdump 0x448710..0x44887e): matches. The
// quaternion call is ECX = the relative matrix (0x44879e lea ecx,[esp+0xe0]) with the output
// pushed (0x448799), and the vector part is read from +0x0/+0x4/+0x8 with w at +0xc
// (0x4487aa..0x4487bd, 0x44881a), so the i, j, k, w order is confirmed at this call site.
// The binary wraps the orientation step in a one pass loop (counter at esp+0xc = 1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sqrt(double x);   // FSQRT
extern double fabs(double x);   // FABS
extern double atan2(double y, double x);

// blam-cc: up in EAX, forward in ECX, out on stack
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward,
    real_matrix4x3 *out); // 0x4cb970

// blam-cc: out in EAX, in in ECX
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0

extern void (*matrix4x3_multiply_ptr)(void *a, void *b, void *out); // 0x00696664

// blam-cc: ECX -> m, stack -> out
extern void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out); // 0x4cbc00, math module

// blam-cc: EAX -> target, ECX -> current, EDX -> out
// Computes target - current for the first 8 floats (position, focus_offset, distance, field of
// view), then the relative rotation between the target and current forward/up bases (as an
// axis*angle vector, angle folded into -pi..pi) for the trailing 3 floats.
void observer_compute_remaining_offset(float *target, float *current, float *out)
{
    real_matrix4x3 current_matrix, target_matrix, target_matrix_inverse, relative_matrix;
    real_quaternion relative_rotation;
    real_vector3d axis;
    float axis_length;
    float angle;
    int32_t i;

    for (i = 0; i < 8; i++) {
        out[i] = target[i] - current[i];
    }
    target += 8;   // now target->forward (3) then target->up (3)
    current += 8;  // now current->forward (3) then current->up (3)

    matrix4x3_from_forward_up((real_vector3d *)(current + 3), (real_vector3d *)current,
        &current_matrix);
    matrix4x3_from_forward_up((real_vector3d *)(target + 3), (real_vector3d *)target,
        &target_matrix);
    matrix4x3_inverse(&target_matrix_inverse, &target_matrix);
    matrix4x3_multiply_ptr(&current_matrix, &target_matrix_inverse, &relative_matrix);
    quaternion_from_matrix4x3(&relative_matrix, &relative_rotation);

    axis.i = relative_rotation.i;
    axis.j = relative_rotation.j;
    axis.k = relative_rotation.k;

    axis_length = (float)sqrt((double)(axis.j * axis.j + axis.k * axis.k + axis.i * axis.i));
    if (fabs((double)axis_length) < 9.999999747378752e-05) {
        axis_length = 0.0f;
    } else {
        float inv_length = 1.0f / axis_length;
        axis.i = inv_length * axis.i;
        axis.j = inv_length * axis.j;
        axis.k = inv_length * axis.k;
    }

    angle = (float)atan2((double)axis_length, (double)relative_rotation.w);
    angle = angle + angle;
    if (3.1415927f < angle) {
        axis.i = -axis.i;
        axis.j = -axis.j;
        axis.k = -axis.k;
        angle = 6.2831855f - angle;
    }

    out[8] = angle * axis.i;
    out[9] = angle * axis.j;
    out[10] = angle * axis.k;
}

#if 0
Original Ghidra decompilation (0x448710):

void FUN_00448710(void)

{
  float fVar1;
  float fVar2;
  float *in_EAX;
  int iVar3;
  float *in_ECX;
  float *in_EDX;
  float *pfVar4;
  float10 fVar5;
  float10 fVar6;
  int local_fc;
  float fStack_f8;
  float fStack_f4;
  float fStack_f0;
  float fStack_ec;
  undefined1 local_e8 [56];
  undefined1 local_b0 [56];
  undefined1 local_78 [56];
  undefined1 local_40 [60];

  iVar3 = 8;
  do {
    fVar1 = *in_EAX;
    pfVar4 = in_EDX + 1;
    fVar2 = *in_ECX;
    in_ECX = in_ECX + 1;
    in_EAX = in_EAX + 1;
    iVar3 = iVar3 + -1;
    *in_EDX = fVar1 - fVar2;
    in_EDX = pfVar4;
  } while (iVar3 != 0);
  local_fc = 1;
  do {
    matrix4x3_from_forward_up(local_b0);
    matrix4x3_from_forward_up(local_e8);
    matrix4x3_inverse();
    (*(code *)PTR_matrix4x3_multiply_00696664)(local_b0,local_78,local_40);
    quaternion_from_matrix4x3(&fStack_f8);
    *pfVar4 = fStack_f8;
    pfVar4[1] = fStack_f4;
    pfVar4[2] = fStack_f0;
    fVar5 = SQRT((float10)pfVar4[1] * (float10)pfVar4[1] +
                 (float10)pfVar4[2] * (float10)pfVar4[2] + (float10)*pfVar4 * (float10)*pfVar4);
    if (ABS(fVar5) < (float10)9.999999747378752e-05) {
      fVar5 = (float10)0.0;
    }
    else {
      fVar6 = (float10)1.0 / fVar5;
      *pfVar4 = (float)(fVar6 * (float10)*pfVar4);
      pfVar4[1] = (float)(fVar6 * (float10)pfVar4[1]);
      pfVar4[2] = (float)(fVar6 * (float10)pfVar4[2]);
    }
    fVar5 = (float10)fpatan(fVar5,(float10)fStack_ec);
    fVar5 = fVar5 + fVar5;
    if ((float10)3.1415927 < fVar5) {
      *pfVar4 = -*pfVar4;
      pfVar4[1] = -pfVar4[1];
      pfVar4[2] = -pfVar4[2];
      fVar5 = (float10)6.2831855 - fVar5;
    }
    *pfVar4 = (float)(fVar5 * (float10)*pfVar4);
    local_fc = local_fc + -1;
    pfVar4[1] = (float)(fVar5 * (float10)pfVar4[1]);
    pfVar4[2] = (float)(fVar5 * (float10)pfVar4[2]);
    pfVar4 = pfVar4 + 3;
  } while (local_fc != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

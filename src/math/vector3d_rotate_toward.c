// vector3d_rotate_toward  (Ghidra: FUN_004cd950; renamed, no established name)
// address 0x4cd950, size 217 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: math_functions.md: "Rotates a source direction (ESI) toward a target direction
//   (ECX) by at most the angle given by sin/cos params, returning whether a rotation was
//   needed." cos_threshold doubles as both the "already close enough" dot-product test and the
//   fixed cosine handed to vector3d_rotate_about_axis: this never partially rotates, it either
//   snaps straight to target (dot already >= cos_threshold) or rotates by exactly the given
//   angle toward it, using axis = target x source (or, when that is degenerate, an arbitrary
//   perpendicular to target).
// register convention: target pointer in ECX (in_ECX), source pointer in ESI (unaff_ESI),
//   output pointer in EDI (unaff_EDI); sin/cos as the recognized stack parameters (param_1,
//   param_2).
//   // blam-cc: ECX -> target, ESI -> source, EDI -> out, stack -> (sin_angle, cos_angle)
//
// UNSURE: the two calls with no visible arguments (vector3d_build_perpendicular and the second
// vector3d_normalize_with_length, plus the final vector3d_rotate_about_axis) are reconstructed
// from their own established signatures and the only pointers this function has in scope; the
// exact register plumbing at those call sites could not be read back from the decompile.

// RETURN WIDTH (verified in the disassembly): the success/failure result is written with
// `mov al,1` / `xor al,al` and never zero-extended, so only AL carries the result and the
// return type is a byte, not an int. Declared uint8_t below; reading it as a 32-bit value
// would pick up whatever the upper 24 bits of EAX happened to hold.

#include "tags.h"
#include "math.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820

// Returns 1 and writes a direction rotated (by exactly sin_angle/cos_angle) from source toward
// target into *out, or returns 0 and copies target straight into *out when source is already
// within cos_angle of target.
uint8_t vector3d_rotate_toward(real_vector3d *target, real_vector3d *source, real_vector3d *out, real sin_angle, real cos_angle)
{
    real_vector3d axis;
    real length;

    if (cos_angle <= source->k * target->k + target->i * source->i + target->j * source->j) {
        *out = *target;
        return 0;
    }

    axis.i = target->k * source->j - target->j * source->k;
    axis.j = target->i * source->k - source->i * target->k;
    axis.k = target->j * source->i - target->i * source->j;

    length = vector3d_normalize_with_length(&axis);
    if (length == 0.0f) {
        vector3d_build_perpendicular(&axis, target);
        vector3d_normalize_with_length(&axis);
    }

    *out = *source;
    vector3d_rotate_about_axis(out, &axis, sin_angle, cos_angle);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4cd950):

undefined4 FUN_004cd950(undefined4 param_1,float param_2)

{
  float *in_ECX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar1;
  float fVar2;
  float fVar3;
  float fVar4;

  if (param_2 <= unaff_ESI[2] * in_ECX[2] + *in_ECX * *unaff_ESI + in_ECX[1] * unaff_ESI[1]) {
    *unaff_EDI = *in_ECX;
    unaff_EDI[1] = in_ECX[1];
    unaff_EDI[2] = in_ECX[2];
    return 0;
  }
  fVar2 = in_ECX[2] * unaff_ESI[1] - in_ECX[1] * unaff_ESI[2];
  fVar3 = *in_ECX * unaff_ESI[2] - *unaff_ESI * in_ECX[2];
  fVar4 = in_ECX[1] * *unaff_ESI - *in_ECX * unaff_ESI[1];
  fVar1 = (float10)vector3d_normalize_with_length(fVar2,fVar3,fVar4);
  if ((float10)0.0 == fVar1) {
    vector3d_build_perpendicular();
    vector3d_normalize_with_length(fVar2,fVar3,fVar4);
  }
  *unaff_EDI = *unaff_ESI;
  unaff_EDI[1] = unaff_ESI[1];
  unaff_EDI[2] = unaff_ESI[2];
  vector3d_rotate_about_axis(param_1,param_2);
  return 1;
}
#endif

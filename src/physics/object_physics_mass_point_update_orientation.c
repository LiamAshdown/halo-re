// object_physics_mass_point_update_orientation  (Ghidra: FUN_005096f0, still unnamed there;
//   renamed per out/phase4/physics_types_notes.md section 5's general guidance for this whole
//   call family: "object_physics_* / mass_point_*", since the fallback values this function is
//   handed are &object->forward/&object->up, not an antenna widget)
// address 0x5096f0, size 227 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN 2026-09-27 static loop from objdump 0x5096f0..0x5097d2: EAX axis, EDI/ESI outputs, stack source forward/up; forward renormalized)
// evidence: out/phase4/physics_functions.md summary ("Rotates and re-orthonormalizes a vertex's
//   forward/twist orientation vectors by a small axis-angle rotation each tick"); its one call
//   site (out/phase2/physics/01.md, inside 0x5097e0/object_physics_integrate_and_test_at_rest)
//   is `FUN_005096f0(iVar5 + 0x74, iVar5 + 0x80)`, and types/objects.h places object.forward at
//   +0x74 and object.up at +0x80, which fixes param_1/param_2 as the fallback forward/up pair
//   used when the rotation axis is zero-length; unit_update_recoil_decay.c (src/units) rewrites
//   the identical axis-angle idiom (copy axis to a local, normalize for length+direction,
//   matrix4x3_from_axis_angle(sin, cos), matrix4x3_transform_vector) with the same callee
//   signatures reused verbatim here.
// register convention: in_EAX -> axis (real_vector3d *, the un-normalized rotation axis for
//   this tick, read-only), unaff_ESI -> up (real_vector3d *, in/out), unaff_EDI -> forward
//   (real_vector3d *, in/out). param_1/param_2 are Ghidra's own recognized stack parameters
//   (fallback_forward, fallback_up), used only when axis normalizes to zero length.
//   // blam-cc: EAX -> axis, ESI -> up, EDI -> forward, stack -> fallback_forward, fallback_up
// UNSURE: the two matrix4x3_transform_vector calls show only one recovered argument each
//   (the rotation matrix); which of out/v/m landed in which register is not visible in this
//   function's own decompile. Read as rotating forward and up IN PLACE by the axis-angle
//   matrix, mirroring unit_update_recoil_decay.c's out/v/m usage for the identical idiom.

#include "tags.h"
#include "math.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, ECX
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
                                       real cos_angle); // 0x4cb880
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cbe50
extern double sin(double x);
extern double cos(double x);

// Rotates forward and up in place by the small rotation axis gives this tick (treating the
// axis's length as the rotation angle in radians), then re-orthonormalizes up against forward
// (Gram-Schmidt: subtract up's projection onto forward, renormalize) to correct drift.
// When axis normalizes to zero length (no rotation this tick), forward/up are instead reset
// to fallback_forward/fallback_up unchanged.
void object_physics_mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward,
                                                   real_vector3d *fallback_forward, real_vector3d *fallback_up)
{
    real_vector3d local_axis = *axis;
    real length = vector3d_normalize_with_length(&local_axis);

    if (length != 0.0f) {
        // 0x509728..0x50976c (REWRITTEN 2026-09-27): the rotation is applied to the SOURCE vectors (stack arguments:
        // fallback_forward -> EDI output, fallback_up -> ESI output), and the rotated forward is renormalized
        // (0x509760) before up is orthogonalized against it. The draft rotated the outputs in place and skipped the
        // forward renormalization.
        real_matrix4x3 rotation;
        matrix4x3_from_axis_angle(&rotation, &local_axis, (real)sin((double)length), (real)cos((double)length));
        matrix4x3_transform_vector(forward, fallback_forward, &rotation);
        matrix4x3_transform_vector(up, fallback_up, &rotation);
        vector3d_normalize_with_length(forward);

        real neg_dot = -(forward->k * up->k + up->j * forward->j + forward->i * up->i);
        up->i = neg_dot * forward->i + up->i;
        up->j = neg_dot * forward->j + up->j;
        up->k = neg_dot * forward->k + up->k;
        vector3d_normalize_with_length(up);
        return;
    }

    *forward = *fallback_forward;
    *up = *fallback_up;
}

#if 0
Original Ghidra decompilation (0x5096f0):

void FUN_005096f0(float *param_1,float *param_2)

{
  float fVar1;
  undefined4 *in_EAX;
  undefined4 uVar2;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar3;
  float10 fVar4;
  undefined1 local_4c [60];
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  local_10 = *in_EAX;
  local_c = in_EAX[1];
  local_8 = in_EAX[2];
  fVar3 = (float10)vector3d_normalize_with_length();
  if (fVar3 != (float10)0.0) {
    fVar4 = (float10)fcos(fVar3);
    fVar3 = (float10)fsin(fVar3);
    uVar2 = matrix4x3_from_axis_angle((float)fVar3,(float)fVar4);
    matrix4x3_transform_vector(uVar2);
    matrix4x3_transform_vector(local_4c);
    vector3d_normalize_with_length();
    fVar1 = -(*unaff_EDI * *unaff_ESI + unaff_ESI[1] * unaff_EDI[1] + unaff_EDI[2] * unaff_ESI[2]);
    *unaff_ESI = fVar1 * *unaff_EDI + *unaff_ESI;
    unaff_ESI[1] = fVar1 * unaff_EDI[1] + unaff_ESI[1];
    unaff_ESI[2] = fVar1 * unaff_EDI[2] + unaff_ESI[2];
    vector3d_normalize_with_length();
    return;
  }
  *unaff_EDI = *param_1;
  unaff_EDI[1] = param_1[1];
  unaff_EDI[2] = param_1[2];
  *unaff_ESI = *param_2;
  unaff_ESI[1] = param_2[1];
  unaff_ESI[2] = param_2[2];
  return;
}
#endif

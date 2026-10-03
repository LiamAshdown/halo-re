// vector3d_rotate_basis_by_axis_angle  (Ghidra: FUN_00448880; renamed for this rewrite)
// address 0x448880, size 116 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/camera.h's observer_parameter_derivatives.rotation comment ("0x448880 applies
// it with two vector3d_rotate_about_axis calls"); confirmed against objdump: copies the incoming
// axis*angle vector to a local scratch, normalises it (the length is the rotation angle), and if
// nonzero rotates forward and up about that axis by sin/cos of the angle, using the established
// vector3d_rotate_about_axis convention (EAX -> v, ECX -> axis).
// register convention: axis_angle vector in EAX (in_EAX); forward and up on the stack (Ghidra
// would have recognized these as param_1, param_2 had it modeled the calling convention).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern double fsin(double angle); // FSIN
extern double fcos(double angle); // FCOS

// blam-cc: EAX -> v, ECX -> axis (0x4cd820)
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis,
    real sin_angle, real cos_angle);

// blam-cc: EAX -> axis_angle; forward, up = stack
// Rotates forward and up about axis_angle's direction by axis_angle's length (in radians).
// Does nothing if axis_angle is (near) zero.
void vector3d_rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward, Vector3D *up)
{
    real_vector3d axis = *(real_vector3d *)axis_angle;
    real angle;
    real sin_angle, cos_angle;

    angle = vector3d_normalize_with_length(&axis);
    if (angle != 0.0f) {
        sin_angle = (real)fsin((double)angle);
        cos_angle = (real)fcos((double)angle);
        vector3d_rotate_about_axis((real_vector3d *)forward, &axis, sin_angle, cos_angle);
        vector3d_rotate_about_axis((real_vector3d *)up, &axis, sin_angle, cos_angle);
    }
}

#if 0
Original Ghidra decompilation (0x448880):

void FUN_00448880(void)

{
  undefined4 extraout_EDX;
  float10 fVar1;
  float10 fVar2;

  fVar1 = (float10)vector3d_normalize_with_length();
  if (fVar1 != (float10)0.0) {
    fVar2 = (float10)fsin(fVar1);
    fVar1 = (float10)fcos(fVar1);
    vector3d_rotate_about_axis((float)fVar2,(float)fVar1);
    vector3d_rotate_about_axis((float)fVar2,extraout_EDX);
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// unit_rotate_basis_about_axis  (Ghidra: unit_rotate_basis_about_axis, renamed)
// address 0x55e6b0, size 392 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN from objdump 0x55e6b0..0x55e837: EAX = object only; the axis is
//   the angular velocity (+0x8c), both rotations take (EAX vector, ECX axis, sin, cos) and the rebuilt up is
//   normalised)
// evidence: object.forward/up at 0x074/0x080 (objects.h); global_forward3d/global_up3d indirect
//   pointers 0x00696718/0x00696720 (see the ground-adjust cluster's notes); matches
//   functions.md's summary, "Rotates a unit's stored forward and right basis vectors about a
//   common axis, keeping them orthonormal."
// UNSURE: every vector3d_normalize_with_length / vector3d_rotate_about_axis call here is
//   register-only in Ghidra's decompile (no visible arguments at all, not even the partial
//   binding this module's other callers show); this rewrite reproduces the exact sequence of
//   float reads/writes and calls without claiming to know which vector or axis each one
//   actually operates on. rotation_axis is a guess at the "common axis" functions.md describes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;             // 0x008603b0
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_vector3d *global_up3d_pointer;      // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v);    // 0x401990
// vector3d_rotate_about_axis (0x4cd820) rotates the vector in EAX about the axis in ECX in
// place, by the (sin_angle, cos_angle) pair pushed on the stack -- the callee own
// decompilation is a Rodrigues formula over in_EAX / in_ECX / param_1 / param_2, and
// src/math/vector3d_rotate_toward.c reads it the same way. Ghidra binds only the two stack
// arguments at the call sites below, so the declaration is left unprototyped.
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
  // real signature (vector3d_rotate_about_axis.c): void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 0 of 4 args at this call site
extern double fcos(double x);
extern double fsin(double x);

// UNSURE (see file header): rotates the object's forward and up vectors by an angle derived
// from normalizing rotation_axis, then re-derives up as forward crossed with a rotated copy of
// itself to keep the basis orthonormal, falling back to global_forward3d/global_up3d if the
// result degenerates.
void unit_rotate_basis_about_axis(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    real_vector3d axis = obj->angular_velocity;
    float angle = vector3d_normalize_with_length(&axis);
    float angle_cos = (float)fcos(angle);
    float angle_sin = (float)fsin(angle);
    real_vector3d up;
    real_vector3d right;

    vector3d_rotate_about_axis(&obj->forward, &axis, angle_sin, angle_cos);
    vector3d_normalize_with_length(&obj->forward);
    up = obj->up;
    vector3d_rotate_about_axis(&up, &axis, angle_sin, angle_cos);
    right.i = up.k * obj->forward.j - up.j * obj->forward.k;
    right.j = up.i * obj->forward.k - up.k * obj->forward.i;
    right.k = up.j * obj->forward.i - up.i * obj->forward.j;
    obj->up.i = right.j * obj->forward.k - right.k * obj->forward.j;
    obj->up.j = right.k * obj->forward.i - right.i * obj->forward.k;
    obj->up.k = right.i * obj->forward.j - right.j * obj->forward.i;
    if (vector3d_normalize_with_length(&obj->up) == 0.0f) {
        obj->forward = *global_forward3d_pointer;
        obj->up = *global_up3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x55e6b0):

void FUN_0055e6b0(void)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  undefined *puVar7;
  undefined *puVar8;
  uint in_EAX;
  float *pfVar9;
  unkbyte10 Var10;
  float10 fVar11;
  float10 fVar12;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  Var10 = vector3d_normalize_with_length();
  fVar11 = (float10)fcos(Var10);
  pfVar1 = (float *)(iVar2 + 0x74);
  fVar12 = (float10)fsin(Var10);
  vector3d_rotate_about_axis((float)fVar12,(float)fVar11);
  vector3d_normalize_with_length();
  pfVar9 = (float *)(iVar2 + 0x80);
  fVar3 = *pfVar9;
  fVar4 = *(float *)(iVar2 + 0x84);
  fVar5 = *(float *)(iVar2 + 0x88);
  vector3d_rotate_about_axis((float)fVar12,(float)fVar11);
  fVar6 = fVar5 * *(float *)(iVar2 + 0x78) - fVar4 * *(float *)(iVar2 + 0x7c);
  fVar5 = fVar3 * *(float *)(iVar2 + 0x7c) - fVar5 * *pfVar1;
  fVar3 = fVar4 * *pfVar1 - fVar3 * *(float *)(iVar2 + 0x78);
  *pfVar9 = fVar5 * *(float *)(iVar2 + 0x7c) - fVar3 * *(float *)(iVar2 + 0x78);
  *(float *)(iVar2 + 0x84) = fVar3 * *pfVar1 - fVar6 * *(float *)(iVar2 + 0x7c);
  *(float *)(iVar2 + 0x88) = fVar6 * *(float *)(iVar2 + 0x78) - fVar5 * *pfVar1;
  fVar11 = (float10)vector3d_normalize_with_length();
  puVar7 = PTR_DAT_00696718;
  if ((float10)0.0 == fVar11) {
    *pfVar1 = *(float *)PTR_DAT_00696718;
    puVar8 = PTR_DAT_00696720;
    *(undefined4 *)(iVar2 + 0x78) = *(undefined4 *)(puVar7 + 4);
    *(undefined4 *)(iVar2 + 0x7c) = *(undefined4 *)(puVar7 + 8);
    *pfVar9 = *(float *)puVar8;
    *(undefined4 *)(iVar2 + 0x84) = *(undefined4 *)(puVar8 + 4);
    *(undefined4 *)(iVar2 + 0x88) = *(undefined4 *)(puVar8 + 8);
  }
  return;
}
#endif

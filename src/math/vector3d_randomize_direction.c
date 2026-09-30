// vector3d_randomize_direction  (Ghidra: FUN_004cd1b0; renamed, no established name)
// address 0x4cd1b0, size 289 bytes
// name confidence: 0.75   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md: "Helper that derives a randomly rotated direction
//   (used for weapon/effect dispersion) from a sphere sample point and a random angle." The
//   disassembly confirms exactly that: one draw picks an entry of the quasi-uniform
//   sphere_point_table, its cross product with the input direction gives a rotation axis
//   perpendicular to that direction, and a second draw picks the rotation angle in [lo, hi).
// register convention: input direction in EAX, output vector in EBX (also the return value,
//   mov eax,ebx at 0x4cd2ca), seed pointer in EDI; the angle range (lo, hi) on the stack.
//   // blam-cc: EAX -> direction, EBX -> out, EDI -> seed, stack -> (lo, hi)
//
// VERIFIED against the disassembly at 0x4cd1b0 (objdump -d -M intel). Ghidra loses the entire
// middle of this function because the cross product and the normalize both work on an anonymous
// stack temporary reached through ECX:
//   0x4cd1b6  *ebx = *eax                              the output starts as a copy of the input
//   0x4cd1c8  seed advance #1, then
//             movsx ecx,WORD ds:0x6b7af8               sphere_point_table_count
//             index = ((seed >> 16) * count) >> 16
//             mov ecx,ds:0x6b7af4 / lea ecx,[ecx+index*12]   &sphere_point_table[index]
//   0x4cd20d  [esp+0x14..0x1c] = direction x sample, then copied down to [esp+0x8..0x10]
//   0x4cd25f  lea ecx,[esp+0x8] / call 0x401990        normalize THAT temporary, keep the length
//   0x4cd268  fcomp ds:0x672ac0 (0.0)                  bail out unless the length is > 0
//   0x4cd275  seed advance #2, angle = lo + (seed >> 16) * (1/65536) * (hi - lo)
//   0x4cd2a5  lea ecx,[esp+0x10] (the normalized axis) / mov eax,ebx (the output vector)
//   0x4cd2b7  fcos / fsin / call 0x4cd820              vector3d_rotate_about_axis(out, axis, sin, cos)
// An earlier reading had the rotation axis being the output vector itself, which is a geometric
// no-op (a vector has no component perpendicular to an axis parallel to itself) and was flagged
// as such; the axis is in fact direction x sample, which is perpendicular to the direction by
// construction. That is what makes this a cone-spread sampler.
//
// The `test eax,eax / fadd ds:0x672bc0` at 0x4cd286 is the compiler's standard unsigned-to-float
// fixup (0x672bc0 is 4294967296.0); `seed >> 16` never exceeds 0xffff, so that branch is dead.
// Ghidra says so too ("Removing unreachable block (ram,0x004cd292)").

#include "tags.h"
#include "math.h"
#include "fn_math.h"


extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, v in EAX, axis in ECX

// fsin/fcos are single x87 instructions in the original code; declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double cos(double x);
extern double sin(double x);

extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026

// Deviates `direction` by a random angle in [lo, hi) about a random axis perpendicular to it,
// writing the result to `out` and returning `out`.
real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
                                            random_seed *seed, real lo, real hi)
{
    real_vector3d axis;
    real_point3d *sample;
    int16_t index;
    real length;
    real angle;

    out->i = direction->x;
    out->j = direction->y;
    out->k = direction->z;

    *seed = *seed * k_random_multiplier + k_random_increment;
    index = (int16_t)(((*seed >> k_random_value_shift) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);
    sample = &sphere_point_table[index];

    axis.i = sample->z * direction->y - sample->y * direction->z;
    axis.j = sample->x * direction->z - sample->z * direction->x;
    axis.k = sample->y * direction->x - sample->x * direction->y;

    length = vector3d_normalize_with_length(&axis);
    if (0.0f < length) {
        *seed = *seed * k_random_multiplier + k_random_increment;
        angle = (real)(*seed >> k_random_value_shift) * 1.5259022e-05f * (hi - lo) + lo;
        vector3d_rotate_about_axis(out, &axis, (real)sin((double)angle), (real)cos((double)angle));
    }
    return out;
}

#if 0
Original Ghidra decompilation (0x4cd1b0):

/* WARNING: Removing unreachable block (ram,0x004cd292) */

void FUN_004cd1b0(float param_1,float param_2)

{
  undefined4 *in_EAX;
  undefined4 *unaff_EBX;
  uint uVar1;
  uint *unaff_EDI;
  float10 fVar2;
  float10 fVar3;

  *unaff_EBX = *in_EAX;
  unaff_EBX[1] = in_EAX[1];
  unaff_EBX[2] = in_EAX[2];
  uVar1 = *unaff_EDI * 0x19660d + 0x3c6ef35f;
  *unaff_EDI = uVar1;
  fVar2 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 < fVar2) {
    uVar1 = uVar1 * 0x19660d + 0x3c6ef35f;
    *unaff_EDI = uVar1;
    fVar2 = ((float10)param_2 - (float10)param_1) *
            (float10)(uVar1 >> 0x10) * (float10)1.5259022e-05 + (float10)param_1;
    fVar3 = (float10)fcos(fVar2);
    fVar2 = (float10)fsin(fVar2);
    vector3d_rotate_about_axis((float)fVar2,(float)fVar3);
  }
  return;
}
#endif

// projectile_compute_rotation  (Ghidra: FUN_004c0180; renamed per
// out/phase4/projectiles_types_notes.md: "byte-for-byte analogue of item_compute_rotation
// (0x4bd500) on projectile 0x264..0x278")
// address 0x4c0180, size 194 bytes
// VERIFIED against disassembly 0x4c0180..0x4c0242 (2026-09-30)
// name confidence: 0.8   rewrite confidence: 0.9 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/projectiles.h projectile_data.rotation_axis/rotation_sine/rotation_cosine
//   (0x264/0x270/0x274), projectile_flags._projectile_rotation_valid_bit (0x01);
//   types/objects.h object.angular_velocity (0x08c); global 0x008603b0 object_data. Direct
//   sibling of src/items/item_compute_rotation.c, minus that function's "object at rest"
//   (object flags 0x20) gate around the axis write -- Ghidra shows no such test here, so it is
//   not reproduced.
// register convention: object index in EAX (in_EAX).
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern double sqrt(double x); // a single x87 FSQRT instruction, see src/math/quaternion_normalize.c
extern double fsin(double x);
extern double fcos(double x);

// Recomputes a projectile's rotation_axis / rotation_sine / rotation_cosine from its current
// angular velocity. Unlike item_compute_rotation, this one always refreshes the axis when the
// angular velocity is nonzero, regardless of any "at rest" flag.
void projectile_compute_rotation(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real magnitude = (real)sqrt((double)obj->angular_velocity.k * (double)obj->angular_velocity.k +
                                 (double)obj->angular_velocity.j * (double)obj->angular_velocity.j +
                                 (double)obj->angular_velocity.i * (double)obj->angular_velocity.i);

    if (magnitude != 0.0f) {
        real inverse = 1.0f / magnitude;
        proj->flags |= _projectile_rotation_valid_bit;
        proj->rotation_axis.i = inverse * obj->angular_velocity.i;
        proj->rotation_axis.j = inverse * obj->angular_velocity.j;
        proj->rotation_axis.k = inverse * obj->angular_velocity.k;
        proj->rotation_sine = (real)fsin((double)magnitude);
        proj->rotation_cosine = (real)fcos((double)magnitude);
        return;
    }
    proj->flags &= ~_projectile_rotation_valid_bit;
    proj->rotation_sine = 0.0f;
    proj->rotation_cosine = 1.0f;
}

#if 0
Original Ghidra decompilation (0x4c0180):

void FUN_004c0180(void)

{
  int iVar1;
  uint in_EAX;
  float10 fVar2;
  float10 fVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar2 = SQRT((float10)*(float *)(iVar1 + 0x94) * (float10)*(float *)(iVar1 + 0x94) +
               (float10)*(float *)(iVar1 + 0x90) * (float10)*(float *)(iVar1 + 0x90) +
               (float10)*(float *)(iVar1 + 0x8c) * (float10)*(float *)(iVar1 + 0x8c));
  if (fVar2 != (float10)0.0) {
    fVar3 = (float10)1.0 / fVar2;
    *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) | 1;
    *(float *)(iVar1 + 0x264) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x8c));
    *(float *)(iVar1 + 0x268) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x90));
    *(float *)(iVar1 + 0x26c) = (float)(fVar3 * (float10)*(float *)(iVar1 + 0x94));
    fVar3 = (float10)fsin(fVar2);
    *(float *)(iVar1 + 0x270) = (float)fVar3;
    fVar2 = (float10)fcos(fVar2);
    *(float *)(iVar1 + 0x274) = (float)fVar2;
    return;
  }
  *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xfffffffe;
  *(undefined4 *)(iVar1 + 0x270) = 0;
  *(undefined4 *)(iVar1 + 0x274) = 0x3f800000;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

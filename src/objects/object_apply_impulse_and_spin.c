// object_apply_impulse_and_spin  (Ghidra: FUN_004bef80; renamed per
// out/phase4/projectiles_types_notes.md "0x4bef80 owns nothing: adds a velocity delta plus a
// random spin (from the sphere_point_table at 0x006b7af4) to an unparented object, calls
// 0x4c0180 and clears object flag 0x20. Only object fields. It is the detach counterpart of the
// attach responses, which set that flag.")
// address 0x4bef80, size 316 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: src/projectiles/README.md "Misattributed functions": "a generic
//   object_apply_impulse_and_spin-shaped helper: touches only object.velocity,
//   object.angular_velocity and object flag 0x20, plus 0x4c0180. Owns nothing in
//   projectile_data"; types/objects.h object_flags._object_at_rest_bit comment: "both attach
//   paths set it and 0x4bef80 (impulse and spin) clears it (0x4bf0af)" -- the objects module's
//   own type recovery already names and cross-references this function. Its two callers,
//   src/objects/object_damage_notify_and_impulse.c (0x4efcf0) and the units module's
//   unit_release_thrown_grenade (0x56e440), are both in this module's or a sibling module's
//   address range, neither in projectiles or items; picked up here in objects. Confirmed by
//   `objdump -d -M intel --start-address=0x4bef80 --stop-address=0x4bf0c0 bin/halo.exe`.
// register convention: object index in EAX (in_EAX, unchanged for the whole function and
//   forwarded to projectile_compute_rotation's own EAX object_index argument at the call),
//   velocity delta pointer in EDX (in_EDX, a real_vector3d *). No stack arguments.
//   // blam-cc: EAX -> object_index, EDX -> delta_velocity
// UNSURE: this function calls projectile_compute_rotation (0x4c0180), which recomputes
//   projectile_data.rotation_axis/sine/cosine (projectile-only tag offsets) from the object's
//   angular velocity. Since this function itself touches only generic object fields, the call
//   is preserved exactly as the disassembly shows it (same object_index); it is presumably safe
//   in practice because both of this function's callers act on objects that are, or can be,
//   projectiles (a thrown grenade, or an object taking damage that happens to be one), but that
//   is not independently confirmed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;       // 0x008603b0
extern random_seed random_seed_global; // 0x00719cd0
extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors (types/math.h)
extern int16_t sphere_point_table_count;        // 0x006b7af8
extern void projectile_compute_rotation(uint32_t object_index); // 0x4c0180, projectiles module
extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

// Adds *delta_velocity to the object's velocity and a random spin (magnitude proportional to
// |delta_velocity|, direction drawn from the sphere_point_table) to its angular velocity, then
// refreshes its rotation state and clears the "at rest" flag -- the detach counterpart of the
// attach responses (projectile_attach_apply, projectile_response), which set that flag. Only
// runs when the object has no parent; a parented object is left untouched.
void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint32_t draw;
    int16_t index;
    real_point3d sample;
    real magnitude, spin_scale;

    if (obj->parent_object != k_datum_index_none) {
        return;
    }

    obj->velocity.i = delta_velocity->i + obj->velocity.i;
    obj->velocity.j = obj->velocity.j + delta_velocity->j;
    obj->velocity.k = obj->velocity.k + delta_velocity->k;

    draw = random_seed_global * k_random_multiplier + k_random_increment;
    index = (int16_t)(((draw >> k_random_value_shift) * sphere_point_table_count) >> 16);
    random_seed_global = draw;
    sample = sphere_point_table[index];
    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;

    magnitude = (real)sqrt((double)(delta_velocity->j * delta_velocity->j +
        delta_velocity->k * delta_velocity->k + delta_velocity->i * delta_velocity->i)); // x87 order 0x4bf02e..0x4bf043
    spin_scale = (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f * magnitude * 1.5707964f;

    obj->angular_velocity.i = sample.x * spin_scale + obj->angular_velocity.i;
    obj->angular_velocity.j = sample.y * spin_scale + obj->angular_velocity.j;
    obj->angular_velocity.k = sample.z * spin_scale + obj->angular_velocity.k;

    projectile_compute_rotation(object_index);
    obj->flags = obj->flags & ~(uint32_t)_object_at_rest_bit;
}

#if 0
Original Ghidra decompilation (0x4bef80):

void FUN_004bef80(void)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  uint in_EAX;
  uint uVar6;
  float *in_EDX;
  int iVar7;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(int *)(iVar2 + 0x11c) == -1) {
    iVar7 = (int)DAT_006b7af8;
    *(float *)(iVar2 + 0x68) = *in_EDX + *(float *)(iVar2 + 0x68);
    *(float *)(iVar2 + 0x6c) = *(float *)(iVar2 + 0x6c) + in_EDX[1];
    *(float *)(iVar2 + 0x70) = *(float *)(iVar2 + 0x70) + in_EDX[2];
    uVar6 = random_seed_global * 0x19660d + 0x3c6ef35f;
    pfVar1 = (float *)(DAT_006b7af4 + (short)((uVar6 >> 0x10) * iVar7 >> 0x10) * 0xc);
    fVar3 = pfVar1[1];
    fVar4 = pfVar1[2];
    random_seed_global = uVar6 * 0x19660d + 0x3c6ef35f;
    fVar5 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 *
            SQRT(*in_EDX * *in_EDX + in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1]) * 1.5707964;
    *(float *)(iVar2 + 0x8c) = *pfVar1 * fVar5 + *(float *)(iVar2 + 0x8c);
    *(float *)(iVar2 + 0x90) = fVar3 * fVar5 + *(float *)(iVar2 + 0x90);
    *(float *)(iVar2 + 0x94) = fVar4 * fVar5 + *(float *)(iVar2 + 0x94);
    projectile_compute_rotation();
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) & 0xffffffdf;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// particle_creation_physics_jet  (Ghidra: no function created; the phase-4 types agent carved a
//   stub name "missed_455610" from the .rdata dispatch-table evidence)
// address 0x455610, size 301 bytes
// VERIFIED against disassembly 0x455610..0x45573d (2026-09-30)
// name confidence 0.55, rewrite confidence 0.4 (see the UNSURE note)
// evidence: out/phase4/effects_types_notes.md section 2: the three-entry `.rdata` table at
//   0x00657444 lists 0x455610 as entry [2], "particle creation physics, jet". Same
//   ParticleSystemType.physics_constants.pointer lookup as particle_creation_physics_explosion.c
//   (this batch); the random draw is again effect_random_direction_from_table.c's own body
//   (0x00719cd4 LCG then the 0x006b7af4/0x006b7af8 table), inlined here because its result is
//   blended per axis with marker->node_transform.forward rather than used as a plain unit
//   vector, so it cannot be swapped for a call to that helper. marker->node_transform.forward
//   (types/objects.h object_marker +0x38, a real_matrix4x3 whose .forward sub-field, math.h
//   +0x04, lands on marker+0x3c/0x40/0x44) matches the three reads at param_4+0x3c/0x40/0x44.
//   The final vector3d_cross_product call's hidden EAX/ECX arguments (EAX->out, ECX->a, stack->b
//   -- src/math/vector3d_cross_product.c) were resolved from objdump 0x455610..0x45573c: both branches load EAX from
//   `lea eax,[edx+0x34]` (edx = particle) unconditionally before the branch, i.e. &particle->
//   direction; the `fVar2 != 0.0` branch loads ECX from the global at 0x00696720
//   (types/math.h global_up3d_pointer) after pushing the OLD ecx (&particle->unknown_28, set
//   earlier by `lea ecx,[edx+0x28]`) as the stack argument, while the `fVar2 == 0.0` branch skips
//   that reload entirely and keeps ECX at that same &particle->unknown_28.
// FIXED 2026-09-30: the cross-product operands were swapped (a = ECX, b = stack): the original computes
//   direction = up x velocity (k2 != 0) or velocity x forward (k2 == 0).
// register convention: identical to every other entry of this dispatch table (system, type_index,
//   particle, marker); blam-cc: system, type_index, particle, marker.
// UNSURE: the exact roles of the three physics_constants values (k0/k1/k2 by position only,
//   matching particle_creation_physics_explosion.c's naming) are not established: k0 and k1
//   together blend a random table direction against the marker's forward axis (k1 weights the
//   random component, 1-k1 the forward component, both scaled by k0/30 -- a per-tick fraction of
//   a per-second constant) into unknown_28, and k2 selects which vector the final cross product
//   is measured against (world up when nonzero, the marker's forward axis when zero).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_up3d_pointer; // 0x00696720
extern random_seed effect_random_seed;    // 0x00719cd4
extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, foreign (math);
    // blam-cc: EAX -> out, ECX -> a, stack -> b (see src/math/vector3d_cross_product.c); 0x455717 / 0x45572d

// ParticleSystem.particle_creation_physics dispatch table entry 2, "jet". Blends a random sphere-
// table direction with the marker's forward axis (weighted by the type's physics constants) into
// unknown_28, spawns the particle at the marker's position, and derives its sprite direction as
// the cross product of unknown_28 with either world up or the marker's forward axis, depending
// on whether the third physics constant is nonzero.
void particle_creation_physics_jet(particle_system *system, int32_t type_index,
    particle_system_particle *particle, object_marker *marker)
{
    ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
        (*(uint8_t **)((uint8_t *)tag_instances[system->definition_index & 0xffff].data + 0x60)) +
        (int32_t)type_index * 0x80);
    float *physics_constants = *(float **)&((struct ParticleSystemType *)particle_type)->physics_constants.pointer;
    float k0 = physics_constants[0];
    float k1 = physics_constants[1];
    float k2 = physics_constants[2];
    float random_weight = k1 * k0 * 0.033333335f;
    float forward_weight = (1.0f - k1) * k0 * 0.033333335f;
    int16_t table_index;

    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    table_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
        (uint32_t)(int32_t)sphere_point_table_count) >> 16);

    particle->velocity.x = sphere_point_table[table_index].x * random_weight +
        forward_weight * marker->node_transform.forward.i + system->velocity.i;
    particle->velocity.y = sphere_point_table[table_index].y * random_weight +
        forward_weight * marker->node_transform.forward.j + system->velocity.j;
    particle->velocity.z = sphere_point_table[table_index].z * random_weight +
        forward_weight * marker->node_transform.forward.k + system->velocity.k;

    particle->position.x = marker->node_transform.position.x;
    particle->position.y = marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z;

    if (k2 != 0.0f) {
        vector3d_cross_product((real_vector3d *)&particle->direction,
            global_up3d_pointer, (real_vector3d *)&particle->velocity);
    } else {
        vector3d_cross_product((real_vector3d *)&particle->direction,
            (real_vector3d *)&particle->velocity, &marker->node_transform.forward);
    }
}

#if 0
Original Ghidra decompilation (0x455610):

void missed_455610(int param_1,short param_2,int param_3,int param_4)

{
  int iVar1;
  float fVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int iVar8;

  pfVar3 = *(float **)
            (param_2 * 0x80 +
             *(int *)(*(int *)((*(uint *)(param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                     0x60) + 0x60);
  fVar2 = pfVar3[2];
  fVar7 = pfVar3[1] * *pfVar3 * 0.033333335;
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  iVar8 = (int)(short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10);
  iVar1 = DAT_006b7af4 + iVar8 * 0xc;
  fVar4 = *(float *)(iVar1 + 4);
  fVar5 = *(float *)(iVar1 + 8);
  fVar6 = (1.0 - pfVar3[1]) * *pfVar3 * 0.033333335;
  *(float *)(param_3 + 0x28) =
       *(float *)(DAT_006b7af4 + iVar8 * 0xc) * fVar7 + fVar6 * *(float *)(param_4 + 0x3c) +
       *(float *)(param_1 + 0x2c);
  *(float *)(param_3 + 0x2c) =
       fVar4 * fVar7 + fVar6 * *(float *)(param_4 + 0x40) + *(float *)(param_1 + 0x30);
  *(float *)(param_3 + 0x30) =
       fVar5 * fVar7 + fVar6 * *(float *)(param_4 + 0x44) + *(float *)(param_1 + 0x34);
  *(undefined4 *)(param_3 + 0x1c) = *(undefined4 *)(param_4 + 0x60);
  *(undefined4 *)(param_3 + 0x20) = *(undefined4 *)(param_4 + 100);
  *(undefined4 *)(param_3 + 0x24) = *(undefined4 *)(param_4 + 0x68);
  if (fVar2 != 0.0) {
    vector3d_cross_product((float *)(param_3 + 0x28));
    return;
  }
  vector3d_cross_product((float *)(param_4 + 0x3c));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

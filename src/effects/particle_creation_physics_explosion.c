// particle_creation_physics_explosion  (Ghidra: no function created; the phase-4 types agent
//   carved the stub name "missed_4554e0" from the .rdata dispatch-table evidence)
// address 0x4554e0, size 304 bytes
// VERIFIED against disassembly 0x4554e0..0x455610 (2026-09-30). system->unknown_54 is a byte at +0x54 gating the ABS
//   of the vertical speed (0x455586)
// name confidence 0.55, rewrite confidence 0.4 (see the UNSURE notes)
// evidence: out/phase4/effects_types_notes.md section 2: the three-entry `.rdata` table at
//   0x00657444 lists 0x4554e0 as entry [1], "particle creation physics, explosion". Signature
//   matches particle_system_spawn.c's dispatch shape exactly. ParticleSystemType.physics_
//   constants.pointer (types/tags.h, TagReflexive at +0x5c, pointer sub-field at +0x60) is read
//   as a raw `float *` of (at least) three ParticleSystemPhysicsConstant values -- exactly
//   effects_types_notes.md's own account of that reflexive. The random direction draw
//   (LCG update of 0x00719cd4 then a 0x006b7af4/0x006b7af8 table lookup) is byte-for-byte
//   src/effects/effect_random_direction_from_table.c's own body, called directly here instead of
//   re-inlining it. Every particle_system_particle field (unknown_28, direction, position) and
//   marker->node_transform.position match the same fields particle_creation_physics_default.c
//   already established. The final `vector3d_rotate_about_axis` call's two register arguments
//   (EAX -> v, ECX -> axis; see src/ai/actor_look_pick_random_point_in_cone.c) were resolved
//   from objdump 0x4554e0..0x45560f: ECX is loaded from the global at 0x00696720
//   (types/math.h global_up3d_pointer) immediately before the call, and EAX was last set by
//   `lea eax,[ecx+0x34]` (ecx = particle) at 0x4555bc, i.e. `&particle->direction` -- neither is
//   visible in Ghidra's own decompile of this call.
// register convention: identical to every other entry of this dispatch table (system, type_index,
//   particle, marker); blam-cc: system, type_index, particle, marker.
// UNSURE: the exact roles of the three physics_constants values (named k0/k1/k2 below by
//   position only) are not established -- k0 scales the horizontal (x, y) launch speed, k1
//   scales the vertical (z) launch speed and also re-scales the whole horizontal/vertical
//   velocity a second time when combining with the system's own velocity, matching the
//   decompiled arithmetic exactly but without physical names for k0/k1/k2 themselves.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_up3d_pointer; // 0x00696720

extern void effect_random_direction_from_table(real_point3d *out); // 0x4505e0, this module
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis,
    real sin_angle, real cos_angle); // 0x4cd820, foreign (math); EAX -> v, ECX -> axis

// ParticleSystem.particle_creation_physics dispatch table entry 1, "explosion". Draws a random
// direction from the shared sphere-point table, scales its horizontal and vertical components
// separately by the type's physics constants (optionally forcing the vertical component
// positive), offsets the particle's spawn position by that scaled impulse relative to the
// marker, sets its horizontal-only launch direction (then rotates it 90 degrees about world up),
// and combines the scaled impulse with the system's own velocity into unknown_28.
void particle_creation_physics_explosion(particle_system *system, int32_t type_index,
    particle_system_particle *particle, object_marker *marker)
{
    ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
        (*(uint8_t **)((uint8_t *)tag_instances[system->definition_index & 0xffff].data + 0x60)) +
        (int32_t)type_index * 0x80);
    float *physics_constants = *(float **)&((struct ParticleSystemType *)particle_type)->physics_constants.pointer;
    float k0 = physics_constants[0];
    float k1 = physics_constants[1];
    float k2 = physics_constants[2];
    real_point3d direction;
    float scaled_x, scaled_y, scaled_z;

    effect_random_direction_from_table(&direction);

    scaled_x = k0 * direction.x;
    scaled_y = k0 * direction.y;
    scaled_z = k1 * direction.z;
    if (*(uint8_t *)&system->unknown_54 != 0) { // UNSURE: see the header note above
        scaled_z = (scaled_z < 0.0f) ? -scaled_z : scaled_z;
    }

    particle->position.x = scaled_x + marker->node_transform.position.x;
    particle->position.y = scaled_y + marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z + scaled_z;

    particle->direction.i = scaled_x;
    particle->direction.j = scaled_y;
    particle->direction.k = 0.0f;

    particle->velocity.x = scaled_x * k2 + system->velocity.i;
    particle->velocity.y = scaled_y * k2 + system->velocity.j;
    particle->velocity.z = k2 * scaled_z + system->velocity.k;

    vector3d_rotate_about_axis(&particle->direction, global_up3d_pointer, 1.0f, 0.0f);
}

#if 0
Original Ghidra decompilation (0x4554e0):

void missed_4554e0(int param_1,short param_2,int param_3,int param_4)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  float fVar5;
  float fVar6;
  int iVar7;

  pfVar4 = *(float **)
            (param_2 * 0x80 +
             *(int *)(*(int *)((*(uint *)(param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                     0x60) + 0x60);
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  fVar2 = *pfVar4;
  fVar3 = pfVar4[1];
  fVar5 = pfVar4[2];
  iVar7 = (int)(short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10);
  iVar1 = DAT_006b7af4 + iVar7 * 0xc;
  pfVar4 = (float *)(param_3 + 0x28);
  *pfVar4 = *(float *)(DAT_006b7af4 + iVar7 * 0xc);
  *(undefined4 *)(param_3 + 0x2c) = *(undefined4 *)(iVar1 + 4);
  *(undefined4 *)(param_3 + 0x30) = *(undefined4 *)(iVar1 + 8);
  fVar6 = fVar2 * *pfVar4;
  *pfVar4 = fVar6;
  fVar2 = fVar2 * *(float *)(param_3 + 0x2c);
  *(float *)(param_3 + 0x2c) = fVar2;
  fVar3 = fVar3 * *(float *)(param_3 + 0x30);
  *(float *)(param_3 + 0x30) = fVar3;
  if (*(char *)(param_1 + 0x54) != '\0') {
    *(float *)(param_3 + 0x30) = ABS(fVar3);
  }
  *(float *)(param_3 + 0x1c) = fVar6 + *(float *)(param_4 + 0x60);
  *(float *)(param_3 + 0x20) = fVar2 + *(float *)(param_4 + 100);
  fVar3 = *(float *)(param_4 + 0x68);
  *(float *)(param_3 + 0x34) = fVar6;
  *(float *)(param_3 + 0x38) = fVar2;
  *(float *)(param_3 + 0x24) = fVar3 + *(float *)(param_3 + 0x30);
  *(undefined4 *)(param_3 + 0x3c) = 0;
  *pfVar4 = fVar6 * fVar5 + *(float *)(param_1 + 0x2c);
  *(float *)(param_3 + 0x2c) = fVar2 * fVar5 + *(float *)(param_1 + 0x30);
  *(float *)(param_3 + 0x30) = fVar5 * *(float *)(param_3 + 0x30) + *(float *)(param_1 + 0x34);
  vector3d_rotate_about_axis(0x3f800000,0);
  return;
}
#endif

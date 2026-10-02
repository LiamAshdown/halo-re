// particle_update_physics_default  (Ghidra: no function created; the phase-4 types agent carved
//   the stub name "missed_455350" from the .rdata dispatch-table evidence)
// address 0x455350, size 369 bytes
// VERIFIED against disassembly 0x455350..0x4554c1 (2026-09-30)
// name confidence 0.6, rewrite confidence 0.9 (VERIFIED against objdump 0x455350..0x4554c0)
// evidence: out/phase4/effects_types_notes.md section 2: the one-entry `.rdata` table at
//   0x00657450 lists 0x455350 as entry [0], "particle update physics, default". particle_system_
//   update.c (0x4544f0, already rewritten) calls this table with the exact signature used below:
//   `particle_update_physics_table[current_state->particle_update_physics](self, type_index,
//   delta_time, particle)`. Every field was checked against objdump 0x455350..0x455493 and
//   types/tags.h: ParticleSystemType.particle_states (+0x78, reflexive pointer),
//   ParticleSystemTypeParticleState.radius_multiplier (+0x80) and .point_physics.tag_id (+0x90),
//   particle_system_type_state.radius (types/effects.h +0x28), ParticleSystemType.radius (+0x2c)
//   and .flags (+0x20, ParticleSystemTypeFlags: particles_die_in_water/_in_air/_on_ground land
//   exactly on the 0x10/0x20/0x40 bits this function tests against point_physics_tick's result).
// register convention: four plain stack arguments (system [esp+4], type_index [esp+8], delta_time
//   [esp+0xc], particle [esp+0x10]); blam-cc: stack -> (system, type_index, delta_time, particle).
// Cleanup-pass review (objdump 0x455350..0x4554c0): FUN_0050b9e0 takes EAX = a 0x40-byte stack
//   PointPhysics, ECX / EDX = the current / next state's point_physics tags and the fraction on
//   the stack, and blends them into the local, which is what point_physics_tick then gets; the
//   draft passed only the fraction and used the return value. Fixed.

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

extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition,
    bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind,
    real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt); // 0x50b530, foreign (physics)
extern void point_physics_interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction); // 0x50b9e0, blam-cc: EAX, ECX, EDX, stack;
    // physics, outside this pass: out = from*(1-fraction) + to*fraction field by field (0x50b9e0..0x50ba76);
    // blam-cc: EAX -> out (left intact, the caller keeps using it), ECX -> from, EDX -> to, stack -> fraction

// ParticleSystemTypeParticleState.particle_update_physics dispatch table entry 0, the default
// implementation. Picks the particle's current point_physics tag (or, mid-transition, blends the
// current and next states' physics/radius by the transition fraction), runs one point_physics_
// tick on it, and kills the particle if it hit a surface/medium its type says should kill it.
void particle_update_physics_default(particle_system *system, int16_t type_index, real dt,
    particle_system_particle *particle)
{
    {
        ParticleSystemType *particle_type = (ParticleSystemType *)((uint8_t *)
            (*(uint8_t **)((uint8_t *)tag_instances[system->definition_index & 0xffff].data + 0x60)) +
            (int32_t)type_index * 0x80);
        particle_system_type_state *type_state = &system->type_states[type_index];
        ParticleSystemTypeParticleState *states = (ParticleSystemTypeParticleState *)
            (*(uint8_t **)&((struct ParticleSystemType *)particle_type)->particle_states.pointer);
        ParticleSystemTypeParticleState *state = &states[particle->state_index];
        real radius;
        PointPhysics *physics;
        PointPhysics blended; // the 0x40-byte local at [esp+0x10]
        uint32_t collision_flags;

        if (particle->next_state_index == -1) {
            physics = (PointPhysics *)tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & 0xffff].data;
            radius = state->radius_multiplier * type_state->radius * particle_type->radius;
        } else {
            ParticleSystemTypeParticleState *next_state = &states[particle->next_state_index];
            real fraction = particle->state_time_remaining / particle->state_duration;

            if (fraction < 0.0f) {
                fraction = 0.0f;
            } else if (fraction > 1.0f) {
                fraction = 1.0f;
            }

            radius = ((1.0f - fraction) * next_state->radius_multiplier + fraction * state->radius_multiplier) *
                     type_state->radius * particle_type->radius;
            point_physics_interpolate(&blended,
                (PointPhysics *)tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)state)->point_physics.tag_id & 0xffff].data,
                (PointPhysics *)tag_instances[*(uint32_t *)&((struct ParticleSystemTypeParticleState *)next_state)->point_physics.tag_id & 0xffff].data,
                fraction);
            physics = &blended;
        }

        collision_flags = point_physics_tick((real_vector3d *)&particle->velocity, 0, physics,
            &particle->location, (uint32_t)-1, &particle->position, (real_vector3d *)0,
            (real_vector3d *)0, (int16_t *)0, radius, dt);

        if (((collision_flags & 1) != 0 && (particle_type->flags & 0x20) != 0) ||
            ((collision_flags & 2) != 0 && (particle_type->flags & 0x10) != 0) ||
            ((collision_flags & 4) != 0 && (particle_type->flags & 0x40) != 0)) {
            particle->active = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x455350):

void missed_455350(float param_1,short param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;

  iVar5 = param_2 * 0x80 +
          *(int *)(*(int *)((*(uint *)((int)param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                  0x60);
  iVar1 = param_2 * 0x40 + 0x58 + (int)param_1;
  iVar4 = *(short *)(param_4 + 8) * 0x178 + *(int *)(iVar5 + 0x78);
  if (*(short *)(param_4 + 10) == -1) {
    uVar2 = *(undefined4 *)((*(uint *)(iVar4 + 0x90) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    param_1 = *(float *)(iVar4 + 0x80) * *(float *)(iVar1 + 0x28) * *(float *)(iVar5 + 0x2c);
  }
  else {
    _param_2 = *(float *)(param_4 + 0xc) / *(float *)(param_4 + 0x10);
    if (0.0 <= _param_2) {
      if (1.0 < _param_2) {
        _param_2 = 1.0;
      }
    }
    else {
      _param_2 = 0.0;
    }
    param_1 = ((1.0 - _param_2) *
               *(float *)(*(short *)(param_4 + 10) * 0x178 + *(int *)(iVar5 + 0x78) + 0x80) +
              _param_2 * *(float *)(iVar4 + 0x80)) * *(float *)(iVar1 + 0x28) *
              *(float *)(iVar5 + 0x2c);
    uVar2 = FUN_0050b9e0(_param_2);
  }
  uVar3 = point_physics_tick(0,uVar2,param_4 + 0x14,0xffffffff,param_4 + 0x1c,0,0,0,param_1,param_3)
  ;
  if (((((uVar3 & 1) != 0) && ((*(byte *)(iVar5 + 0x20) & 0x20) != 0)) ||
      (((uVar3 & 2) != 0 && ((*(byte *)(iVar5 + 0x20) & 0x10) != 0)))) ||
     (((uVar3 & 4) != 0 && ((*(byte *)(iVar5 + 0x20) & 0x40) != 0)))) {
    *(undefined1 *)(param_4 + 3) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

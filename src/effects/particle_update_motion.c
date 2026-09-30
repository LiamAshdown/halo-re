// particle_update_motion  (Ghidra: FUN_004561a0, still unnamed there; named directly by
//   types/effects.h: "particle_update_motion 0x4561a0 (position, velocity, location, flags)")
// address 0x4561a0, size 929 bytes
// name confidence: 0.6   rewrite confidence: 0.85 (VERIFIED against objdump 0x4561a0..0x456540; material call FIXED)
// evidence: types/effects.h particle fields (flags +0x02 incl. _particle_at_rest_bit,
//   object_index +0x08, location +0x28, position +0x30, velocity +0x48, unknown_3c/0x54/0x58);
//   types/tags.h Particle (flags bits by enum order: dies_at_rest=0x10,
//   dies_on_contact_with_structure=0x20, dies_on_contact_with_water=0x80,
//   dies_on_contact_with_air=0x100; collision_effect/death_effect/
//   sir_marty_exchanged_his_children_for_thine (material_effects) TagDependency.tag_id;
//   contact_deterioration +0x88), PointPhysics (mass_scale, air_friction); types/physics.h
//   point_physics_result_flags; src/physics/point_physics_tick.c establishes point_physics_tick
//   0x50b530's full 11 argument signature (velocity via ESI, the rest on the stack).
// UNSURE (heavily): this function's world space branch calls point_physics_tick 0x50b530 with
//   velocity passed via ESI (a register argument Ghidra drops entirely at this call site) and
//   with `dt` missing from the visible argument list too (reconstructed here as `delta_time`,
//   which is what every other caller in the codebase passes for that slot). The three
//   collision-normal locals (`local_c[8]` + `local_4`) are reconstructed as one contiguous
//   real_vector3d out_normal, since real_vector3d is exactly 12 bytes and `local_4`'s later use
//   (`0.8 < local_4`, a "did we land on a floor-like surface" test) has no other visible
//   assignment. `particle_impact_response_dispatch`'s fourcc argument (in_ECX at that call site)
//   is not shown here either, so it is passed as 0, which is certainly wrong when the dispatch
//   is actually reached; flagged there too.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "physics.h"
#include "effects.h"

extern data_array *particle_data;   // 0x0087abd0
extern tag_instance *tag_instances; // 0x0087bc14

extern double sqrt(double x); // x87 FSQRT
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0,
                                    // objects module
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module
extern real particle_current_radius(datum_index particle_handle); // 0x4566f0, this module
extern void particle_impact(datum_index particle_handle); // 0x456550, this module
extern void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index,
    real intensity); // 0x4565a0, EAX self, ECX fourcc, ESI definition_index, stack intensity
extern uint8_t any_local_player_within_10_units(real_point3d *position); // 0x453330, players module
extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type,
    int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param,
    real_point3d *position, real_vector3d *offset); // 0x453490, this module
extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg,
    PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4,
    real_point3d *position, real_vector3d *wind, real_vector3d *out_normal,
    int16_t *out_material_type, real radius, real dt); // 0x50b530, physics module

// Per-tick motion update for one particle. A particle at rest just re-validates its attached
// object (if any). An object-attached particle otherwise decays its own velocity by a simplified
// version of the PointPhysics air-friction formula and integrates position directly (no
// collision test). A free-standing particle instead runs the full point-physics collision tick,
// firing its collision/material effect and death-on-contact flags, and both paths settle the
// particle to rest once its velocity drops below a small threshold (or, for a world space
// particle, once it lands on a roughly upward-facing surface).
// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads ECX; particle_handle arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> particle_handle, delta_time
uint8_t particle_update_motion(datum_index particle_handle, real delta_time)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;
    uint8_t settled = 0;

    if ((self->flags & _particle_at_rest_bit) != 0) {
        if (self->object_index == k_datum_index_none) {
            return 1;
        }
        if (object_try_and_get(self->object_index, 0xffffffff) != 0) {
            return 1;
        }
        datum_delete(particle_data, particle_handle);
        return 0;
    }

    if (self->object_index == k_datum_index_none) {
        // World space particle: full point-physics collision tick.
        PointPhysics *physics = (PointPhysics *)tag_instances[tag->physics.tag_id.index].data;
        real radius = particle_current_radius(particle_handle);
        real_vector3d out_normal;
        int16_t out_material_type;
        uint32_t collision_flags;
        uint8_t collided;

        collision_flags = point_physics_tick(&self->velocity, 0, physics, &self->location,
            0xffffffff, &self->position, (real_vector3d *)0, &out_normal, &out_material_type,
            radius, delta_time); // UNSURE, see file header

        collided = (collision_flags & _point_physics_collided_bit) != 0;

        if (collided) {
            if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu ||
                *(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id != 0u) {
                real speed = (real)sqrt((double)(self->velocity.k * self->velocity.k +
                    self->velocity.j * self->velocity.j + self->velocity.i * self->velocity.i)) - 0.5f;
                speed = (speed < 0.0f) ? 0.0f : (speed > 1.0f ? 1.0f : speed);

                if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu) {
                    // 0x4562c6..0x4562d5: ECX = the dependency's group (tag +0x48), ESI = its tag index, EAX = self
                    particle_impact_response_dispatch(self, *(tag_group *)&tag->collision_effect.tag_fourcc,
                        *(datum_index *)&tag->collision_effect.tag_id, speed);
                }
                // 0x4562e5..0x45630f: the inner test is against -1 (the outer one above is against 0), and EDI
                //   is the collision normal
                if (*(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id != 0xffffffffu &&
                    any_local_player_within_10_units(&self->position) != 0) {
                    material_effects_play_at_marker(
                        *(uint32_t *)&tag->sir_marty_exchanged_his_children_for_thine.tag_id,
                        8, out_material_type, (uint32_t *)&self->location,
                        *(uint32_t *)&speed, &self->position, &out_normal);
                }
            }
            if ((tag->flags & 0x20) != 0) { // dies_on_contact_with_structure
                if (*(uint32_t *)&tag->collision_effect.tag_id != 0xffffffffu) {
                    datum_delete(particle_data, particle_handle);
                    return 0;
                }
                particle_impact(particle_handle);
                return 0;
            }
        }

        if (((collision_flags & _point_physics_in_air_bit) != 0 && (tag->flags & 0x100) != 0) ||
            ((collision_flags & _point_physics_in_water_bit) != 0 && (tag->flags & 0x80) != 0)) {
            particle_impact(particle_handle);
            return 0;
        }

        if (collided || (collision_flags & _point_physics_hit_water_surface_bit) != 0) {
            settled = out_normal.k > 0.8f;
            self->inverse_animation_period = self->inverse_animation_period +
                tag->contact_deterioration; // UNSURE: raw offset +0x20 is
                                    // inverse_animation_period, not a motion field; preserved
                                    // literally even though it looks like a bug in the original
        }
    } else {
        // Object-attached particle: simplified exponential velocity decay, no collision test.
        PointPhysics *physics = (PointPhysics *)tag_instances[tag->physics.tag_id.index].data;
        real radius;
        real friction, mass_related, decay;

        if ((self->flags & _particle_first_person_bit) == 0 &&
            object_try_and_get(self->object_index, 0xffffffff) == 0) {
            datum_delete(particle_data, particle_handle);
            return 0;
        }

        radius = particle_current_radius(particle_handle);
        friction = radius * physics->air_friction * radius;
        mass_related = radius * physics->mass_scale * radius * radius;

        if (mass_related == 0.0f) {
            decay = (friction == 0.0f) ? 1.0f : 0.0f;
        } else {
            decay = 1.0f - (friction / mass_related) * delta_time;
            if (decay < 0.0f) {
                decay = 0.0f;
            } else if (decay > 1.0f) {
                decay = 1.0f;
            }
        }

        settled = 1;
        self->velocity.i = self->velocity.i * decay;
        self->velocity.j = self->velocity.j * decay;
        self->velocity.k = self->velocity.k * decay;
        self->position.x = self->position.x + self->velocity.i * delta_time;
        self->position.y = self->position.y + self->velocity.j * delta_time;
        self->position.z = self->position.z + self->velocity.k * delta_time;
    }

    if (self->velocity.k * self->velocity.k + self->velocity.j * self->velocity.j +
        self->velocity.i * self->velocity.i >= 0.0625f) {
        self->direction = self->velocity;
    } else if (settled) {
        if ((tag->flags & 0x10) != 0) { // dies_at_rest
            particle_impact(particle_handle);
            return 0;
        }
        self->flags |= _particle_at_rest_bit;
    }

    self->rotation = self->rotation + delta_time * self->angular_velocity; // UNSURE: contradicts
                                    // types/effects.h's "no reader in this module" note for both
                                    // fields; kept as read here since the disassembly plainly
                                    // does it
    return 1;
}

#if 0
Original Ghidra decompilation (0x4561a0):

undefined4 FUN_004561a0(uint param_1,float param_2)

{
  float fVar1;
  ushort uVar2;
  uint *puVar3;
  bool bVar4;
  char cVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float10 fVar11;
  float10 fVar12;
  float local_20;
  undefined4 local_14;
  float local_10;
  undefined1 local_c [8];
  float local_4;

  iVar7 = *(int *)(DAT_0087abd0 + 0x34);
  iVar8 = (param_1 & 0xffff) * 0x70;
  uVar2 = *(ushort *)(iVar8 + 2 + iVar7);
  iVar9 = iVar8 + iVar7;
  puVar3 = *(uint **)(DAT_0087bc14 + ((*(uint *)(iVar8 + 4 + iVar7) & 0xffff) * 8 + 5) * 4);
  if ((uVar2 & 2) != 0) {
    if (*(int *)(iVar9 + 8) == -1) {
      return 1;
    }
    iVar7 = object_try_and_get(0xffffffff);
    if (iVar7 != 0) {
      return 1;
    }
LAB_0045652e:
    datum_delete();
    return 0;
  }
  iVar7 = *(int *)(DAT_0087bc14 + ((puVar3[8] & 0xffff) * 8 + 5) * 4);
  bVar4 = false;
  if (*(int *)(iVar9 + 8) == -1) {
    fVar10 = (float10)FUN_004566f0(param_2);
    uVar6 = FUN_0050b530(0,iVar7,iVar9 + 0x28,0xffffffff,iVar9 + 0x30,0,local_c,&local_14,
                         (float)fVar10);
    local_10 = (float)(uVar6 & 4);
    if (local_10 != 0.0) {
      if ((puVar3[0x15] != 0xffffffff) || (puVar3[0xc] != 0)) {
        local_20 = SQRT(*(float *)(iVar9 + 0x50) * *(float *)(iVar9 + 0x50) +
                        *(float *)(iVar9 + 0x4c) * *(float *)(iVar9 + 0x4c) +
                        *(float *)(iVar9 + 0x48) * *(float *)(iVar9 + 0x48)) - 0.5;
        if (0.0 <= local_20) {
          if (1.0 < local_20) {
            local_20 = 1.0;
          }
        }
        else {
          local_20 = 0.0;
        }
        if (puVar3[0x15] != 0xffffffff) {
          FUN_004565a0(local_20);
        }
        if ((puVar3[0xc] != 0xffffffff) && (cVar5 = FUN_00453330(), cVar5 != '\0')) {
          FUN_00453490(8,local_14,iVar9 + 0x28,local_20);
        }
      }
      if ((*puVar3 & 0x20) != 0) {
        if (puVar3[0x15] != 0xffffffff) {
          datum_delete();
          return 0;
        }
        FUN_00456550();
        return 0;
      }
    }
    if ((((uVar6 & 1) != 0) && ((*puVar3 & 0x100) != 0)) ||
       (((uVar6 & 2) != 0 && ((*puVar3 & 0x80) != 0)))) goto LAB_00456374;
    if ((local_10 != 0.0) || ((uVar6 & 8) != 0)) {
      bVar4 = 0.8 < local_4;
      *(float *)(iVar9 + 0x20) = (float)puVar3[0x22] + *(float *)(iVar9 + 0x20);
    }
  }
  else {
    if (((uVar2 & 0x40) == 0) && (iVar8 = object_try_and_get(0xffffffff), iVar8 == 0))
    goto LAB_0045652e;
    fVar10 = (float10)FUN_004566f0();
    fVar1 = (float)(fVar10 * (float10)*(float *)(iVar7 + 0x24) * fVar10);
    fVar10 = fVar10 * (float10)*(float *)(iVar7 + 4) * fVar10 * fVar10;
    if (fVar10 == (float10)0.0) {
      if (fVar1 == 0.0) {
LAB_00456497:
        fVar10 = (float10)1.0;
      }
      else {
        fVar10 = (float10)0.0;
      }
    }
    else {
      fVar10 = (float10)1.0 - ((float10)fVar1 / fVar10) * (float10)param_2;
      if ((float10)0.0 <= fVar10) {
        if ((float10)1.0 < fVar10) goto LAB_00456497;
      }
      else {
        fVar10 = (float10)0.0;
      }
    }
    fVar11 = fVar10 * (float10)*(float *)(iVar9 + 0x48);
    bVar4 = true;
    *(float *)(iVar9 + 0x48) = (float)fVar11;
    fVar12 = fVar10 * (float10)*(float *)(iVar9 + 0x4c);
    local_10 = (float)fVar12;
    *(float *)(iVar9 + 0x4c) = (float)fVar12;
    fVar10 = fVar10 * (float10)*(float *)(iVar9 + 0x50);
    *(float *)(iVar9 + 0x50) = (float)fVar10;
    *(float *)(iVar9 + 0x30) = (float)fVar11 * param_2 + *(float *)(iVar9 + 0x30);
    *(float *)(iVar9 + 0x34) = local_10 * param_2 + *(float *)(iVar9 + 0x34);
    *(float *)(iVar9 + 0x38) =
         (float)(fVar10 * (float10)param_2 + (float10)*(float *)(iVar9 + 0x38));
  }
  fVar1 = *(float *)(iVar9 + 0x48);
  if (0.0625 <= *(float *)(iVar9 + 0x50) * *(float *)(iVar9 + 0x50) +
                *(float *)(iVar9 + 0x4c) * *(float *)(iVar9 + 0x4c) + fVar1 * fVar1) {
    *(float *)(iVar9 + 0x3c) = *(float *)(iVar9 + 0x48);
    *(undefined4 *)(iVar9 + 0x40) = *(undefined4 *)(iVar9 + 0x4c);
    *(undefined4 *)(iVar9 + 0x44) = *(undefined4 *)(iVar9 + 0x50);
  }
  else if (bVar4) {
    if ((*puVar3 & 0x10) != 0) {
LAB_00456374:
      FUN_00456550();
      return 0;
    }
    *(byte *)(iVar9 + 2) = *(byte *)(iVar9 + 2) | 2;
  }
  *(float *)(iVar9 + 0x54) = param_2 * *(float *)(iVar9 + 0x58) + *(float *)(iVar9 + 0x54);
  return 1;
}
#endif

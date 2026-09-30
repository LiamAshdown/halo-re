// particle_system_update  (Ghidra: FUN_004544f0, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "particle_system_update 0x4544f0")
// address 0x4544f0, size 1608 bytes
// name confidence: 0.5   rewrite confidence: 0.35 (the second-least-verified file in this batch,
//   after particle_system_spawn: the goto-heavy control flow was restructured into loops rather
//   than transliterated 1:1, so double-check the transition/expiry loops against the original
//   if anything here looks suspicious)
// evidence: types/effects.h particle_system (flags +0x04, position +0x20, velocity +0x2c,
//   color +0x38, scale +0x14, type_states[4] +0x58), particle_system_type_state (every field),
//   particle_system_particle (every field including values/next_values at +0x48/+0x64), and the
//   struct comment "next_values ... copied down over values when the transition completes" this
//   function is the source of; types/tags.h ParticleSystemType (system_update_physics +0x48,
//   particle_update_physics +0xb2 of the *current* ParticleSystemTypeStates row, flags +0x20 with
//   _disabled_bit 0x100, particle_states TagReflexive +0x74, states TagReflexive +0x6c of the
//   type -- wait, +0x60/+0x6c are the states reflexive's own count/pointer, matching
//   particle_system_spawn's +0x5c/+0x60 pair one level up) and ParticleSystemTypeStates /
//   ParticleSystemTypeParticleState (duration_bounds, transition_time_bounds); the two .rdata
//   dispatch tables (system_update_physics at 0x0065743c, particle_update_physics at 0x00657450);
//   this module's particle_system_advance_type_state, particle_system_advance_particle_state and
//   particle_system_roll_particle_state; src/objects/object_get_position.c and
//   object_get_root_object_velocities.c and object_function_get_value.c for the three
//   object-side callees.
// register convention: none -- delta_time and handle are both Ghidra-recognized stack
//   parameters.
// UNSURE: the call to particle_system_spawn here passes a third visible argument (delta_time)
//   that particle_system_spawn's own decompiled body never reads; treated as Ghidra noise and
//   dropped, matching that function's real (self, type_index) signature. UNSURE: object.flags
//   bit 0x800 (_object_needs_cluster_update_bit) gating the emitting-flag recheck is preserved
//   literally though the semantic connection is unclear. UNSURE: after a particle is unlinked
//   and deleted, the walk re-reads that same (just-freed) particle's next_particle field to
//   continue the list -- kept exactly as decoded, matching this codebase's general policy of
//   preserving even highly suspect memory patterns rather than "fixing" them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "fn_math.h"

extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8
extern data_array *object_data;                   // 0x008603b0
extern tag_instance *tag_instances;               // 0x0087bc14
extern random_seed effect_random_seed;             // 0x00719cd4
extern void (*particle_system_update_physics_table[2])(particle_system *self, float delta_time); // 0x0065743c, .rdata
extern void (*particle_update_physics_table[1])(particle_system *self, int32_t type_index,
    float delta_time, particle_system_particle *particle); // 0x00657450, .rdata; UNSURE signature

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, objects module
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, objects module
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector,
    float *out_value); // 0x4f6e70, objects module
extern void particle_system_spawn(particle_system *self, int32_t type_index, float dt); // 0x453b10, this module
extern void particle_system_delete(datum_index handle); // 0x453f60, this module

extern void particle_system_advance_type_state(particle_system_type_state *state,
    ParticleSystemType *type, particle_system *system); // 0x4543b0, this module
extern void particle_system_advance_particle_state(particle_system_particle *particle,
    ParticleSystemType *type); // 0x454450, this module
extern void particle_system_roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states,
    particle_state_values *out); // 0x454250, this module

void particle_system_update(float delta_time, datum_index handle)
{
    particle_system *self = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition = (ParticleSystem *)tag_instances[self->definition_index & 0xffff].data;
    int32_t type_index;
    int32_t types_alive = 0;

    if (self->object_index != (datum_index)0xffffffff) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        float function_value;

        if ((obj->flags & _object_needs_cluster_update_bit) != 0 &&
            object_function_get_value(self->object_index, self->scale_function_index,
                                       &function_value)) {
            self->flags |= _particle_system_emitting_bit;
        } else {
            self->flags &= ~(uint32_t)_particle_system_emitting_bit;
        }

        object_get_position(&self->position, self->object_index);
        object_get_root_object_velocities(self->object_index, &self->velocity, (real_vector3d *)0);
        self->velocity.i *= 30.0f;
        self->velocity.j *= 30.0f;
        self->velocity.k *= 30.0f;
    }

    particle_system_update_physics_table[definition->system_update_physics](self, delta_time);

    for (type_index = 0; type_index < (int32_t)definition->particle_types.count; type_index++) {
        ParticleSystemType *type =
            &((ParticleSystemType *)definition->particle_types.pointer)[type_index];
        particle_system_type_state *state = &self->type_states[type_index];

        if ((type->flags & 0x100) != 0) { // disabled
            continue;
        }
        state->state_time_remaining -= delta_time;
        if (state->state_index == -1) {
            continue;
        }

        {
            ParticleSystemTypeStates *current_state;
            uint8_t skip_interpolation = 0;

            for (;;) {
                current_state = &((ParticleSystemTypeStates *)type->states.pointer)[state->state_index];
                if (0.0f <= state->state_time_remaining) {
                    break;
                }

                {
                    int16_t next_index = state->next_state_index;
                    float min_bound, max_bound, rolled;

                    if (next_index == -1) {
                        particle_system_advance_type_state(state, type, self);
                        min_bound = current_state->transition_time_bounds[0];
                        max_bound = current_state->transition_time_bounds[1];
                    } else {
                        state->state_index = next_index;
                        state->next_state_index = -1;
                        current_state =
                            &((ParticleSystemTypeStates *)type->states.pointer)[next_index];
                        min_bound = current_state->duration_bounds[0];
                        max_bound = current_state->duration_bounds[1];
                    }

                    rolled = random_real_range_seeded(&effect_random_seed, min_bound, max_bound);
                    state->state_duration = rolled;
                    state->state_time_remaining += rolled;

                    if (state->state_index == -1) {
                        skip_interpolation = 1;
                        break;
                    }
                }
            }

            if (!skip_interpolation) {
                if (state->next_state_index == -1) {
                    state->scale = current_state->scale_multiplier;
                    state->animation_rate = current_state->animation_rate_multiplier;
                    state->rotation_rate = current_state->rotation_rate_multiplier;
                    state->color = current_state->color_multiplier;
                    state->radius = current_state->radius_multiplier;
                    state->minimum_particle_count = current_state->minimum_particle_count;
                    state->particle_creation_rate = current_state->particle_creation_rate;
                } else {
                    ParticleSystemTypeStates *next_state = &((ParticleSystemTypeStates *)
                        type->states.pointer)[state->next_state_index];
                    float fraction = state->state_time_remaining / state->state_duration;

                    fraction = (fraction < 0.0f) ? 0.0f : (1.0f < fraction ? 1.0f : fraction);

                    state->scale = (1.0f - fraction) * next_state->scale_multiplier +
                                   fraction * current_state->scale_multiplier;
                    state->animation_rate = (1.0f - fraction) * next_state->animation_rate_multiplier +
                                   fraction * current_state->animation_rate_multiplier;
                    state->rotation_rate = (1.0f - fraction) * next_state->rotation_rate_multiplier +
                                   fraction * current_state->rotation_rate_multiplier;
                    state->color.alpha = (1.0f - fraction) * next_state->color_multiplier.alpha +
                                   fraction * current_state->color_multiplier.alpha;
                    state->color.red = (1.0f - fraction) * next_state->color_multiplier.red +
                                   fraction * current_state->color_multiplier.red;
                    state->color.green = (1.0f - fraction) * next_state->color_multiplier.green +
                                   fraction * current_state->color_multiplier.green;
                    state->color.blue = (1.0f - fraction) * next_state->color_multiplier.blue +
                                   fraction * current_state->color_multiplier.blue;
                    state->radius = (1.0f - fraction) * next_state->radius_multiplier +
                                   fraction * current_state->radius_multiplier;
                    state->minimum_particle_count =
                        (1.0f - fraction) * next_state->minimum_particle_count +
                        fraction * current_state->minimum_particle_count;
                    state->particle_creation_rate =
                        (1.0f - fraction) * next_state->particle_creation_rate +
                        fraction * current_state->particle_creation_rate;
                }

                if ((type->flags & 0x200) != 0) { // tint_by_effect_color
                    state->color.alpha *= self->color.alpha;
                    state->color.red *= self->color.red;
                    state->color.green *= self->color.green;
                    state->color.blue *= self->color.blue;
                }
                if ((type->flags & 0x800) != 0) { // minimum_count_scales_with_effect
                    state->minimum_particle_count *= self->scale;
                }
                if ((type->flags & 0x1000) != 0) { // creation_rate_scales_with_effect
                    state->particle_creation_rate *= self->scale;
                }
                if ((type->flags & 0x2000) != 0) { // scale_scales_with_effect
                    state->scale *= self->scale;
                }
                if ((type->flags & 0x4000) != 0) { // animation_rate_scales_with_effect
                    state->animation_rate *= self->scale;
                }
                if ((type->flags & 0x8000) != 0) { // rotation_rate_scales_with_effect
                    state->rotation_rate *= self->scale;
                }
            }

            if (state->state_index == -1) {
                continue;
            }

            {
                uint16_t particle_index;
                uint16_t previous_particle = 0xffff;

                if ((self->flags & _particle_system_emitting_bit) != 0) {
                    particle_system_spawn(self, type_index, delta_time); // 0x45481a: the third push is delta_time
                }

                particle_index = (uint16_t)state->first_particle;
                while (particle_index != 0xffff) {
                    particle_system_particle *particle = &((particle_system_particle *)
                        particle_system_particle_data->data)[particle_index];

                    particle->state_time_remaining -= delta_time;

                    if (particle->state_index == -1 && 0 < (int32_t)type->particle_states.count) {
                        ParticleSystemTypeParticleState *initial_state =
                            (ParticleSystemTypeParticleState *)type->particle_states.pointer;
                        float rolled;

                        particle->state_index = 0;
                        rolled = random_real_range_seeded(&effect_random_seed,
                            initial_state->duration_bounds[0], initial_state->duration_bounds[1]);
                        particle->state_duration = rolled;
                        particle->state_time_remaining = rolled;
                        particle_system_roll_particle_state(0,
                            (ParticleSystemTypeParticleState *)type->particle_states.pointer,
                            &particle->values);
                    }

                    if (particle->active == 0) {
                        particle->state_index = -1;
                    }

                    if (particle->state_index != -1) {
                        while (particle->state_index != -1 && particle->state_time_remaining < 0.0f) {
                            ParticleSystemTypeParticleState *particle_states =
                                (ParticleSystemTypeParticleState *)type->particle_states.pointer;
                            int16_t next_index = particle->next_state_index;
                            ParticleSystemTypeParticleState *entry;
                            float min_bound, max_bound, rolled;

                            if (next_index == -1) {
                                particle_system_advance_particle_state(particle, type);
                                entry = &particle_states[particle->state_index];
                                min_bound = entry->transition_time_bounds[0];
                                max_bound = entry->transition_time_bounds[1];
                            } else {
                                particle->state_index = next_index;
                                particle->next_state_index = -1;
                                entry = &particle_states[next_index];
                                min_bound = entry->duration_bounds[0];
                                max_bound = entry->duration_bounds[1];
                            }

                            rolled = random_real_range_seeded(&effect_random_seed, min_bound, max_bound);
                            particle->state_duration = rolled;
                            particle->state_time_remaining += rolled;

                            if (particle->next_state_index == -1) {
                                particle->values = particle->next_values;
                            } else {
                                particle_system_roll_particle_state(particle->next_state_index,
                                    particle_states, &particle->next_values);
                            }
                        }

                        if (particle->state_index != -1) {
                            float animation_rate, rotation_rate;

                            if (particle->next_state_index == -1) {
                                particle->rotation += particle->values.rotation_rate *
                                                       state->rotation_rate * delta_time;
                                animation_rate = particle->values.animation_rate;
                            } else {
                                float fraction = particle->state_time_remaining /
                                                  particle->state_duration;
                                fraction = (fraction < 0.0f) ? 0.0f :
                                           (1.0f < fraction ? 1.0f : fraction);
                                rotation_rate = fraction * particle->values.rotation_rate +
                                                (1.0f - fraction) * particle->next_values.rotation_rate;
                                particle->rotation += rotation_rate * state->rotation_rate * delta_time;
                                animation_rate = fraction * particle->values.animation_rate +
                                                 (1.0f - fraction) * particle->next_values.animation_rate;
                            }
                            particle->frame += animation_rate * state->animation_rate * delta_time;

                            particle_update_physics_table[current_state->particle_update_physics](
                                self, type_index, delta_time, particle);

                            previous_particle = particle_index;
                            particle_index = (uint16_t)particle->next_particle;
                            continue;
                        }
                    }

                    // particle expired entirely: unlink and delete it
                    if (previous_particle == 0xffff) {
                        state->first_particle = particle->next_particle;
                    } else {
                        ((particle_system_particle *)particle_system_particle_data->data)
                            [previous_particle].next_particle = particle->next_particle;
                    }
                    datum_delete(particle_system_particle_data, particle_index);
                    particle_index = (uint16_t)particle->next_particle; // UNSURE: reads the
                                    // just-deleted particle's field, see file header
                    state->particle_count = state->particle_count - 1;
                }
                types_alive++;
            }
        }
    }

    self->flags &= ~(uint32_t)_particle_system_in_update_bit;
    if (types_alive == 0 && self->object_index == (datum_index)0xffffffff) {
        particle_system_delete(handle);
    }
}

#if 0
Original Ghidra decompilation (0x4544f0):

/* WARNING: Removing unreachable block (ram,0x004546c6) */
/* WARNING: Removing unreachable block (ram,0x004548b1) */
/* WARNING: Removing unreachable block (ram,0x0045498f) */

void FUN_004544f0(float param_1,uint param_2)

{
  float *pfVar1;
  short *psVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  ushort uVar6;
  int iVar7;
  char cVar8;
  uint uVar9;
  float *pfVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  undefined4 *puVar15;
  float *pfVar16;
  undefined4 *puVar17;
  int iStack_18;
  short local_10;
  int iStack_c;

  iVar11 = (param_2 & 0xffff) * 0x158;
  iVar7 = *(int *)(DAT_0087abd4 + 0x34);
  uVar9 = *(uint *)(iVar11 + 0xc + iVar7);
  iVar12 = iVar11 + iVar7;
  iVar7 = *(int *)((*(uint *)(iVar11 + 8 + iVar7) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = 0;
  if (uVar9 != 0xffffffff) {
    if (((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) + 0x10) &
         0x800) == 0) || (cVar8 = object_function_get_value(), cVar8 == '\0')) {
      uVar9 = *(uint *)(iVar12 + 4) & 0xfffffffe;
    }
    else {
      uVar9 = *(uint *)(iVar12 + 4) | 1;
    }
    *(uint *)(iVar12 + 4) = uVar9;
    object_get_position();
    FUN_004f6aa0();
    *(float *)(iVar12 + 0x2c) = *(float *)(iVar12 + 0x2c) * 30.0;
    *(float *)(iVar12 + 0x30) = *(float *)(iVar12 + 0x30) * 30.0;
    *(float *)(iVar12 + 0x34) = *(float *)(iVar12 + 0x34) * 30.0;
  }
  (**(code **)(&DAT_0065743c + *(short *)(iVar7 + 0x48) * 4))(iVar12,param_1);
  iVar11 = 0;
  iStack_18 = 0;
  sVar5 = 0;
  if (*(int *)(iVar7 + 0x5c) < 1) {
LAB_00454b13:
    *(uint *)(iVar12 + 4) = *(uint *)(iVar12 + 4) & 0xfffffffd;
    if ((local_10 == 0) && (*(int *)(iVar12 + 0xc) == -1)) {
      particle_system_delete_453f60(param_2);
    }
    return;
  }
LAB_004545e1:
  local_10 = sVar5;
  iVar13 = iVar11 * 0x80 + *(int *)(iVar7 + 0x60);
  psVar2 = (short *)(iVar11 * 0x40 + 0x58 + iVar12);
  if (((*(uint *)(iVar13 + 0x20) & 0x100) == 0) &&
     (*(float *)(psVar2 + 2) = *(float *)(psVar2 + 2) - param_1, *psVar2 != -1)) {
LAB_00454620:
    iVar11 = *psVar2 * 0xc0 + *(int *)(iVar13 + 0x6c);
    sVar5 = psVar2[1];
    if (*(float *)(psVar2 + 2) < 0.0) goto code_r0x00454646;
    pfVar1 = (float *)(psVar2 + 6);
    if (sVar5 == -1) {
      pfVar10 = (float *)(iVar11 + 0x34);
      pfVar16 = pfVar1;
      for (iVar14 = 10; iVar14 != 0; iVar14 = iVar14 + -1) {
        *pfVar16 = *pfVar10;
        pfVar10 = pfVar10 + 1;
        pfVar16 = pfVar16 + 1;
      }
    }
    else {
      fVar3 = *(float *)(psVar2 + 2) / *(float *)(psVar2 + 4);
      if (0.0 <= fVar3) {
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = 0.0;
      }
      iVar14 = 10;
      pfVar10 = pfVar1;
      pfVar16 = (float *)(sVar5 * 0xc0 + 0x34 + *(int *)(iVar13 + 0x6c));
      do {
        iVar14 = iVar14 + -1;
        *pfVar10 = (1.0 - fVar3) * *pfVar16 +
                   fVar3 * *(float *)((iVar11 - (int)pfVar1) + 0x30 + (int)(pfVar10 + 1));
        pfVar10 = pfVar10 + 1;
        pfVar16 = pfVar16 + 1;
      } while (iVar14 != 0);
    }
    if ((*(uint *)(iVar13 + 0x20) & 0x200) != 0) {
      *(float *)(psVar2 + 0xc) = *(float *)(iVar12 + 0x38) * *(float *)(psVar2 + 0xc);
      *(float *)(psVar2 + 0xe) = *(float *)(iVar12 + 0x3c) * *(float *)(psVar2 + 0xe);
      *(float *)(psVar2 + 0x10) = *(float *)(iVar12 + 0x40) * *(float *)(psVar2 + 0x10);
      *(float *)(psVar2 + 0x12) = *(float *)(iVar12 + 0x44) * *(float *)(psVar2 + 0x12);
    }
    if ((*(uint *)(iVar13 + 0x20) & 0x800) != 0) {
      *(float *)(psVar2 + 0x16) = *(float *)(iVar12 + 0x14) * *(float *)(psVar2 + 0x16);
    }
    if ((*(uint *)(iVar13 + 0x20) & 0x1000) != 0) {
      *(float *)(psVar2 + 0x18) = *(float *)(iVar12 + 0x14) * *(float *)(psVar2 + 0x18);
    }
    if ((*(uint *)(iVar13 + 0x20) & 0x2000) != 0) {
      *pfVar1 = *(float *)(iVar12 + 0x14) * *pfVar1;
    }
    if ((*(uint *)(iVar13 + 0x20) & 0x4000) != 0) {
      *(float *)(psVar2 + 8) = *(float *)(iVar12 + 0x14) * *(float *)(psVar2 + 8);
    }
    if ((char)((uint)*(undefined4 *)(iVar13 + 0x20) >> 8) < '\0') {
      *(float *)(psVar2 + 10) = *(float *)(iVar12 + 0x14) * *(float *)(psVar2 + 10);
    }
LAB_00454802:
    if (*psVar2 == -1) goto LAB_00454af4;
    iStack_c = 0;
    if ((*(byte *)(iVar12 + 4) & 1) != 0) {
      FUN_00453b10(iVar12,iStack_18,param_1);
    }
    uVar6 = psVar2[0x1e];
joined_r0x0045483b:
    do {
      if (uVar6 == 0xffff) goto LAB_00454af0;
      iVar11 = (uint)uVar6 * 0x80 + *(int *)(DAT_0087abd8 + 0x34);
      *(float *)(iVar11 + 0xc) = *(float *)(iVar11 + 0xc) - param_1;
      if ((*(short *)(iVar11 + 8) == -1) && (0 < *(int *)(iVar13 + 0x74))) {
        *(undefined2 *)(iVar11 + 8) = 0;
        fVar3 = *(float *)(*(int *)(iVar13 + 0x78) + 0x20);
        DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
        fVar3 = fVar3 + (*(float *)(*(int *)(iVar13 + 0x78) + 0x24) - fVar3) *
                        (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
        *(float *)(iVar11 + 0xc) = fVar3;
        *(float *)(iVar11 + 0x10) = fVar3;
        FUN_00454250();
      }
      if (*(char *)(iVar11 + 3) == '\0') {
        *(undefined2 *)(iVar11 + 8) = 0xffff;
      }
      if (*(short *)(iVar11 + 8) != -1) {
        do {
          iVar14 = *(short *)(iVar11 + 8) * 0x178 + *(int *)(iVar13 + 0x78);
          if (0.0 <= *(float *)(iVar11 + 0xc)) break;
          sVar5 = *(short *)(iVar11 + 10);
          if (sVar5 == -1) {
            FUN_00454450();
            fVar3 = *(float *)(iVar14 + 0x2c);
            fVar4 = *(float *)(iVar14 + 0x28);
          }
          else {
            *(short *)(iVar11 + 8) = sVar5;
            *(undefined2 *)(iVar11 + 10) = 0xffff;
            iVar14 = sVar5 * 0x178 + *(int *)(iVar13 + 0x78);
            fVar3 = *(float *)(iVar14 + 0x24);
            fVar4 = *(float *)(iVar14 + 0x20);
          }
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          fVar4 = fVar4 + (fVar3 - fVar4) * (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
          *(float *)(iVar11 + 0x10) = fVar4;
          *(float *)(iVar11 + 0xc) = fVar4 + *(float *)(iVar11 + 0xc);
          if (*(short *)(iVar11 + 10) == -1) {
            puVar15 = (undefined4 *)(iVar11 + 100);
            puVar17 = (undefined4 *)(iVar11 + 0x48);
            for (iVar14 = 7; iVar14 != 0; iVar14 = iVar14 + -1) {
              *puVar17 = *puVar15;
              puVar15 = puVar15 + 1;
              puVar17 = puVar17 + 1;
            }
          }
          else {
            FUN_00454250();
          }
        } while (*(short *)(iVar11 + 8) != -1);
        if (*(short *)(iVar11 + 8) != -1) {
          sVar5 = *psVar2;
          iVar14 = *(int *)(iVar13 + 0x6c);
          if (*(short *)(iVar11 + 10) == -1) {
            *(float *)(iVar11 + 0x40) =
                 *(float *)(iVar11 + 0x50) * *(float *)(psVar2 + 10) * param_1 +
                 *(float *)(iVar11 + 0x40);
            fVar3 = *(float *)(iVar11 + 0x4c);
          }
          else {
            fVar3 = *(float *)(iVar11 + 0xc) / *(float *)(iVar11 + 0x10);
            if (0.0 <= fVar3) {
              if (1.0 < fVar3) {
                fVar3 = 1.0;
              }
            }
            else {
              fVar3 = 0.0;
            }
            *(float *)(iVar11 + 0x40) =
                 (fVar3 * *(float *)(iVar11 + 0x50) + (1.0 - fVar3) * *(float *)(iVar11 + 0x6c)) *
                 *(float *)(psVar2 + 10) * param_1 + *(float *)(iVar11 + 0x40);
            fVar3 = fVar3 * *(float *)(iVar11 + 0x4c) + (1.0 - fVar3) * *(float *)(iVar11 + 0x68);
          }
          *(float *)(iVar11 + 0x44) =
               fVar3 * *(float *)(psVar2 + 8) * param_1 + *(float *)(iVar11 + 0x44);
          (**(code **)(&DAT_00657450 + *(short *)(sVar5 * 0xc0 + iVar14 + 0xb2) * 4))
                    (iVar12,iStack_18,param_1,iVar11);
          uVar6 = *(ushort *)(iVar11 + 4);
          iStack_c = iVar11;
          goto joined_r0x0045483b;
        }
      }
      if (iStack_c == 0) {
        *(undefined4 *)(psVar2 + 0x1e) = *(undefined4 *)(iVar11 + 4);
      }
      else {
        *(undefined4 *)(iStack_c + 4) = *(undefined4 *)(iVar11 + 4);
      }
      datum_delete();
      uVar6 = *(ushort *)(iVar11 + 4);
      psVar2[0x1d] = psVar2[0x1d] + -1;
    } while( true );
  }
  goto LAB_00454af4;
code_r0x00454646:
  if (sVar5 == -1) {
    FUN_004543b0(iVar12);
    fVar3 = *(float *)(iVar11 + 0x2c);
    fVar4 = *(float *)(iVar11 + 0x28);
  }
  else {
    *psVar2 = sVar5;
    psVar2[1] = -1;
    iVar11 = sVar5 * 0xc0 + *(int *)(iVar13 + 0x6c);
    fVar3 = *(float *)(iVar11 + 0x24);
    fVar4 = *(float *)(iVar11 + 0x20);
  }
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  fVar4 = fVar4 + (fVar3 - fVar4) * (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
  *(float *)(psVar2 + 4) = fVar4;
  *(float *)(psVar2 + 2) = fVar4 + *(float *)(psVar2 + 2);
  if (*psVar2 == -1) goto LAB_00454802;
  goto LAB_00454620;
LAB_00454af0:
  local_10 = local_10 + 1;
LAB_00454af4:
  iStack_18 = iStack_18 + 1;
  iVar11 = (int)(short)iStack_18;
  sVar5 = local_10;
  if (*(int *)(iVar7 + 0x5c) <= iVar11) goto LAB_00454b13;
  goto LAB_004545e1;
}
#endif

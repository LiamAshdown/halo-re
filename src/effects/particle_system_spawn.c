// particle_system_spawn  (Ghidra: FUN_00453b10, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "particle_system_spawn 0x453b10")
// address 0x453b10, size 1085 bytes
// name confidence: 0.4   rewrite confidence: 0.3 (the least-verified file in this batch: the
//   three particle_creation_physics procedures Ghidra never split out, and
//   particle_system_particle's 0x28..0x34 fields they fill, are only typed by stride per
//   out/phase4/effects_types_notes.md section 2 and its "unresolved offsets" table; several
//   callees below are treated as opaque per existing codebase precedent rather than
//   re-examined)
// evidence: types/effects.h particle_system.flags (+0x04, _particle_system_in_update_bit) and
//   type_states[4] (+0x58); types/tags.h ParticleSystemType (flags +0x20 with
//   do_not_draw_in_first_person 0x10000 / do_not_draw_in_third_person 0x20000,
//   particle_creation_physics +0x54, size 0x80) and ParticleSystemTypeStates
//   (particle_creation_physics +0xb0, size 0xc0); the .rdata dispatch table
//   particle_creation_physics[3] at 0x00657444 (types/effects.h header comment); this module's
//   player_weapon_locality_for_object (0x453a10); src/objects/object_lights_update_all.c
//   establishes first_person_weapon_get_marker_data's signature; src/objects/
//   object_get_node_local_transform.c and src/physics/collision_test_movement_segment.c /
//   src/objects/object_set_cluster_and_parent.c establish object_get_root_location and scenario_location_from_point as
//   already-opaque cross-module calls elsewhere in this codebase.
// register convention: none -- self (a particle_system* record pointer, not a handle) and the
//   type index are both Ghidra-recognized stack parameters.
// UNSURE (extensive): render_local_player_gunner_seat_visible (a parameterless first/third-person view predicate),
//   object_get_root_location and scenario_location_from_point are all called here exactly as Ghidra shows them, with no
//   attempt to recover arguments Ghidra dropped, following this codebase's existing treatment
//   of the same three functions elsewhere. The marker-lookup scratch buffer is sized as 8
//   object_marker records (0x360 bytes) because both lookup calls pass a maximum count of 8,
//   even though Ghidra's own frame only reserved 60 bytes for it -- a known Ghidra
//   under-sizing, not a deliberate choice. The per-particle creation-physics dispatch call's
//   fourth argument (a marker picked at random out of that array) and the exact meaning of the
//   spawned particle's 0x28/0x2c/0x30/0x34 fields are left to the three opaque physics
//   procedures, consistent with the module notes' "found through the three .rdata dispatch
//   tables" section.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header

extern data_array *object_data;                   // 0x008603b0
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;               // 0x0087bc14
extern uint8_t particle_systems_enabled;          // 0x0069c566
extern int16_t current_local_player_index;        // 0x007c3108, DAT_007c3108; UNSURE name, the
                                    // "current local player" index used across this batch
extern uint8_t *first_person_weapon_globals;      // 0x006b2d98, stride 0x1ea0
extern const real_point3d *global_origin3d_pointer;     // 0x00696714
extern random_seed effect_random_seed;             // 0x00719cd4
extern void (*particle_creation_physics_table[3])(particle_system *system, int32_t type_index,
    particle_system_particle *particle, object_marker *marker); // 0x00657444, .rdata; UNSURE
                                    // signature, see file header

extern datum_index datum_new(data_array *array); // 0x4d0480
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern int32_t player_weapon_locality_for_object(datum_index weapon_object_index); // 0x453a10,
                                    // this module
extern uint8_t render_local_player_gunner_seat_visible(void); // 0x50fcd0, UNSURE: opaque first/third person predicate
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t max_count); // 0x4f6080, objects module
extern int16_t first_person_weapon_get_marker_data(uint32_t object_index, int32_t marker,
    void *out_buffer, int32_t max_count); // 0x492ad0, opaque per src/objects/object_lights_update_all.c
extern void object_get_root_location(uint32_t object_index); // 0x4f6b10, opaque per
                                    // src/objects/object_light_recompute_transform.c
extern void scenario_location_from_point(void *out_leaf_reference); // 0x53e780, opaque per
                                    // src/physics/collision_test_movement_segment.c

void particle_system_spawn(particle_system *self, int32_t type_index)
{
    particle_system_type_state *state = &self->type_states[type_index];
    ParticleSystem *definition = (ParticleSystem *)tag_instances[self->definition_index & 0xffff].data;
    ParticleSystemType *type =
        &((ParticleSystemType *)definition->particle_types.pointer)[type_index];
    uint8_t in_update = (self->flags & _particle_system_in_update_bit) != 0;
    ParticleSystemTypeStates *current_state = (ParticleSystemTypeStates *)0;
    int32_t weapon_locality;

    if (!in_update) {
        current_state =
            &((ParticleSystemTypeStates *)type->states.pointer)[state->state_index];
    }

    weapon_locality = player_weapon_locality_for_object(self->object_index);

    if (weapon_locality == 0 ||
        (((type->flags & 0x20000) == 0 || (weapon_locality != -1 && render_local_player_gunner_seat_visible() != 0)) &&
         ((type->flags & 0x10000) == 0 || (weapon_locality != 1 || render_local_player_gunner_seat_visible() == 0)))) {
        int16_t target_count;

        if (!in_update) {
            // steady state: state->particle_creation_rate is the runtime (already
            // scale-multiplied) copy of ParticleSystemTypeStates.particle_creation_rate, and
            // state->creation_fraction carries the fractional remainder between ticks.
            int32_t whole = (int32_t)state->particle_creation_rate;
            float fraction = (state->particle_creation_rate - (float)whole) +
                              state->creation_fraction;

            target_count = state->particle_count + (int16_t)whole;
            state->creation_fraction = fraction;
            if (1.0f < fraction) {
                target_count = target_count + 1;
                state->creation_fraction = fraction - 1.0f;
            }
        } else if ((type->flags & 0x400) == 0) { // initial_count_scales_with_effect
            target_count = (int16_t)type->initial_particle_count;
        } else {
            target_count = (int16_t)((float)type->initial_particle_count * self->scale);
        }

        if (particle_systems_enabled == 1) {
            target_count = (int16_t)((float)target_count * self->scale); // UNSURE: DAT_0069c566
                                    // guards this scale a second time in the decompile; kept
                                    // literally
        }

        if (state->particle_count < target_count) {
            datum_index object_index = self->object_index;
            object_marker markers[8];
            int32_t marker_count;

            if (object_index == (datum_index)0xffffffff) {
                // UNSURE: the decompile builds two separate 12-byte scratch blocks here
                // (local_324.. from global_origin3d_pointer and local_300.. from self->position)
                // that do not sit contiguously in Ghidra's frame, so which object_marker
                // sub-fields they really land on could not be pinned down; modeled here as the
                // marker's position, which is the one placement every caller of this path needs.
                markers[0].node_transform.position = self->position;
                markers[0].node_transform.left.i = global_origin3d_pointer->x;
                markers[0].node_transform.left.j = global_origin3d_pointer->y;
                markers[0].node_transform.left.k = global_origin3d_pointer->z;
                marker_count = 1;
            } else {
                object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                ObjectAttachment *attachment =
                    (ObjectAttachment *)((uint8_t *)tag_instances[obj->definition_tag & 0xffff].data +
                                          0x144) + self->attachment_index;

                marker_count = object_get_node_local_transform(object_index, attachment->marker.string,
                                                                markers, 8);
                object_get_root_location(object_index);
                if (marker_count == 0) {
                    int32_t weapon_object = *(int32_t *)(first_person_weapon_globals +
                        current_local_player_index * 0x1ea0 + 8);
                    if (weapon_object != -1) {
                        marker_count = first_person_weapon_get_marker_data(weapon_object,
                            attachment->marker.string - (char *)0, markers, 8); // UNSURE: the
                                    // marker name argument here is the same TagString pointer
                                    // Ghidra shows, kept verbatim
                        object_get_root_location(weapon_object);
                    }
                }
            }

            if (self->scale_function_index != -1) {
                int16_t attempts = 0;

                while (state->particle_count < target_count && marker_count != 0 &&
                       attempts < 0x80) {
                    datum_index handle = datum_new(particle_system_particle_data);
                    if (handle == (datum_index)0xffffffff) {
                        break;
                    }

                    {
                        particle_system_particle *particle =
                            &((particle_system_particle *)particle_system_particle_data->data)
                                [handle & 0xffff];
                        ParticleSystemParticleCreationPhysics_t physics =
                            in_update ? type->particle_creation_physics
                                      : current_state->particle_creation_physics;

                        particle->state_index = -1;
                        particle->next_state_index = -1;
                        particle->active = 1;
                        particle->ping_pong_forward = 1;
                        particle->frame = -1.0f;

                        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                        particle->rotation = (float)(effect_random_seed >> 16) * 1.5259022e-05f *
                                             6.2831855f;
                        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;

                        particle_creation_physics_table[physics](self, type_index, particle,
                            &markers[(int16_t)(((uint64_t)(effect_random_seed >> 16) *
                                                 (uint32_t)marker_count) >> 16)]);
                        scenario_location_from_point(&particle->location);

                        if (particle->location.leaf_index == -1) {
                            datum_delete(particle_system_particle_data, handle);
                        } else {
                            state->particle_count = state->particle_count + 1;
                            particle->next_particle = state->first_particle;
                            state->first_particle = handle;
                        }
                    }
                    attempts = attempts + 1;
                }
            }
        }
    }

    if ((float)state->particle_count < state->minimum_particle_count) {
        state->state_time_remaining = state->state_time_remaining * 0.3f;
    }
}

#if 0
Original Ghidra decompilation (0x453b10):

void FUN_00453b10(int param_1,undefined4 param_2)

{
  short *psVar1;
  uint *puVar2;
  int iVar3;
  char cVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  short sVar10;
  int iVar11;
  float10 extraout_ST0;
  float10 fVar12;
  short local_37c;
  int local_368;
  undefined1 local_360 [60];
  undefined4 local_324;
  undefined4 local_320;
  undefined4 local_31c;
  undefined4 local_300;
  undefined4 local_2fc;
  undefined4 local_2f8;

  psVar1 = (short *)((short)param_2 * 0x40 + 0x58 + param_1);
  iVar7 = (short)param_2 * 0x80 +
          *(int *)(*(int *)((*(uint *)(param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x60);
  uVar9 = *(uint *)(param_1 + 4) >> 1;
  if ((uVar9 & 1) == 0) {
    local_368 = *psVar1 * 0xc0 + *(int *)(iVar7 + 0x6c);
  }
  else {
    local_368 = 0;
  }
  sVar5 = FUN_00453a10(*(undefined4 *)(param_1 + 0xc));
  if ((sVar5 == 0) ||
     ((((*(uint *)(iVar7 + 0x20) & 0x20000) == 0 ||
       ((sVar5 != -1 && (cVar4 = FUN_0050fcd0(), cVar4 != '\0')))) &&
      (((*(uint *)(iVar7 + 0x20) & 0x10000) == 0 ||
       ((sVar5 != 1 || (cVar4 = FUN_0050fcd0(), cVar4 == '\0')))))))) {
    if ((uVar9 & 1) == 0) {
      sVar5 = __ftol();
      local_37c = psVar1[0x1d] + sVar5;
      fVar12 = (extraout_ST0 - (float10)(int)sVar5) + (float10)*(float *)(psVar1 + 0x1a);
      *(float *)(psVar1 + 0x1a) = (float)fVar12;
      if ((float10)1.0 < fVar12) {
        local_37c = local_37c + 1;
        *(float *)(psVar1 + 0x1a) = (float)(fVar12 - (float10)1.0);
      }
    }
    else if ((*(uint *)(iVar7 + 0x20) & 0x400) == 0) {
      local_37c = *(short *)(iVar7 + 0x24);
    }
    else {
      local_37c = __ftol();
    }
    if (DAT_0069c566 == '\x01') {
      local_37c = __ftol();
    }
    if (psVar1[0x1d] < local_37c) {
      uVar8 = *(uint *)(param_1 + 0xc);
      if (uVar8 == 0xffffffff) {
        local_300 = *(undefined4 *)(param_1 + 0x20);
        local_2fc = *(undefined4 *)(param_1 + 0x24);
        local_2f8 = *(undefined4 *)(param_1 + 0x28);
        local_324 = *(undefined4 *)PTR_DAT_00696714;
        local_320 = *(undefined4 *)(PTR_DAT_00696714 + 4);
        local_31c = *(undefined4 *)(PTR_DAT_00696714 + 8);
        sVar5 = 1;
      }
      else {
        puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc);
        sVar5 = object_get_node_local_transform
                          (uVar8,*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                         + 0x144) + 0x10 + *(short *)(param_1 + 0x10) * 0x48,
                           local_360,8);
        FUN_004f6b10();
        if ((sVar5 == 0) &&
           (iVar3 = *(int *)(DAT_007c3108 * 0x1ea0 + 8 + DAT_006b2d98), iVar3 != -1)) {
          sVar5 = first_person_weapon_get_marker_data
                            (iVar3,*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                    ) + 0x144) + 0x10 +
                                   *(short *)(param_1 + 0x10) * 0x48,local_360,8);
          FUN_004f6b10();
        }
      }
      if (*(short *)(param_1 + 0x1c) != -1) {
        sVar6 = psVar1[0x1d];
        sVar10 = 0;
        iVar3 = DAT_0087abd8;
        while ((((sVar6 < local_37c && (sVar5 != 0)) && (sVar10 < 0x80)) &&
               (uVar8 = datum_new(), uVar8 != 0xffffffff))) {
          iVar11 = (uVar8 & 0xffff) * 0x80 + *(int *)(iVar3 + 0x34);
          if ((uVar9 & 1) == 0) {
            sVar6 = *(short *)(local_368 + 0xb0);
          }
          else {
            sVar6 = *(short *)(iVar7 + 0x54);
          }
          *(undefined2 *)(iVar11 + 8) = 0xffff;
          *(undefined2 *)(iVar11 + 10) = 0xffff;
          *(undefined1 *)(iVar11 + 3) = 1;
          *(undefined1 *)(iVar11 + 2) = 1;
          *(undefined4 *)(iVar11 + 0x44) = 0xbf800000;
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          *(float *)(iVar11 + 0x40) = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          (**(code **)(&DAT_00657444 + sVar6 * 4))
                    (param_1,param_2,iVar11,
                     local_360 + (short)((DAT_00719cd4 >> 0x10) * (int)sVar5 >> 0x10) * 0x6c);
          FUN_0053e780();
          iVar3 = DAT_0087abd8;
          if (*(short *)(iVar11 + 0x18) == -1) {
            datum_delete();
          }
          else {
            psVar1[0x1d] = psVar1[0x1d] + 1;
            *(undefined4 *)(iVar11 + 4) = *(undefined4 *)(psVar1 + 0x1e);
            *(uint *)(psVar1 + 0x1e) = uVar8;
          }
          sVar10 = sVar10 + 1;
          sVar6 = psVar1[0x1d];
        }
      }
    }
    if ((float)(int)psVar1[0x1d] < *(float *)(psVar1 + 0x16)) {
      *(float *)(psVar1 + 2) = *(float *)(psVar1 + 2) * 0.3;
    }
  }
  return;
}
#endif

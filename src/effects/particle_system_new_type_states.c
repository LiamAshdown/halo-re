// particle_system_new_type_states  (Ghidra: FUN_004538b0, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "particle_system_new_type_states 0x4538b0")
// address 0x4538b0, size 338 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/effects.h particle_system.location (+0x18, bsp_leaf_reference),
//   particle_system.flags (+0x04, _particle_system_in_update_bit) and type_states[4] (+0x58,
//   each particle_system_type_state 0x40 bytes: state_index +0x00, next_state_index +0x02,
//   state_time_remaining +0x04, state_duration +0x08, ping_pong_forward +0x38, particle_count
//   +0x3a, first_particle +0x3c); types/tags.h ParticleSystemType (states TagReflexive +0x68)
//   and ParticleSystemTypeStates (duration_bounds[2] at +0x20); src/effects/contrail_generate_points.c
//   establishes bsp3d_node_find_leaf's (globals, point, index) signature; src/math/random_real_range_seeded.c
//   is the exact LCG this function inlines to roll the first state's duration. Confirmed against
//   objdump -d -M intel, 0x4538b0..0x453928, for the bsp3d_node_find_leaf call's register arguments.
// register convention: none -- handle is the single Ghidra-recognized stack parameter.
// UNSURE: the byte cleared at [esp+0x13] before the bsp3d_node_find_leaf call (`bl=0`) is not read
//   afterward in this function and is not modeled as an argument. UNSURE: 0x3a83126f is exactly
//   0.001f, used as a delta_time to run one immediate micro-update right after creation.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *particle_system_data; // 0x0087abd4
extern tag_instance *tag_instances;      // 0x0087bc14
extern ModelCollisionGeometryBSP *global_collision_bsp;             // 0x00746f90
extern random_seed effect_random_seed;    // 0x00719cd4
extern uint8_t *global_structure_bsp; // 0x00746f9c; +0xe4 is the per-leaf lookup table
    // (same name and type as the other eleven files in this module that touch it), see
                                    // src/physics/breakable_surface_damage_in_blast_radius.c

extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0
extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170
extern uint8_t particle_system_update(float delta_time, datum_index handle); // 0x4544f0, this
                                    // module; UNSURE of the real parameter order, see that file

uint8_t particle_system_new_type_states(datum_index handle)
{
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition =
        (ParticleSystem *)tag_instances[system->definition_index & 0xffff].data;
    uint8_t all_types_ok = 1;
    uint8_t any_type_ok = 0;
    int32_t leaf_index;
    int32_t i;

    leaf_index = bsp3d_node_find_leaf(global_collision_bsp, &system->position, 0);
    system->location.leaf_index = leaf_index;
    system->location.cluster_index = (leaf_index == -1) ? (int16_t)0xffff :
        *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) +
                      (leaf_index & 0x7fffffff) * 0x10 + 8); // UNSURE: global_structure_bsp is
                                    // an objects/structures-module global, kept as raw offset
                                    // arithmetic here, matching the object.location_cluster
                                    // resolution idiom used everywhere else in this module
    system->flags |= _particle_system_in_update_bit;

    if (0 < (int32_t)definition->particle_types.count) {
        for (i = 0; i < (int32_t)definition->particle_types.count; i++) {
            ParticleSystemType *type =
                &((ParticleSystemType *)definition->particle_types.pointer)[i];
            particle_system_type_state *state = &system->type_states[i];

            if (type->states.count == 0) {
                all_types_ok = 0;
            } else {
                ParticleSystemTypeStates *first_state =
                    (ParticleSystemTypeStates *)type->states.pointer;

                state->state_index = 0;
                state->next_state_index = -1;
                state->ping_pong_forward = 1;
                state->particle_count = 0;
                state->first_particle = (datum_index)0xffffffff;

                if (0 < (int32_t)type->states.count) {
                    float duration = random_real_range_seeded(&effect_random_seed,
                        first_state->duration_bounds[0], first_state->duration_bounds[1]);
                    any_type_ok = 1;
                    state->state_time_remaining = duration;
                    state->state_duration = duration;
                }
            }
        }

        if (any_type_ok) {
            if (all_types_ok) {
                particle_system_update(0.001f, handle);
            }
            return all_types_ok;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4538b0):

/* WARNING: Removing unreachable block (ram,0x0045399e) */

char FUN_004538b0(uint param_1)

{
  undefined2 *puVar1;
  float fVar2;
  bool bVar3;
  undefined2 uVar4;
  int iVar5;
  int iVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  char local_5;

  iVar8 = (param_1 & 0xffff) * 0x158;
  iVar9 = iVar8 + *(int *)(DAT_0087abd4 + 0x34);
  iVar8 = *(int *)((*(uint *)(iVar8 + 8 + *(int *)(DAT_0087abd4 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  local_5 = '\x01';
  bVar3 = false;
  iVar5 = FUN_005013a0();
  *(int *)(iVar9 + 0x18) = iVar5;
  if (iVar5 == -1) {
    uVar4 = 0xffff;
  }
  else {
    uVar4 = *(undefined2 *)(iVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  *(undefined2 *)(iVar9 + 0x1c) = uVar4;
  *(uint *)(iVar9 + 4) = *(uint *)(iVar9 + 4) | 2;
  sVar7 = 0;
  if (0 < *(int *)(iVar8 + 0x5c)) {
    iVar5 = 0;
    do {
      iVar6 = iVar5 * 0x80 + *(int *)(iVar8 + 0x60);
      puVar1 = (undefined2 *)(iVar5 * 0x40 + 0x58 + iVar9);
      if (*(int *)(iVar6 + 0x68) == 0) {
        local_5 = '\0';
      }
      else {
        *puVar1 = 0;
        puVar1[1] = 0xffff;
        *(undefined1 *)(puVar1 + 0x1c) = 1;
        puVar1[0x1d] = 0;
        *(undefined4 *)(puVar1 + 0x1e) = 0xffffffff;
        if (0 < *(int *)(iVar6 + 0x68)) {
          fVar2 = *(float *)(*(int *)(iVar6 + 0x6c) + 0x20);
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          bVar3 = true;
          fVar2 = fVar2 + (*(float *)(*(int *)(iVar6 + 0x6c) + 0x24) - fVar2) *
                          (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
          *(float *)(puVar1 + 2) = fVar2;
          *(float *)(puVar1 + 4) = fVar2;
        }
      }
      sVar7 = sVar7 + 1;
      iVar5 = (int)sVar7;
    } while (iVar5 < *(int *)(iVar8 + 0x5c));
    if (bVar3) {
      if (local_5 != '\0') {
        FUN_004544f0(0x3a83126f,param_1);
      }
      return local_5;
    }
  }
  return '\0';
}
#endif

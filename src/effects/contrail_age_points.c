// contrail_age_points  (Ghidra: FUN_0044d470; named per out/phase4/effects_types_notes.md, which
// refers to this address by this name directly throughout the contrail_point field table:
// "contrail_age_points 0x44d470 owns the state machine", "collapses runs of these from the tail
// of the list")
// address 0x44d470, size 944 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: types/tags.h ContrailPointState (duration[2] 0x00, transition_duration[2] 0x08,
// physics TagDependency 0x10, width 0x40, scale_flags 0x64) and Contrail.point_states (tag
// TagReflexive at 0x138, count at 0x138/pointer at 0x13c per the +0x138/+0x13c offsets read
// here); types/effects.h contrail_point (flags/state_index/age/inverse_duration/location/
// position/next_point) and contrail_point_flags (skip_render 0x01, in_transition 0x02,
// expired 0x04).
// register convention: __cdecl, contrail handle and delta time are both ordinary stack
// parameters (Ghidra's own param_1, param_2 -- no unaff/in_ registers in this one).
// UNSURE: point_physics_tick (the per-point render segment submit, outside this batch's address range)
// is called with every argument fully visible in the decompile, so they are preserved exactly as
// literal values/expressions with generic types rather than a guessed, over-specific prototype.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *contrail_data;       // 0x0087abec
extern data_array *contrail_point_data; // 0x0087abe8
extern tag_instance *tag_instances;     // 0x0087bc14
extern random_seed effect_random_seed;  // 0x00719cd4

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle
extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition,
    bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind,
    real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt); // 0x50b530, ESI velocity, stack

// Advances the age/state machine of every point on all four of a contrail's point lists,
// submitting a render segment for each live, non-transitional point, and trims fully-expired
// runs from the tail of each list.
void contrail_age_points(datum_index contrail_handle, real delta_time)
{
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    ContrailPointState *states = (ContrailPointState *)tag->point_states.pointer;
    int32_t state_count = (int32_t)tag->point_states.count;
    int list;

    for (list = 0; list < 4; list++) {
        datum_index visited[1023];
        int16_t visited_count = 0;
        datum_index current = self->first_point[list];

        while (current != k_datum_index_none) {
            contrail_point *point = &((contrail_point *)contrail_point_data->data)[(uint16_t)current];

            if ((point->flags & _contrail_point_expired_bit) == 0) {
                point->age = point->age + delta_time * point->inverse_duration;

                for (;;) {
                    for (;;) {
                        if (point->inverse_duration != 0.0f && point->age <= 1.0f) {
                            goto render;
                        }
                        if ((point->flags & _contrail_point_in_transition_bit) == 0) {
                            break;
                        }
                        {
                            ContrailPointState *next_state = &states[point->state_index + 1];
                            real duration;

                            point->state_index = point->state_index + 1;
                            point->age = 0.0f;
                            duration = next_state->duration[0];
                            if ((next_state->scale_flags & 1) != 0) {
                                duration = point->scale * next_state->duration[0];
                            }
                            {
                                real span = next_state->duration[1] - next_state->duration[0];
                                if ((next_state->scale_flags & 2) != 0) {
                                    span = span * point->scale;
                                }
                                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                                duration = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * span + duration;
                            }
                            point->inverse_duration = duration;
                            if (duration != 0.0f) {
                                point->inverse_duration = 1.0f / duration;
                            }
                            point->flags = point->flags & ~_contrail_point_in_transition_bit;
                        }
                    }

                    if (state_count <= point->state_index + 1) {
                        break;
                    }

                    {
                        ContrailPointState *current_state = &states[point->state_index];
                        real duration;

                        point->age = 0.0f;
                        duration = current_state->transition_duration[0];
                        if ((current_state->scale_flags & 4) != 0) {
                            duration = current_state->transition_duration[0] * point->scale;
                        }
                        {
                            real span = current_state->transition_duration[1] - current_state->transition_duration[0];
                            if ((current_state->scale_flags & 8) != 0) {
                                span = span * point->scale;
                            }
                            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                            duration = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * span + duration;
                        }
                        point->inverse_duration = duration;
                        if (duration != 0.0f) {
                            point->inverse_duration = 1.0f / duration;
                        }
                        point->flags = point->flags | _contrail_point_in_transition_bit;
                    }
                }

                point->flags = point->flags | _contrail_point_expired_bit;
            }

render:
            if ((point->flags & _contrail_point_skip_render_bit) != 0) {
                point->flags = point->flags & ~_contrail_point_skip_render_bit;
            } else if ((point->flags & _contrail_point_expired_bit) == 0) {
                ContrailPointState *current_state = &states[point->state_index];

                if (current_state->physics.tag_id.index != 0xffff || current_state->physics.tag_id.id != 0xffff) {
                    // 0x44d6f3..0x44d715: ESI = &point->velocity (+0x28), stack (0, the physics tag, &location,
                    // -1, &position, 0, 0, 0, width * 0.5, dt). The draft dropped the ESI velocity, shifting every
                    // argument one slot (crash: out_leaf -1, position NULL in the ambient probe).
                    point_physics_tick(&point->velocity, 0,
                        (PointPhysics *)tag_instances[current_state->physics.tag_id.index].data,
                        &point->location, 0xffffffff, &point->position, 0, 0, 0,
                        current_state->width * 0.5f, delta_time);
                }
            }

            visited[visited_count] = current;
            visited_count = visited_count + 1;
            current = point->next_point;
        }

        while (1 < visited_count) {
            contrail_point *tail = &((contrail_point *)contrail_point_data->data)[(uint16_t)visited[visited_count - 1]];
            contrail_point *before_tail = &((contrail_point *)contrail_point_data->data)[(uint16_t)visited[visited_count - 2]];

            if ((tail->flags & _contrail_point_expired_bit) == 0 ||
                (before_tail->flags & _contrail_point_expired_bit) == 0 ||
                tail->next_point != k_datum_index_none) {
                break;
            }

            before_tail->next_point = k_datum_index_none;
            self->point_count[list] = self->point_count[list] - 1;
            datum_delete(contrail_point_data, visited[visited_count - 1]);
            visited_count = visited_count - 1;
        }

        if (self->point_count[list] == 1 &&
            (((contrail_point *)contrail_point_data->data)[(uint16_t)self->first_point[list]].flags &
                _contrail_point_expired_bit) != 0) {
            datum_delete(contrail_point_data, self->first_point[list]);
            self->first_point[list] = k_datum_index_none;
            self->point_count[list] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x44d470):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_0044d470(uint param_1,float param_2)

{
  short *psVar1;
  float fVar2;
  byte bVar3;
  uint uVar4;
  float fVar5;
  int iVar6;
  int iVar7;
  uint *puVar8;
  float *pfVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  uint auStackY_21000 [32743];
  short *local_1028;
  uint local_1020;
  int local_101c;
  int local_1010;
  uint auStack_1000 [1023];
  undefined4 uStack_4;

  uStack_4 = 0x44d47a;
  iVar6 = (param_1 & 0xffff) * 0x44;
  iVar7 = iVar6 + *(int *)(DAT_0087abec + 0x34);
  iVar6 = *(int *)((*(uint *)(iVar6 + 4 + *(int *)(DAT_0087abec + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  local_1028 = (short *)(iVar7 + 0x2c);
  puVar8 = (uint *)(iVar7 + 0x34);
  local_101c = 0;
  local_1010 = 4;
  do {
    sVar10 = 0;
    local_1020 = *puVar8;
    if (*puVar8 != 0xffffffff) {
      do {
        iVar11 = (local_1020 & 0xffff) * 0x38;
        iVar12 = iVar11 + *(int *)(DAT_0087abe8 + 0x34);
        if ((*(byte *)(iVar11 + 2 + *(int *)(DAT_0087abe8 + 0x34)) & 4) == 0) {
          *(float *)(iVar12 + 4) = param_2 * *(float *)(iVar12 + 8) + *(float *)(iVar12 + 4);
          while( true ) {
            while( true ) {
              if ((*(float *)(iVar12 + 8) != 0.0) && (*(float *)(iVar12 + 4) <= 1.0))
              goto LAB_0044d6b2;
              if ((*(byte *)(iVar12 + 2) & 2) == 0) break;
              pfVar9 = (float *)((*(char *)(iVar12 + 3) + 1) * 0x68 + *(int *)(iVar6 + 0x13c));
              *(char *)(iVar12 + 3) = *(char *)(iVar12 + 3) + '\x01';
              *(undefined4 *)(iVar12 + 4) = 0;
              fVar2 = *pfVar9;
              fVar5 = fVar2;
              if (((uint)pfVar9[0x19] & 1) != 0) {
                fVar5 = *(float *)(iVar12 + 0xc) * fVar2;
              }
              fVar2 = pfVar9[1] - fVar2;
              if (((uint)pfVar9[0x19] & 2) != 0) {
                fVar2 = fVar2 * *(float *)(iVar12 + 0xc);
              }
              DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
              fVar5 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * fVar2 + fVar5;
              *(float *)(iVar12 + 8) = fVar5;
              if (fVar5 != 0.0) {
                *(float *)(iVar12 + 8) = 1.0 / fVar5;
              }
              *(byte *)(iVar12 + 2) = *(byte *)(iVar12 + 2) & 0xfd;
            }
            if (*(int *)(iVar6 + 0x138) <= *(char *)(iVar12 + 3) + 1) break;
            iVar11 = *(char *)(iVar12 + 3) * 0x68 + *(int *)(iVar6 + 0x13c);
            *(undefined4 *)(iVar12 + 4) = 0;
            fVar2 = *(float *)(iVar11 + 8);
            fVar5 = fVar2;
            if ((*(uint *)(iVar11 + 100) & 4) != 0) {
              fVar5 = fVar2 * *(float *)(iVar12 + 0xc);
            }
            fVar2 = *(float *)(iVar11 + 0xc) - fVar2;
            if ((*(uint *)(iVar11 + 100) & 8) != 0) {
              fVar2 = fVar2 * *(float *)(iVar12 + 0xc);
            }
            DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
            fVar5 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * fVar2 + fVar5;
            *(float *)(iVar12 + 8) = fVar5;
            if (fVar5 != 0.0) {
              *(float *)(iVar12 + 8) = 1.0 / fVar5;
            }
            *(byte *)(iVar12 + 2) = *(byte *)(iVar12 + 2) | 2;
          }
          *(byte *)(iVar12 + 2) = *(byte *)(iVar12 + 2) | 4;
        }
LAB_0044d6b2:
        bVar3 = *(byte *)(iVar12 + 2);
        if ((bVar3 & 1) == 0) {
          if ((bVar3 & 4) == 0) {
            iVar11 = *(char *)(iVar12 + 3) * 0x68 + *(int *)(iVar6 + 0x13c);
            uVar4 = *(uint *)(iVar11 + 0x1c);
            if (uVar4 != 0xffffffff) {
              FUN_0050b530(0,*(undefined4 *)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                           iVar12 + 0x14,0xffffffff,iVar12 + 0x1c,0,0,0,
                           *(float *)(iVar11 + 0x40) * 0.5,param_2);
            }
          }
        }
        else {
          *(byte *)(iVar12 + 2) = bVar3 & 0xfe;
        }
        uVar4 = *(uint *)(iVar12 + 0x34);
        iVar11 = (int)sVar10;
        sVar10 = sVar10 + 1;
        auStack_1000[iVar11] = local_1020;
        local_1020 = uVar4;
      } while (uVar4 != 0xffffffff);
      while (1 < sVar10) {
        sVar10 = sVar10 + -1;
        iVar11 = (auStack_1000[sVar10] & 0xffff) * 0x38 + *(int *)(DAT_0087abe8 + 0x34);
        iVar12 = (auStack_1000[sVar10 + -1] & 0xffff) * 0x38 + *(int *)(DAT_0087abe8 + 0x34);
        if ((((*(byte *)(iVar11 + 2) & 4) == 0) || ((*(byte *)(iVar12 + 2) & 4) == 0)) ||
           (*(int *)(iVar11 + 0x34) != -1)) break;
        *(undefined4 *)(iVar12 + 0x34) = 0xffffffff;
        psVar1 = (short *)(iVar7 + 0x2c + local_101c * 2);
        *psVar1 = *psVar1 + -1;
        datum_delete();
      }
    }
    if ((*local_1028 == 1) &&
       ((*(byte *)((*puVar8 & 0xffff) * 0x38 + 2 + *(int *)(DAT_0087abe8 + 0x34)) & 4) != 0)) {
      datum_delete();
      *puVar8 = 0xffffffff;
      *local_1028 = 0;
    }
    local_101c = local_101c + 1;
    puVar8 = puVar8 + 1;
    local_1028 = local_1028 + 1;
    local_1010 = local_1010 + -1;
    if (local_1010 == 0) {
      return;
    }
  } while( true );
}
#endif

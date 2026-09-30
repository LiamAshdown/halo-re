// actor_score_firing_positions_by_threat  (Ghidra: actor_score_firing_positions_by_threat, renamed)
// address 0x4112b0, size 1405 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: row 0 of the scoring table at 0x006555c0, kinds mask 0xffff, so it runs for
//   every goal kind. It reads actor.recognition[4].firing_position_index, the registered
//   danger block (flee_from_point / danger_segment_end / danger_center / danger_radius /
//   danger_unknown_294), query.danger_spheres and query.marked_group_mask, and it is the
//   function that pins the 0x3c candidate stride (its cursor steps by 0xf floats).
// register convention: the four arguments are the Ghidra-recognized stack parameters, but
//   the first arrives in a float slot and holds the actor index. The two distance helpers
//   and object_get_position are all invoked with no visible arguments -- see UNSURE.
//
// UNSURE: this is the least settled of the three scoring rules. The two
// point3d_distance_squared_to_segment calls and the segment3d_distance_squared_to_segment
// call take every operand in registers or on the x87 stack, so their arguments below are
// reconstructed from the live values, not read off the call sites. Same for
// object_get_position, whose result the original writes into the same locals that held the
// danger segment delta.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include "units.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern float point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction,
    real_point3d *point); // 0x4cde30, EAX, ECX, EDX   // 0x4cde30
extern float segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start,
    real_vector3d *a_direction, real_vector3d *b_direction); // 0x4cdef0, stack, EBX, ESI, EDI
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, EAX -> out, ECX -> object_index

// blam-cc: stack -> actor_index (in a float slot), query, count, candidates
// The always-on scoring rule. Scores are desirability, so every term below RAISES the
// score of a good candidate. It throws out candidates the actor has already recognized,
// rewards candidates that keep their distance from the registered danger (rejecting the
// ones inside its inner radius outright), applies the flat marked-group bonus, rewards
// distance from each query danger sphere, and finally -- when the query asks for it --
// rewards candidates that are not in the forward cone of the actor own vehicle.
void actor_score_firing_positions_by_threat(datum_index actor_index,
                                            actor_firing_position_query *query,
                                            uint16_t count,
                                            actor_firing_position_candidate *candidates)
{
    actor *self;
    actor_firing_position_candidate *c;
    real_point3d *p;
    real_vector3d segment;
    real_point3d vehicle_position;
    object *vehicle;
    float dx, dy, dz;
    float distance_squared;
    float radius;
    float bonus;
    float best_ratio;
    float ratio;
    float cosine;
    int16_t k;
    int32_t i;
    int32_t j;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        // Already-recognized positions are not worth moving to again.
        if (c->firing_position_index != -1) {
            for (k = 0; k < 4; k++) {
                if (c->firing_position_index == self->recognition[k].firing_position_index) {
                    c->rejected = 1;
                    if (query->collect_all == 0) {
                        c->valid = 0;
                        goto next_candidate;
                    }
                    break;
                }
            }
        }

        if (query->danger_active != 0) {
            p = (real_point3d *)c->position;
            segment.i = self->danger_segment_end.x - self->flee_from_point.x;
            segment.j = self->danger_segment_end.y - self->flee_from_point.y;
            segment.k = self->danger_segment_end.z - self->flee_from_point.z;

            dx = p->x - self->danger_center.x;
            dy = p->y - self->danger_center.y;
            dz = p->z - self->danger_center.z;
            radius = self->danger_radius + 2.5f;
            if (dx * dx + dy * dy + dz * dz < radius * radius) {
                // FIXED (0x4113ca..0x4113d0): EAX = flee_from_point (segment start), ECX = &segment, EDX = the point
                distance_squared = point3d_distance_squared_to_segment(&self->flee_from_point, &segment, p);
                bonus = 0.0f;
                if (self->danger_unknown_294 * self->danger_unknown_294 <= distance_squared) {
                    radius = self->danger_unknown_294 + 2.5f;
                    if (radius * radius <= distance_squared) {
                        bonus = 20.0f;
                    } else {
                        bonus = ((float)sqrt((double)distance_squared) - self->danger_unknown_294) * 8.0f;
                    }
                } else {
                    c->rejected = 1;
                    if (query->collect_all == 0) {
                        c->valid = 0;
                        goto next_candidate;
                    }
                }
                c->score = bonus + c->score;
            }

            // Second test: the actor own body position against the same danger, this time
            // rejecting candidates whose own short segment crosses the danger segment.
            real_vector3d scaled_direction; // [esp+0x28]: the candidate direction * 3 (0x411505..0x411527)

            scaled_direction.i = c->direction_from_actor.i * 3.0f;
            scaled_direction.j = c->direction_from_actor.j * 3.0f;
            scaled_direction.k = c->direction_from_actor.k * 3.0f;
            dx = self->body_position.x - self->danger_center.x;
            dy = self->body_position.y - self->danger_center.y;
            dz = self->body_position.z - self->danger_center.z;
            radius = self->danger_radius + 3.0f;
            if (dx * dx + dy * dy + dz * dz < radius * radius &&
                self->danger_unknown_294 < self->danger_unknown_2d4 &&
                self->danger_unknown_294 * self->danger_unknown_294 <
                    point3d_distance_squared_to_segment(&self->flee_from_point, &segment, &self->body_position) &&
                0.0001f < (c->direction_from_actor.i * 3.0f) * (c->direction_from_actor.i * 3.0f) +
                          (c->direction_from_actor.j * 3.0f) * (c->direction_from_actor.j * 3.0f) +
                          (c->direction_from_actor.k * 3.0f) * (c->direction_from_actor.k * 3.0f) &&
                // FIXED (0x411550..0x411563): stack = flee_from_point, EBX = the body position, ESI = the candidate
                //   direction * 3, EDI = &segment
                segment3d_distance_squared_to_segment(&self->flee_from_point, &self->body_position, &scaled_direction,
                    &segment) <
                    self->danger_unknown_294 * self->danger_unknown_294) {
                c->rejected = 1;
                if (query->collect_all == 0) {
                    c->valid = 0;
                    goto next_candidate;
                }
            }
        }

        p = (real_point3d *)c->position;
        // The ScenarioFiringPosition group_index sits immediately after its position.
        if ((query->marked_group_mask & (1u << (((uint8_t *)p)[12] & 0x1f))) != 0) {
            c->score = query->marked_group_penalty + c->score;
        }

        if (query->danger_sphere_count > 0) {
            best_ratio = 1.0f;
            for (j = 0; j < query->danger_sphere_count; j++) {
                actor_firing_position_danger_sphere *sphere = &query->danger_spheres[j];
                dx = sphere->position.x - p->x;
                dy = sphere->position.y - p->y;
                dz = sphere->position.z - p->z;
                ratio = (dx * dx + dy * dy + dz * dz) / (sphere->radius * sphere->radius);
                if (ratio < best_ratio) {
                    best_ratio = ratio;
                }
            }
            bonus = (best_ratio < 1.0f) ? (float)sqrt((double)best_ratio) * 10.0f : 10.0f;
            c->score = bonus + c->score;
        }

next_candidate:
        ;
    }

    if (query->check_vehicle_aim_cone == 0 || self->active_unit_index == (datum_index)0xffffffff) {
        return;
    }

    vehicle = (object *)((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
    object_get_position(&vehicle_position, self->active_unit_index);

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }
        p = (real_point3d *)c->position;
        dx = p->x - vehicle_position.x;
        dy = p->y - vehicle_position.y;
        dz = p->z - vehicle_position.z;
        distance_squared = dx * dx + dy * dy + dz * dz;
        if (distance_squared < 0.0001f || distance_squared >= 900.0f) {
            continue;
        }
        cosine = (dx * ((vehicle_object *)vehicle)->base.forward.i +
                  dy * ((vehicle_object *)vehicle)->base.forward.j +
                  dz * ((vehicle_object *)vehicle)->base.forward.k) / (float)sqrt((double)distance_squared);

        if ((query->vehicle_ignore_velocity == 0 &&
             ((vehicle_object *)vehicle)->base.velocity.k * ((vehicle_object *)vehicle)->base.velocity.k +
             ((vehicle_object *)vehicle)->base.velocity.j * ((vehicle_object *)vehicle)->base.velocity.j +
             ((vehicle_object *)vehicle)->base.velocity.i * ((vehicle_object *)vehicle)->base.velocity.i <=
                 0.0069444445f) ||
            distance_squared >= 64.0f || cosine >= 0.70710677f ||
            (c->rejected = 1, query->collect_all != 0)) {
            if (cosine >= 0.0f) {
                bonus = 15.0f;
                if (cosine > 0.8660254f) {
                    bonus = (cosine - 0.8660254f) * 111.96151f + 15.0f;
                }
            } else {
                bonus = 15.0f + cosine * 15.0f;
            }
            c->score = bonus + c->score;
        } else {
            c->valid = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4112b0):

void FUN_004112b0(float param_1,int param_2,ushort param_3,int param_4)

{
  float fVar1;
  float *pfVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  int iVar10;
  char *pcVar11;
  uint uVar12;
  int iVar13;
  float *pfVar14;
  float10 fVar15;
  uint local_20;
  float local_c;
  float local_8;
  float local_4;

  iVar10 = ((uint)param_1 & 0xffff) * 0x724;
  iVar13 = *(int *)(DAT_00880360 + 0x34) + iVar10;
  if (0 < (short)param_3) {
    local_20 = (uint)param_3;
    pfVar14 = (float *)(param_4 + 0x14);
    do {
      if (*(char *)(pfVar14 + 7) != '\0') {
        if (*(short *)(pfVar14 + -4) != -1) {
          sVar9 = 0;
          do {
            if (*(short *)(pfVar14 + -4) ==
                *(short *)(*(int *)(DAT_00880360 + 0x34) + iVar10 + 0x3ca + sVar9 * 4)) {
              *(undefined1 *)((int)pfVar14 + 0x1d) = 1;
              if (*(char *)(param_2 + 0x14) == '\0') {
                *(undefined1 *)(pfVar14 + 7) = 0;
                goto LAB_0041165e;
              }
              break;
            }
            sVar9 = sVar9 + 1;
          } while (sVar9 < 4);
        }
        if (*(char *)(param_2 + 0x40) != '\0') {
          pfVar2 = (float *)pfVar14[-5];
          local_c = *(float *)(iVar13 + 0x2c8) - *(float *)(iVar13 + 0x2b0);
          local_8 = *(float *)(iVar13 + 0x2cc) - *(float *)(iVar13 + 0x2b4);
          local_4 = *(float *)(iVar13 + 0x2d0) - *(float *)(iVar13 + 0x2b8);
          fVar1 = *pfVar2 - *(float *)(iVar13 + 0x2dc);
          fVar6 = pfVar2[1] - *(float *)(iVar13 + 0x2e0);
          fVar5 = pfVar2[2] - *(float *)(iVar13 + 0x2e4);
          fVar4 = *(float *)(iVar13 + 0x2d8) + 2.5;
          if (fVar1 * fVar1 + fVar6 * fVar6 + fVar5 * fVar5 < fVar4 * fVar4) {
            fVar15 = (float10)point3d_distance_squared_to_segment();
            fVar1 = (float)fVar15;
            fVar4 = 0.0;
            if (*(float *)(iVar13 + 0x294) * *(float *)(iVar13 + 0x294) <= fVar1) {
              fVar4 = *(float *)(iVar13 + 0x294) + 2.5;
              if (fVar4 * fVar4 <= fVar1) {
                fVar4 = 20.0;
              }
              else {
                fVar4 = (SQRT(fVar1) - *(float *)(iVar13 + 0x294)) * 8.0;
              }
            }
            else {
              *(undefined1 *)((int)pfVar14 + 0x1d) = 1;
              if (*(char *)(param_2 + 0x14) == '\0') {
                *(undefined1 *)(pfVar14 + 7) = 0;
                goto LAB_0041165e;
              }
            }
            pfVar14[9] = fVar4 + pfVar14[9];
          }
          fVar1 = *(float *)(iVar13 + 300) - *(float *)(iVar13 + 0x2dc);
          fVar6 = *(float *)(iVar13 + 0x130) - *(float *)(iVar13 + 0x2e0);
          fVar5 = *(float *)(iVar13 + 0x134) - *(float *)(iVar13 + 0x2e4);
          fVar4 = *(float *)(iVar13 + 0x2d8) + 3.0;
          if ((((fVar1 * fVar1 + fVar6 * fVar6 + fVar5 * fVar5 < fVar4 * fVar4) &&
               (*(float *)(iVar13 + 0x294) < *(float *)(iVar13 + 0x2d4))) &&
              (fVar1 = *(float *)(iVar13 + 0x294),
              fVar15 = (float10)point3d_distance_squared_to_segment(),
              (float10)fVar1 * (float10)fVar1 < fVar15)) &&
             (((0.0001 < pfVar14[-2] * 3.0 * pfVar14[-2] * 3.0 +
                         pfVar14[-1] * 3.0 * pfVar14[-1] * 3.0 + *pfVar14 * 3.0 * *pfVar14 * 3.0 &&
               (fVar1 = *(float *)(iVar13 + 0x294),
               fVar15 = (float10)segment3d_distance_squared_to_segment(iVar13 + 0x2b0),
               fVar15 < (float10)fVar1 * (float10)fVar1)) &&
              (*(undefined1 *)((int)pfVar14 + 0x1d) = 1, *(char *)(param_2 + 0x14) == '\0')))) {
            *(undefined1 *)(pfVar14 + 7) = 0;
            goto LAB_0041165e;
          }
        }
        pfVar2 = (float *)pfVar14[-5];
        if ((*(uint *)(param_2 + 0x48) & 1 << (*(byte *)(pfVar2 + 3) & 0x1f)) != 0) {
          pfVar14[9] = *(float *)(param_2 + 0x4c) + pfVar14[9];
        }
        iVar3 = *(int *)(param_2 + 0x50);
        if (0 < iVar3) {
          sVar9 = 0;
          param_1 = 1.0;
          if (0 < iVar3) {
            iVar7 = 0;
            do {
              iVar8 = iVar7 * 0x10 + param_2;
              fVar1 = *(float *)(iVar7 * 0x10 + 0x58 + param_2) - *pfVar2;
              fVar5 = *(float *)(iVar8 + 0x5c) - pfVar2[1];
              fVar4 = *(float *)(iVar8 + 0x60) - pfVar2[2];
              fVar1 = (fVar1 * fVar1 + fVar5 * fVar5 + fVar4 * fVar4) /
                      (*(float *)(iVar8 + 0x54) * *(float *)(iVar8 + 0x54));
              if (fVar1 < param_1) {
                param_1 = fVar1;
              }
              sVar9 = sVar9 + 1;
              iVar7 = (int)sVar9;
            } while (iVar7 < iVar3);
          }
          fVar1 = 10.0;
          if (param_1 < 1.0) {
            fVar1 = SQRT(param_1) * 10.0;
          }
          pfVar14[9] = fVar1 + pfVar14[9];
        }
      }
LAB_0041165e:
      pfVar14 = pfVar14 + 0xf;
      local_20 = local_20 - 1;
    } while (local_20 != 0);
  }
  if ((*(char *)(param_2 + 0x45) != '\0') && (*(uint *)(iVar13 + 0x158) != 0xffffffff)) {
    iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar13 + 0x158) & 0xffff) * 0xc
                     );
    object_get_position();
    if (0 < (short)param_3) {
      pcVar11 = (char *)(param_4 + 0x30);
      uVar12 = (uint)param_3;
      do {
        if (*pcVar11 != '\0') {
          pfVar14 = *(float **)(pcVar11 + -0x30);
          fVar1 = *pfVar14 - local_c;
          fVar4 = pfVar14[1] - local_8;
          fVar5 = pfVar14[2] - local_4;
          fVar6 = fVar1 * fVar1 + fVar4 * fVar4 + fVar5 * fVar5;
          if ((0.0001 <= ABS(fVar6)) && (fVar6 < 900.0)) {
            fVar1 = (fVar1 * *(float *)(iVar10 + 0x74) +
                    fVar4 * *(float *)(iVar10 + 0x78) + fVar5 * *(float *)(iVar10 + 0x7c)) /
                    SQRT(fVar6);
            if (((*(char *)(param_2 + 0x46) == '\0') &&
                (*(float *)(iVar10 + 0x70) * *(float *)(iVar10 + 0x70) +
                 *(float *)(iVar10 + 0x6c) * *(float *)(iVar10 + 0x6c) +
                 *(float *)(iVar10 + 0x68) * *(float *)(iVar10 + 0x68) <= 0.0069444445)) ||
               (((64.0 <= fVar6 || (0.70710677 <= fVar1)) ||
                (pcVar11[1] = '\x01', *(char *)(param_2 + 0x14) != '\0')))) {
              if (0.0 <= fVar1) {
                fVar4 = 15.0;
                if (0.8660254 < fVar1) {
                  fVar4 = (fVar1 - 0.8660254) * 111.96151 + 15.0;
                }
              }
              else {
                fVar4 = 15.0 - fVar1 * -15.0;
              }
              *(float *)(pcVar11 + 8) = fVar4 + *(float *)(pcVar11 + 8);
            }
            else {
              *pcVar11 = '\0';
            }
          }
        }
        pcVar11 = pcVar11 + 0x3c;
        uVar12 = uVar12 - 1;
      } while (uVar12 != 0);
    }
  }
  return;
}
#endif

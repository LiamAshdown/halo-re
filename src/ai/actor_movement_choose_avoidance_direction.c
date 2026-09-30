// actor_movement_choose_avoidance_direction  (Ghidra: FUN_004193d0; steers a moving actor round obstacles)
// address 0x4193d0, size 3829 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4193d0..0x41a2c4 (the draft declared both obstacle helpers with the wrong arguments,
//   so none of its ray tests or interpolations were real). Stack (actor, desired offset, out rotation vector,
//   out scale). Eight directions in the unit's (forward, left, up) frame are weighted: a bias towards the
//   direction chosen last time (+0x5d8), nine near rays (a blocked one lowers the directions it covers and
//   sets the closeness), two rays per direction (blocked: down by how close, clear: up once clear for 75
//   ticks, counters at +0x5c8), and a penalty against sideways motion. With the best direction known, an actor
//   facing well away from its goal (forwardness below -0.2) turns round towards the best direction (held at
//   +0x5f0 for 90 ticks); one heading roughly the right way leans away from the blocked side; one with a
//   clearly better direction to the side yaws towards it. The result is a rotation vector (axis * angle) and a
//   scale 0..2; nothing to do gives zero.
// blam-cc: stack -> (actor_index, desired, out_direction, out_scale)

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_point3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"


extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern uint32_t global_structure_bsp; // 0x00746f9c
extern uint32_t global_structure_collision_bsp; // 0x00746f98
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern double sqrt(double x);
extern double fabs(double x);

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, EAX, ECX
extern void actor_movement_collect_obstacle_candidates(actor_movement_context *context); // 0x418ce0, stack
extern int16_t actor_movement_test_obstacle_ray(real_vector3d *out_elevation, const float *sample,
    real_point3d *out_end_point, actor_movement_context *context, float *out_distance,
    uint8_t *out_clear_counter); // 0x418f70, EAX, ECX, EDX, EDI, stack; AX: 0 clear, 1 obstacle, 2 structure
extern uint8_t actor_avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples,
    int16_t count, const float *values, float *out_index, float *out_value); // 0x419240, ECX, EBX, stack
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX

extern float actor_avoidance_samples_a[16][7]; // 0x00880380, two rays per direction
extern float actor_avoidance_circle[8][3];     // 0x00880540, the eight directions (forward, left, up frame)
extern float actor_avoidance_samples_b[9][7];  // 0x008805a0, the near rays
extern const float actor_avoidance_near_weights[9][8]; // 0x00655748, per near ray, its weight on each direction
extern const float actor_avoidance_ray_weights[2];     // 0x00655868, 0.8 / 1.2

// The frame 0x4193d0 builds: F+0x20 result, F+0x3c closeness, F+0x58 the eight direction weights, F+0xa0 the
// context handed to the obstacle helpers.
void actor_movement_choose_avoidance_direction(uint32_t actor_index, real_vector3d *desired, real_vector3d *out_direction,
                                               float *out_scale)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    const real_vector3d *zero = global_origin3d_pointer;
    real_vector3d result = *zero;
    float out = 0.0f;
    datum_index unit_index = ((actor *)act)->active_unit_index;
    uint8_t *obj;
    actor_movement_context context;
    float weights[8];
    float closeness = 0.0f;
    real_vector3d elevation;
    real_point3d end_point;
    float distance;
    int16_t i;
    int16_t k;
    int16_t best;
    float best_weight;
    real_vector3d d;
    real_vector3d e;
    float forwardness;
    float along;
    float index_out;
    float delta;
    float scale;
    int16_t *best_saved = (int16_t *)(act + 0x5d8);
    int16_t *hold = (int16_t *)(act + 0x5f0);
    int16_t held;

    if (unit_index == k_datum_index_none) {
        unit_index = ((actor *)act)->unit_index;
        if (unit_index == k_datum_index_none) {
            *out_direction = result;
            *out_scale = 0.0f;
            return;
        }
    }
    obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    context.structure_bsp = global_structure_bsp;
    context.collision_bsp = global_structure_collision_bsp;
    context.unit_index = unit_index;
    object_get_position(&context.position, unit_index);
    context.forward = *(real_vector3d *)&((object *)obj)->forward.i;
    context.up = *(real_vector3d *)&((object *)obj)->up.i;
    // 0x4194c8: left = up x forward
    context.left.i = context.forward.k * context.up.j - context.up.k * context.forward.j;
    context.left.j = context.up.k * context.forward.i - context.up.i * context.forward.k;
    context.left.k = context.up.i * context.forward.j - context.forward.i * context.up.j;
    context.search_radius = 12.0f;
    context.ray_scale = 1.0f;
    actor_movement_collect_obstacle_candidates(&context);

    for (i = 0; i < 8; i++) {
        weights[i] = 0.0f;
    }
    if (*best_saved >= 0 && *best_saved < 8) { // keep turning the way we turned last time
        int16_t b = *best_saved;

        weights[b] += 0.4f;
        weights[(b + 1) & 7] += 0.32f;
        weights[(b + 2) & 7] += 0.2f;
        weights[(b + 7) & 7] += 0.32f;
        weights[(b + 6) & 7] += 0.2f;
    }

    // 0x419640: nine near rays; a blocked one pushes the directions it covers down (weights negative in the table)
    for (k = 0; k < 9; k++) {
        if (actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_b[k], &end_point, &context, &distance,
                                             0) > 0) {
            float v = 1.0f - distance;
            float c = v + v;

            if (c > 1.0f) {
                c = 1.0f;
            }
            for (i = 0; i < 8; i++) {
                weights[i] += c * actor_avoidance_near_weights[k][i];
            }
            if (!(closeness > v)) {
                closeness = v;
            }
        }
    }

    // 0x419700: two rays per direction, each with a byte counting how long it has been clear
    for (k = 0; k < 8; k++) {
        int16_t hit[2];
        float ray_distance[2];
        float acc = 0.0f;
        uint8_t blocked = 0;
        int16_t j;

        for (j = 0; j < 2; j++) {
            hit[j] = actor_movement_test_obstacle_ray(&elevation, actor_avoidance_samples_a[k * 2 + j], &end_point,
                                                      &context, &ray_distance[j], act + 0x5c8 + k * 2 + j);
        }
        for (j = 1; j >= 0; j--) {
            float weight = actor_avoidance_ray_weights[j];

            if (hit[j] == 0) {
                if (blocked) {
                    acc += 1.0f * weight;
                } else {
                    uint8_t clear_ticks = act[0x5c8 + k * 2 + j];
                    float v = 0.0f;

                    if (clear_ticks >= 75) {
                        v = 1.0f - 75.0f / (float)clear_ticks;
                        if (!(v > 0.0f)) {
                            v = 0.0f;
                        } else if (v > 1.0f) {
                            v = 1.0f;
                        }
                    }
                    acc += v * weight;
                }
            } else {
                float v = (1.0f - ray_distance[j]) * 2.0f;

                if (v > 1.0f) {
                    v = 1.0f;
                }
                blocked = 1;
                acc -= v * weight;
            }
        }
        weights[k] += acc;
        weights[(k + 1) & 7] += acc * 0.8f;
        weights[(k + 2) & 7] += acc * 0.5f;
        weights[(k + 7) & 7] += acc * 0.8f;
        weights[(k + 6) & 7] += acc * 0.5f;
    }

    // 0x4198e6: while moving, the directions against the sideways motion lose weight
    {
        float vx = ((object *)obj)->angular_velocity.i;
        float vy = ((object *)obj)->angular_velocity.j;
        float vz = ((object *)obj)->angular_velocity.k;
        float speed = (float)sqrt(vx * vx + vy * vy + vz * vz);

        if (speed > 0.02f) {
            float s = (speed - 0.02f) * 12.5f;
            real_vector3d motion;
            float length;

            if (s > 1.0f) {
                s = 1.0f;
            }
            s *= 0.8f;
            motion.i = 0.0f;
            motion.j = context.up.j * vy + context.up.k * vz + context.up.i * vx;
            motion.k = -(context.left.j * vy + context.left.k * vz + context.left.i * vx);
            length = (float)sqrt(motion.k * motion.k + motion.j * motion.j);
            if (fabs(length) >= 9.999999747378752e-05) {
                float inverse = 1.0f / length;
                float value;

                motion.i = 0.0f * inverse;
                motion.j *= inverse;
                motion.k *= inverse;
                if (length > 0.0f &&
                    actor_avoidance_interpolate_sample(&motion, (real_vector3d *)actor_avoidance_circle, 8, weights,
                                                       &index_out, &value) &&
                    value > 0.5f) {
                    for (i = 0; i < 8; i++) {
                        float dot = motion.k * actor_avoidance_circle[i][2] + motion.i * actor_avoidance_circle[i][0] +
                                    motion.j * actor_avoidance_circle[i][1];

                        if (dot < 0.0f) {
                            weights[i] += dot * s;
                        }
                    }
                }
            }
        }
    }

    best = -1;
    best_weight = -3.4028235e38f;
    for (i = 0; i < 8; i++) {
        if (weights[i] > best_weight) {
            best_weight = weights[i];
            best = i;
        }
    }

    // 0x419acf: the desired direction in the actor's frame
    d = *desired;
    e = *zero;
    forwardness = 1.0f;
    along = 0.0f;
    {
        float length = (float)sqrt(d.k * d.k + d.j * d.j + d.i * d.i);

        if (fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            d.i *= inverse;
            d.j *= inverse;
            d.k *= inverse;
            if (length > 0.0f) {
                float length_2;

                e.i = 0.0f;
                forwardness = context.forward.k * d.k + context.forward.j * d.j + context.forward.i * d.i;
                e.j = d.j * context.left.j + d.k * context.left.k + d.i * context.left.i;
                e.k = d.j * context.up.j + d.k * context.up.k + d.i * context.up.i;
                length_2 = (float)sqrt(e.k * e.k + e.j * e.j);
                if (fabs(length_2) >= 9.999999747378752e-05) {
                    float inverse_2 = 1.0f / length_2;

                    e.i = 0.0f * inverse_2;
                    e.j *= inverse_2;
                    e.k *= inverse_2;
                    if (length_2 > 0.0f) {
                        actor_avoidance_interpolate_sample(&e, (real_vector3d *)actor_avoidance_circle, 8, weights,
                                                           &index_out, &along);
                    }
                }
            }
        }
    }
    delta = best_weight - along;
    if (closeness > 0.6f) {
        float t = (closeness - 0.6f) * 2.5f;

        scale = (1.0f > t ? t : 1.0f) + 1.0f;
    } else {
        float t = closeness * 3.3333333f;

        scale = 1.0f > t ? t : 1.0f;
    }

    held = *hold;
    if (forwardness < -0.2f) {
        uint8_t turn_around = 0;

        if (held != -1 && held < 90) {
            turn_around = 1;
        } else {
            float vx = ((object *)obj)->angular_velocity.i;
            float vy = ((object *)obj)->angular_velocity.j;
            float vz = ((object *)obj)->angular_velocity.k;

            if (vx * vx + vy * vy + vz * vz > 0.0025f) {
                turn_around = (uint8_t)(delta > 2.0f && best_weight > 2.0f);
            } else {
                turn_around = (uint8_t)(scale > 0.5f);
            }
        }
        if (turn_around) {
            // 0x419e38: steer round towards the best direction, turning about desired x best
            real_vector3d *c = (real_vector3d *)actor_avoidance_circle[best];
            real_vector3d v;
            real_vector3d axis;
            float length;
            float t;

            *hold = held != -1 ? held + 1 : 0;
            v.i = context.forward.i * c->i + zero->i + context.left.i * c->j + context.up.i * c->k;
            v.j = context.forward.j * c->i + zero->j + context.left.j * c->j + context.up.j * c->k;
            v.k = context.forward.k * c->i + zero->k + context.left.k * c->j + context.up.k * c->k;
            axis.i = v.k * desired->j - v.j * desired->k;
            axis.j = v.i * desired->k - v.k * desired->i;
            axis.k = v.j * desired->i - v.i * desired->j;
            length = (float)sqrt(axis.k * axis.k + axis.j * axis.j + axis.i * axis.i);
            if (fabs(length) >= 9.999999747378752e-05) {
                float inverse = 1.0f / length;

                axis.i *= inverse;
                axis.j *= inverse;
                axis.k *= inverse;
                if (length > 0.0f) {
                    float angle = vector3d_angle_between_4cd4f0(&v, desired);

                    result.i = axis.i * angle;
                    result.j = axis.j * angle;
                    result.k = axis.k * angle;
                }
            }
            t = (2.0f - along) * 0.5f - 0.5f;
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            out = t > scale ? t : scale;
            *best_saved = best;
            goto done;
        }
    }

    *hold = -1;
    if (!(forwardness < 0.5f)) {
        // 0x41a143: going roughly the right way; lean sideways from the blocked side
        real_vector3d *c;
        float length;

        if (!(closeness > 0.0f)) {
            goto reset;
        }
        c = (real_vector3d *)actor_avoidance_circle[best];
        result = *zero;
        result.i = context.left.i * -c->k + result.i;
        result.j = context.left.j * -c->k + result.j;
        result.k = context.left.k * -c->k + result.k;
        result.i = c->j * context.up.i + result.i;
        result.j = context.up.j * c->j + result.j;
        result.k = context.up.k * c->j + result.k;
        length = (float)sqrt(result.k * result.k + result.j * result.j + result.i * result.i);
        if (fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            result.i *= inverse;
            result.j *= inverse;
            result.k *= inverse;
            if (length > 0.0f) {
                float k2 = scale * 1.0471976f;

                result.i *= k2;
                result.j *= k2;
                result.k *= k2;
            }
        }
        out = scale;
        *best_saved = best;
        goto done;
    }
    if (delta > 1.3f) {
        real_vector3d *c = (real_vector3d *)actor_avoidance_circle[best];

        if (e.k * c->k + e.j * c->j + e.i * c->i > 0.5f) {
            // 0x419df5: a clearly better direction to the side: yaw towards it
            float t = delta * 0.7692308f - 0.5f;
            float w;

            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            out = t > scale ? t : scale;
            w = 1.0471976f * out;
            if (e.k * c->j - e.j * c->k > 0.0f) {
                w = -w;
            }
            *best_saved = best;
            result.i = context.forward.i * w;
            result.j = context.forward.j * w;
            result.k = context.forward.k * w;
            goto done;
        }
    }
reset:
    out = 0.0f;
    *best_saved = -1;
done:
    *out_direction = result;
    *out_scale = out;
}

#if 0
Original Ghidra decompilation (0x4193d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void actor_movement_choose_avoidance_direction
               (uint param_1,float *param_2,float *param_3,float *param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined4 *puVar4;
  float fVar5;
  float fVar6;
  bool bVar7;
  undefined *puVar8;
  char cVar9;
  short sVar10;
  undefined2 uVar11;
  int iVar12;
  short sVar13;
  uint uVar14;
  float *pfVar15;
  uint uVar16;
  float *pfVar17;
  float *pfVar18;
  int iVar19;
  uint uVar20;
  byte *pbVar21;
  uint uVar22;
  uint uVar23;
  short *psVar24;
  float10 fVar25;
  undefined4 *local_60dc;
  float local_60d8;
  float local_60d4 [2];
  undefined4 *local_60cc;
  float local_60c8;
  float local_60c4;
  float local_60c0;
  float local_60bc;
  byte *local_60b8;
  float local_60b4;
  float *local_60b0;
  float local_60ac;
  float local_60a8;
  undefined4 local_60a4;
  float local_60a0;
  float local_609c;
  float local_6098;
  int local_6094;
  float local_6090 [18];
  undefined4 local_6048;
  undefined4 local_6044;
  uint local_6040;
  float local_6030;
  float local_602c;
  float local_6028;
  float local_6024;
  float local_6020;
  float local_601c;
  float local_6018;
  float local_6014;
  float local_6010;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x4193da;
  local_6090[8] = 0.0;
  local_60c8 = *(float *)PTR_DAT_00696714;
  local_60c4 = *(float *)(PTR_DAT_00696714 + 4);
  local_60c0 = *(float *)(PTR_DAT_00696714 + 8);
  local_60bc = (float)((param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34));
  local_6040 = *(uint *)((int)local_60bc + 0x158);
  if ((local_6040 == 0xffffffff) &&
     (local_6040 = *(uint *)((int)local_60bc + 0x18), puVar4 = (undefined4 *)0.0,
     local_6040 == 0xffffffff)) goto LAB_0041a299;
  iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_6040 & 0xffff) * 0xc);
  local_6048 = DAT_00746f9c;
  local_6044 = DAT_00746f98;
  local_6094 = iVar12;
  object_get_position();
  pfVar18 = (float *)(iVar12 + 0x74);
  local_6030 = *pfVar18;
  local_602c = *(float *)(iVar12 + 0x78);
  local_6028 = *(float *)(iVar12 + 0x7c);
  pfVar15 = (float *)(iVar12 + 0x80);
  local_6018 = *pfVar15;
  local_6014 = *(float *)(iVar12 + 0x84);
  local_6010 = *(float *)(iVar12 + 0x88);
  local_6024 = *(float *)(iVar12 + 0x7c) * *(float *)(iVar12 + 0x84) -
               *(float *)(iVar12 + 0x88) * *(float *)(iVar12 + 0x78);
  local_6020 = *(float *)(iVar12 + 0x88) * *pfVar18 - *pfVar15 * *(float *)(iVar12 + 0x7c);
  local_601c = *pfVar15 * *(float *)(iVar12 + 0x78) - *pfVar18 * *(float *)(iVar12 + 0x84);
  local_4 = 0x41400000;
  local_8 = 0x3f800000;
  actor_movement_collect_obstacle_candidates(&local_6048);
  sVar10 = *(short *)((int)local_60bc + 0x5d8);
  local_6090[0] = 0.0;
  local_6090[1] = 0.0;
  local_6090[2] = 0.0;
  local_6090[3] = 0.0;
  local_6090[4] = 0.0;
  local_6090[5] = 0.0;
  local_6090[6] = 0.0;
  local_60ac = 0.0;
  local_6090[7] = 0.0;
  if ((-1 < sVar10) && (sVar10 < 8)) {
    iVar12 = (int)sVar10;
    uVar14 = iVar12 + 1U & 0x80000007;
    if ((int)uVar14 < 0) {
      uVar14 = (uVar14 - 1 | 0xfffffff8) + 1;
    }
    uVar16 = iVar12 + 2U & 0x80000007;
    if ((int)uVar16 < 0) {
      uVar16 = (uVar16 - 1 | 0xfffffff8) + 1;
    }
    uVar20 = iVar12 + 7U & 0x80000007;
    if ((int)uVar20 < 0) {
      uVar20 = (uVar20 - 1 | 0xfffffff8) + 1;
    }
    uVar23 = iVar12 + 6U & 0x80000007;
    if ((int)uVar23 < 0) {
      uVar23 = (uVar23 - 1 | 0xfffffff8) + 1;
    }
    local_6090[iVar12] = local_6090[iVar12] + 0.4;
    local_6090[(short)uVar14] = local_6090[(short)uVar14] + 0.32000002;
    local_6090[(short)uVar16] = local_6090[(short)uVar16] + 0.2;
    local_6090[(short)uVar20] = local_6090[(short)uVar20] + 0.32000002;
    local_6090[(short)uVar23] = local_6090[(short)uVar23] + 0.2;
  }
  pfVar18 = (float *)&DAT_00655748;
  local_60dc = (undefined4 *)0x9;
  do {
    sVar10 = actor_movement_test_obstacle_ray(&local_60a8,0);
    if (0 < sVar10) {
      fVar2 = 1.0 - local_60a8;
      iVar12 = 8;
      pfVar15 = local_6090;
      pfVar17 = pfVar18;
      do {
        fVar3 = fVar2 + fVar2;
        if (1.0 < fVar2 + fVar2) {
          fVar3 = 1.0;
        }
        fVar1 = *pfVar17;
        pfVar17 = pfVar17 + 1;
        iVar12 = iVar12 + -1;
        *pfVar15 = fVar3 * fVar1 + *pfVar15;
        pfVar15 = pfVar15 + 1;
      } while (iVar12 != 0);
      if (local_60ac <= fVar2) {
        local_60ac = fVar2;
      }
    }
    pfVar18 = pfVar18 + 8;
    local_60dc = (undefined4 *)((int)local_60dc + -1);
  } while (local_60dc != (undefined4 *)0x0);
  local_60b0 = local_6090;
  local_60b8 = (byte *)((int)local_60bc + 0x5c9);
  uVar14 = 2;
  local_60cc = &DAT_00880380;
  local_60a8 = 1.12104e-44;
  do {
    local_60dc = &local_60a4;
    pfVar18 = &local_60d8;
    pbVar21 = local_60b8 + -1;
    local_60b4 = 2.8026e-45;
    do {
      uVar11 = actor_movement_test_obstacle_ray(pfVar18,pbVar21);
      *(undefined2 *)local_60dc = uVar11;
      local_60cc = local_60cc + 7;
      local_60dc = (undefined4 *)((int)local_60dc + 2);
      pbVar21 = pbVar21 + 1;
      pfVar18 = pfVar18 + 1;
      local_60b4 = (float)((int)local_60b4 + -1);
    } while (local_60b4 != 0.0);
    fVar2 = 0.0;
    bVar7 = false;
    psVar24 = (short *)((int)&local_60a4 + 2);
    iVar12 = 0;
    iVar19 = 2;
    pbVar21 = local_60b8;
    do {
      fVar3 = 1.0;
      if (*psVar24 == 0) {
        if (bVar7) {
LAB_004197f5:
          fVar2 = fVar3 * *(float *)((int)&DAT_0065586c + iVar12) + fVar2;
        }
        else if (*pbVar21 < 0x4b) {
          fVar2 = *(float *)((int)&DAT_0065586c + iVar12) * 0.0 + fVar2;
        }
        else {
          fVar3 = 1.0 - 75.0 / (float)*pbVar21;
          if (0.0 <= fVar3) {
            if (1.0 < fVar3) {
              fVar3 = 1.0;
            }
            goto LAB_004197f5;
          }
          fVar2 = *(float *)((int)&DAT_0065586c + iVar12) * 0.0 + fVar2;
        }
      }
      else {
        fVar3 = 1.0 - *(float *)((int)local_60d4 + iVar12);
        fVar3 = fVar3 + fVar3;
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
        bVar7 = true;
        fVar2 = fVar2 - fVar3 * *(float *)((int)&DAT_0065586c + iVar12);
      }
      psVar24 = psVar24 + -1;
      pbVar21 = pbVar21 + -1;
      iVar12 = iVar12 + -4;
      iVar19 = iVar19 + -1;
    } while (iVar19 != 0);
    uVar16 = uVar14 - 1 & 0x80000007;
    if ((int)uVar16 < 0) {
      uVar16 = (uVar16 - 1 | 0xfffffff8) + 1;
    }
    uVar20 = uVar14 & 0x80000007;
    if ((int)uVar20 < 0) {
      uVar20 = (uVar20 - 1 | 0xfffffff8) + 1;
    }
    uVar23 = uVar14 + 5 & 0x80000007;
    if ((int)uVar23 < 0) {
      uVar23 = (uVar23 - 1 | 0xfffffff8) + 1;
    }
    uVar22 = uVar14 + 4 & 0x80000007;
    if ((int)uVar22 < 0) {
      uVar22 = (uVar22 - 1 | 0xfffffff8) + 1;
    }
    *local_60b0 = fVar2 + *local_60b0;
    local_60b8 = local_60b8 + 2;
    local_60b0 = local_60b0 + 1;
    uVar14 = uVar14 + 1;
    local_6090[(short)uVar16] = fVar2 * 0.8 + local_6090[(short)uVar16];
    local_60dc = (undefined4 *)(fVar2 * 0.5);
    local_60a8 = (float)((int)local_60a8 + -1);
    local_6090[(short)uVar20] = (float)local_60dc + local_6090[(short)uVar20];
    local_6090[(short)uVar23] = fVar2 * 0.8 + local_6090[(short)uVar23];
    local_6090[(short)uVar22] = (float)local_60dc + local_6090[(short)uVar22];
  } while (local_60a8 != 0.0);
  fVar2 = SQRT(*(float *)(local_6094 + 0x94) * *(float *)(local_6094 + 0x94) +
               *(float *)(local_6094 + 0x90) * *(float *)(local_6094 + 0x90) +
               *(float *)(local_6094 + 0x8c) * *(float *)(local_6094 + 0x8c));
  local_60a8 = 0.0;
  if (0.02 < fVar2) {
    local_60a8 = (fVar2 - 0.02) * 12.5;
    if (1.0 < local_60a8) {
      local_60a8 = 1.0;
    }
    local_60a8 = local_60a8 * 0.8;
    fVar2 = local_6018 * *(float *)(local_6094 + 0x8c) +
            local_6010 * *(float *)(local_6094 + 0x94) + local_6014 * *(float *)(local_6094 + 0x90);
    fVar3 = -(local_6024 * *(float *)(local_6094 + 0x8c) +
             local_601c * *(float *)(local_6094 + 0x94) + local_6020 * *(float *)(local_6094 + 0x90)
             );
    fVar1 = SQRT(fVar2 * fVar2 + fVar3 * fVar3);
    if (0.0001 <= ABS(fVar1)) {
      fVar5 = 1.0 / fVar1;
      if (((0.0 < fVar1) &&
          (cVar9 = FUN_00419240(8,local_6090,&local_60dc,&local_60a4), cVar9 != '\0')) &&
         (0.5 < local_60a4)) {
        pfVar18 = local_6090;
        pfVar15 = (float *)&DAT_00880544;
        iVar12 = 8;
        do {
          fVar1 = fVar2 * fVar5 * *pfVar15 + fVar5 * 0.0 * pfVar15[-1] + fVar5 * fVar3 * pfVar15[1];
          if (fVar1 < 0.0) {
            *pfVar18 = fVar1 * local_60a8 + *pfVar18;
          }
          pfVar15 = pfVar15 + 3;
          pfVar18 = pfVar18 + 1;
          iVar12 = iVar12 + -1;
        } while (iVar12 != 0);
      }
    }
  }
  puVar8 = PTR_DAT_00696714;
  sVar10 = -1;
  local_60b8 = (byte *)0xff7fffff;
  sVar13 = 0;
  pfVar18 = local_6090;
  do {
    if ((float)local_60b8 < *pfVar18) {
      local_60b8 = (byte *)*pfVar18;
      sVar10 = sVar13;
    }
    sVar13 = sVar13 + 1;
    pfVar18 = pfVar18 + 1;
  } while (sVar13 < 8);
  fVar2 = *param_2;
  fVar3 = param_2[1];
  fVar1 = param_2[2];
  local_60d8 = *(float *)PTR_DAT_00696714;
  local_60d4[0] = *(float *)(PTR_DAT_00696714 + 4);
  local_60d4[1] = *(float *)(PTR_DAT_00696714 + 8);
  local_60b4 = 1.0;
  local_60b0 = (float *)0x0;
  fVar5 = SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1);
  if (0.0001 <= ABS(fVar5)) {
    fVar6 = 1.0 / fVar5;
    fVar2 = fVar2 * fVar6;
    fVar3 = fVar3 * fVar6;
    fVar1 = fVar1 * fVar6;
    if (0.0 < fVar5) {
      local_60d8 = 0.0;
      local_60b4 = local_6030 * fVar2 + local_602c * fVar3 + local_6028 * fVar1;
      local_60d4[0] = fVar2 * local_6024 + fVar1 * local_601c + fVar3 * local_6020;
      local_60d4[1] = fVar2 * local_6018 + fVar1 * local_6010 + fVar3 * local_6014;
      fVar2 = SQRT(local_60d4[0] * local_60d4[0] + local_60d4[1] * local_60d4[1]);
      if (0.0001 <= ABS(fVar2)) {
        fVar3 = 1.0 / fVar2;
        local_60d8 = fVar3 * 0.0;
        local_60d4[0] = local_60d4[0] * fVar3;
        local_60d4[1] = fVar3 * local_60d4[1];
        if (0.0 < fVar2) {
          FUN_00419240(8,local_6090,&local_60a4,&local_60b0);
        }
      }
    }
  }
  fVar2 = local_60bc;
  local_60dc = (undefined4 *)((float)local_60b8 - (float)local_60b0);
  if (local_60ac <= 0.6) {
    local_60cc = (undefined4 *)(local_60ac * 3.3333333);
    if (1.0 <= (float)local_60cc) {
      local_60cc = (undefined4 *)0x3f800000;
    }
  }
  else {
    fVar3 = (local_60ac - 0.6) * 2.5000002;
    if (1.0 <= fVar3) {
      fVar3 = 1.0;
    }
    local_60cc = (undefined4 *)(fVar3 + 1.0);
  }
  if (-0.2 <= local_60b4) goto LAB_00419d85;
  sVar13 = *(short *)((int)local_60bc + 0x5f0);
  if ((sVar13 == -1) || (0x59 < sVar13)) {
    if (*(float *)(local_6094 + 0x94) * *(float *)(local_6094 + 0x94) +
        *(float *)(local_6094 + 0x90) * *(float *)(local_6094 + 0x90) +
        *(float *)(local_6094 + 0x8c) * *(float *)(local_6094 + 0x8c) <= 0.0025000002) {
      if ((float)local_60cc <= 0.5) goto LAB_00419d85;
      goto LAB_00419e38;
    }
    if ((2.0 < (float)local_60dc) && (2.0 < (float)local_60b8)) goto LAB_00419e38;
LAB_00419d85:
    *(undefined2 *)((int)local_60bc + 0x5f0) = 0xffff;
    puVar4 = local_60cc;
    if (local_60b4 < 0.5) {
      if ((1.3 < (float)local_60dc) &&
         (iVar12 = (int)sVar10,
         0.5 < local_60d8 * (float)(&DAT_00880540)[iVar12 * 3] +
               local_60d4[0] * (float)(&DAT_00880544)[iVar12 * 3] +
               local_60d4[1] * (float)(&DAT_00880548)[iVar12 * 3])) {
        puVar4 = (undefined4 *)((float)local_60dc * 0.7692308 - 0.5);
        if (0.0 <= (float)puVar4) {
          if (1.0 < (float)puVar4) {
            puVar4 = (undefined4 *)0x3f800000;
          }
        }
        else {
          puVar4 = (undefined4 *)0x0;
        }
        if ((float)puVar4 <= (float)local_60cc) {
          puVar4 = local_60cc;
        }
        local_60c0 = (float)puVar4 * 1.0471976;
        if (0.0 < local_60d4[1] * (float)(&DAT_00880544)[iVar12 * 3] -
                  local_60d4[0] * (float)(&DAT_00880548)[iVar12 * 3]) {
          local_60c0 = -local_60c0;
        }
        *(short *)((int)local_60bc + 0x5d8) = sVar10;
        local_60c8 = local_6030 * local_60c0;
        local_60c4 = local_602c * local_60c0;
        local_60c0 = local_6028 * local_60c0;
        goto LAB_0041a299;
      }
LAB_0041a288:
      *(undefined2 *)((int)local_60bc + 0x5d8) = 0xffff;
      puVar4 = (undefined4 *)local_6090[8];
      goto LAB_0041a299;
    }
    if (local_60ac <= 0.0) goto LAB_0041a288;
    fVar3 = (float)(&DAT_00880544)[sVar10 * 3];
    fVar1 = -(float)(&DAT_00880548)[sVar10 * 3];
    local_60c8 = fVar3 * local_6018 + local_6024 * fVar1 + *(float *)puVar8;
    local_60c4 = local_6014 * fVar3 + local_6020 * fVar1 + *(float *)(puVar8 + 4);
    local_60c0 = local_6010 * fVar3 + local_601c * fVar1 + *(float *)(puVar8 + 8);
    fVar3 = SQRT(local_60c8 * local_60c8 + local_60c4 * local_60c4 + local_60c0 * local_60c0);
    if (0.0001 <= ABS(fVar3)) {
      fVar1 = 1.0 / fVar3;
      local_60c8 = local_60c8 * fVar1;
      local_60c4 = local_60c4 * fVar1;
      local_60c0 = local_60c0 * fVar1;
      if (0.0 < fVar3) {
        fVar3 = (float)local_60cc * 1.0471976;
        local_60c8 = local_60c8 * fVar3;
        local_60c4 = local_60c4 * fVar3;
        local_60c0 = local_60c0 * fVar3;
      }
    }
  }
  else {
LAB_00419e38:
    if (sVar13 == -1) {
      *(undefined2 *)((int)local_60bc + 0x5f0) = 0;
    }
    else {
      *(short *)((int)local_60bc + 0x5f0) = sVar13 + 1;
    }
    iVar12 = (int)sVar10;
    fVar3 = (float)(&DAT_00880540)[iVar12 * 3];
    fVar1 = (float)(&DAT_00880544)[iVar12 * 3];
    local_60bc = (float)(&DAT_00880548)[iVar12 * 3];
    fVar5 = local_60bc * local_6018 + fVar1 * local_6024 + local_6030 * fVar3 + *(float *)puVar8;
    fVar6 = local_6014 * local_60bc +
            local_6020 * fVar1 + local_602c * fVar3 + *(float *)(puVar8 + 4);
    fVar3 = local_6010 * local_60bc +
            local_601c * fVar1 + local_6028 * fVar3 + *(float *)(puVar8 + 8);
    local_60a0 = fVar3 * param_2[1] - fVar6 * param_2[2];
    local_609c = fVar5 * param_2[2] - fVar3 * *param_2;
    local_6098 = fVar6 * *param_2 - fVar5 * param_2[1];
    fVar3 = SQRT(local_60a0 * local_60a0 + local_609c * local_609c + local_6098 * local_6098);
    if (0.0001 <= ABS(fVar3)) {
      local_60d4[1] = 1.0 / fVar3;
      local_60d8 = local_60a0 * local_60d4[1];
      local_60d4[0] = local_609c * local_60d4[1];
      local_60d4[1] = local_6098 * local_60d4[1];
      if (0.0 < fVar3) {
        fVar25 = (float10)vector3d_angle_between_4cd4f0();
        local_60c8 = (float)((float10)local_60d8 * fVar25);
        local_60c4 = (float)((float10)local_60d4[0] * fVar25);
        local_60c0 = (float)((float10)local_60d4[1] * fVar25);
      }
    }
    puVar4 = (undefined4 *)((2.0 - (float)local_60b0) * 0.5 - 0.5);
    if (0.0 <= (float)puVar4) {
      if (1.0 < (float)puVar4) {
        puVar4 = (undefined4 *)0x3f800000;
      }
    }
    else {
      puVar4 = (undefined4 *)0x0;
    }
    if ((float)puVar4 <= (float)local_60cc) {
      *(short *)((int)fVar2 + 0x5d8) = sVar10;
      puVar4 = local_60cc;
      goto LAB_0041a299;
    }
  }
  *(short *)((int)fVar2 + 0x5d8) = sVar10;
LAB_0041a299:
  *param_3 = local_60c8;
  param_3[1] = local_60c4;
  param_3[2] = local_60c0;
  *param_4 = (float)puVar4;
  return;
}
#endif

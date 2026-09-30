// vehicle_calculate_wing_flex_controls  (Ghidra: FUN_005734d0; renamed from the phase2
//   proposal)
// address 0x5734d0, size 992 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary; dispatched from
//   vehicle_update's case 4)
// rewrite confidence: 0.1 -- the densest function in this batch by call count; the per-node
//   flex loop, scenario_location_water_surface_distance's role and the two matrix4x3_transform_vector /
//   matrix4x3_inverse_transform_vector calls (both register-only) are reproduced with raw
//   offsets and Ghidra's own float locals rather than invented field names.
// evidence: types/units.h vehicle_data.flags (0x4cc, bits 3 and 8), .forward_velocity (0x4d4),
//   .ground_lean (0x4ec), .airborne_ticks (0x4d0); types/units.h unit_data.throttle (0x278),
//   .driver_seat_power (0x338); types/objects.h object.velocity/forward/up/angular_velocity;
//   types/tags.h Vehicle.maximum_forward_speed (0x2f8), .speed_acceleration (0x300); the
//   physics.tag_id-at-0x8c idiom (contact-point count at Physics+0x74, per-node array at
//   Physics+0x78, stride 0x80, matching every sibling in this batch); callee
//   vehicle_create_hover_thruster_midpoint_effects (0x574bc0, this batch).
// register convention: unit object index in EAX (param_1); a turn/bank angle in a second
//   register (param_2, a float); a per-node output array pointer in ECX (param_3, stride
//   0x60); a contact-point array pointer in EDX (param_4, stride 0x130, matching the sibling
//   ground-effect functions).
//   // blam-cc: EAX -> unit_index, second register -> angle, ECX -> node_output, EDX -> contact_points
// UNSURE: essentially every field derived from the Physics tag's per-node array and the
//   matrix/vector helper calls; see the file header before trusting this file's math.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer;      // 0x00696720
extern real_point3d *global_origin3d_pointer;   // 0x00696714
extern real_vector3d *g_006966e4;           // 0x006966e4, UNSURE identity (a 2-float constant)

extern float scenario_location_water_surface_distance(void); // 0x53ee00, UNSURE signature, out of this module's range
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_inverse_transform_vector(real_matrix4x3 *m); // 0x4cc010, UNSURE args  // real signature (matrix4x3_inverse_transform_vector.c): void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); Ghidra recovered 1 of 3 args at this call site
extern void matrix4x3_transform_vector(real_matrix4x3 *m); // 0x4cbe50, UNSURE args at this call site  // real signature (matrix4x3_transform_vector.c): void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); Ghidra recovered 1 of 3 args at this call site
extern void vector3d_clamp_length(float max_length); // 0x459300, UNSURE args
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0, UNSURE args here
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void object_physics_tick(uint32_t unit_index, void *node_output, void *contact_points,
                          void *extra_force, void *extra_torque); // 0x507840, UNSURE signature

extern double sqrt(double x);
extern double fabs(double x);
extern float fabsf(float x);

// Computes per-marker flex/sway transforms (e.g. for wing or control-surface animation) on a
// flying vehicle-type unit each tick, and updates its ground_lean toward the fraction of
// contact points reporting partial contact.
// UNSURE: reproduced only partially; see the file header before trusting this file's math.
void vehicle_calculate_wing_flex_controls(uint32_t unit_index, float angle, uint8_t *node_output,
                                           uint8_t *contact_points)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    int32_t node_count = *(int32_t *)(physics_tag + 0x68);
    float bank_lookup = scenario_location_water_surface_distance(); // UNSURE args
    real_vector3d push = *(real_vector3d *)global_origin3d_pointer;
    real_vector3d angular = *(real_vector3d *)global_origin3d_pointer;
    int32_t i;

    for (i = 0; i < node_count; i++) {
        uint8_t *entry = node_output + i * 0x60;
        *(float *)(entry + 0x18) = unit->driver_seat_power;
        *(uint32_t *)(entry + 0x1c) = 0;
        *(uint32_t *)(entry + 0x20) = 0;
        *(uint32_t *)(entry + 0x24) = 0;
        *(uint32_t *)(entry + 0x28) = 0x3f800000;
    }

    if (bank_lookup >= 0.5f || obj->up.k <= -0.2f) {
        object_physics_tick(unit_index, node_output, contact_points, &push, &angular);
        goto ground_lean_update;
    }

    {
        real_matrix4x3 basis;

        matrix4x3_from_forward_up(&obj->up, &obj->forward, &basis);
        basis.position = obj->position;
        matrix4x3_inverse_transform_vector(&basis); // UNSURE args

        if (vehicle->ground_lean > 0.0f) {
            float accel = tag->maximum_forward_speed;
            real_vector3d desired;

            if ((vehicle->flags & 8) != 0) {
                accel *= 0.8f;
            }
            desired.i = accel * unit->throttle.i;
            desired.j = accel * unit->throttle.j;
            desired.k = 0.0f;

            if (vehicle->landing_ticks != 0 && fabsf(angle) > 0.7853982f) {
                float t = (float)vehicle->landing_ticks * 0.05f;
                if (t > 0.98f) t = 0.98f;
                accel = (1.0f - t) * tag->speed_acceleration;
            } else {
                accel = tag->speed_acceleration;
            }

            vector3d_clamp_length(accel); // UNSURE args
            matrix4x3_transform_vector(&basis); // UNSURE args

            {
                float scale = *(float *)(physics_tag + 8) * vehicle->ground_lean;
                push.i += desired.i * scale;
                push.j += desired.j * scale;
                push.k += desired.k * scale;
            }
        }

        if (vehicle->ground_lean > 0.0f) {
            float target = (float)sqrt(fabs((double)angle) * 0.0069813174) * ((angle >= 0.0f) ? 1.0f : -1.0f);
            if (fabsf(target) > 0.0001f && angle / target < 2.0f) {
                target = angle * 0.5f;
            }
            target -= (obj->angular_velocity.i * obj->up.i + obj->angular_velocity.j * obj->up.j +
                       obj->angular_velocity.k * obj->up.k);
            if (target < -0.0034906587f) target = -0.0034906587f;
            else if (target > 0.0034906587f) target = 0.0034906587f;
            target *= *(float *)(physics_tag + 0x58) * vehicle->ground_lean;
            angular.i += target * obj->up.i;
            angular.j += target * obj->up.j;
            angular.k += target * obj->up.k;
        }

        if (vehicle->ground_lean < 1.0f) {
            // UNSURE: this block (basis rebuild from forward/up cross products, an
            // idle-turn-style smoothing of a target throttle direction, and a final
            // acceleration/turn-rate blend) is reproduced only structurally; see file header.
            real_vector3d right;
            right.i = obj->up.j * obj->forward.k - obj->up.k * obj->forward.j;
            right.j = obj->forward.i * obj->up.k - obj->forward.k * obj->up.i;
            right.k = obj->forward.j * obj->up.i - obj->forward.i * obj->up.j;

            {
                float lateral_len = (float)sqrt((double)(obj->forward.j * obj->forward.j + obj->forward.i * obj->forward.i));
                float right_len = (float)sqrt((double)(right.i * right.i + right.j * right.j));
                float fx = obj->forward.i, fy = obj->forward.j;
                float rx = right.i, ry = right.j;

                if (fabsf(lateral_len) >= 0.0001f) {
                    fx *= 1.0f / lateral_len;
                    fy *= 1.0f / lateral_len;
                }
                if (fabsf(right_len) >= 0.0001f) {
                    rx *= 1.0f / right_len;
                    ry *= 1.0f / right_len;
                }

                {
                    float tx, ty;
                    if (obj->up.k <= 0.0f) {
                        tx = unit->throttle.i * 0.0015514038f + g_006966e4->i;
                        ty = unit->throttle.j * 0.0015514038f;
                    } else {
                        float f = ry * obj->forward.i;
                        tx = (g_006966e4->i - (fx * obj->forward.i + fy * obj->up.j)) -
                             (ry * obj->angular_velocity.i + rx * obj->angular_velocity.j) * 15.0f;
                        ty = (g_006966e4->j - (f + rx * obj->up.j)) -
                             -(fx * obj->angular_velocity.i + fy * obj->angular_velocity.j) * 15.0f;
                        {
                            float a = fabsf(tx * unit->throttle.i) + 1.0f;
                            float b = fabsf(ty * unit->throttle.j) + 1.0f;
                            if (a < 0.3f) a = 0.3f; else if (a > 2.5f) a = 2.5f;
                            if (b < 0.3f) b = 0.3f; else if (b > 2.5f) b = 2.5f;
                            {
                                float blend = (1.0f - obj->up.j) * 0.0038785094f;
                                float txx = blend * tx + a * unit->throttle.i * 0.0015514038f + g_006966e4->i;
                                float tyy = blend * ty;
                                tx = txx; ty = tyy;
                            }
                        }
                    }

                    {
                        float accel = tx * *(float *)(physics_tag + 0x54);
                        float turn = -((ty + 0.0f) * *(float *)(physics_tag + 0x50));
                        float fade = 1.0f - vehicle->ground_lean;

                        push.i += (turn * obj->forward.i + fx * accel + global_origin3d_pointer->x) * fade; // UNSURE
                        push.j += (turn * obj->forward.j + fy * accel + global_origin3d_pointer->y) * fade;
                        push.k += (turn * obj->forward.k + right.k * accel + global_origin3d_pointer->z) * fade;
                    }
                }
            }
        }

        if ((vehicle->flags & 8) != 0) {
            float along = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                          obj->forward.k * obj->velocity.k) / tag->maximum_forward_speed;
            real_vector3d cross;
            if (along < 0.0f) along = 0.0f;
            else if (along > 1.0f) along = 1.0f;

            cross.i = obj->up.j * obj->forward.k - obj->forward.j * obj->up.k;
            cross.j = obj->forward.i * obj->up.k - obj->up.i * obj->forward.k;
            cross.k = obj->up.i * obj->forward.j - obj->up.j * obj->forward.i;

            if (along > 0.0f) {
                float f1 = *(float *)(physics_tag + 0x54) * vehicle->ground_lean * along * -0.005817764f;
                float f2 = *(float *)(physics_tag + 8) * vehicle->ground_lean * along * 0.004f;
                push.i += cross.i * f1;
                push.j += cross.j * f1;
                push.k += cross.k * f1;
                angular.i += f2 * global_up3d_pointer->i;
                angular.j += f2 * global_up3d_pointer->j;
                angular.k += f2 * global_up3d_pointer->k;
            }

            if (vehicle->airborne_ticks != 0) {
                real_vector3d axis = cross;
                real length;
                vector3d_cross_product(&axis, &cross, &cross); // UNSURE operand identity
                length = vector3d_normalize_with_length(&axis);
                if (length > 0.0f) {
                    float t = 1.0f - (float)vehicle->airborne_ticks * 0.033333335f;
                    float scale, s1, s2;
                    if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
                    scale = (1.0f - vehicle->ground_lean) * *(float *)(physics_tag + 8) * t;
                    s1 = scale * 0.002f;
                    s2 = scale * 0.001f;
                    push.i += s2 * global_up3d_pointer->i + cross.i * s1;
                    push.j += s2 * global_up3d_pointer->j + cross.j * s1;
                    push.k += s2 * global_up3d_pointer->k + s1 * cross.k;
                }
            }
        }

        push.i *= unit->driver_seat_power; push.j *= unit->driver_seat_power; push.k *= unit->driver_seat_power;
        angular.i *= unit->driver_seat_power; angular.j *= unit->driver_seat_power; angular.k *= unit->driver_seat_power;
    }

    object_physics_tick(unit_index, node_output, contact_points, &push, &angular);

ground_lean_update:
    {
        float base = (vehicle->ground_lean >= 0.4f) ? vehicle->ground_lean : 0.4f;
        int32_t count = *(int32_t *)(physics_tag + 0x74);
        int32_t active = 0;
        int32_t partial = 0;
        int32_t j;
        float fraction = 0.0f;
        float delta;

        for (j = 0; j < count; j++) {
            if (*(int16_t *)(*(uint8_t **)(physics_tag + 0x78) + j * 0x80 + 0x20) != -1) {
                active++;
                if ((contact_points[j * 0x130] & 0x10) != 0) {
                    partial++;
                }
            }
        }
        if (active > 0) {
            fraction = (float)partial / (float)active;
        }

        fraction *= base;
        if (fraction < 0.0f) fraction = 0.0f;
        else if (fraction > 1.0f) fraction = 1.0f;

        delta = fraction - vehicle->ground_lean;
        if (delta <= 0.1f) {
            if (delta < -0.1f) {
                fraction = vehicle->ground_lean - 0.1f;
            }
        } else {
            fraction = vehicle->ground_lean + 0.1f;
        }
        vehicle->ground_lean = fraction;
    }

    vehicle_create_hover_thruster_midpoint_effects(unit_index);
}

#if 0
Original Ghidra decompilation (0x5734d0):

void FUN_005734d0(uint param_1,float param_2,int param_3,int param_4)

{
  float *pfVar1;
  float *pfVar2;
  float fVar3;
  uint *puVar4;
  float fVar5;
  uint uVar6;
  undefined *puVar7;
  int iVar8;
  short sVar9;
  short sVar10;
  int iVar11;
  float10 fVar12;
  undefined1 local_94 [40];
  uint local_6c;
  uint local_68;
  uint local_64;
  int local_5c;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  int local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_5c = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar11 = *(int *)((*(uint *)(local_5c + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_40 = iVar11;
  fVar12 = (float10)FUN_0053ee00();
  local_2c = *(float *)PTR_DAT_00696714;
  local_28 = *(float *)(PTR_DAT_00696714 + 4);
  local_24 = *(float *)(PTR_DAT_00696714 + 8);
  local_38 = *(float *)PTR_DAT_00696714;
  local_34 = *(float *)(PTR_DAT_00696714 + 4);
  local_30 = *(float *)(PTR_DAT_00696714 + 8);
  local_3c = (float)puVar4[0xce];
  sVar9 = 0;
  if (0 < *(int *)(iVar11 + 0x68)) {
    iVar8 = 0;
    do {
      iVar8 = iVar8 * 0x60 + param_3;
      *(float *)(iVar8 + 0x18) = local_3c;
      sVar9 = sVar9 + 1;
      *(undefined4 *)(iVar8 + 0x1c) = 0;
      *(undefined4 *)(iVar8 + 0x20) = 0;
      *(undefined4 *)(iVar8 + 0x24) = 0;
      *(undefined4 *)(iVar8 + 0x28) = 0x3f800000;
      iVar8 = (int)sVar9;
    } while (iVar8 < *(int *)(iVar11 + 0x68));
  }
  if ((fVar12 < (float10)0.5) && (-0.2 < (float)puVar4[0x22])) {
    pfVar2 = (float *)(puVar4 + 0x20);
    pfVar1 = (float *)(puVar4 + 0x1d);
    matrix4x3_from_forward_up(local_94);
    local_6c = puVar4[0x17];
    local_68 = puVar4[0x18];
    local_64 = puVar4[0x19];
    matrix4x3_inverse_transform_vector(local_94);
    if (0.0 < (float)puVar4[0x13b]) {
      fVar3 = *(float *)(local_5c + 0x2f8);
      if ((puVar4[0x133] & 8) != 0) {
        fVar3 = fVar3 * 0.8;
      }
      local_8 = *(float *)(local_5c + 0x300);
      local_54 = fVar3 * (float)puVar4[0x9f];
      local_18 = 0.0;
      local_20 = fVar3 * (float)puVar4[0x9e] - local_4c;
      local_1c = local_54 - local_48;
      if ((*(byte *)((int)puVar4 + 0x4d3) != 0) && (0.7853982 < ABS(param_2))) {
        local_10 = (float)(uint)*(byte *)((int)puVar4 + 0x4d3);
        fVar3 = (float)(int)local_10 * 0.05;
        if (0.98 < fVar3) {
          fVar3 = 0.98;
        }
        local_8 = (1.0 - fVar3) * local_8;
      }
      vector3d_clamp_length(local_8);
      matrix4x3_transform_vector(local_94);
      fVar3 = *(float *)(local_40 + 8) * (float)puVar4[0x13b];
      local_2c = local_20 * fVar3 + local_2c;
      local_28 = local_1c * fVar3 + local_28;
      local_24 = fVar3 * local_18 + local_24;
    }
    puVar7 = PTR_DAT_00696720;
    if (0.0 < (float)puVar4[0x13b]) {
      if (param_2 == 0.0) {
        local_8 = 0.0;
      }
      else if (0.0 <= param_2) {
        local_8 = 1.4013e-45;
      }
      else {
        local_8 = -NAN;
      }
      fVar3 = SQRT(ABS(param_2) * 0.0069813174) * (float)(int)local_8;
      if ((0.0001 < ABS(fVar3)) && (param_2 / fVar3 < 2.0)) {
        fVar3 = param_2 * 0.5;
      }
      fVar3 = fVar3 - ((float)puVar4[0x23] * *pfVar2 +
                      (float)puVar4[0x21] * (float)puVar4[0x24] +
                      (float)puVar4[0x22] * (float)puVar4[0x25]);
      if (-0.0034906587 <= fVar3) {
        if (0.0034906587 < fVar3) {
          fVar3 = 0.0034906587;
        }
      }
      else {
        fVar3 = -0.0034906587;
      }
      fVar3 = fVar3 * *(float *)(local_40 + 0x58) * (float)puVar4[0x13b];
      local_38 = fVar3 * *pfVar2 + local_38;
      local_34 = fVar3 * (float)puVar4[0x21] + local_34;
      local_30 = fVar3 * (float)puVar4[0x22] + local_30;
    }
    if ((float)puVar4[0x13b] < 1.0) {
      local_54 = *(float *)PTR_DAT_006966e4;
      local_50 = *(float *)(PTR_DAT_006966e4 + 4);
      local_4c = (float)puVar4[0x1f] * (float)puVar4[0x21] -
                 (float)puVar4[0x1e] * (float)puVar4[0x22];
      local_48 = *pfVar1 * (float)puVar4[0x22] - *pfVar2 * (float)puVar4[0x1f];
      local_44 = *pfVar2 * (float)puVar4[0x1e] - *pfVar1 * (float)puVar4[0x21];
      local_c = *pfVar1;
      local_8 = (float)puVar4[0x1e];
      fVar3 = SQRT(local_8 * local_8 + local_c * local_c);
      if (0.0001 <= ABS(fVar3)) {
        fVar3 = 1.0 / fVar3;
        local_c = local_c * fVar3;
        local_8 = local_8 * fVar3;
      }
      fVar3 = SQRT(local_4c * local_4c + local_48 * local_48);
      local_14 = local_4c;
      local_10 = local_48;
      if (0.0001 <= ABS(fVar3)) {
        fVar3 = 1.0 / fVar3;
        local_14 = local_4c * fVar3;
        local_10 = local_48 * fVar3;
      }
      if ((float)puVar4[0x22] <= 0.0) {
        fVar3 = (float)puVar4[0x9e] * 0.0015514038 + local_54;
        fVar5 = (float)puVar4[0x9f] * 0.0015514038;
      }
      else {
        fVar3 = local_14 * *pfVar2;
        local_14 = (*(float *)PTR_DAT_006966e4 - (local_c * *pfVar2 + local_8 * (float)puVar4[0x21])
                   ) - (local_14 * (float)puVar4[0x23] + local_10 * (float)puVar4[0x24]) * 15.0;
        local_10 = (*(float *)(PTR_DAT_006966e4 + 4) - (fVar3 + local_10 * (float)puVar4[0x21])) -
                   -(local_c * (float)puVar4[0x23] + local_8 * (float)puVar4[0x24]) * 15.0;
        if (local_14 * (float)puVar4[0x9e] == 0.0) {
          local_8 = 0.0;
        }
        else if (0.0 <= local_14 * (float)puVar4[0x9e]) {
          local_8 = 1.4013e-45;
        }
        else {
          local_8 = -NAN;
        }
        fVar3 = (float)(int)local_8;
        if (local_10 * (float)puVar4[0x9f] == 0.0) {
          local_8 = 0.0;
        }
        else if (0.0 <= local_10 * (float)puVar4[0x9f]) {
          local_8 = 1.4013e-45;
        }
        else {
          local_8 = -NAN;
        }
        fVar3 = ABS(local_14) * fVar3 + 1.0;
        if (0.3 <= fVar3) {
          if (2.5 < fVar3) {
            fVar3 = 2.5;
          }
        }
        else {
          fVar3 = 0.3;
        }
        fVar5 = ABS(local_10) * (float)(int)local_8 + 1.0;
        if (0.3 <= fVar5) {
          if (2.5 < fVar5) {
            fVar5 = 2.5;
          }
        }
        else {
          fVar5 = 0.3;
        }
        local_50 = fVar5 * (float)puVar4[0x9f] * 0.0015514038 + local_50;
        fVar5 = (1.0 - (float)puVar4[0x22]) * 0.0038785094;
        fVar3 = fVar5 * local_14 + fVar3 * (float)puVar4[0x9e] * 0.0015514038 + local_54;
        fVar5 = fVar5 * local_10;
      }
      fVar3 = fVar3 * *(float *)(local_40 + 0x54);
      local_20 = local_4c * fVar3 + *(float *)PTR_DAT_00696714;
      fVar5 = -((fVar5 + local_50) * *(float *)(local_40 + 0x50));
      local_1c = fVar5 * (float)puVar4[0x1e] + local_48 * fVar3 + *(float *)(PTR_DAT_00696714 + 4);
      local_18 = fVar5 * (float)puVar4[0x1f] + local_44 * fVar3 + *(float *)(PTR_DAT_00696714 + 8);
      local_8 = 1.0 - (float)puVar4[0x13b];
      local_38 = (fVar5 * *pfVar1 + local_20) * local_8 + local_38;
      local_34 = local_1c * local_8 + local_34;
      local_30 = local_18 * local_8 + local_30;
    }
    if ((puVar4[0x133] & 8) != 0) {
      fVar3 = (*pfVar1 * (float)puVar4[0x1a] +
              (float)puVar4[0x1e] * (float)puVar4[0x1b] + (float)puVar4[0x1f] * (float)puVar4[0x1c])
              / *(float *)(local_5c + 0x2f8);
      if (0.0 <= fVar3) {
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = 0.0;
      }
      local_4c = (float)puVar4[0x21] * (float)puVar4[0x1f] -
                 (float)puVar4[0x1e] * (float)puVar4[0x22];
      local_48 = *pfVar1 * (float)puVar4[0x22] - *pfVar2 * (float)puVar4[0x1f];
      local_44 = (float)puVar4[0x1e] * *pfVar2 - (float)puVar4[0x21] * *pfVar1;
      if (0.0 < fVar3) {
        fVar5 = *(float *)(local_40 + 0x54) * (float)puVar4[0x13b] * fVar3 * -0.005817764;
        local_38 = local_4c * fVar5 + local_38;
        local_34 = local_48 * fVar5 + local_34;
        local_30 = local_44 * fVar5 + local_30;
        fVar3 = *(float *)(local_40 + 8) * (float)puVar4[0x13b] * fVar3 * 0.004;
        local_2c = fVar3 * *(float *)PTR_DAT_00696720 + local_2c;
        local_28 = fVar3 * *(float *)(PTR_DAT_00696720 + 4) + local_28;
        local_24 = fVar3 * *(float *)(PTR_DAT_00696720 + 8) + local_24;
      }
      uVar6 = puVar4[0x134];
      local_20 = local_4c;
      local_1c = local_48;
      local_18 = local_44;
      if ((byte)uVar6 != 0) {
        vector3d_cross_product(&local_4c);
        fVar12 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 < fVar12) {
          local_10 = (float)(uint)(byte)uVar6;
          fVar3 = 1.0 - (float)(int)local_10 * 0.033333335;
          if (0.0 <= fVar3) {
            if (1.0 < fVar3) {
              fVar3 = 1.0;
            }
          }
          else {
            fVar3 = 0.0;
          }
          fVar3 = (1.0 - (float)puVar4[0x13b]) * *(float *)(local_40 + 8) * fVar3;
          fVar5 = fVar3 * 0.002;
          fVar3 = fVar3 * 0.001;
          local_2c = fVar3 * *(float *)puVar7 + local_20 * fVar5 + local_2c;
          local_28 = fVar3 * *(float *)(puVar7 + 4) + local_1c * fVar5 + local_28;
          local_24 = fVar3 * *(float *)(puVar7 + 8) + fVar5 * local_18 + local_24;
        }
      }
    }
    local_2c = local_2c * local_3c;
    local_28 = local_28 * local_3c;
    local_24 = local_24 * local_3c;
    local_38 = local_38 * local_3c;
    local_34 = local_34 * local_3c;
    local_30 = local_30 * local_3c;
    iVar11 = local_40;
  }
  FUN_00507840(param_1,param_3,param_4,&local_2c,&local_38);
  if (0.4 <= (float)puVar4[0x22]) {
    fVar3 = (float)puVar4[0x22];
  }
  else {
    fVar3 = 0.4;
  }
  local_10 = *(float *)(iVar11 + 0x74);
  fVar5 = 0.0;
  sVar9 = 0;
  sVar10 = 0;
  local_8 = 0.0;
  if (0 < (int)local_10) {
    iVar8 = 0;
    do {
      if ((*(short *)(iVar8 * 0x80 + 0x20 + *(int *)(iVar11 + 0x78)) != -1) &&
         (sVar9 = sVar9 + 1, (*(byte *)(iVar8 * 0x130 + param_4) & 0x10) != 0)) {
        local_8 = (float)((int)local_8 + 1);
      }
      sVar10 = sVar10 + 1;
      iVar8 = (int)sVar10;
    } while (iVar8 < (int)local_10);
    if (0 < sVar9) {
      local_10 = (float)(int)sVar9;
      fVar5 = (float)(int)local_8._0_2_ / (float)(int)local_10;
    }
  }
  fVar5 = fVar5 * fVar3;
  if (0.0 <= fVar5) {
    if (1.0 < fVar5) {
      fVar5 = 1.0;
    }
  }
  else {
    fVar5 = 0.0;
  }
  if (fVar5 - (float)puVar4[0x13b] <= 0.1) {
    if (fVar5 - (float)puVar4[0x13b] < -0.1) {
      fVar5 = (float)puVar4[0x13b] - 0.1;
    }
  }
  else {
    fVar5 = (float)puVar4[0x13b] + 0.1;
  }
  puVar4[0x13b] = (uint)fVar5;
  vehicle_create_hover_thruster_midpoint_effects(param_1);
  return;
}
#endif

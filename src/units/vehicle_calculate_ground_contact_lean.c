// vehicle_calculate_ground_contact_lean  (Ghidra: FUN_00573f60; renamed from the phase2
//   proposal)
// address 0x573f60, size 1279 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary; dispatched from
//   vehicle_calculate_mounted_controls_dispatch's "not > 0" branch)
// rewrite confidence: 0.1 -- the densest function in this batch: several register-dropped
//   matrix3x3/quaternion calls in the middle (matrix3x3_transpose, matrix3x3_multiply,
//   quaternion_from_matrix3x3, quaternion_to_axis_angle) could not be bound to arguments at
//   all, and Physics-tag fields at +8/+0x50/+0x54/+0x58/+0x68 are not documented anywhere.
//   Reproduced with Ghidra's own float locals rather than invented names for that section.
// evidence: types/units.h vehicle_data.forward_velocity (0x4d4), .ground_contact_fraction
//   (0x4f0); types/units.h unit_data.desired_facing_vector (0x224), .unknown_338 (0x338),
//   .driver_unit_index (0x324); types/objects.h object.velocity/forward/up/angular_velocity;
//   types/tags.h Vehicle.maximum_forward_speed/maximum_reverse_speed (0x2f8/0x2fc),
//   .speed_acceleration/.speed_deceleration (0x300/0x304), .maximum_left_turn (0x308),
//   .turn_rate (0x314), .fixed_gun_pitch (0x364, used as a fixed rotation angle here); callee
//   FUN_00572a90 (this batch, out of scope, a math helper) and object_physics_tick (established
//   5-argument shape elsewhere in this batch, though here it is called with an extra pointer
//   pair that does not fit that shape -- see UNSURE).
// register convention: unit object index in EAX (param_1); an output record pointer in ECX
//   (param_2, byte-offset-addressed); an output transform pointer in EDX (param_3).
//   // blam-cc: EAX -> unit_index, ECX -> out_record, EDX -> out_transform
// UNSURE: essentially every field derived from the Physics tag and the quaternion section; see
//   file header.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer;      // 0x00696720
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_matrix4x3 *g_00696738;          // 0x00696738, UNSURE identity (a constant 4x3 basis)

extern void object_physics_tick(uint32_t unit_index, void *out_record, void *out_transform,
                          void *param_4, void *param_5); // 0x507840, UNSURE signature, differs
                          // from the 5-uint32 shape used elsewhere in this batch
extern void FUN_00572a90(float a, float b); // 0x572a90, this batch (out of scope, math helper)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_rotate_pair_in_plane(float sin_angle, float cos_angle); // 0x4cd790, UNSURE  // real signature (vector3d_rotate_pair_in_plane.c): void vector3d_rotate_pair_in_plane(real_vector3d *a, real_vector3d *b, real sin_angle, real cos_angle); Ghidra recovered 2 of 4 args at this call site
extern void vector3d_rotate_about_axis_perpendicular(float sin_angle, float cos_angle); // 0x4cd700, UNSURE  // real signature (vector3d_rotate_about_axis_perpendicular.c): void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 2 of 4 args at this call site
extern void matrix3x3_from_forward_up(void *out); // 0x4cc560, UNSURE args  // real signature (matrix3x3_from_forward_up.c): void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out); Ghidra recovered 1 of 3 args at this call site
extern void matrix3x3_transpose(void *m); // 0x4cc500, UNSURE args  // real signature (matrix3x3_transpose.c): void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in); Ghidra recovered 1 of 2 args at this call site
extern void matrix3x3_multiply(void *m); // 0x4cc5f0, UNSURE args  // real signature (matrix3x3_multiply.c): void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b); Ghidra recovered 1 of 3 args at this call site
extern void quaternion_from_matrix3x3(float *out_quat); // 0x4cc780, UNSURE args  // real signature (quaternion_from_matrix3x3.c): real_quaternion * quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out); Ghidra recovered 1 of 2 args at this call site
extern void quaternion_to_axis_angle(void); // 0x4cdb90, UNSURE args  // real signature (quaternion_to_axis_angle.c): void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out); Ghidra recovered 0 of 3 args at this call site
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);

// Computes a ground-contact-relative lean transform for a vehicle when its supporting object's
// physics type is 2, otherwise passes through unchanged (via object_physics_tick).
// UNSURE: reproduced only partially; see the file header before trusting this file's math.
void vehicle_calculate_ground_contact_lean(uint32_t unit_index, void *out_record, void *out_transform)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;

    if (*(int32_t *)(physics_tag + 0x68) != 2) {
        object_physics_tick(unit_index, 0, out_transform, 0, 0);
        return;
    }

    {
        real_vector3d scaled_forward;
        real_vector3d local_54;
        float speed_fraction;
        real_matrix4x3 basis; // local_48, 72 bytes -- UNSURE exact size/shape

        scaled_forward.i = vehicle->forward_velocity * obj->forward.i;
        scaled_forward.j = vehicle->forward_velocity * obj->forward.j;
        scaled_forward.k = vehicle->forward_velocity * obj->forward.k;

        speed_fraction = (vehicle->forward_velocity <= 0.0f)
            ? -(vehicle->forward_velocity / tag->maximum_reverse_speed)
            : (vehicle->forward_velocity / tag->maximum_forward_speed);
        FUN_00572a90(speed_fraction * tag->speed_acceleration, speed_fraction * tag->speed_deceleration);

        {
            float physics8 = *(float *)(physics_tag + 8);
            local_54.i = scaled_forward.i /* UNSURE: reuses local_60/5c/58 from FUN_00572a90's
                                              output, not modeled here */ * physics8 * unit->unknown_338;
            local_54.j = scaled_forward.j * physics8 * unit->unknown_338;
            local_54.k = scaled_forward.k * physics8 * unit->unknown_338;
        }

        matrix3x3_from_forward_up(&basis);

        {
            real_vector3d facing = unit->desired_facing_vector;
            real_vector3d axis;
            real length;
            uint32_t driver = unit->driver_unit_index;

            axis.i = facing.i * -facing.k + global_up3d_pointer->i;
            axis.j = facing.j * -facing.k + global_up3d_pointer->j;
            axis.k = -facing.k * facing.k + global_up3d_pointer->k;
            length = vector3d_normalize_with_length(&axis);
            if (length == 0.0f) {
                axis = *global_forward3d_pointer;
            }

            if (driver != 0xffffffff) {
                object *driver_obj = ((object_header *)object_data->data)[driver & 0xffff].data;
                (void)driver_obj; // UNSURE: only used to re-derive an index below
            }
            {
                object *ref_obj = (driver != 0xffffffff)
                    ? ((object_header *)object_data->data)[driver & 0xffff].data : obj;
                unit_data *ref_unit = (unit_data *)((uint8_t *)ref_obj + k_unit_data_offset);
                if (ref_unit->actor_index == k_datum_index_none) {
                    double c = cos((double)tag->fixed_gun_pitch);
                    double s = sin((double)tag->fixed_gun_pitch);
                    vector3d_rotate_pair_in_plane((float)s, (float)c);
                }
            }

            {
                double turn_angle = ((double)(facing.i * obj->velocity.j - facing.j * obj->velocity.i) /
                                      (double)tag->maximum_forward_speed) * (double)tag->maximum_left_turn;
                double c = cos(turn_angle);
                double s = sin(turn_angle);
                vector3d_rotate_about_axis_perpendicular((float)s, (float)c);
            }

            {
                real_vector3d cross;
                cross.i = axis.j * facing.k - axis.k * facing.j;
                cross.j = axis.k * facing.i - axis.i * facing.k;
                cross.k = axis.j * facing.i - axis.i * facing.j; // UNSURE: matches Ghidra literally
                matrix3x3_transpose(&basis);
                matrix3x3_multiply(&basis);
                quaternion_from_matrix3x3((float *)&cross);
                quaternion_to_axis_angle();
            }
        }

        {
            float scale = (float)(-unit->unknown_338 * tag->turn_rate * 0.31830987);
            float avg = (*(float *)(physics_tag + 0x58) + *(float *)(physics_tag + 0x54) +
                         *(float *)(physics_tag + 0x50)) * 0.33333334f;
            real_vector3d out_vec;
            float speed;

            out_vec.k = scaled_forward.k * scale - obj->angular_velocity.k; // UNSURE ordering
            out_vec.i = (scaled_forward.i * scale - obj->angular_velocity.i) * avg * unit->unknown_338;
            out_vec.j = (scaled_forward.j * scale - obj->angular_velocity.j) * avg * unit->unknown_338;
            out_vec.k = out_vec.k * avg * unit->unknown_338;

            speed = (float)(sqrt((double)(obj->angular_velocity.k * obj->angular_velocity.k +
                                          obj->angular_velocity.j * obj->angular_velocity.j +
                                          obj->angular_velocity.i * obj->angular_velocity.i)) /
                             tag->turn_rate);

            {
                float delta;
                if (speed <= vehicle->ground_contact_fraction) {
                    float bound = vehicle->ground_contact_fraction * vehicle->ground_contact_fraction * 0.05f;
                    if (bound <= 0.005f) bound = 0.005f;
                    bound = -bound;
                    delta = speed - vehicle->ground_contact_fraction;
                    if (delta <= bound) {
                        delta = bound;
                    }
                } else {
                    float bound = (1.0f - vehicle->ground_contact_fraction) * (1.0f - vehicle->ground_contact_fraction) * 0.2f;
                    if (bound < 0.01f) bound = 0.01f;
                    else if (bound > 0.05f) bound = 0.05f;
                    delta = speed - vehicle->ground_contact_fraction;
                    if (delta > bound) {
                        delta = bound;
                    }
                }
                vehicle->ground_contact_fraction += delta;
            }

            *(float *)((uint8_t *)out_record + 0x18) = unit->unknown_338;
            *(real_matrix4x3 *)((uint8_t *)out_record + 0x1c) = *g_00696738; // UNSURE partial copy
            *(float *)((uint8_t *)out_record + 0x78) = unit->unknown_338;
            *(real_matrix4x3 *)((uint8_t *)out_record + 0x7c) = *g_00696738; // UNSURE partial copy

            object_physics_tick(unit_index, out_record, out_transform, &local_54, &out_vec);
        }
    }
}

#if 0
Original Ghidra decompilation (0x573f60):

void FUN_00573f60(uint param_1,int param_2,undefined4 param_3)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  undefined *puVar6;
  undefined4 uVar7;
  int iVar8;
  float10 fVar9;
  float10 fVar10;
  float local_9c;
  float local_94;
  float local_90;
  float local_8c;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  undefined1 local_48 [72];

  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(iVar8 + 8 + *(int *)(DAT_008603b0 + 0x34));
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*(uint *)(iVar3 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar4 + 0x68) != 2) {
    FUN_00507840(param_1,0,param_3,0,0);
    return;
  }
  local_8c = (float)puVar2[0x135];
  local_94 = local_8c * (float)puVar2[0x1d];
  local_90 = local_8c * (float)puVar2[0x1e];
  local_8c = local_8c * (float)puVar2[0x1f];
  if ((float)puVar2[0x135] <= 0.0) {
    fVar1 = -((float)puVar2[0x135] / *(float *)(iVar3 + 0x2fc));
  }
  else {
    fVar1 = (float)puVar2[0x135] / *(float *)(iVar3 + 0x2f8);
  }
  FUN_00572a90(fVar1 * *(float *)(iVar3 + 0x300),fVar1 * *(float *)(iVar3 + 0x304));
  fVar1 = *(float *)(iVar4 + 8);
  fVar5 = (float)puVar2[0xce];
  local_54 = local_60 * fVar1 * fVar5;
  local_50 = local_5c * fVar1 * fVar5;
  local_4c = local_58 * fVar1 * fVar5;
  matrix3x3_from_forward_up(local_48);
  local_84 = (float)puVar2[0x89];
  local_80 = (float)puVar2[0x8a];
  local_7c = (float)puVar2[0x8b];
  fVar1 = -local_7c;
  local_6c = local_84 * fVar1 + *(float *)PTR_DAT_00696720;
  local_68 = local_80 * fVar1 + *(float *)(PTR_DAT_00696720 + 4);
  local_64 = fVar1 * local_7c + *(float *)(PTR_DAT_00696720 + 8);
  fVar9 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar9) {
    local_6c = *(float *)PTR_DAT_00696718;
    local_68 = *(float *)(PTR_DAT_00696718 + 4);
    local_64 = *(float *)(PTR_DAT_00696718 + 8);
  }
  iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  if (*(uint *)(iVar8 + 0x324) != 0xffffffff) {
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar8 + 0x324) & 0xffff) * 0xc);
  }
  if (*(int *)(iVar8 + 500) == -1) {
    fVar9 = (float10)fcos((float10)*(float *)(iVar3 + 0x364));
    fVar10 = (float10)fsin((float10)*(float *)(iVar3 + 0x364));
    vector3d_rotate_pair_in_plane((float)fVar10,(float)fVar9);
  }
  fVar9 = (((float10)local_84 * (float10)(float)puVar2[0x1b] -
           (float10)local_80 * (float10)(float)puVar2[0x1a]) / (float10)*(float *)(iVar3 + 0x2f8)) *
          (float10)*(float *)(iVar3 + 0x308);
  fVar10 = (float10)fcos(fVar9);
  fVar9 = (float10)fsin(fVar9);
  vector3d_rotate_about_axis_perpendicular((float)fVar9,(float)fVar10);
  local_94 = local_68 * local_7c - local_64 * local_80;
  local_90 = local_64 * local_84 - local_6c * local_7c;
  local_8c = local_80 * local_6c - local_68 * local_84;
  local_78 = local_94;
  local_74 = local_90;
  local_70 = local_8c;
  uVar7 = matrix3x3_transpose();
  matrix3x3_multiply(uVar7);
  quaternion_from_matrix3x3(&local_94);
  quaternion_to_axis_angle();
  fVar1 = -fVar5 * *(float *)(iVar3 + 0x314) * 0.31830987;
  local_58 = local_58 * fVar1 - (float)puVar2[0x25];
  fVar5 = (*(float *)(iVar4 + 0x58) + *(float *)(iVar4 + 0x54) + *(float *)(iVar4 + 0x50)) *
          0.33333334;
  local_8c = (float)puVar2[0xce];
  local_94 = (local_60 * fVar1 - (float)puVar2[0x23]) * fVar5 * local_8c;
  local_90 = (local_5c * fVar1 - (float)puVar2[0x24]) * fVar5 * local_8c;
  local_8c = local_58 * fVar5 * local_8c;
  fVar1 = SQRT((float)puVar2[0x25] * (float)puVar2[0x25] +
               (float)puVar2[0x24] * (float)puVar2[0x24] + (float)puVar2[0x23] * (float)puVar2[0x23]
              ) / *(float *)(iVar3 + 0x314);
  if (fVar1 <= (float)puVar2[0x13c]) {
    local_9c = (float)puVar2[0x13c] * (float)puVar2[0x13c] * 0.05;
    if (local_9c <= 0.005) {
      local_9c = 0.005;
    }
    local_9c = -local_9c;
    fVar5 = fVar1 - (float)puVar2[0x13c];
    if (fVar1 - (float)puVar2[0x13c] <= local_9c) goto LAB_005743b1;
  }
  else {
    local_9c = (1.0 - (float)puVar2[0x13c]) * (1.0 - (float)puVar2[0x13c]) * 0.2;
    if (0.01 <= local_9c) {
      if (0.05 < local_9c) {
        local_9c = 0.05;
      }
    }
    else {
      local_9c = 0.01;
    }
    fVar5 = fVar1 - (float)puVar2[0x13c];
    if (local_9c < fVar1 - (float)puVar2[0x13c]) goto LAB_005743b1;
  }
  local_9c = fVar5;
LAB_005743b1:
  puVar2[0x13c] = (uint)(local_9c + (float)puVar2[0x13c]);
  *(uint *)(param_2 + 0x18) = puVar2[0xce];
  puVar6 = PTR_DAT_00696738;
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)PTR_DAT_00696738;
  *(undefined4 *)(param_2 + 0x20) = *(undefined4 *)(puVar6 + 4);
  *(undefined4 *)(param_2 + 0x24) = *(undefined4 *)(puVar6 + 8);
  *(undefined4 *)(param_2 + 0x28) = *(undefined4 *)(puVar6 + 0xc);
  *(uint *)(param_2 + 0x78) = puVar2[0xce];
  *(undefined4 *)(param_2 + 0x7c) = *(undefined4 *)puVar6;
  *(undefined4 *)(param_2 + 0x80) = *(undefined4 *)(puVar6 + 4);
  *(undefined4 *)(param_2 + 0x84) = *(undefined4 *)(puVar6 + 8);
  *(undefined4 *)(param_2 + 0x88) = *(undefined4 *)(puVar6 + 0xc);
  FUN_00507840(param_1,param_2,param_3,&local_54,&local_94);
  return;
}
#endif

// vehicle_calculate_lean_controls  (Ghidra: FUN_00572df0; renamed from the phase2 proposal)
// address 0x572df0, size 779 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary; dispatched from
//   vehicle_update's case 2)
// rewrite confidence: 0.15 -- the output record (unaff_ESI) is entirely register-resident with
//   no traceable origin, so its dozen-plus field writes are reproduced at their literal byte
//   offsets rather than through a named struct.
// evidence: types/units.h vehicle_data.turning_velocity (0x4dc); types/objects.h object.velocity
//   (0x068, "puVar2[0x1a..0x1c]"); the physics.tag_id-at-0x8c double-tag_instances-lookup idiom
//   (Vehicle tag -> Physics tag -> a field at Physics+0x68 compared against 3) matches
//   vehicle_calculate_turret_controls.c and vehicle_calculate_steering_wheel_controls.c (this
//   batch), there compared against 2.
// register convention: unit object index in EAX (param_1); an output record pointer in ESI
//   (unaff_ESI); a second stack parameter (param_2) forwarded to object_physics_tick unexamined.
//   // blam-cc: EAX -> unit_index, ESI -> out_record, stack -> param_2
// UNSURE: essentially every write below +0. vector3d_angle_between_4cd4f0's return value is
//   never consumed by anything visible, and vector3d_cross_product's two calls here (like the
//   ones in vehicle_update) may be building values this decompile shows going unused.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer; // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
                                        real cos_angle); // 0x4cd820
extern real vector3d_angle_between_4cd4f0(void); // 0x4cd4f0, UNSURE signature  // real signature (vector3d_angle_between_4cd4f0.c): real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); Ghidra recovered 0 of 2 args at this call site
extern void object_physics_tick(uint32_t unit_index, uint32_t param_2, void *transform,
                          uint32_t param_4, uint32_t param_5); // 0x507840, UNSURE signature
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);

// Computes a lean/tilt rotation control transform for a vehicle-type unit based on its current
// angular velocity, only when the supporting object's physics type is 3; otherwise dispatches
// generically.
// UNSURE: reproduced only partially; see the file header.
void vehicle_calculate_lean_controls(uint32_t unit_index, void *param_2)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;
    uint8_t *out_record = 0; // UNSURE: stands in for unaff_ESI; see file header

    if (*(int32_t *)(physics_tag + 0x68) != 3) {
        object_physics_tick(unit_index, 0, param_2, 0, 0);
        return;
    }

    {
        double speed = fabs(sqrt((double)(obj->velocity.k * obj->velocity.k +
                                          obj->velocity.j * obj->velocity.j +
                                          obj->velocity.i * obj->velocity.i)) * 2.5);
        vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
        double lean_fraction = (speed > 1.0) ? 1.0 : speed;
        double half_angle = (1.0 - lean_fraction) * (double)(vehicle->turning_velocity * 0.5f);

        *(float *)(out_record + 4) = vehicle->forward_velocity;
        *(uint32_t *)(out_record + 0xc) = 0x3b449ba6;
        *(uint32_t *)(out_record + 0x1c) = 0;
        *(uint32_t *)(out_record + 0x20) = 0;
        *(float *)(out_record + 0x24) = (float)sin(half_angle);
        *(float *)(out_record + 0x28) = (float)cos(half_angle);
        *(uint32_t *)(out_record + 0x6c) = 0x3b449ba6;
        *(uint32_t *)(out_record + 0x7c) = 0;
        *(uint32_t *)(out_record + 0x80) = 0;
        *(uint32_t *)(out_record + 0x84) = 0;
        *(uint32_t *)(out_record + 0x88) = 0x3f800000;
        *(uint32_t *)(out_record + 0xcc) = 0x3ba3d70a;
        *(uint32_t *)(out_record + 0xe8) = 0x3f800000;
        *(uint32_t *)(out_record + 0xdc) = 0;
        *(uint32_t *)(out_record + 0xe0) = 0;
        *(uint32_t *)(out_record + 0xe4) = 0;
    }

    {
        real_vector3d axis = obj->angular_velocity;
        real length = vector3d_normalize_with_length(&axis);
        if (length != 0.0f) {
            real_vector3d unused1, unused2;
            float angle;

            vector3d_cross_product(&unused1, &obj->up, &axis); // UNSURE operand order/use
            vector3d_cross_product(&unused2, &obj->forward, &axis); // UNSURE operand order/use
            angle = (unused2.i * global_up3d_pointer->i + unused2.j * global_up3d_pointer->j +
                     unused2.k * global_up3d_pointer->k) * 6.2831855f;
            vector3d_rotate_about_axis(&axis, &axis, (real)sin((double)angle), (real)cos((double)angle)); // UNSURE args
            vector3d_angle_between_4cd4f0();
        }
    }

    object_physics_tick(unit_index, 0, out_record, 0, 0);
}

#if 0
Original Ghidra decompilation (0x572df0):

void FUN_00572df0(uint param_1,undefined4 param_2)

{
  float fVar1;
  uint *puVar2;
  undefined *puVar3;
  float *pfVar4;
  int unaff_ESI;
  float10 fVar5;
  float10 fVar6;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (*(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c)
                        & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x68) != 3) {
    FUN_00507840(param_1,0,param_2,0,0);
    return;
  }
  fVar5 = ABS(SQRT((float10)(float)puVar2[0x1c] * (float10)(float)puVar2[0x1c] +
                   (float10)(float)puVar2[0x1b] * (float10)(float)puVar2[0x1b] +
                   (float10)(float)puVar2[0x1a] * (float10)(float)puVar2[0x1a]) * (float10)2.5);
  fVar1 = (float)puVar2[0x137];
  if ((float10)1.0 < fVar5) {
    fVar5 = (float10)1.0;
  }
  *(uint *)(unaff_ESI + 4) = puVar2[0x135];
  fVar5 = ((float10)1.0 - fVar5) * (float10)(fVar1 * 0.5);
  *(undefined4 *)(unaff_ESI + 0xc) = 0x3b449ba6;
  puVar3 = PTR_DAT_00696720;
  fVar6 = (float10)fsin(fVar5);
  *(undefined4 *)(unaff_ESI + 0x1c) = 0;
  *(undefined4 *)(unaff_ESI + 0x20) = 0;
  *(float *)(unaff_ESI + 0x24) = (float)fVar6;
  fVar5 = (float10)fcos(fVar5);
  *(float *)(unaff_ESI + 0x28) = (float)fVar5;
  *(undefined4 *)(unaff_ESI + 0x6c) = 0x3b449ba6;
  *(undefined4 *)(unaff_ESI + 0x7c) = 0;
  *(undefined4 *)(unaff_ESI + 0x80) = 0;
  *(undefined4 *)(unaff_ESI + 0x84) = 0;
  *(undefined4 *)(unaff_ESI + 0x88) = 0x3f800000;
  *(undefined4 *)(unaff_ESI + 0xcc) = 0x3ba3d70a;
  *(undefined4 *)(unaff_ESI + 0xe8) = 0x3f800000;
  *(undefined4 *)(unaff_ESI + 0xdc) = 0;
  *(undefined4 *)(unaff_ESI + 0xe0) = 0;
  *(undefined4 *)(unaff_ESI + 0xe4) = 0;
  fVar5 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 != fVar5) {
    vector3d_cross_product(puVar2 + 0x20);
    pfVar4 = (float *)vector3d_cross_product(puVar2 + 0x1d);
    fVar5 = ((float10)*pfVar4 * (float10)*(float *)puVar3 +
            (float10)pfVar4[1] * (float10)*(float *)(puVar3 + 4) +
            (float10)pfVar4[2] * (float10)*(float *)(puVar3 + 8)) * (float10)6.2831855;
    fVar6 = (float10)fcos(fVar5);
    fVar5 = (float10)fsin(fVar5);
    vector3d_rotate_about_axis((float)fVar5,(float)fVar6);
    vector3d_angle_between_4cd4f0();
  }
  FUN_00507840(param_1);
  return;
}
#endif

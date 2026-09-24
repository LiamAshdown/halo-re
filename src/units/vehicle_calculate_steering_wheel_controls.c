// vehicle_calculate_steering_wheel_controls  (Ghidra: FUN_00572cd0; renamed from the phase2
//   proposal)
// address 0x572cd0, size 288 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary; dispatched from
//   vehicle_update's case 1)
// rewrite confidence: 0.3
// evidence: types/units.h vehicle_data.wheel_rotation (0x4e0, "0x572cd0 accumulates
//   forward_velocity into it and wraps it at wheel_circumference"), .forward_velocity (0x4d4),
//   .turning_velocity (0x4dc); types/tags.h Vehicle.wheel_circumference (0x310); parallel
//   structure to vehicle_calculate_turret_controls.c (this batch).
// register convention: unit object index in EAX (param_1); an output transform pointer in EDI
//   (unaff_EDI); a second stack parameter (param_2) forwarded to object_physics_tick unexamined.
//   // blam-cc: EAX -> unit_index, EDI -> out_transform, stack -> param_2
// UNSURE: see vehicle_calculate_turret_controls.c for the FUN_00628cca and Physics-tag-field
//   caveats, which apply identically here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern float FUN_00628cca(float value, float modulus); // 0x628cca, UNSURE signature
extern void object_physics_tick(uint32_t unit_index, uint32_t param_2, void *transform,
                          uint32_t param_4, uint32_t param_5); // 0x507840, UNSURE signature
extern double cos(double x);
extern double sin(double x);

// Computes a single-axis (steering-wheel-style) rotation control transform for a vehicle-type
// unit each tick: accumulates and wraps the wheel-rotation angle, then either dispatches
// generically or writes a pair of scalar+quaternion blocks (rotated by half the turning angle
// about a fixed axis) when the supporting object's physics type is 2.
void vehicle_calculate_steering_wheel_controls(uint32_t unit_index, void *param_2)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;
    float *out_transform = 0; // UNSURE: stands in for unaff_EDI; see vehicle_calculate_turret_controls.c
    float wrapped;

    vehicle->wheel_rotation = vehicle->forward_velocity + vehicle->wheel_rotation;
    wrapped = FUN_00628cca(vehicle->wheel_rotation, tag->wheel_circumference);
    vehicle->wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->wheel_rotation = wrapped + tag->wheel_circumference;
    }

    if (*(int32_t *)(physics_tag + 0x68) != 2) {
        object_physics_tick(unit_index, 0, param_2, 0, 0);
        return;
    }

    {
        float turning = vehicle->turning_velocity;
        float c = (float)cos((double)turning * 0.5);
        float s = (float)sin((double)turning * 0.5);

        out_transform[0] = vehicle->forward_velocity;
        out_transform[7] = 0.0f;
        out_transform[8] = 0.0f;
        out_transform[9] = s;
        out_transform[10] = c;
        out_transform[0x18] = vehicle->forward_velocity;
        out_transform[0x1f] = 0.0f;
        out_transform[0x20] = 0.0f;
        out_transform[0x21] = -s;
        out_transform[0x22] = c;
    }
    object_physics_tick(unit_index, 0, out_transform, 0, 0);
}

#if 0
Original Ghidra decompilation (0x572cd0):

void FUN_00572cd0(uint param_1,undefined4 param_2)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  uint *unaff_EDI;
  float10 fVar5;
  float10 fVar6;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*(uint *)(iVar3 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar2[0x138] = (uint)((float)puVar2[0x135] + (float)puVar2[0x138]);
  fVar5 = (float10)FUN_00628cca();
  puVar2[0x138] = (uint)(float)fVar5;
  if (fVar5 < (float10)0.0) {
    puVar2[0x138] = (uint)(float)(fVar5 + (float10)*(float *)(iVar3 + 0x310));
  }
  if (*(int *)(iVar4 + 0x68) != 2) {
    FUN_00507840(param_1,0,param_2,0,0);
    return;
  }
  fVar1 = (float)puVar2[0x137];
  *unaff_EDI = puVar2[0x135];
  unaff_EDI[7] = 0;
  fVar5 = (float10)fcos((float10)fVar1 * (float10)0.5);
  unaff_EDI[8] = 0;
  fVar6 = (float10)fsin((float10)(float)((float10)fVar1 * (float10)0.5));
  unaff_EDI[9] = (uint)(float)fVar6;
  unaff_EDI[10] = (uint)(float)fVar5;
  unaff_EDI[0x18] = puVar2[0x135];
  unaff_EDI[0x1f] = 0;
  unaff_EDI[0x21] = (uint)(float)-fVar6;
  unaff_EDI[0x20] = 0;
  unaff_EDI[0x22] = (uint)(float)fVar5;
  FUN_00507840(param_1);
  return;
}
#endif

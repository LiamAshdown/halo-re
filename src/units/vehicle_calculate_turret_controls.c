// vehicle_calculate_turret_controls  (Ghidra: FUN_00572b60; renamed from the phase2 proposal)
// address 0x572b60, size 361 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary; dispatched from
//   vehicle_update's case 0)
// rewrite confidence: 0.9 (VERIFIED against objdump; EDI buffer FIXED)
// evidence: types/units.h vehicle_data.forward_velocity (0x4d4), .turning_velocity (0x4dc),
//   .left_wheel_rotation/.right_wheel_rotation (0x4e4/0x4e8); types/tags.h Vehicle.wheel_circumference
//   (0x310); the physics.tag_id-at-0x8c idiom (here indexing a SECOND tag_instances lookup, so
//   iVar7 is the Physics tag's own data) matches every other use in this module.
// register convention: unit object index in EAX (param_1); an output transform pointer in EDI
//   (unaff_EDI); a second stack parameter (param_2) forwarded to object_physics_tick unexamined.
//   // blam-cc: EAX -> unit_index, EDI -> out_transform, stack -> param_2
// UNSURE: FUN_00628cca's arguments are register-only; read as (accumulated_value,
//   wheel_circumference) matching the wrap-then-renormalize idiom, but this is a guess.
// UNSURE: the Physics tag field at offset 0x68 (tested against the literal 2) is not named in
//   any header available to this module.
// UNSURE: out_transform's shape (two 9-float scalar+quaternion-shaped blocks at +0 and +0x18)
//   is inferred purely from which offsets are written; declared as a raw float array.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern double fmod(double x, double y); // CRT fmod (0x628cca: _CIfmod, x87 fprem; name entry "fmod" at 0x006844f0)
extern void object_physics_tick(uint32_t unit_index, uint32_t powered_states, void *transform,
                          uint32_t extra_force, uint32_t extra_torque); // 0x507840, UNSURE signature

// Computes the dual-axis (pitch/yaw) turret control transform for a vehicle-type unit each
// tick, accumulating and wrapping left/right wheel-rotation-shaped angle accumulators, and
// dispatches to object_physics_tick -- either generically, or (when the supporting object's physics
// type is 2) by writing a pair of scalar+identity-quaternion blocks directly into out_transform.
// FIXED (objdump 0x572c62..0x572ca0): EDI is the caller's powered-mass-point buffer (vehicle_update [esp+0x88]);
//   with physics type 2 its two entries get the left/right drive and object_physics_tick(unit, EDI, contacts, 0,
//   0) runs. The draft wrote through a NULL stand-in and passed it as the contact buffer.
// blam-cc: stack -> unit_index, param_2 (contact points); EDI -> powered_states
void vehicle_calculate_turret_controls(uint32_t unit_index, void *mass_points, float *powered_states)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    float *out_transform = powered_states; // EDI
    float forward = vehicle->forward_velocity;
    float turning = vehicle->turning_velocity;
    uint8_t *physics_tag = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    float wrapped;

    vehicle->left_wheel_rotation = (forward - turning) + vehicle->left_wheel_rotation;
    wrapped = (float)fmod(vehicle->left_wheel_rotation, tag->wheel_circumference);
    vehicle->left_wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->left_wheel_rotation = wrapped + tag->wheel_circumference;
    }

    vehicle->right_wheel_rotation = (turning + forward) + vehicle->right_wheel_rotation;
    wrapped = (float)fmod(vehicle->right_wheel_rotation, tag->wheel_circumference);
    vehicle->right_wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->right_wheel_rotation = wrapped + tag->wheel_circumference;
    }

    if (*(int32_t *)(physics_tag + 0x68) != 2) {
        object_physics_tick(unit_index, 0, mass_points, 0, 0);
        return;
    }

    out_transform[0] = forward - turning;
    out_transform[7] = 0.0f;
    out_transform[8] = 0.0f;
    out_transform[9] = 0.0f;
    out_transform[10] = 1.0f;
    out_transform[0x18] = turning + forward;
    out_transform[0x1f] = 0.0f;
    out_transform[0x20] = 0.0f;
    out_transform[0x21] = 0.0f;
    out_transform[0x22] = 1.0f;
    object_physics_tick(unit_index, (uint32_t)out_transform, mass_points, 0, 0);
}

#if 0
Original Ghidra decompilation (0x572b60):

void FUN_00572b60(uint param_1,undefined4 param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint *puVar5;
  int iVar6;
  int iVar7;
  float *unaff_EDI;
  float10 fVar8;

  puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  fVar1 = (float)puVar5[0x135];
  fVar2 = (float)puVar5[0x137];
  iVar6 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  fVar3 = (float)puVar5[0x137];
  fVar4 = (float)puVar5[0x135];
  iVar7 = *(int *)((*(uint *)(iVar6 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar5[0x139] = (uint)((fVar1 - fVar2) + (float)puVar5[0x139]);
  fVar8 = (float10)FUN_00628cca();
  puVar5[0x139] = (uint)(float)fVar8;
  if (fVar8 < (float10)0.0) {
    puVar5[0x139] = (uint)(float)(fVar8 + (float10)*(float *)(iVar6 + 0x310));
  }
  puVar5[0x13a] = (uint)(fVar3 + fVar4 + (float)puVar5[0x13a]);
  fVar8 = (float10)FUN_00628cca();
  puVar5[0x13a] = (uint)(float)fVar8;
  if (fVar8 < (float10)0.0) {
    puVar5[0x13a] = (uint)(float)(fVar8 + (float10)*(float *)(iVar6 + 0x310));
  }
  if (*(int *)(iVar7 + 0x68) != 2) {
    FUN_00507840(param_1,0,param_2,0,0);
    return;
  }
  *unaff_EDI = fVar1 - fVar2;
  unaff_EDI[7] = 0.0;
  unaff_EDI[8] = 0.0;
  unaff_EDI[9] = 0.0;
  unaff_EDI[10] = 1.0;
  unaff_EDI[0x18] = fVar3 + fVar4;
  unaff_EDI[0x1f] = 0.0;
  unaff_EDI[0x20] = 0.0;
  unaff_EDI[0x21] = 0.0;
  unaff_EDI[0x22] = 1.0;
  FUN_00507840(param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

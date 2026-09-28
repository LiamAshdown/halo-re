// unit_set_facing_from_index_table  (Ghidra: FUN_00570de0; renamed from the phase2 proposal)
// address 0x570de0, size 244 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.85 (REWRITTEN: position argument is the spawn record's position (EDI), from 0x570e44..0x570e92; rest verified)
// evidence: types/units.h vehicle_data.cinematic_facing_index (0x5b0, "0x570de0 indexes the
//   cinematic direction table of the scenario with it"); types/tags.h Object's physics
//   TagDependency lands at absolute tag offset 0x80..0x90, so tag+0x8c is its tag_id and tag+4
//   is bounding_radius (Object.bounding_radius, right after object_type/flags); types/objects.h
//   object.flags (0x010, bit 0x20 = extension_of_parent), object.position (0x05c); callees
//   object_reset_velocity_and_wake, object_set_position_and_orientation (established
//   4-argument form in src/units/biped_update.c).
// UNSURE: the two branches computing the facing angle from global_scenario (either a
//   0x94-stride array at scenario+0x37c, or a table whose stride/base come from a second
//   type-definition-like pointer table at 0x0069bfe0) are scenario/HS-module structures not
//   covered by any header available to this module; reproduced with raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *global_scenario;     // 0x00746f8c, UNSURE: scenario tag data pointer
extern int32_t control_binding_device_type;         // UNSURE global (a game-mode selector)
extern uint8_t *object_type_definitions_ex; // 0x0069bfe0, UNSURE: a second type-definition table
extern real_vector3d *global_up3d_pointer; // 0x00696720

extern void object_reset_velocity_and_wake(uint32_t object_index); // 0x4f5160
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                                 real_vector3d *up, real_point3d *position); // 0x4f51c0
extern double cos(double x); // fcos
extern double sin(double x); // fsin

// When a global cinematic/lookup mode is active (global_scenario != 0), orients the unit to
// face a direction taken from an indexed table (selected by the vehicle's cinematic_facing_index
// and the current game-mode selector), and adjusts its extension_of_parent flag and height
// depending on whether its tag defines a physics reference.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
void unit_set_facing_from_index_table(uint32_t object_index)
{
    object *obj;
    Object *tag;
    int16_t facing_index;
    float angle;
    real_vector3d forward;

    if (global_scenario == 0) {
        return;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    facing_index = *(int16_t *)((uint8_t *)obj + 0x5b0); // vehicle_data.cinematic_facing_index

    object_reset_velocity_and_wake(object_index);

    // FIXED (0x570e44..0x570e92): EDI = the spawn record's position (scenario +0x37c entry, 0x94
    //   bytes, yaw +0xc, in game-engine mode 5; otherwise the vehicle placement's +0x08, yaw +0x14),
    //   so the vehicle is moved back to its spawn. The draft passed its current position.
    {
        real_point3d *spawn_position;

        if (control_binding_device_type == 5) {
            uint8_t *entry = *(uint8_t **)(global_scenario + 0x37c) + facing_index * 0x94;

            spawn_position = (real_point3d *)entry;
            angle = *(float *)(entry + 0xc);
        } else {
            int16_t stride = *(int16_t *)(object_type_definitions_ex + 0xe);
            int16_t base_field_offset = *(int16_t *)(object_type_definitions_ex + 10);
            uint8_t *base = *(uint8_t **)(global_scenario + 4 + base_field_offset);

            spawn_position = (real_point3d *)(base + facing_index * stride + 0x8);
            angle = *(float *)((uint8_t *)spawn_position + 0xc);
        }

        forward.i = (float)cos((double)angle);
        forward.j = (float)sin((double)angle);
        forward.k = 0.0f;
        object_set_position_and_orientation(object_index, &forward, global_up3d_pointer, spawn_position);
    }

    if (*(int32_t *)((uint8_t *)tag + 0x8c) == -1) { // tag->physics.tag_id
        obj->flags |= 0x20;
    } else {
        obj->flags &= ~0x20u;
        obj->position.z += tag->bounding_radius * 0.5f;
    }
}

#if 0
Original Ghidra decompilation (0x570de0):

void FUN_00570de0(uint param_1)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  float10 fVar6;
  float local_c;
  float local_8;
  undefined4 local_4;

  iVar4 = global_scenario;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (global_scenario != 0) {
    object_reset_velocity_and_wake(param_1);
    local_4 = 0;
    if (DAT_006f1cb8 == 5) {
      fVar1 = *(float *)((short)puVar2[0x16c] * 0x94 + 0xc + *(int *)(iVar4 + 0x37c));
    }
    else {
      fVar1 = *(float *)((int)(short)puVar2[0x16c] * (int)*(short *)(PTR_PTR_0069bfe0 + 0xe) +
                         *(int *)(*(short *)(PTR_PTR_0069bfe0 + 10) + 4 + iVar4) + 0x14);
    }
    fVar6 = (float10)fcos((float10)fVar1);
    local_c = (float)fVar6;
    fVar6 = (float10)fsin((float10)fVar1);
    local_8 = (float)fVar6;
    object_set_position_and_orientation(param_1,&local_c,PTR_DAT_00696720);
    if (*(int *)(iVar3 + 0x8c) == -1) {
      uVar5 = puVar2[4] | 0x20;
    }
    else {
      uVar5 = puVar2[4] & 0xffffffdf;
    }
    puVar2[4] = uVar5;
    if (*(int *)(iVar3 + 0x8c) != -1) {
      puVar2[0x19] = (uint)(*(float *)(iVar3 + 4) * 0.5 + (float)puVar2[0x19]);
    }
  }
  return;
}
#endif

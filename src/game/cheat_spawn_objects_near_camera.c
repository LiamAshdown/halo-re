// cheat_spawn_objects_near_camera  (Ghidra: cheat_spawn_objects_near_camera, already named)
// address 0x45a800, size 441 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: types/objects.h object_placement_data (0x88 bytes: position 0x18, up 0x40); the
//   struct writes at local_70/6c/68 (position) and local_48/44/40 (up) land exactly on those
//   fields relative to a base at local_88.
// register convention: `tag_array` and `count` are Ghidra's own recognized __cdecl parameters.
//
// UNSURE, LOW CONFIDENCE: object_get_position(), object_get_orientation(&local_94) and
// object_placement_data_initialize(tag, -1) are all called with incomplete argument lists
// (Ghidra elides the observer/camera object index throughout, and object_get_orientation's
// first output -- forward -- has no attributable destination here even though its second
// output, up, clearly lands in the placement struct). The float10 value
// object_placement_data_initialize appears to return and feed into fcos/fsin is, per the same
// x87-stack-tracking failure documented elsewhere in this codebase (see
// src/math/vector3d_angle_between_4cd5e0.c), almost certainly actually the result of the
// `fpatan` call two lines above it (a yaw angle from the camera's forward vector), collapsed
// here accordingly. Transcribed with placeholder camera-state locals rather than invented
// register plumbing; not independently verified against a disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern uint32_t cheat_get_target_object_index(void); // this batch, 0x45a7a0
extern int16_t network_game_mode;   // 0x00719720
extern tag_instance *tag_instances; // 0x0087bc14
extern void *object_type_role_table[12]; // 0x0069bfdc, an ARRAY of the 12 object type definitions (was a pointer variable)
                                       // tag's object-type byte, +0x10 tested against -1

extern void object_get_position(real_point3d *out, datum_index object_index); // 0x4f6900, blam-cc: EAX -> out, ECX -> object_index (matches src/objects/object_get_position.c)
extern void object_get_orientation(real_vector3d *out_forward, datum_index object_index, real_vector3d *out_up); // 0x4f6970, blam-cc: EAX -> out_forward, ECX -> object_index, stack -> out_up (matches src/objects/object_get_orientation.c)
extern double atan2(double y, double x); // x87 FPATAN
extern double sin(double x);             // x87 FSIN
extern double cos(double x);             // x87 FCOS
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0

// Spawns up to `count` object tags from `tag_array` at positions offset 1.5 units in front of
// the debug-selected camera object and 0.8 units above it, oriented to match the camera's up
// vector and facing yaw.
void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count)
{
    uint32_t camera_object;
    real_point3d camera_position;
    real_vector3d camera_forward;
    real_vector3d camera_up;
    uint16_t remaining;
    TagDependency *record;
    datum_index tag_handle;
    real yaw;
    object_placement_data placement;
    uint32_t role;

    camera_object = cheat_get_target_object_index();
    if (camera_object != 0xffffffff) {
        object_get_position(&camera_position, camera_object);      // UNSURE: object index elided by Ghidra
        object_get_orientation(&camera_forward, camera_object, &camera_up); // UNSURE: object index elided by Ghidra

        if (0 < count) {
            remaining = count;
            record = &tag_array[0]; // matches Ghidra's `(int *)(tag_array + 0xc)`:
                                     // TagDependency::tag_id already sits at byte offset 0xc
            do {
                tag_handle = *(datum_index *)&record->tag_id;
                if (tag_handle != k_datum_index_none) {
                    yaw = (real)atan2((double)camera_forward.i, (double)camera_forward.j); // UNSURE operand order
                    object_placement_data_initialize(&placement, tag_handle, k_datum_index_none); // UNSURE args
                    placement.up = camera_up;
                    role = 3;
                    placement.position.x = (real)cos((double)yaw) * 1.5f + camera_position.x; // UNSURE axis mapping
                    placement.position.y = (real)sin((double)yaw) * 1.5f + camera_position.y; // UNSURE axis mapping
                    placement.position.z = camera_position.z + 0.8f;
                    if (network_game_mode == 2) {
                        Object *tag = (Object *)tag_instances[placement.definition_tag & 0xffff].data;
                        if (*(int32_t *)((uint8_t *)object_type_role_table[tag->object_type] + 0x10) != -1) { // TYPES-GAP
                            role = 0;
                        }
                    }
                    object_new_with_datum_role_control(&placement, role);
                }
                record = record + 1; // matches Ghidra's `local_b4 = local_b4 + 4` (4 ints == sizeof(TagDependency))
                remaining = remaining - 1;
            } while (remaining != 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x45a800), from tools/pack.py 0x45a800:

void __cdecl cheat_spawn_objects_near_camera(int tag_array,ushort count)

{
  int iVar1;
  undefined4 uVar2;
  unkbyte10 Var3;
  float10 fVar4;
  int *local_b4;
  uint local_b0;
  float local_ac;
  float local_a8;
  float local_a0;
  float local_9c;
  float local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  uint local_88 [6];
  float local_70;
  float local_6c;
  float local_68;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;

  iVar1 = FUN_0045a7a0();
  if (iVar1 != -1) {
    object_get_position();
    object_get_orientation(&local_94);
    if (0 < (short)count) {
      local_b0 = (uint)count;
      local_b4 = (int *)(tag_array + 0xc);
      do {
        if (*local_b4 != -1) {
          fpatan((float10)local_ac,(float10)local_a8);
          Var3 = object_placement_data_initialize(*local_b4,0xffffffff);
          fVar4 = (float10)fcos(Var3);
          local_48 = local_94;
          local_44 = local_90;
          local_40 = local_8c;
          uVar2 = 3;
          local_70 = (float)(fVar4 * (float10)1.5 + (float10)local_a0);
          fVar4 = (float10)fsin(Var3);
          local_6c = (float)(fVar4 * (float10)1.5 + (float10)local_9c);
          local_68 = local_98 + 0.8;
          if ((DAT_00719720 == 2) &&
             (*(int *)((&PTR_PTR_0069bfdc)
                       [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10)
              != -1)) {
            uVar2 = 0;
          }
          object_new_with_datum_role_control(local_88,uVar2);
        }
        local_b4 = local_b4 + 4;
        local_b0 = local_b0 - 1;
      } while (local_b0 != 0);
    }
  }
  return;
}
#endif

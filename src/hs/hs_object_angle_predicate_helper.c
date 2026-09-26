// hs_object_angle_predicate_helper  (Ghidra: FUN_004878f0)
// address 0x4878f0, size 180 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN (objdump 0x4878f0..0x4879a3; the draft passed none of the look-cone arguments). EAX = the target
//   object, [esp+4] = the viewing unit, [esp+8] = the cone angle in degrees. The target point is the "head" marker
//   (0x0066bfa0) of a unit target (object_try_and_get(ECX target, 3)) -- node_transform.position, +0x60, read
//   whatever the marker count -- otherwise the object's centre (+0xa0). Returns
//   unit_point_within_look_cone(stack degrees * 0.017453292 (0x672c38), ECX viewer, EDI &point); -1 gives 0.
// blam-cc: EAX -> object_index, stack -> viewer_unit, angle_degrees

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"

extern data_array *object_data; // 0x008603b0
extern char ai_marker_name_a[]; // 0x0066bfa0, "head"
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum); // 0x4f6080
extern uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point);
    // 0x56c100, blam-cc: stack, ECX, EDI

uint8_t hs_object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees)
{
    real_point3d point;

    if (object_index == k_datum_index_none) {
        return 0;
    }
    if (object_try_and_get(object_index, 3) != 0) {
        object_marker marker;

        object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1);
        point = *(real_point3d *)((uint8_t *)&marker + 0x60);
    } else {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);

        point = *(real_point3d *)(object + 0xa0);
    }
    return unit_point_within_look_cone(angle_degrees * 0.017453292f, viewer_unit, &point);
}

#if 0
Original Ghidra decompilation (0x4878f0):

uint FUN_004878f0(undefined4 param_1,float param_2)

{
  uint in_EAX;
  int iVar1;
  uint uVar2;

  uVar2 = in_EAX & 0xffffff00;
  if (in_EAX != 0xffffffff) {
    iVar1 = object_try_and_get(3);
    if (iVar1 != 0) {
      FUN_004f6080();
    }
    uVar2 = FUN_0056c100(param_2 * 0.017453292);
  }
  return uVar2;
}
#endif

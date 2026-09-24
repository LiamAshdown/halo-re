// unit_point_within_look_cone  (Ghidra: FUN_0056c100)
// address 0x56c100, size 208 bytes, name confidence 0.4, rewrite confidence 0.25
// functions.md: "Returns whether a given world point lies within a cone of half-angle param_1
// around the unit's forward direction." (evidence below shows it is actually the *looking*
// vector, not the raw object forward.)
// evidence: types/units.h unit_data.looking_vector (0x260).
// blam-cc: param_1 -> cos_angle_source (radians, passed to fcos), in_ECX -> unit_index,
//   unaff_EDI -> world_point.
// UNSURE: the call to vector3d_normalize_with_length() here has no visible input or output in
// the decompilation -- its result is never read back, and the dot product below uses the raw
// (unnormalized) point-minus-marker difference. This is reproduced literally: the normalize
// call happens (in case it has a side effect this rewrite cannot see) but does not feed the
// comparison.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;    // 0x008603b0
extern char s_primary_eye_marker[]; // 0x0066bfa0, shared with unit_get_primary_eye_marker_position.c

extern double fcos(double x);
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                                uint32_t param_4); // 0x4f6080

uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point)
    // blam-cc: param_1, in_ECX, unaff_EDI
{
    if (unit_index == 0xffffffff) {
        return 0;
    }
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    object_marker marker;
    object_get_node_local_transform(unit_index, s_primary_eye_marker, &marker, 0);

    float px = world_point->x, py = world_point->y, pz = world_point->z;
    real_vector3d unused_normalize_target = { 0.0f, 0.0f, 0.0f }; // UNSURE: see file header
    vector3d_normalize_with_length(&unused_normalize_target);

    float cos_angle = (float)fcos((double)cone_angle);
    float dot = (px - marker.transform.position.x) * unit->looking_vector.i +
                (py - marker.transform.position.y) * unit->looking_vector.j +
                (pz - marker.transform.position.z) * unit->looking_vector.k;
    return cos_angle < dot;
}

#if 0
Original Ghidra decompilation (0x56c100):

undefined4 FUN_0056c100(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  uint in_ECX;
  float *unaff_EDI;
  float10 fVar5;
  float local_c;
  float local_8;
  float local_4;

  if (in_ECX != 0xffffffff) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    object_get_node_local_transform();
    fVar1 = *unaff_EDI;
    fVar2 = unaff_EDI[1];
    fVar3 = unaff_EDI[2];
    vector3d_normalize_with_length();
    fVar5 = (float10)fcos((float10)param_1);
    if (fVar5 < (float10)(fVar1 - local_c) * (float10)*(float *)(iVar4 + 0x260) +
                (float10)(fVar2 - local_8) * (float10)*(float *)(iVar4 + 0x264) +
                (float10)(fVar3 - local_4) * (float10)*(float *)(iVar4 + 0x268)) {
      return 1;
    }
  }
  return 0;
}
#endif

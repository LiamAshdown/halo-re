// unit_get_secondary_eye_marker_position  (Ghidra: FUN_00569280)
// address 0x569280, size 45 bytes
// name confidence: 0.3 (functions.md: "Retrieves the world position of a second fixed named
//   marker via FUN_004f6080 and returns it through the implicit ESI output pointer")
// rewrite confidence: 0.3
// evidence: same as unit_get_primary_eye_marker_position.c (0x568f50), the only other function
//   with this exact shape in this module.
// register convention: object index in EAX (implicit, forwarded), destination real_point3d* in ESI.
//   // blam-cc: in_EAX -> object_index (forwarded), unaff_ESI -> out
// UNSURE: the marker-name argument (ECX) is the raw data address 0x00672034; not confirmed by
//   reading its text.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern char *s_secondary_eye_marker; // 0x00672034, UNSURE exact text

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                                uint32_t param_4); // 0x4f6080

void unit_get_secondary_eye_marker_position(uint32_t object_index, real_point3d *out) // blam-cc: in_EAX -> object_index, unaff_ESI -> out
{
    object_marker marker;
    object_get_node_local_transform(object_index, s_secondary_eye_marker, &marker, 0);
    *out = marker.transform.position;
    return;
}

#if 0
Original Ghidra decompilation (0x569280):

void FUN_00569280(void)

{
  undefined4 *unaff_ESI;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  object_get_node_local_transform();
  *unaff_ESI = local_c;
  unaff_ESI[1] = local_8;
  unaff_ESI[2] = local_4;
  return;
}
#endif

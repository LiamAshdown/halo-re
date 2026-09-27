// unit_get_primary_eye_marker_position  (Ghidra: FUN_00568f50)
// address 0x568f50, size 45 bytes
// name confidence: 0.3 (functions.md: "Retrieves the world position of a fixed named marker via
//   FUN_004f6080 and returns it through the implicit ESI output pointer")
// rewrite confidence: 0.3
// evidence: types/objects.h object_marker (transform real_matrix4x3 at 0x04); types/math.h
//   real_matrix4x3.position (0x28, so marker.transform.position is at object_marker+0x2c).
// register convention: object index in EAX (implicit, forwarded to the callee), destination
//   real_point3d* in ESI.
//   // blam-cc: ECX -> object_index, ESI -> out
// UNSURE: the marker-name argument (ECX) is never dereferenced as a string in this function's
//   own decompilation, only cross-referenced as the raw data address 0x0066bfa0; that same
//   address is the marker name argument at dozens of AI look/aim call sites elsewhere in the
//   binary, consistent with a fixed name such as "eyepoint", but this rewrite does not read the
//   actual bytes to confirm it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern char s_primary_eye_marker[]; // 0x0066bfa0, UNSURE exact text

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                                uint32_t param_4); // 0x4f6080

void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out) // blam-cc: ECX -> object_index, ESI -> out
{
    object_marker marker;
    object_get_node_local_transform(object_index, s_primary_eye_marker, &marker, 1) /* FIXED: the original pushes 1, the maximum marker count */;
    *out = marker.node_transform.position; // [esp+0x70] after four pushes = marker +0x60, the world position
    return;
}

#if 0
Original Ghidra decompilation (0x568f50):

void FUN_00568f50(void)

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

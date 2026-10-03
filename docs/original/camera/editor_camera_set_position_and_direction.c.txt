// editor_camera_set_position_and_direction  (Ghidra: FUN_00446e30; renamed, Blam-style, not previously named)
// address 0x446e30, size 91 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x446e30..0x446e8a; matches; ECX survives the call (camera_debug_load_from_file relies on it).
// evidence: out/phase4/camera_functions.md "Converts a position and direction vector into a pov
//   struct's position plus yaw/pitch angles." The seven floats written (position, yaw, pitch,
//   an untouched roll, and a 70 degree fov default) match types/camera.h editor_camera_data
//   exactly; camera_types_notes.md: "0x445940 fills it through 0x446e30 and then sets roll
//   (+0x14) and fov (+0x18)", i.e. this function leaves those two fields for its caller and only
//   derives yaw/pitch from the direction vector.
// register convention: output in EAX (in_EAX), direction vector in ECX (in_ECX), position in
//   EDX (in_EDX); no stack parameters.
//   // blam-cc: EAX -> out, ECX -> direction, EDX -> position

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

// cos/sin/atan2/sqrt are single x87 instructions in the original code; declared locally instead
// of via <math.h> because -I types shadows that header name with types/math.h.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double atan2(double y, double x);
extern double sqrt(double x);

// blam-cc: EAX -> out, ECX -> direction, EDX -> position
// Fills `out`'s position from `position` and derives yaw/pitch from `direction`; leaves roll and
// field_of_view at 0 / 70 degrees for the caller to overwrite if it wants something else.
void editor_camera_set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position)
{
    out->roll = 0.0f;
    out->field_of_view = 1.2217305f; // 70 degrees
    out->position = *position;

    out->yaw = (real)atan2((double)direction->j, (double)direction->i);
    out->pitch = (real)atan2((double)direction->k,
                              sqrt((double)direction->i * (double)direction->i +
                                   (double)direction->j * (double)direction->j));
}

#if 0
Original Ghidra decompilation (0x446e30):

void FUN_00446e30(void)

{
  undefined4 *in_EAX;
  float *in_ECX;
  undefined4 *in_EDX;
  float10 fVar1;

  in_EAX[1] = 0;
  *in_EAX = 0;
  in_EAX[3] = 0;
  in_EAX[4] = 0;
  in_EAX[5] = 0;
  in_EAX[6] = 0x3f9c61aa;
  *in_EAX = *in_EDX;
  in_EAX[1] = in_EDX[1];
  in_EAX[2] = in_EDX[2];
  fVar1 = (float10)fpatan((float10)in_ECX[1],(float10)*in_ECX);
  in_EAX[3] = (float)fVar1;
  fVar1 = (float10)fpatan((float10)in_ECX[2],
                          SQRT((float10)*in_ECX * (float10)*in_ECX +
                               (float10)in_ECX[1] * (float10)in_ECX[1]));
  in_EAX[4] = (float)fVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

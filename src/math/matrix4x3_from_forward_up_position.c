// matrix4x3_from_forward_up_position  (Ghidra: FUN_004cbd60; renamed, Blam-style, not previously named)
// address 0x4cbd60, size 35 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/math_functions.md ("Builds a full matrix4x3 (rotation from two vectors,
//   plus a translation copied from another object) in one call"). Calls
//   matrix4x3_from_forward_up @0x4cb970 (which itself takes up/forward via EAX/ECX and the
//   output matrix on the stack) and then copies a separate 3-float position into the output
//   matrix's translation (+0x28..+0x30).
// register convention: up vector in EAX (unaff, forwarded unchanged into the callee), forward
//   vector in ECX (unaff, likewise forwarded), position in ESI (unaff_ESI); output matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> up, ECX -> forward, ESI -> position, stack -> out
// UNSURE: up/forward are never read by name in this function's own body -- they are inferred to
//   be pure register passthroughs into matrix4x3_from_forward_up, based on that callee's own
//   confirmed EAX/ECX usage and the fact that this function's caller must supply them somehow.

#include "tags.h"
#include "math.h"

extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970

// Builds a full matrix4x3 (rotation from two vectors, plus a translation copied from another
// object) in one call.
void matrix4x3_from_forward_up_position(real_vector3d *up, real_vector3d *forward,
                                         real_point3d *position, real_matrix4x3 *out)
{
    matrix4x3_from_forward_up(up, forward, out);
    out->position = *position;
}

#if 0
Original Ghidra decompilation (0x4cbd60):

void FUN_004cbd60(int param_1)

{
  undefined4 *unaff_ESI;

  FUN_004cb970(param_1);
  *(undefined4 *)(param_1 + 0x28) = *unaff_ESI;
  *(undefined4 *)(param_1 + 0x2c) = unaff_ESI[1];
  *(undefined4 *)(param_1 + 0x30) = unaff_ESI[2];
  return;
}
#endif

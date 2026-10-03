// matrix4x3_extract_forward_up_position  (Ghidra: FUN_004cbd90; renamed, Blam-style, not previously named)
// address 0x4cbd90, size 72 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/math.h real_matrix4x3 section ("FUN_004cbd90 unpacks forward at +0x04 and up
//   at +0x1c" -- a third independent confirmation of the row layout); out/phase4/math_functions.md
//   ("Unpacks two rotation-basis vectors and the translation out of a matrix4x3 structure").
// register convention: output up vector in EAX (in_EAX), output forward vector in ECX
//   (in_ECX); matrix and output position as the two recognized stack parameters (param_1,
//   param_2).
//   // blam-cc: EAX -> out_up, ECX -> out_forward, stack -> (m, out_position)

#include "tags.h"
#include "math.h"

// Unpacks two rotation-basis vectors and the translation out of a matrix4x3 structure.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void matrix4x3_extract_forward_up_position(real_vector3d *out_up, real_vector3d *out_forward,
                                            real_matrix4x3 *m, real_point3d *out_position)
{
    *out_forward = m->forward;
    *out_up = m->up;
    *out_position = m->position;
}

#if 0
Original Ghidra decompilation (0x4cbd90):

void FUN_004cbd90(int param_1,undefined4 *param_2)

{
  undefined4 *in_EAX;
  undefined4 *in_ECX;

  *in_ECX = *(undefined4 *)(param_1 + 4);
  in_ECX[1] = *(undefined4 *)(param_1 + 8);
  in_ECX[2] = *(undefined4 *)(param_1 + 0xc);
  *in_EAX = *(undefined4 *)(param_1 + 0x1c);
  in_EAX[1] = *(undefined4 *)(param_1 + 0x20);
  in_EAX[2] = *(undefined4 *)(param_1 + 0x24);
  *param_2 = *(undefined4 *)(param_1 + 0x28);
  param_2[1] = *(undefined4 *)(param_1 + 0x2c);
  param_2[2] = *(undefined4 *)(param_1 + 0x30);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

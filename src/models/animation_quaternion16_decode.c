// animation_quaternion16_decode  (Ghidra: FUN_004d6330, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d6330, size 77 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/models_types_notes.md "The uncompressed stream uses four int16
//   components (types/tags.h ModelAnimationsRotation, 8 bytes), decoded by
//   animation_quaternion16_decode 0x4d6330 (ECX = source, EAX = out) with no normalize."
//   Each component is a straight int16 -> float conversion scaled by 1/32767
//   (3.051851e-05 at 0x00672bd0), no bit shuffling and no quaternion_normalize call, unlike
//   the 48-bit codec (animation_quaternion48_decode, 0x4d6380).
// register convention: source int16[4] in ECX (in_ECX), output real_quaternion in EAX
//   (in_EAX).
//   // blam-cc: ECX -> source, EAX -> out

#include "tags.h"
#include "math.h"

// Decodes the four int16 components of an uncompressed animation rotation (i, j, k, w, in
// that struct order) into a real_quaternion, each scaled by 1/32767. Does not normalize.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void animation_quaternion16_decode(int16_t *source, real_quaternion *out)
{
    out->i = (real)source[0] * 3.051851e-05f;
    out->j = (real)source[1] * 3.051851e-05f;
    out->k = (real)source[2] * 3.051851e-05f;
    out->w = (real)source[3] * 3.051851e-05f;
}

#if 0
Original Ghidra decompilation (0x4d6330):

void FUN_004d6330(void)

{
  float *in_EAX;
  short *in_ECX;

  *in_EAX = (float)(int)*in_ECX * 3.051851e-05;
  in_EAX[1] = (float)(int)in_ECX[1] * 3.051851e-05;
  in_EAX[2] = (float)(int)in_ECX[2] * 3.051851e-05;
  in_EAX[3] = (float)(int)in_ECX[3] * 3.051851e-05;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

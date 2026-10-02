// vector3d_unpack_normal_11_11_10  (Ghidra: vector3d_unpack_normal_11_11_10, already named)
// address 0x513400, size 132 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: inverse of vector3d_pack_normal_11_11_10 @0x5132d0 (same 11:11:10 field widths,
//   reciprocal scale constants: 9.536743e-07 == 1/1023.5/1024, 4.7683716e-07 == 1/511.5/1024).
// register convention: destination vector in in_EAX (also the return value), packed value in
//   in_ECX. // blam-cc: EAX -> out, ECX -> packed

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> out, ECX -> packed
// Unpacks an 11:11:10 bit signed-normal-encoded direction vector (see
// vector3d_pack_normal_11_11_10) into out->{i,j,k} and returns out.
real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed)
{
    out->i = ((float)(int32_t)(packed << 0x15) * 9.536743e-07f + 1.0f) * 0.0004885198f;
    out->j = ((float)(int32_t)((packed >> 0xb) << 0x15) * 9.536743e-07f + 1.0f) * 0.0004885198f;
    out->k = ((float)(int32_t)(packed & 0xffc00000) * 4.7683716e-07f + 1.0f) * 0.0009775171f;
    return out;
}

#if 0
Original Ghidra decompilation (0x513400):

void vector3d_unpack_normal_11_11_10(void)

{
  float *in_EAX;
  uint in_ECX;

  *in_EAX = ((float)(int)(in_ECX << 0x15) * 9.536743e-07 + 1.0) * 0.0004885198;
  in_EAX[1] = ((float)(int)((in_ECX >> 0xb) << 0x15) * 9.536743e-07 + 1.0) * 0.0004885198;
  in_EAX[2] = ((float)(int)(in_ECX & 0xffc00000) * 4.7683716e-07 + 1.0) * 0.0009775171;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

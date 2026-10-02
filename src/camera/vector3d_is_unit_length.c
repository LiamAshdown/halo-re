// vector3d_is_unit_length  (Ghidra: vector3d_is_unit_length, already named)
// address 0x4476e0, size 89 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: types/camera.h calls this and real_is_valid / real_approximately_equal "observer
// assertion helpers"; confirmed against objdump that the real return value is a clean 0/1 in AL,
// the same as real_approximately_equal (see that file for the CONCAT31 decompiler artifact note).
// register convention: vector pointer in EAX (in_EAX); no stack parameters.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


// blam-cc: EAX -> v
// Returns whether v is approximately unit length (|v|^2 - 1, within 0.001), guarding against NaN.
uint8_t vector3d_is_unit_length(Vector3D *v)
{
    float length_squared_minus_one;

    length_squared_minus_one = (v->i * v->i + v->j * v->j + v->k * v->k) - 1.0f;
    if (_isnan((double)length_squared_minus_one) != 0) {
        return 0;
    }
    if (length_squared_minus_one < 0.0f) {
        length_squared_minus_one = -length_squared_minus_one;
    }
    return (uint8_t)(length_squared_minus_one < 0.001);
}

#if 0
Original Ghidra decompilation (0x4476e0):

uint vector3d_is_unit_length(void)

{
  float fVar1;
  float *in_EAX;
  uint uVar2;
  ushort uVar3;

  fVar1 = (in_EAX[2] * in_EAX[2] + in_EAX[1] * in_EAX[1] + *in_EAX * *in_EAX) - 1.0;
  uVar2 = __isnan((double)fVar1);
  if (uVar2 == 0) {
    fVar1 = ABS(fVar1);
    uVar3 = (ushort)(fVar1 < 0.001) << 8 | (ushort)NAN(fVar1) << 10 |
            (ushort)(fVar1 == 0.001) << 0xe;
    uVar2 = (uint)uVar3;
    if (fVar1 < 0.001) {
      return CONCAT31((uint3)(byte)(uVar3 >> 8),1);
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

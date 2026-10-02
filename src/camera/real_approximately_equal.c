// real_approximately_equal  (Ghidra: real_approximately_equal, already named)
// address 0x447680, size 55 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: types/camera.h calls this and real_is_valid / vector3d_is_unit_length "observer
// assertion helpers"; confirmed against objdump that the real return value is a clean 0/1 in AL
// ("mov al,1"/"xor al,al"; Ghidra's CONCAT31/bit-packed rendering of the same code is a
// decompiler artifact of the FCOM/FNSTSW sequence, not real behaviour).
// register convention: __cdecl, both values on the stack (Ghidra's recognized param_1, param_2).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "camera.h"


// Returns whether a and b are equal within a small epsilon (0.001), guarding against NaN.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t real_approximately_equal(float a, float b)
{
    float difference;

    if (_isnan((double)(a - b)) != 0) {
        return 0;
    }
    difference = a - b;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return (uint8_t)(difference < 0.001);
}

#if 0
Original Ghidra decompilation (0x447680):

uint real_approximately_equal(float param_1,float param_2)

{
  float fVar1;
  uint uVar2;
  ushort uVar3;

  uVar2 = __isnan((double)(param_1 - param_2));
  if (uVar2 == 0) {
    fVar1 = ABS(param_1 - param_2);
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

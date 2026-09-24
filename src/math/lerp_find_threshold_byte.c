// lerp_find_threshold_byte  (Ghidra: FUN_004cf7a0; renamed, no established name)
// address 0x4cf7a0, size 108 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: math_functions.md: "Finds the largest byte value (0-255) such that
//   lerp(lo,hi,byte/255) is at or below the given threshold". The scan does not start at 255:
//   the function first computes the exact inverse lerp, (threshold - lo) / (hi - lo) * 255,
//   truncates it with __ftol and keeps only AL, then walks that byte down until
//   lerp(lo,hi,b/255) <= threshold. Byte 0xff is special-cased to exactly `hi` rather than
//   going through the multiply, so the top of the range is exact.
// register convention: __cdecl, all three arguments on the stack (lo, hi, threshold); the
//   result is returned in AL only (mov al,cl at 0x4cf806), so it is a byte-wide return.
//
// VERIFIED against the disassembly at 0x4cf7a0 (objdump -d -M intel):
//   0x4cf7a3  fld [esp+0x10] / fsub [esp+0xc] / fstp [esp]      -> span   = hi - lo, spilled once
//   0x4cf7ae  fld [esp+0x14] / fsub [esp+0xc] / fdiv [esp]      -> (threshold - lo) / span
//   0x4cf7b9  fmul ds:0x672b60                                  -> * 255.0   (read from the exe)
//   0x4cf7bf  call 0x6391b4 (__ftol) ; mov cl,al                -> start byte, truncated to 8 bits
//   0x4cf7e6  fmul ds:0x672ad4                                  -> * 0.003921568859368563 (1/255)
//   0x4cf7f7  fcomp / fnstsw / test ah,5 / jp                   -> "threshold >= value" breaks
// Ghidra shows the __ftol call with no argument because the operand is on the x87 stack; the
// earlier rewrite guessed a literal 255.0 there, which is wrong -- the scan is seeded with the
// inverse lerp and usually terminates after one or two steps.
// `span` is computed once in float precision and reloaded from the stack each iteration, so the
// multiply order below (b * (1/255) * span + lo) is the one the FPU actually performs.

#include "tags.h"
#include "math.h"

extern int __ftol(double value); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation

uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold)
{
    real span;
    uint8_t b;
    real value;

    span = hi - lo;
    b = (uint8_t)__ftol((double)((threshold - lo) / span * 255.0f)); // only AL is kept

    while (1) {
        if (b == 0) {
            return 0;
        }
        value = hi;
        if (b != 0xff) {
            value = (real)b * 0.003921569f * span + lo;
        }
        if (value <= threshold) {
            break;
        }
        b = b - 1;
    }
    return b;
}

#if 0
Original Ghidra decompilation (0x4cf7a0):

byte FUN_004cf7a0(float param_1,float param_2,float param_3)

{
  float fVar1;
  byte bVar2;

  bVar2 = FUN_006391b4();
  while( true ) {
    if (bVar2 == 0) {
      return 0;
    }
    fVar1 = param_2;
    if (bVar2 != 0xff) {
      fVar1 = (float)bVar2 * 0.003921569 * (param_2 - param_1) + param_1;
    }
    if (fVar1 <= param_3) break;
    bVar2 = bVar2 - 1;
  }
  return bVar2;
}
#endif

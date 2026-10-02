// ui_real_to_int_truncate  (Ghidra: FUN_004ab590, named in phase 4)
// address 0x4ab590, size 56 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: rewritten from objdump 0x4ab590..0x4ab5c7 in the phase-4 review. Renamed from
// ui_round_half_away_from_zero: it is the inline float to int truncation idiom (fist in the
// default round-to-nearest mode, then undo a round away from zero). The sign test is on the
// raw bits of the argument (js on the dword), not on the rounded value, and the correction
// is -1 for a positive value whose remainder (value - rounded, stored as a float) is
// negative (bits above 0x80000000: add 0x7fffffff sets the carry, sbb) and +1 for a negative
// value whose remainder is positive (bits > 0 as signed). The result is the value truncated
// toward zero, the same as __ftol.
// register convention: plain cdecl, one float stack argument; returns EAX.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern long lrintf(float x); // fist in the default round-to-nearest mode

int32_t ui_real_to_int_truncate(float value)
{
    int32_t rounded = (int32_t)lrintf(value);
    float remainder = value - (float)rounded;
    uint32_t value_bits;
    uint32_t remainder_bits;

    memcpy(&value_bits, &value, sizeof(value_bits));
    memcpy(&remainder_bits, &remainder, sizeof(remainder_bits));
    if ((int32_t)value_bits >= 0) {
        if (remainder_bits > 0x80000000u) {
            rounded--;
        }
    } else if ((int32_t)remainder_bits > 0) {
        rounded++;
    }
    return rounded;
}

#if 0
Original Ghidra decompilation (0x4ab590):

int FUN_004ab590(float param_1)

{
  float fVar1;
  uint uVar2;
  undefined4 local_8;

  fVar1 = param_1 - (float)(int)ROUND(param_1);
  if ((int)param_1 < 0) {
    uVar2 = (uint)(0 < (int)fVar1);
  }
  else {
    uVar2 = -(uint)(0x80000000 < (uint)fVar1);
  }
  local_8 = (int)ROUND(param_1) + uVar2;
  return local_8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

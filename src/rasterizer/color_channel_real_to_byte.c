// color_channel_real_to_byte  (Ghidra: color_channel_real_to_byte, already named)
// address 0x5132b0, size 27 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: sole caller quantizes a normalized color channel; trivial scale-and-round helper,
//   the smallest function in the module.
// register convention: none -- __cdecl, param_1 is the recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t fistp_round(float x); // harness/x87_shims.c: FISTP in the current (round-to-nearest-even) mode

// VERIFIED against disassembly 0x5132b0..0x5132ca (2026-09-30): the original is fmul 255.0, fstp dword, fld, FISTP
// (round to nearest, ties to even, NOT `+ 0.5` then truncate: they differ on exact halves and on negatives) and
// returns only AL. The multiply can overflow past 255 for an out-of-range input; the cast to uint8_t keeps the low
// byte exactly as the original's `mov al, [esp]` does.
uint8_t __cdecl color_channel_real_to_byte(float channel)
{
    return (uint8_t)fistp_round(channel * 255.0f);
}

#if 0
Original Ghidra decompilation (0x5132b0):

uchar __cdecl color_channel_real_to_byte(float param_1)

{
  undefined1 local_4;

  local_4 = (uchar)(int)ROUND(param_1 * 255.0);
  return local_4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

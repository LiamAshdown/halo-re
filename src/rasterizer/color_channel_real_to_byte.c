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

// Converts a normalized [0,1] float color channel to a rounded byte (0..255). The multiply can
// overflow past 255 for an out-of-range input; Ghidra's ROUND() truncates the cast to uint8_t
// exactly as shown, so that overflow is preserved here rather than clamped.
uint8_t __cdecl color_channel_real_to_byte(float channel)
{
    return (uint8_t)(int32_t)(channel * 255.0f + 0.5f); // ROUND(channel * 255.0)
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

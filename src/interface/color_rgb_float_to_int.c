// color_rgb_float_to_int  (Ghidra: color_rgb_float_to_int, already named)
// address 0x4ab5d0, size 90 bytes
// name confidence: 0.55 (existing Ghidra name)   rewrite confidence: 0.85
// evidence: matches the given name and cc (__cdecl); phase-4 summary "Converts a 3-component
// float RGB color into a packed 0x00RRGGBB integer."
// register convention: __cdecl, rgb as the recognized parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"


extern int32_t ROUND(float x); // MSVC round-to-nearest helper
// Packs a 3-float {r, g, b} color (0..1 range) into a 0x00RRGGBB integer.
uint32_t color_rgb_float_to_int(const float *rgb)
{
    return ((uint32_t)(int32_t)ROUND(rgb[2] * 255.0f) & 0xff) |
           (((uint32_t)(int32_t)ROUND(rgb[1] * 255.0f) & 0xff) << 8) |
           (((uint32_t)(int32_t)ROUND(rgb[0] * 255.0f) & 0xff) << 0x10);
}

#if 0
Original Ghidra decompilation (0x4ab5d0):

uint __cdecl color_rgb_float_to_int(float *rgb)

{
  return (int)ROUND(rgb[2] * 255.0) & 0xffU | ((int)ROUND(rgb[1] * 255.0) & 0xffU) << 8 |
         ((int)ROUND(*rgb * 255.0) & 0xffU) << 0x10;
}
#endif

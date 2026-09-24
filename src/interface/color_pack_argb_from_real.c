// color_pack_argb_from_real  (Ghidra: color_pack_argb_from_real, already named)
// address 0x497900, size 99 bytes, cc=__cdecl
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: Ghidra already resolved the cdecl signature and name; packs a {a,r,g,b} float
// ColorARGB (tags.h) into a 0xAARRGGBB uint32_t, rounding each channel to the nearest byte.
// register convention: cdecl, color pointer as the recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t ROUND(float x); // MSVC round-to-nearest helper

// Packs a ColorARGB (float a,r,g,b) into a 0xAARRGGBB uint32_t, rounding each channel to 0..255.
uint32_t color_pack_argb_from_real(ColorARGB *color)
{
    return (uint32_t)ROUND(color->blue * 255.0f) | (uint32_t)ROUND(color->green * 255.0f) << 8 |
           (uint32_t)ROUND(color->red * 255.0f) << 0x10 | (uint32_t)ROUND(color->alpha * 255.0f) << 0x18;
}

#if 0
Original Ghidra decompilation (0x497900):

uint __cdecl color_pack_argb_from_real(float *color)

{
  return (int)ROUND(color[3] * 255.0) | (int)ROUND(color[2] * 255.0) << 8 |
         (int)ROUND(color[1] * 255.0) << 0x10 | (int)ROUND(*color * 255.0) << 0x18;
}
#endif

// color_rgb_int_to_real  (Ghidra: color_rgb_int_to_real, already named)
// address 0x43f630, size 105 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Converts a packed 24-bit RGB color (low 3 bytes
// of in_ECX) into a 3-component float color."); types/tags.h ColorRGB field order
// (red, green, blue). Ghidra's own unreachable-block warnings are dead branches left over from
// the byte-extraction shifts and carry no semantics of their own.
// register convention: EAX = out (ColorRGB *), ECX = packed RGB (uint32_t, low 3 bytes used).
//   // blam-cc: EAX -> out, ECX -> packed color

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> out, ECX -> packed color
void color_rgb_int_to_real(ColorRGB *out, uint32_t packed)
{
    out->red   = (float)((packed >> 16) & 0xff) * 0.003921569f; // 1/255
    out->green = (float)((packed >> 8) & 0xff) * 0.003921569f;
    out->blue  = (float)(packed & 0xff) * 0.003921569f;
}

#if 0
Original Ghidra decompilation (0x43f630):

/* WARNING: Removing unreachable block (ram,0x0043f669) */
/* WARNING: Removing unreachable block (ram,0x0043f646) */
/* WARNING: Removing unreachable block (ram,0x0043f688) */

void color_rgb_int_to_real(void)

{
  float *in_EAX;
  uint in_ECX;

  *in_EAX = (float)(in_ECX >> 0x10 & 0xff) * 0.003921569;
  in_EAX[1] = (float)(in_ECX >> 8 & 0xff) * 0.003921569;
  in_EAX[2] = (float)(in_ECX & 0xff) * 0.003921569;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

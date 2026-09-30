// color_argb_int_to_real  (Ghidra: color_argb_int_to_real, already named)
// address 0x43f5a0, size 135 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Converts a packed 32-bit ARGB color into a
// 4-component float color (each channel /255)."); types/tags.h ColorARGB field order
// (alpha, red, green, blue); Ghidra's own unreachable-block warnings are dead branches left
// over from the byte-extraction shifts and carry no semantics of their own.
// register convention: EAX = out (ColorARGB *), ECX = packed ARGB (uint32_t).
//   // blam-cc: EAX -> out, ECX -> packed color

#include "tags.h"
#include "memory.h"
#include "bitmaps.h"
#include "fn_bitmaps.h"

// blam-cc: EAX -> out, ECX -> packed color
void color_argb_int_to_real(ColorARGB *out, uint32_t packed)
{
    out->alpha = (float)((packed >> 24) & 0xff) * 0.003921569f; // 1/255
    out->red   = (float)((packed >> 16) & 0xff) * 0.003921569f;
    out->green = (float)((packed >> 8) & 0xff) * 0.003921569f;
    out->blue  = (float)(packed & 0xff) * 0.003921569f;
}

#if 0
Original Ghidra decompilation (0x43f5a0):

/* WARNING: Removing unreachable block (ram,0x0043f5f7) */
/* WARNING: Removing unreachable block (ram,0x0043f5d3) */
/* WARNING: Removing unreachable block (ram,0x0043f616) */
/* WARNING: Removing unreachable block (ram,0x0043f5b0) */

void color_argb_int_to_real(void)

{
  float *in_EAX;
  uint in_ECX;

  *in_EAX = (float)(in_ECX >> 0x18) * 0.003921569;
  in_EAX[1] = (float)(in_ECX >> 0x10 & 0xff) * 0.003921569;
  in_EAX[2] = (float)(in_ECX >> 8 & 0xff) * 0.003921569;
  in_EAX[3] = (float)(in_ECX & 0xff) * 0.003921569;
  return;
}
#endif

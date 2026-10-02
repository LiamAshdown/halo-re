// color_argb_scale_alpha  (Ghidra: color_argb_scale_alpha, already named)
// address 0x497970, size 52 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: matches the given name exactly -- keeps a packed 0xAARRGGBB color's RGB bits and
// rescales its alpha byte by a float factor.
// register convention: packed color in EAX (in_EAX, unresolved register read), scale factor as
// the recognized stack parameter.
// blam-cc: EAX -> packed_color, stack -> scale

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t ROUND(float x); // MSVC round-to-nearest helper

// blam-cc: EAX -> packed_color, stack -> scale
// Rescales a packed 0xAARRGGBB color's alpha byte by `scale`, leaving RGB untouched.
uint32_t color_argb_scale_alpha(uint32_t packed_color, float scale)
{
    return (packed_color & 0xffffff) | (uint32_t)ROUND((float)(packed_color >> 0x18) * scale) << 0x18;
}

#if 0
Original Ghidra decompilation (0x497970):

uint color_argb_scale_alpha(float param_1)

{
  uint in_EAX;

  return in_EAX & 0xffffff | (int)ROUND((float)(in_EAX >> 0x18) * param_1) << 0x18;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

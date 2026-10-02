// color_real_to_argb_pack  (already named; task-provided, cc=__cdecl per Ghidra)
// address 0x44da60, size 106 bytes
// name confidence: 0.7 (already carries this name; Ghidra fully recognized the cdecl signature)
// rewrite confidence: 0.75 (trivial pack, confirmed against the decompilation; same pattern as
//   the existing src/interface/color_pack_argb_from_real.c, which packs a single ColorARGB
//   struct instead of a separate alpha + 3-float rgb array)
// evidence: out/phase4/effects_types_notes.md "0x44da60 color_real_to_argb_pack | math or
//   cseries | generic quantise-and-pack".
// register convention: cdecl, per Ghidra: float alpha, float *rgb (3 floats, r/g/b order).
// blam-cc: color_real_to_argb_pack(float alpha, float *rgb) -- cdecl

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ROUND(float x); // MSVC round-to-nearest helper

uint32_t color_real_to_argb_pack(float alpha, float *rgb)
{
    return ((uint32_t)ROUND(rgb[2] * 255.0f) & 0xff) |
           (((uint32_t)ROUND(rgb[1] * 255.0f) & 0xff) << 8) |
           (((uint32_t)ROUND(rgb[0] * 255.0f) & 0xff) << 0x10) |
           ((uint32_t)ROUND(alpha * 255.0f) << 0x18);
}

#if 0
Original Ghidra decompilation (0x44da60):

uint __cdecl color_real_to_argb_pack(float alpha,float *rgb)

{
  return (int)ROUND(rgb[2] * 255.0) & 0xffU | ((int)ROUND(rgb[1] * 255.0) & 0xffU) << 8 |
         ((int)ROUND(*rgb * 255.0) & 0xffU) << 0x10 | (int)ROUND(alpha * 255.0) << 0x18;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

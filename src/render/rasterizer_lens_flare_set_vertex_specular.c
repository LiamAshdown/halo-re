// rasterizer_lens_flare_set_vertex_specular  (Ghidra: FUN_00512120; new name, evidence below)
// address 0x512120, size 44 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: types/render.h globals list: "0x0069e708 lens_flare_vertex_specular, written by
//   0x512120". Converts a normalized [0,1] intensity to a byte (x * 255, truncated) and
//   broadcasts it into all four bytes of the packed ARGB specular value every lens flare vertex
//   shares.
// register convention: one stack argument, a float.
//   // blam-cc: stack -> intensity
// UNSURE: the shared .rdata constant at 0x00672b60 is assumed to be 255.0 (the standard
//   normalized-to-byte scale used throughout this binary's color packing code); not confirmed by
//   any typed reader in this module.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t lens_flare_vertex_specular; // 0x0069e708
extern float text_color_scale;              // 0x00672b60 UNSURE: assumed to be 255.0

// __ftol (0x6391b4, input on the FPU stack, chops toward zero) is written as a (long long) cast below

// Scales a normalized intensity to a byte and broadcasts it into all four bytes of the packed
// ARGB specular color every lens flare vertex shares.
void rasterizer_lens_flare_set_vertex_specular(float intensity)
{
    uint32_t byte_value;
    uint32_t packed;

    byte_value = (uint32_t)(int32_t)(long long)((double)(intensity * text_color_scale)) & 0xff;
    packed = byte_value;
    packed = (packed << 8) | byte_value;
    packed = (packed << 8) | byte_value;
    packed = (packed << 8) | byte_value;
    lens_flare_vertex_specular = packed;
}

#if 0
Original Ghidra decompilation (0x512120):

void FUN_00512120(void)

{
  uint uVar1;

  uVar1 = __ftol();
  uVar1 = uVar1 & 0xff;
  DAT_0069e708 = ((uVar1 << 8 | uVar1) << 8 | uVar1) << 8 | uVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

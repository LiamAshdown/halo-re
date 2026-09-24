// rasterizer_shader_environment_select_draw_functions  (Ghidra: already named)
// address 0x52b630, size 84 bytes
// name confidence: 0.55   rewrite confidence: 0.95
// evidence: raw disassembly (phase 4 review; the earlier file tested the wrong capability and
//   pointed both globals at the same function). It compares D3DCAPS9.MaxStreams (0x007c117c)
//   against 1 and PixelShaderVersion (0x007c118c) against ps_1_1 and stores one procedure pair:
//     MaxStreams <= 1:    0x5276c0 / 0x528be0 rasterizer_shader_model_draw_limited
//     below ps_1_1:       0x527ae0 / 0x529230 rasterizer_shader_model_draw_fixed_function
//     otherwise:          0x528050 / 0x529e00 rasterizer_shader_model_draw_pixel_shader
//   0x007c0470 is called by rasterizer_shader_environment_draw_dispatch 0x52b050 for
//   shader_type 3 (environment) and 0x007c0474 for every other shader type. 0x5276c0, 0x527ae0
//   and 0x528050 are complete functions of their own (int3 padding before each, their own
//   prologue and ret) that Ghidra never defined, so they have no rewrite in this directory.
// register convention: none.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;            // 0x007c10c0
extern void *shader_environment_draw_simple; // 0x007c0470 procedure for shader_type 3
extern void *shader_environment_draw;        // 0x007c0474 procedure for the other types

extern void FUN_005276c0(void);  // 0x5276c0 not a Ghidra function; environment draw, one stream
extern void FUN_00527ae0(void);  // 0x527ae0 not a Ghidra function; environment draw, fixed function
extern void FUN_00528050(void);  // 0x528050 not a Ghidra function; environment draw, pixel shader
extern void rasterizer_shader_model_draw_limited(void);      // 0x528be0
extern void rasterizer_shader_model_draw_fixed_function(void); // 0x529230
extern void rasterizer_shader_model_draw_pixel_shader(void);  // 0x529e00

void rasterizer_shader_environment_select_draw_functions(void)
{
    if ((int32_t)rasterizer_caps.max_streams <= 1) {
        shader_environment_draw_simple = (void *)FUN_005276c0;
        shader_environment_draw = (void *)rasterizer_shader_model_draw_limited;
        return;
    }
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        shader_environment_draw_simple = (void *)FUN_00527ae0;
        shader_environment_draw = (void *)rasterizer_shader_model_draw_fixed_function;
        return;
    }
    shader_environment_draw_simple = (void *)FUN_00528050;
    shader_environment_draw = (void *)rasterizer_shader_model_draw_pixel_shader;
}

#if 0
Original Ghidra decompilation (0x52b630):

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void rasterizer_shader_environment_select_draw_functions(void)

{
  if (DAT_007c117c < 2) {
    DAT_007c0470 = &LAB_005276c0;
    DAT_007c0474 = FUN_00528be0;
    return;
  }
  if (DAT_007c118c < 0xffff0101) {
    DAT_007c0470 = &LAB_00527ae0;
    DAT_007c0474 = FUN_00529230;
    return;
  }
  DAT_007c0470 = &LAB_00528050;
  DAT_007c0474 = rasterizer_shader_environment_draw_pixelshader;
  return;
}
#endif

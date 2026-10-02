// rasterizer_glass_draw_procedures_select  (Ghidra: FUN_00523ec0)
// address 0x523ec0, size 74 bytes
// phase 4 review: renamed from the rasterizer_decal_draw_* family. The procedure table
//   0x007c0480 holds ShaderTransparentGlass draw procedures: the shader fields these functions
//   read are glass fields (background_tint_map +0x70, diffuse_map +0x164, diffuse_detail_map
//   +0x178, reflection_map +0xb8, bump_map +0xcc; types/tags.h), not decal fields.
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase2/results/rasterizer_01.json ("Chooses the pre- or post-1.1 pixel-shader-
// capability set of decal-drawing function pointers."); types/rasterizer.h documents
// `rasterizer_glass_draw_procedures[3]` at 0x007c0480 as exactly this address's output. No register
// ambiguity.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_glass_draw_procedures[3];                // 0x007c0480

extern void rasterizer_glass_tint_draw_fixed_function(transparent_geometry_group *group);   // 0x523980
extern void rasterizer_glass_reflection_draw_fixed_function(transparent_geometry_group *group, uint32_t reflection_kind); // 0x523b90
extern void rasterizer_glass_diffuse_draw_fixed_function(transparent_geometry_group *group); // 0x523d10
extern void rasterizer_glass_tint_draw(transparent_geometry_group *group); // 0x522930
extern void rasterizer_glass_reflection_draw(transparent_geometry_group *group, int16_t reflection_kind); // 0x522c60
extern void rasterizer_glass_diffuse_draw(transparent_geometry_group *group); // 0x523690

void rasterizer_glass_draw_procedures_select(void)
{
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        rasterizer_glass_draw_procedures[1] = (void *)rasterizer_glass_tint_draw_fixed_function;
        rasterizer_glass_draw_procedures[2] = (void *)rasterizer_glass_reflection_draw_fixed_function;
        rasterizer_glass_draw_procedures[0] = (void *)rasterizer_glass_diffuse_draw_fixed_function;
        return;
    }
    rasterizer_glass_draw_procedures[1] = (void *)rasterizer_glass_tint_draw;
    rasterizer_glass_draw_procedures[2] = (void *)rasterizer_glass_reflection_draw;
    rasterizer_glass_draw_procedures[0] = (void *)rasterizer_glass_diffuse_draw;
}

#if 0
Original Ghidra decompilation (0x523ec0):

void FUN_00523ec0(void)

{
  if (DAT_007c118c < 0xffff0101) {
    DAT_007c0484 = FUN_00523980;
    DAT_007c0488 = FUN_00523b90;
    DAT_007c0480 = FUN_00523d10;
    return;
  }
  DAT_007c0484 = FUN_00522930;
  DAT_007c0488 = FUN_00522c60;
  DAT_007c0480 = FUN_00523690;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

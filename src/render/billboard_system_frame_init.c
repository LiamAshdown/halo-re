// billboard_system_frame_init  (Ghidra: FUN_00511410; new name, evidence below)
// address 0x511410, size 262 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: types/render.h globals "0x007c30c4 float build_sprite_screen_coverage (0x511410
//   resets, 0x511700 adds)", "0x007c30c8 int16_t build_sprite_large_quad_count", "0x007c30d0
//   real_vector3d build_sprite_view_up (world k axis in view space)", "0x007c30dc real_vector3d
//   build_sprite_view_left (world j axis in view space)". types/math.h global_up3d_pointer
//   (0x00696720) / global_left3d_pointer (0x0069671c). The inline dot-product expressions here
//   are matrix4x3_transform_normal's own formula (normal.i*forward+normal.j*left+normal.k*up per
//   the axis), so this is that transform inlined rather than called.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_math.h"

extern float build_sprite_screen_coverage;      // 0x007c30c4
extern int16_t build_sprite_large_quad_count;   // 0x007c30c8
extern real_vector3d build_sprite_view_up;      // 0x007c30d0
extern real_vector3d build_sprite_view_left;    // 0x007c30dc

extern render_frustum render_frustum_global;    // 0x007c3168, this module

extern const real_vector3d *global_up3d_pointer;   // 0x00696720
extern const real_vector3d *global_left3d_pointer; // 0x0069671c


// Per-frame initialization for the billboard/sprite build system: resets the large-quad
// occlusion accumulators and computes the world up/left axes in view space (through
// render_frustum_global.world_to_view) for use by build_sprite's render-type-2 orientation.
void billboard_system_frame_init(void)
{
    build_sprite_screen_coverage = 0.0f;
    build_sprite_large_quad_count = 0;

    matrix4x3_transform_normal(&build_sprite_view_up, (real_vector3d *)global_up3d_pointer,
                                &render_frustum_global.world_to_view);
    matrix4x3_transform_normal(&build_sprite_view_left, (real_vector3d *)global_left3d_pointer,
                                &render_frustum_global.world_to_view);
}

#if 0
Original Ghidra decompilation (0x511410):

void FUN_00511410(void)

{
  float fVar1;
  float fVar2;
  float fVar3;

  _DAT_007c30c4 = 0;
  DAT_007c30c8 = 0;
  fVar1 = *(float *)PTR_DAT_00696720;
  fVar2 = *(float *)(PTR_DAT_00696720 + 4);
  fVar3 = *(float *)(PTR_DAT_00696720 + 8);
  _DAT_007c30d0 = DAT_007c317c * fVar1 + _DAT_007c3188 * fVar2 + _DAT_007c3194 * fVar3;
  _DAT_007c30d4 = _DAT_007c3180 * fVar1 + _DAT_007c318c * fVar2 + _DAT_007c3198 * fVar3;
  _DAT_007c30d8 = _DAT_007c3184 * fVar1 + _DAT_007c3190 * fVar2 + _DAT_007c319c * fVar3;
  fVar1 = *(float *)PTR_DAT_0069671c;
  fVar2 = *(float *)(PTR_DAT_0069671c + 4);
  fVar3 = *(float *)(PTR_DAT_0069671c + 8);
  _DAT_007c30dc = DAT_007c317c * fVar1 + _DAT_007c3188 * fVar2 + _DAT_007c3194 * fVar3;
  _DAT_007c30e0 = _DAT_007c3180 * fVar1 + _DAT_007c318c * fVar2 + _DAT_007c3198 * fVar3;
  _DAT_007c30e4 = _DAT_007c3184 * fVar1 + _DAT_007c3190 * fVar2 + _DAT_007c319c * fVar3;
  return;
}
#endif

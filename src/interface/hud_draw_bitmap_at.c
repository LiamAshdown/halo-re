// hud_draw_bitmap_at  (Ghidra: FUN_004acbb0, renamed in the phase-4 review)
// address 0x4acbb0, size 152 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4acbb0..0x4acc47 in the phase-4 review. Same flow as
// hud_draw_bitmap_element (0x4acad0) without a placement: the screen position comes in
// ready-made (second stack argument) and one scale is used for both axes. No meter
// parameters (0 pushed as arg 1 of 0x4acd50). The first rewrite built a {0,1,0,1} rect and
// passed the packed color as a float depth. Callers: hud_draw_number (digit glyphs),
// hud_waypoint_draw (arrow), 0x4ad970 (hud message icons), hud_draw_damage_indicators.
// register convention: EAX uv (NULL for the whole bitmap), EDX bitmap, CL pixel_uvs; five stack
// arguments.
//   // blam-cc: uv -> EAX, bitmap -> EDX, pixel_uvs -> CL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hud_bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv,
                                      float *out_extents, int16_t anchor); // 0x4acc50, blam-cc: CL, ESI, EDX, EAX
extern void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale,
                                         void *meter_parameters, BitmapData *bitmap, const float *uv,
                                         const float *extents, float rotation, uint32_t color); // 0x4acd50, blam-cc: EAX, ESI

// blam-cc: uv -> EAX, bitmap -> EDX, pixel_uvs -> CL
// Draws one HUD bitmap quad whose anchor corner (0..4, see hud_bitmap_anchor_extents) sits at
// screen_position, scaled by scale on both axes and rotated by rotation radians.
void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                        const Point2DInt *screen_position, float scale, float rotation, uint32_t color)
{
    float default_uv[4];
    float both_scale[2];
    float extents[4];

    default_uv[0] = 0.0f;
    default_uv[1] = 1.0f;
    default_uv[2] = 0.0f;
    default_uv[3] = 1.0f;
    if (pixel_uvs != 0) {
        default_uv[1] = (float)(int32_t)(int16_t)bitmap->width;
        default_uv[3] = (float)(int32_t)(int16_t)bitmap->height;
    }
    if (uv == 0) {
        uv = default_uv;
    }
    both_scale[0] = scale;
    both_scale[1] = scale;
    hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, anchor);
    hud_draw_rotated_bitmap_quad(screen_position, both_scale, 0, bitmap, uv, extents, rotation, color);
}

#if 0
Original Ghidra decompilation (0x4acbb0):

void FUN_004acbb0(undefined4 param_1)

{
  FUN_004acc50(param_1);
  FUN_004acd50(0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

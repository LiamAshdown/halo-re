// hud_draw_bitmap_element  (Ghidra: FUN_004acad0, renamed in the phase-4 review)
// address 0x4acad0, size 216 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4acad0..0x4acba7 in the phase-4 review. The first rewrite
// had no placement argument (EDX), took arg 1 for a "mode" and passed an uninitialized
// destination rect on. The real flow: default uv {0, 1, 0, 1} (or {0, width, 0, height} for
// an interface bitmap, BL), width and height scale from EDX +4/+8 times the scale argument,
// 0x4ab690 (AL = split screen flag unless scaling flag bit 0, EDX the placement, ECX NULL,
// stack anchor, 0.0, out) writes the screen position into the stack slot of the last
// argument, 0x4acc50 the extents, and 0x4acd50 draws. Callers: hud_meter_draw_fill (arg 1 is
// its hud_meter_color_block), hud_draw_static_element, hud_draw_overlays,
// hud_weapon_crosshairs_draw, hud_render_unit_interface and the unit ammo meters.
// register convention: EAX uv (NULL for the whole bitmap), EDX placement, BL pixel_uvs; seven
// stack arguments, the last one a byte.
//   // blam-cc: uv -> EAX, placement -> EDX, pixel_uvs -> BL

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

extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector); // 0x4ab690, blam-cc: AL has_scale, EDX offset, ECX child placement (selector)
extern void hud_bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv,
                                      float *out_extents, int16_t anchor); // 0x4acc50, blam-cc: CL, ESI, EDX, EAX
extern void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale,
                                         void *meter_parameters, BitmapData *bitmap, const float *uv,
                                         const float *extents, float rotation, uint32_t color); // 0x4acd50, blam-cc: EAX, ESI

// blam-cc: uv -> EAX, placement -> EDX, pixel_uvs -> BL
// Draws one HUD bitmap quad placed by a HUD interface element: its anchor offset, width and
// height scale (times scale) and the anchor corner. uv is {u0, u1, v0, v1}; color is packed
// ARGB; split_screen asks 0x4ab690 to scale the anchor offset for a split screen view.
void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                             void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                             float scale, float rotation, uint32_t color, uint8_t split_screen)
{
    float default_uv[4];
    float element_scale[2];
    float extents[4];
    Point2DInt screen_position; // the binary reuses the stack slot of split_screen
    uint8_t scale_offset;

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
    element_scale[0] = scale * placement->width_scale;
    element_scale[1] = scale * placement->height_scale;

    scale_offset = 0;
    if (split_screen != 0 && (*(const uint8_t *)&placement->scaling_flags & 1) == 0) {
        scale_offset = 1;
    }
    hud_anchor_offset_to_screen_position(anchor, scale_offset, 0.0f, &placement->anchor_offset.x,
                                         &screen_position.x, 0);
    hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, (int16_t)*anchor);
    hud_draw_rotated_bitmap_quad(&screen_position, element_scale, meter_parameters, bitmap, uv, extents,
                                 rotation, color);
}

#if 0
Original Ghidra decompilation (0x4acad0):

void FUN_004acad0(undefined4 param_1,int param_2,undefined2 *param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined1 param_7)

{
  undefined4 *in_EAX;
  char unaff_BL;
  undefined4 local_20;
  float local_1c;
  undefined4 local_18;
  float local_14;
  undefined1 local_10 [16];

  local_20 = 0;
  local_1c = 1.0;
  local_18 = 0;
  local_14 = 1.0;
  if (unaff_BL != '\0') {
    local_1c = (float)(int)*(short *)(param_2 + 4);
    local_14 = (float)(int)*(short *)(param_2 + 6);
  }
  if (in_EAX == (undefined4 *)0x0) {
    in_EAX = &local_20;
  }
  FUN_004ab690(param_3,0,&param_7);
  FUN_004acc50(*param_3);
  FUN_004acd50(param_1,param_2,in_EAX,local_10,param_5,param_6);
  return;
}
#endif

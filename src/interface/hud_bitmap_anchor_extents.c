// hud_bitmap_anchor_extents  (Ghidra: FUN_004acc50, renamed in the phase-4 review)
// address 0x4acc50, size 87 bytes (plus the five entry jump table at 0x4acd34)
// name confidence: 0.6 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4acc50..0x4acd48 in the phase-4 review. The first rewrite
// treated the jump table at 0x4acd34 as a table of function pointers; its five targets
// (0x4acca7, 0x4accbb, 0x4accd1, 0x4acce7, 0x4accff) are in-function cases that each write the
// four floats of EAX: the quad extents around the anchor point for anchor 0 top left, 1 top
// right, 2 bottom left, 3 bottom right, 4 center (constants 0x672bf4 = -0.5, 0x672abc = 0.5).
// There is no bounds check on the anchor; every caller passes a HUDInterfaceChildAnchor (0..4).
// Callers: hud_draw_bitmap_element (0x4acad0), hud_draw_bitmap_at (0x4acbb0) and
// hud_draw_static_element (0x4ac6f0).
// register convention: CL pixel_uvs, ESI bitmap, EDX uv rect, EAX out; anchor on the stack.
//   // blam-cc: CL -> pixel_uvs, ESI -> bitmap, EDX -> uv, EAX -> out_extents, stack -> anchor

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
#include "fn_interface.h"

// blam-cc: CL -> pixel_uvs, ESI -> bitmap, EDX -> uv, EAX -> out_extents, stack -> anchor
// Size of one HUD bitmap quad in screen pixels, laid out around its anchor point. uv is the
// {u0, u1, v0, v1} source rectangle: already in pixels for an interface bitmap (pixel_uvs), or
// normalized, in which case the span is multiplied by the bitmap width and height.
// out_extents is {x0, x1, y0, y1} relative to the anchor point.
void hud_bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv,
                               float *out_extents, int16_t anchor)
{
    int32_t width_factor;
    int32_t height_factor;
    float width;
    float height;

    width_factor = pixel_uvs != 0 ? 1 : (int32_t)(int16_t)bitmap->width;
    width = (uv[1] - uv[0]) * (float)width_factor;
    height_factor = pixel_uvs != 0 ? 1 : (int32_t)(int16_t)bitmap->height;
    height = (uv[3] - uv[2]) * (float)height_factor;

    switch (anchor) {
    case 0: // top left
        out_extents[0] = 0.0f;
        out_extents[1] = width;
        out_extents[2] = 0.0f;
        out_extents[3] = height;
        break;
    case 1: // top right
        out_extents[0] = -width;
        out_extents[1] = 0.0f;
        out_extents[2] = 0.0f;
        out_extents[3] = height;
        break;
    case 2: // bottom left
        out_extents[0] = 0.0f;
        out_extents[1] = width;
        out_extents[2] = -height;
        out_extents[3] = 0.0f;
        break;
    case 3: // bottom right
        out_extents[0] = -width;
        out_extents[1] = 0.0f;
        out_extents[2] = -height;
        out_extents[3] = 0.0f;
        break;
    case 4: // center
        out_extents[0] = width * -0.5f;
        out_extents[1] = width * 0.5f;
        out_extents[2] = -0.5f * height;
        out_extents[3] = height * 0.5f;
        break;
    }
}

#if 0
Original Ghidra decompilation (0x4acc50):

void FUN_004acc50(short param_1)

{
  char in_CL;
  float *in_EDX;
  int unaff_ESI;
  int iVar1;
  int iVar2;

  if (in_CL == '\0') {
    iVar1 = (int)*(short *)(unaff_ESI + 4);
  }
  else {
    iVar1 = 1;
  }
  if (in_CL == '\0') {
    iVar2 = (int)*(short *)(unaff_ESI + 6);
  }
  else {
    iVar2 = 1;
  }
                    /* WARNING: Could not recover jumptable at 0x004acca0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&PTR_LAB_004acd34)[param_1])((in_EDX[1] - *in_EDX) * (float)iVar1,iVar2);
  return;
}
#endif

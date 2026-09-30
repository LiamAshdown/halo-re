// render_billboard_compute_scale  (Ghidra: FUN_00511330; new name, evidence below)
// address 0x511330, size 113 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: phase4 one-liner: "Computes a length/width scale factor for a billboard segment
// based on its marker count or projected depth." render_frustum_global.projection_world_to_screen.i
// (0x007c32ec, frustum + 0x184) matches types/render.h's projection_world_to_screen field.
// register convention (objdump 0x511330..0x5113a0): EAX = build_sprite_data, ECX = scale (in/out
//   float*), stack = (render_type, position, bitmap).
//   // blam-cc: EAX -> data, ECX -> scale, stack -> render_type/position/bitmap
// review fix (phase-4 gate): the third stack argument is the BitmapData* of the sprite (build_sprite
//   0x511700 pushes [ebp-0x10], computed as bitmap_data.pointer + index*0x30 at 0x511790), so the
//   int16 read at +0x04 (movsx) is BitmapData.width, not a marker count; CEA agrees
//   (build_sprite_compute_scale(data, mode, origin, bitmap, scale)).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_render.h"

extern render_frustum render_frustum_global; // 0x007c3168, this module

// blam-cc: EAX -> data, ECX -> scale, stack -> render_type/position/bitmap
void render_billboard_compute_scale(build_sprite_data *data, float *scale, int16_t render_type,
                                     real_point3d *position, BitmapData *bitmap)
{
    int16_t width = (int16_t)bitmap->width; // movsx: read as a signed word

    if ((data->flags & _build_sprite_data_screen_space_bit) == 0) {
        if (render_type == 0 && *scale == 0.0f) {
            *scale = -(position->z / render_frustum_global.projection_world_to_screen.i);
        }
        *scale = (float)width * *scale;
        return;
    }

    if (*scale == 0.0f) {
        *scale = 1.0f;
        *scale = (float)width * *scale;
        return;
    }
    *scale = (float)width * *scale;
}

#if 0
Original Ghidra decompilation (0x511330):

void FUN_00511330(short param_1,int param_2,int param_3)

{
  int in_EAX;
  float *in_ECX;

  if ((*(byte *)(in_EAX + 0x10) & 1) == 0) {
    if ((param_1 == 0) && (*in_ECX == 0.0)) {
      *in_ECX = -(*(float *)(param_2 + 8) / _DAT_007c32ec);
    }
  }
  else if (*in_ECX == 0.0) {
    *in_ECX = 1.0;
    *in_ECX = (float)(int)*(short *)(param_3 + 4) * *in_ECX;
    return;
  }
  *in_ECX = (float)(int)*(short *)(param_3 + 4) * *in_ECX;
  return;
}
#endif

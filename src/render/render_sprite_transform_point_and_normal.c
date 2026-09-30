// render_sprite_transform_point_and_normal  (Ghidra: FUN_00511190; new name, evidence below)
// address 0x511190, size 96 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: types/render.h build_sprite_data flags (_build_sprite_data_screen_space_bit at +0x10
//   bit 0) and build_sprite_flags (_build_sprite_already_transformed_bit); called from build_sprite
//   0x511700 to place a sprite's origin (and optional normal) into view space, either by
//   transforming it through render_frustum's world_to_view (0x007c3178 = render_frustum_global +
//   0x10) or by copying it through unchanged when the caller has already done so.
// register convention (objdump 0x511190..0x5111ef): EDX = position, ESI = normal (may be NULL),
//   EDI = out_normal, stack = (data, flags, out_position).
//   // blam-cc: EDX -> position, ESI -> normal, EDI -> out_normal, stack -> data/flags/out_position
// UNSURE: parameter names/purpose inferred from the call site and struct flags only; not
//   independently confirmed against build_sprite's own callers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_math.h"

extern render_frustum render_frustum_global; // 0x007c3168, this module (render_nonplayer_frame.c)

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0


// Transforms a sprite's origin (and, if given, its normal) from world space into view space
// through render_frustum_global.world_to_view, unless the sprite build is screen-space (data's
// screen_space flag) or the caller marks the values as already transformed, in which case they
// are copied through unchanged.
void render_sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal,
                                               real_vector3d *out_normal, build_sprite_data *data,
                                               uint8_t flags, real_point3d *out_position)
{
    if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
        return;
    }

    if ((flags & _build_sprite_already_transformed_bit) == 0) {
        matrix4x3_transform_point(out_position, position, &render_frustum_global.world_to_view);
        if (normal != 0) {
            matrix4x3_transform_normal(out_normal, normal, &render_frustum_global.world_to_view);
        }
    } else {
        *out_position = *position;
        if (normal != 0) {
            *out_normal = *normal;
        }
    }
}

#if 0
Original Ghidra decompilation (0x511190):

void FUN_00511190(int param_1,byte param_2,undefined4 *param_3)

{
  undefined4 *in_EDX;
  undefined4 *unaff_ESI;
  undefined4 *unaff_EDI;

  if ((*(byte *)(param_1 + 0x10) & 1) == 0) {
    if ((param_2 & 1) == 0) {
      matrix4x3_transform_point(&DAT_007c3178);
      if (unaff_ESI != (undefined4 *)0x0) {
        matrix4x3_transform_normal(&DAT_007c3178);
      }
    }
    else {
      *param_3 = *in_EDX;
      param_3[1] = in_EDX[1];
      param_3[2] = in_EDX[2];
      if (unaff_ESI != (undefined4 *)0x0) {
        *unaff_EDI = *unaff_ESI;
        unaff_EDI[1] = unaff_ESI[1];
        unaff_EDI[2] = unaff_ESI[2];
        return;
      }
    }
  }
  return;
}
#endif

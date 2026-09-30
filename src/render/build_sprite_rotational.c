// build_sprite_rotational  (Ghidra: contrail_draw_segment_blended, phase-2 name; CEA
// build_sprite_rotational(data, flags, first_sequence_index, sprite_index, untransformed_origin,
// untransformed_axis_of_rotation, rotation, scale, color, fade), hint only; renamed)
// address 0x511b40, size 564 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: objdump -d -M intel 0x511b40..0x511d73. Only callers are the particle system
//   renderer (0x455069, 0x4551f4, per out/phase4/render_types_notes.md), not contrail code.
//   - arguments: EAX = build_sprite_data (moved to EBX, which is also build_sprite's data
//     register), nine stack arguments in the CEA order.
//   - render_sprite_transform_point_and_normal 0x511190 (EDX origin, ESI axis, EDI out axis,
//     stack data, flags & 1, out origin), then vector3d_angle_between_4cd4f0 (ECX axis,
//     EDX origin): d = angle - pi/2 (0x672d3c), t = d * d * 4/pi^2 (0x673208), so t is 0 when
//     the axis is perpendicular to the line of sight and 1 when it points at the viewer.
//   - end-on quad (when t > 0.05, t clamped to 1): sequence first + 1, camera facing (mode 0),
//     fade * t. With flags bit 1 the sprite is picked from the rotation angle:
//     (int)(fmod(count * rotation / (2 pi) + 0.5, count) + sprite_index), where count is the
//     sprite count of sequence first + 1 (the low word at sequence + 0x40 + 0x34, read at
//     0x511c0a), and the quad itself gets rotation 0; when d < 0 that index is then replaced by
//     count - sprite_index (0x511c6e, the ftol result in ECX is overwritten). Without flag bit 1
//     the sprite is sprite_index, the quad keeps `rotation`, and d < 0 mirrors it in u
//     (build_sprite flags 3 instead of 1).
//   - side quad (when 1 - t > 0.05): sequence first, mode 0, rotation atan2(axis.j, axis.i)
//     (fpatan with st1 = axis.j, st0 = axis.i), sprite (int)fmod(count * rotation / (2 pi) +
//     0.5, count) with count the sprite count of sequence first, fade * (1 - t), flags 1.
//   - both build_sprite calls pass the already transformed origin, a NULL direction and
//     build_sprite flag bit 0 (already transformed).
//   - 0x628cca is the MSVC CRT _CIfmod (fmod(st1, st0)); 0x6391b4 is __ftol (truncation).
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: EAX = data, nine stack arguments.
//   // blam-cc: EAX=data, stack=(flags, first_sequence_index, sprite_index, origin, axis,
//   //          rotation, scale, color, fade)
// UNSURE: the sequence reflexive of the Bitmap tag is dereferenced without a bounds check for
//   first_sequence_index + 1 (as in the binary); the count - sprite_index replacement for d < 0
//   looks like a mirrored frame pick and is reproduced literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_math.h"

extern tag_instance *tag_instances;       // 0x0087bc14, cache module
extern const ColorARGB *global_white_argb; // 0x006851fc, points at {1,1,1,1} 0x00655138

extern void render_sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal,
    real_vector3d *out_normal, build_sprite_data *data, uint8_t flags, real_point3d *out_position);
    // 0x511190, this module; blam-cc: EDX -> position, ESI -> normal, EDI -> out_normal,
    // stack -> data/flags/out_position

    // 0x4cd4f0, math module; blam-cc: ECX -> a, EDX -> b
extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index,
                         int16_t mode, real_point3d *origin, real_vector3d *direction,
                         float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
    // 0x511700, this module; blam-cc: EBX=data, AX=sequence_index, CX=sprite_index, stack=rest
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod
extern double atan2(double y, double x); // x87 FPATAN

// Draws a sprite that has an axis (a spinning or rotating particle): as a blend of an end-on,
// camera facing quad from the next sequence and a side-on quad rotated to the projected axis,
// weighted by how directly the axis points at the viewer.
void build_sprite_rotational(build_sprite_data *data, uint32_t flags, int16_t first_sequence_index,
                             int16_t sprite_index, real_point3d *origin, real_vector3d *axis,
                             float rotation, float scale, ColorARGB *color, float fade)
    // blam-cc: EAX=data, stack=flags..fade
{
    real_point3d transformed_origin;
    real_vector3d transformed_axis;
    Bitmap *bitmap_group;
    BitmapGroupSequence *sequences;
    real d;
    real t;

    if (color == 0) {
        color = (ColorARGB *)global_white_argb;
    }

    render_sprite_transform_point_and_normal(origin, axis, &transformed_axis, data,
                                             (uint8_t)(flags & 1), &transformed_origin);
    d = vector3d_angle_between_4cd4f0(&transformed_axis, (real_vector3d *)&transformed_origin) -
        1.5707964f;
    t = d * d * 0.40528470f;

    if (t < 0.0f) {
        t = 0.0f;
    } else {
        uint8_t draw = 1;

        if (t > 1.0f) {
            t = 1.0f;
        } else if (t <= 0.05f) {
            draw = 0;
        }
        if (draw) {
            int16_t count;
            int16_t sprite;
            float quad_rotation;
            uint32_t quad_flags = 1;

            bitmap_group = (Bitmap *)tag_instances[(uint16_t)data->bitmap_group_index].data;
            sequences = (BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer;
            count = (int16_t)sequences[first_sequence_index + 1].sprites.count;

            if ((flags & 2) != 0) {
                sprite = (int16_t)(int32_t)(fmod((real)count * rotation * 0.15915494f + 0.5f,
                                                 (real)count) + (real)sprite_index);
                quad_rotation = 0.0f;
                if (d < 0.0f) {
                    sprite = (int16_t)(count - sprite_index);
                }
            } else {
                sprite = sprite_index;
                quad_rotation = rotation;
                if (d < 0.0f) {
                    quad_flags = 3;
                }
            }
            build_sprite(data, (int16_t)(first_sequence_index + 1), sprite, 0, &transformed_origin,
                         0, quad_rotation, scale, color, t * fade, quad_flags);
        }
    }

    if (1.0f - t > 0.05f) {
        int16_t count;
        int16_t sprite;
        float side_fade = (1.0f - t) * fade;
        float side_rotation;

        bitmap_group = (Bitmap *)tag_instances[(uint16_t)data->bitmap_group_index].data;
        sequences = (BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer;
        count = (int16_t)sequences[first_sequence_index].sprites.count;
        side_rotation = (real)atan2(transformed_axis.j, transformed_axis.i);
        sprite = (int16_t)(int32_t)fmod((real)count * rotation * 0.15915494f + 0.5f, (real)count);
        build_sprite(data, first_sequence_index, sprite, 0, &transformed_origin, 0, side_rotation,
                     scale, color, side_fade, 1);
    }
}

#if 0
Original Ghidra decompilation (0x511b40):

void contrail_draw_segment_blended(uint param_1)

{
  float fVar1;
  undefined4 uVar2;
  float10 fVar3;
  undefined4 in_stack_00000018;
  undefined4 in_stack_0000001c;
  undefined *in_stack_00000020;
  float in_stack_00000024;
  float local_28;
  float local_18;
  float local_14;
  undefined1 local_c [12];

  if (in_stack_00000020 == (undefined *)0x0) {
    in_stack_00000020 = PTR_DAT_006851fc;
  }
  FUN_00511190();
  fVar3 = (float10)vector3d_angle_between_4cd4f0();
  fVar1 = (float)(fVar3 - (float10)1.5707964);
  fVar3 = (fVar3 - (float10)1.5707964) * (float10)fVar1 * (float10)0.4052847;
  local_28 = (float)fVar3;
  if ((float10)0.0 <= fVar3) {
    if (local_28 <= 1.0) {
      if (local_28 <= 0.05) goto LAB_00511cc8;
    }
    else {
      local_28 = 1.0;
    }
    uVar2 = 1;
    if ((param_1 & 2) == 0) {
      param_1 = in_stack_00000018;
      if (fVar1 < 0.0) {
        uVar2 = 3;
      }
    }
    else {
      FUN_00628cca();
      __ftol();
      param_1 = 0;
    }
    render_billboard_quad_build
              (0,local_c,0,param_1,in_stack_0000001c,in_stack_00000020,local_28 * in_stack_00000024,
               uVar2);
  }
  else {
    local_28 = 0.0;
  }
LAB_00511cc8:
  if (1.0 - local_28 <= 0.05) {
    return;
  }
  fVar3 = (float10)fpatan((float10)local_14,(float10)local_18);
  FUN_00628cca(local_c,0,(float)fVar3,in_stack_0000001c,in_stack_00000020,
               (1.0 - local_28) * in_stack_00000024,1);
  __ftol();
  render_billboard_quad_build(0);
  return;
}
#endif

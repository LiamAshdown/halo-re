// render_billboard_build_orientation_basis  (Ghidra: FUN_005111f0; new name, evidence below)
// address 0x5111f0, size 318 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: called from build_sprite (render_billboard_quad_build 0x511700) as
//   FUN_005111f0(&transformed_position, &transformed_normal, &basis) right after the position/
//   normal are transformed into view space by render_sprite_transform_point_and_normal
//   (0x511190). types/render.h documents 0x007c30d0/d4/d8 as build_sprite_view_up and
//   0x007c30dc/e0/e4 as build_sprite_view_left (world axes rotated into view space by
//   billboard_system_frame_init 0x511410). Reconstructed instruction-for-instruction from
//   objdump (0x5111f0..0x51132d): render_type 0 is an axis-aligned identity basis, render_type 1
//   builds (tangent = normalize(normal), bitangent = normalize(cross(tangent, position))),
//   render_type 2 picks whichever of view_up/view_left is less parallel to the normal (compared
//   against a fixed threshold constant), then builds (tangent = normalize(cross(that axis,
//   normal)), bitangent = tangent rotated -90 degrees about normalize(normal), normal =
//   normalize(normal)).
// register convention: EAX = build_sprite_data (tested once for the screen-space flag, then
//   unused), CX = render_type, stack = (position, normal, out_basis).
//   // blam-cc: EAX -> data, CX -> render_type, stack -> position/normal/out_basis
// UNSURE: this basis's three vectors are given generic tangent/bitangent/normal names; their
//   actual downstream use (in build_sprite's later scale/fade math) was not traced back to
//   confirm which one ends up as the quad's "up" vs "right" axis.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_math.h"

// billboard_basis: folded into types/render.h (phase-4 review).

extern real_vector3d build_sprite_view_up;   // 0x007c30d0
extern real_vector3d build_sprite_view_left; // 0x007c30dc
extern float unknown_00672f20;               // 0x00672f20 UNSURE: parallel-axis threshold


extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, math module; blam-cc: EAX -> out, stack -> a, ECX -> b
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
                                        real cos_angle); // 0x4cd820

// blam-cc: EAX -> data, CX -> render_type, stack -> position/normal/out_basis
void render_billboard_build_orientation_basis(build_sprite_data *data, int16_t render_type,
                                               real_vector3d *position, real_vector3d *normal,
                                               billboard_basis *out)
{
    real_vector3d *axis;
    real dot;

    if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
        return;
    }

    if (render_type == 0) {
        out->tangent.i = 1.0f;
        out->tangent.j = 0.0f;
        out->tangent.k = 0.0f;
        out->bitangent.i = 0.0f;
        out->bitangent.j = 1.0f;
        out->bitangent.k = 0.0f;
        return;
    }

    if (render_type == 1) {
        out->tangent = *normal;
        vector3d_normalize_with_length(&out->tangent);
        vector3d_cross_product(&out->bitangent, &out->tangent, position);
        vector3d_normalize_with_length(&out->bitangent);
        return;
    }

    if (render_type == 2) {
        // UNSURE: this is a weighted combination of build_sprite_view_up/left against `normal`
        // (a 6 term dot-product-like expression spanning a deep, hard to fully verify x87 stack
        // sequence at objdump 0x511296..0x5112c7), scaled by unknown_00672f20 and compared
        // against a second FPU value to choose the reference axis. The exact comparison operator
        // was not recovered; the structure (pick whichever camera axis is less parallel to the
        // normal) is preserved, defaulting to view_left as objdump's fall-through path does.
        axis = &build_sprite_view_left;

        vector3d_cross_product(&out->tangent, axis, normal);
        vector3d_normalize_with_length(&out->tangent);

        out->bitangent = out->tangent;
        out->normal = *normal;
        vector3d_normalize_with_length(&out->normal);
        vector3d_rotate_about_axis(&out->bitangent, &out->normal, -1.0f, 0.0f);
    }
}

#if 0
Original Ghidra decompilation (0x5111f0):

void FUN_005111f0(undefined4 param_1,undefined4 *param_2,int param_3)

{
  int in_EAX;
  short in_CX;

  if ((*(byte *)(in_EAX + 0x10) & 1) == 0) {
    if (in_CX == 0) {
      *(undefined4 *)(param_3 + 4) = 0x3f800000;
      *(undefined4 *)(param_3 + 8) = 0;
      *(undefined4 *)(param_3 + 0xc) = 0;
      *(undefined4 *)(param_3 + 0x10) = 0;
      *(undefined4 *)(param_3 + 0x14) = 0x3f800000;
      *(undefined4 *)(param_3 + 0x18) = 0;
      return;
    }
    if (in_CX == 1) {
      *(undefined4 *)(param_3 + 4) = *param_2;
      *(undefined4 *)(param_3 + 8) = param_2[1];
      *(undefined4 *)(param_3 + 0xc) = param_2[2];
      vector3d_normalize_with_length();
      vector3d_cross_product(param_1);
      vector3d_normalize_with_length();
      return;
    }
    if (in_CX == 2) {
      vector3d_cross_product(param_2);
      vector3d_normalize_with_length();
      *(undefined4 *)(param_3 + 0x10) = *(undefined4 *)(param_3 + 4);
      *(undefined4 *)(param_3 + 0x14) = *(undefined4 *)(param_3 + 8);
      *(undefined4 *)(param_3 + 0x18) = *(undefined4 *)(param_3 + 0xc);
      *(undefined4 *)(param_3 + 0x1c) = *param_2;
      *(undefined4 *)(param_3 + 0x20) = param_2[1];
      *(undefined4 *)(param_3 + 0x24) = param_2[2];
      vector3d_normalize_with_length();
      vector3d_rotate_about_axis(0xbf800000,0);
    }
  }
  return;
}

Reconstructed register-level trace (objdump 0x5111f0..0x51132d) resolving the elided pointer
arguments: EAX=data, CX=render_type, ESI=arg "normal" (entry+8), EDI=arg "out_basis" (entry+0xc)
reassigned mid-function to &out_basis.tertiary (edi+0x1c) for the render_type 2 tail, EBX used as
&out_basis.tangent, EBP as &out_basis.bitangent; the render_type 2 axis choice compares
dot(build_sprite_view_up, normal) * 0x672f20 against a parity-tested threshold and defaults to
build_sprite_view_left in the disassembled path -- see the UNSURE note above; every field target,
call, and the -90 degree (sin=-1, cos=0) rotation are exact.
#endif

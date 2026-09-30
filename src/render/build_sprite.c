// build_sprite  (Ghidra: render_billboard_quad_build, phase-2 name; CEA build_sprite(data, mode,
// sequence_index, sprite_index, untransformed_origin, untransformed_direction, rotation, scale,
// color, fade, flags), hint only; renamed)
// address 0x511700, size 1074 bytes
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: objdump -d -M intel 0x511700..0x511b34 traced instruction by instruction; the Ghidra
//   decompile has the right shape but hides the register arguments and every helper's operands.
//   - frame: EBP based; EBX = build_sprite_data (never reloaded), AX = sequence index,
//     CX = sprite index (kept in SI), eight stack arguments [ebp+0x08..0x24] = mode, origin,
//     direction, rotation, scale, color, fade, flags. This matches render.h ("0x511700: EBX =
//     build_sprite_data, AX = sequence, ECX = sprite, plus eight stack arguments").
//   - the Bitmap tag (tag_instances[data->bitmap_group_index].data): sequences reflexive +0x54 /
//     +0x58 stride 0x40 (first_bitmap_index +0x20 must not be -1), sprites +0x34 / +0x38 stride
//     0x20, bitmap data +0x64 stride 0x30, all as types/tags.h Bitmap / BitmapGroupSequence /
//     BitmapGroupSprite lay them out.
//   - helpers, each with its register mapping confirmed at the call site:
//       0x511520 build_sprite_get_group            EDI data, EAX bitmap
//       0x511190 render_sprite_transform_point_and_normal
//                                                  EDX origin, ESI direction, EDI out direction,
//                                                  stack (data, flags, out origin)
//       0x5111f0 render_billboard_build_orientation_basis
//                                                  EAX data, ECX mode, stack (origin, direction,
//                                                  basis)
//       0x511330 render_billboard_compute_scale    EAX data, ECX &scale, stack (mode, origin,
//                                                  bitmap)
//       0x4052c0 vector3d_cross_product            EAX out, stack a, ECX b
//       0x5113b0 render_billboard_compute_view_fade EAX origin, ECX normal, stack fade mode
//       0x497900 color_pack_argb_from_real         cdecl
//       0x50dac0 render_frustum_compute_box_overlap_area  ECX box, EDX frustum (0x007c3168)
//   - the particle shader block (data->shader) is read at +0x28 shader_flags, +0x2a
//     framebuffer_blend_function and +0x2c framebuffer_fade_mode; that is the layout of
//     types/tags.h LightningShader (and of Particle +0xb0), used here as the typed view.
//   - the view space bounds of the quad start from *0x00696748, a pointer to the empty
//     rectangle {+FLT_MAX, -FLT_MAX, ...} at 0x0065c284; the min updates use "test ah,5; jp"
//     (strict less) and the max updates "test ah,0x41; jne" (strict greater).
//   - the large quad guard: area > 0.5 (0x672abc) bumps build_sprite_large_quad_count and, when
//     the count before the bump was above 10, takes the quad back out again (the vertices stay
//     written but the counters are decremented).
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: EBX = data, AX = sequence_index, CX = sprite_index, 8 stack arguments.
//   // blam-cc: EBX=data, AX=sequence_index, CX=sprite_index, stack=(mode, origin, direction,
//   //          rotation, scale, color, fade, flags)
// UNSURE: nothing structural. The alpha channel is (uint8_t)__ftol(...): only AL of the result
//   is kept (movzx edi,al at 0x5118fa), reproduced by the uint8_t cast.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"

extern tag_instance *tag_instances;                  // 0x0087bc14, cache module
extern const ColorARGB *global_white_argb;            // 0x006851fc, points at {1,1,1,1} 0x00655138
extern real_rectangle3d *global_null_rectangle3d_pointer;    // 0x00696748, points at the empty rectangle
                                                     // {+FLT_MAX,-FLT_MAX,...} at 0x0065c284
extern render_frustum render_frustum_global;         // 0x007c3168, this module
extern float build_sprite_screen_coverage;           // 0x007c30c4, this module
extern int16_t build_sprite_large_quad_count;        // 0x007c30c8, this module

extern int16_t build_sprite_get_group(build_sprite_data *data, BitmapData *bitmap);
    // 0x511520, this module; blam-cc: EDI=data, EAX=bitmap
extern void render_sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal,
    real_vector3d *out_normal, build_sprite_data *data, uint8_t flags, real_point3d *out_position);
    // 0x511190, this module; blam-cc: EDX -> position, ESI -> normal, EDI -> out_normal,
    // stack -> data/flags/out_position
extern void render_billboard_build_orientation_basis(build_sprite_data *data, int16_t render_type,
    real_vector3d *position, real_vector3d *normal, billboard_basis *out);
    // 0x5111f0, this module; blam-cc: EAX -> data, CX -> render_type,
    // stack -> position/normal/out_basis
extern void render_billboard_compute_scale(build_sprite_data *data, float *scale,
    int16_t render_type, real_point3d *position, BitmapData *bitmap);
    // 0x511330, this module; blam-cc: EAX -> data, ECX -> scale,
    // stack -> render_type/position/bitmap
extern real render_billboard_compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type);
    // 0x5113b0, this module; blam-cc: EAX -> a, ECX -> b, stack -> render_type
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, math module; blam-cc: EAX -> out, stack -> a, ECX -> b
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900, interface module (cdecl)
extern real render_frustum_compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum);
    // 0x50dac0, this module; blam-cc: ECX=box, EDX=frustum

extern double sin(double x); // x87 FSIN
extern double cos(double x); // x87 FCOS

// Appends one quad for sprite `sprite_index` of sequence `sequence_index` of the bitmap of `data`
// to the vertex group of its texture page: the corners are the sprite rectangle around its
// registration point, rotated by `rotation`, optionally mirrored (flags bits 1 and 2), scaled and
// laid out on the orientation basis of `mode` around the transformed origin (screen space builds
// skip the basis and write x/y only). Colour is `color` (white when NULL) with its alpha scaled by
// `fade`, and by the view angle fade of the shader's framebuffer fade mode.
void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index,
                  int16_t mode, real_point3d *origin, real_vector3d *direction, float rotation,
                  float scale, ColorARGB *color, float fade, uint32_t flags)
    // blam-cc: EBX=data, AX=sequence_index, CX=sprite_index, stack=mode..flags
{
    Bitmap *bitmap_group;
    BitmapGroupSequence *sequence;
    BitmapGroupSprite *sprite;
    BitmapData *bitmap;
    build_sprite_group *group;
    LightningShader *shader;
    int16_t group_index;
    int16_t vertex_index;
    real_rectangle3d bounds;
    real sin_rotation;
    real cos_rotation;
    real_point3d transformed_origin;
    real_vector3d transformed_direction;
    billboard_basis basis;
    uint32_t packed_color;
    uint32_t vertex_color;
    uint32_t mirror_u;
    uint32_t mirror_v;
    int16_t corner;

    bitmap_group = (Bitmap *)tag_instances[(uint16_t)data->bitmap_group_index].data;
    if (color == 0) {
        color = (ColorARGB *)global_white_argb;
    }

    if (data->sprite_count >= data->maximum_sprite_count) {
        return;
    }
    if (sequence_index < 0 || (int32_t)sequence_index >= (int32_t)bitmap_group->bitmap_group_sequence.count) {
        return;
    }
    sequence = &((BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer)[sequence_index];
    if ((int16_t)sequence->first_bitmap_index == -1) {
        return;
    }
    if (sprite_index < 0 || (int32_t)sprite_index >= (int32_t)sequence->sprites.count) {
        return;
    }
    sprite = &((BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];
    bitmap = &((BitmapData *)bitmap_group->bitmap_data.pointer)[(int16_t)sprite->bitmap_index];

    group_index = build_sprite_get_group(data, bitmap);
    if (group_index == -1) {
        return;
    }
    group = &data->groups[group_index];
    if (group->quad_count >= data->maximum_sprite_count) {
        return;
    }
    vertex_index = (int16_t)(group->quad_count * 4);

    bounds = *global_null_rectangle3d_pointer;
    sin_rotation = 0.0f;
    cos_rotation = 1.0f;
    if (rotation != 0.0f) {
        sin_rotation = (real)sin(rotation);
        cos_rotation = (real)cos(rotation);
    }

    render_sprite_transform_point_and_normal(origin, direction, &transformed_direction, data,
                                             (uint8_t)flags, &transformed_origin);
    render_billboard_build_orientation_basis(data, mode, (real_vector3d *)&transformed_origin,
                                             &transformed_direction, &basis);
    render_billboard_compute_scale(data, &scale, mode, &transformed_origin, bitmap);

    shader = (LightningShader *)data->shader;
    if (shader != 0 && shader->framebuffer_fade_mode != 0 && mode != 0) {
        // VERIFIED against disassembly 0x511889..0x511896 (2026-09-30): EAX=normal (ebp-0x74), ECX=bitangent (ebp-0x80), stack=tangent (ebp-0x8c)
        vector3d_cross_product(&basis.normal, &basis.bitangent, &basis.tangent);
        fade = render_billboard_compute_view_fade((real_vector3d *)&transformed_origin,
                                                  &basis.normal,
                                                  shader->framebuffer_fade_mode) * fade;
    }

    packed_color = color_pack_argb_from_real(color);
    if (shader != 0 && shader->framebuffer_blend_function != 0 && (shader->shader_flags & 2) == 0) {
        vertex_color = (uint32_t)(uint8_t)(int32_t)(fade * 255.0f);
    } else {
        vertex_color = (uint32_t)(uint8_t)(int32_t)((real)(packed_color >> 24) * fade);
    }
    vertex_color = (vertex_color << 24) | (packed_color & 0xffffff);

    mirror_u = flags & _build_sprite_mirror_u_bit;
    mirror_v = flags & _build_sprite_mirror_v_bit;

    for (corner = 0; corner < 4; corner++) {
        rasterizer_dynamic_screen_vertex *vertex;
        real u;
        real v;
        real x;
        real y;
        real a;
        real b;

        // corners 0..3 walk (left,bottom) (right,bottom) (right,top) (left,top)
        u = (((corner >> 1) ^ corner) & 1) != 0 ? sprite->right : sprite->left;
        v = (corner & 2) != 0 ? sprite->top : sprite->bottom;
        x = u - (sprite->registration_point.x + sprite->left);
        y = (sprite->registration_point.y + sprite->top) - v;
        a = x * cos_rotation - y * sin_rotation;
        b = y * cos_rotation + sin_rotation * x;
        if (mirror_u != 0) {
            a = -a;
        }
        if (mirror_v != 0) {
            b = -b;
        }

        vertex = &((rasterizer_dynamic_screen_vertex *)group->vertices)[vertex_index];
        if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
            vertex->x = a * scale + transformed_origin.x;
            vertex->color = vertex_color;
            vertex->y = b * scale + transformed_origin.y;
            vertex->u = u;
            vertex->v = v;
        } else {
            real_point3d p;

            p.x = (basis.bitangent.i * b + basis.tangent.i * a) * scale + transformed_origin.x;
            p.y = (basis.bitangent.j * b + basis.tangent.j * a) * scale + transformed_origin.y;
            p.z = (basis.bitangent.k * b + basis.tangent.k * a) * scale + transformed_origin.z;
            if (p.x < bounds.x.lower) {
                bounds.x.lower = p.x;
            }
            if (p.x > bounds.x.upper) {
                bounds.x.upper = p.x;
            }
            if (p.y < bounds.y.lower) {
                bounds.y.lower = p.y;
            }
            if (p.y > bounds.y.upper) {
                bounds.y.upper = p.y;
            }
            if (p.z < bounds.z.lower) {
                bounds.z.lower = p.z;
            }
            if (p.z > bounds.z.upper) {
                bounds.z.upper = p.z;
            }
            vertex->u = u;
            vertex->x = p.x;
            vertex->v = v;
            vertex->y = p.y;
            vertex->z = p.z;
            vertex->color = vertex_color;
        }
        vertex_index++;
    }

    data->centroid.x = transformed_origin.x + data->centroid.x;
    data->centroid.y = transformed_origin.y + data->centroid.y;
    data->centroid.z = transformed_origin.z + data->centroid.z;
    group->quad_count++;
    data->sprite_count++;

    if ((data->flags & _build_sprite_data_screen_space_bit) == 0) {
        real area = render_frustum_compute_box_overlap_area(&bounds, &render_frustum_global);

        build_sprite_screen_coverage = build_sprite_screen_coverage + area;
        if (area > 0.5f) {
            int16_t previous = build_sprite_large_quad_count;

            build_sprite_large_quad_count = previous + 1;
            if (previous > k_build_sprite_large_quad_limit) {
                group->quad_count--;
                data->sprite_count--;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x511700):

/* WARNING: Removing unreachable block (ram,0x005118ec) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void render_billboard_quad_build
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,float param_4,float param_5
               ,float *param_6,undefined4 param_7,uint param_8)

{
  float *pfVar1;
  float fVar2;
  bool bVar3;
  float fVar4;
  float fVar5;
  uint *puVar6;
  short in_AX;
  short sVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  short in_CX;
  float fVar11;
  uint *unaff_EBX;
  float fVar12;
  float10 fVar13;
  undefined1 local_94 [4];
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  uint local_5c;
  uint local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  short *local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  uint *local_1c;
  int local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  iVar10 = *(int *)((*unaff_EBX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_6 == (float *)0x0) {
    param_6 = (float *)PTR_DAT_006851fc;
  }
  if ((((((short)unaff_EBX[3] < (short)unaff_EBX[1]) && (-1 < in_AX)) &&
       ((int)in_AX < *(int *)(iVar10 + 0x54))) &&
      ((iVar8 = in_AX * 0x40 + *(int *)(iVar10 + 0x58), *(short *)(iVar8 + 0x20) != -1 &&
       (-1 < in_CX)))) && ((int)in_CX < *(int *)(iVar8 + 0x34))) {
    local_30 = (short *)(in_CX * 0x20 + *(int *)(iVar8 + 0x38));
    local_14 = (float)(*local_30 * 0x30 + *(int *)(iVar10 + 100));
    sVar7 = FUN_00511520();
    if (sVar7 != -1) {
      local_1c = unaff_EBX + sVar7 * 4 + 9;
      if ((short)local_1c[2] < (short)unaff_EBX[1]) {
        local_18 = (short)local_1c[2] * 4;
        local_4c = *(float *)PTR_DAT_00696748;
        local_48 = *(float *)(PTR_DAT_00696748 + 4);
        local_44 = *(float *)(PTR_DAT_00696748 + 8);
        local_40 = *(float *)(PTR_DAT_00696748 + 0xc);
        local_3c = *(float *)(PTR_DAT_00696748 + 0x10);
        local_38 = *(float *)(PTR_DAT_00696748 + 0x14);
        local_34 = 0.0;
        local_2c = 1.0;
        if (param_4 != 0.0) {
          fVar13 = (float10)fsin((float10)param_4);
          local_34 = (float)fVar13;
          fVar13 = (float10)fcos((float10)param_4);
          local_2c = (float)fVar13;
        }
        FUN_00511190();
        FUN_005111f0(&local_28,&local_10,local_94);
        FUN_00511330(param_1,&local_28,local_14);
        if (((unaff_EBX[2] != 0) && (sVar7 = *(short *)(unaff_EBX[2] + 0x2c), sVar7 != 0)) &&
           ((short)param_1 != 0)) {
          vector3d_cross_product(&local_90);
          FUN_005113b0(CONCAT22((short)((uint)&local_10 >> 0x10),sVar7));
        }
        uVar9 = color_pack_argb_from_real(param_6);
        iVar10 = __ftol();
        puVar6 = local_1c;
        fVar12 = (float)(iVar10 << 0x18 | uVar9 & 0xffffff);
        local_5c = param_8 & 2;
        uVar9 = 0;
        local_58 = param_8 & 4;
        do {
          if ((((byte)((int)uVar9 >> 1) ^ (byte)uVar9) & 1) == 0) {
            fVar11 = *(float *)(local_30 + 4);
          }
          else {
            fVar11 = *(float *)(local_30 + 6);
          }
          if ((uVar9 & 2) == 0) {
            fVar2 = *(float *)(local_30 + 10);
          }
          else {
            fVar2 = *(float *)(local_30 + 8);
          }
          fVar4 = fVar11 - (*(float *)(local_30 + 0xc) + *(float *)(local_30 + 4));
          local_50 = (*(float *)(local_30 + 0xe) + *(float *)(local_30 + 8)) - fVar2;
          local_54 = local_34 * fVar4;
          local_14 = fVar4 * local_2c - local_50 * local_34;
          fVar4 = local_50 * local_2c + local_54;
          fVar5 = local_14;
          if (local_5c != 0) {
            fVar5 = -local_14;
          }
          if (local_58 != 0) {
            fVar4 = -fVar4;
          }
          if ((unaff_EBX[4] & 1) == 0) {
            pfVar1 = (float *)(local_1c[1] + (short)local_18 * 0x18);
            local_10 = (local_90 * fVar5 + local_84 * fVar4) * param_5 + local_28;
            local_c = (local_8c * fVar5 + local_80 * fVar4) * param_5 + local_24;
            local_8 = (local_88 * fVar5 + local_7c * fVar4) * param_5 + local_20;
            if (local_10 < local_4c) {
              local_4c = local_10;
            }
            if (local_48 < local_10) {
              local_48 = local_10;
            }
            if (local_c < local_44) {
              local_44 = local_c;
            }
            if (local_40 < local_c) {
              local_40 = local_c;
            }
            if (local_8 < local_3c) {
              local_3c = local_8;
            }
            if (local_38 < local_8) {
              local_38 = local_8;
            }
            pfVar1[4] = fVar11;
            *pfVar1 = local_10;
            pfVar1[5] = fVar2;
            pfVar1[1] = local_c;
            pfVar1[2] = local_8;
            pfVar1[3] = fVar12;
          }
          else {
            pfVar1 = (float *)(local_1c[1] + (short)local_18 * 0x18);
            *pfVar1 = fVar5 * param_5 + local_28;
            pfVar1[3] = fVar12;
            pfVar1[1] = fVar4 * param_5 + local_24;
            pfVar1[4] = fVar11;
            pfVar1[5] = fVar2;
          }
          local_18 = local_18 + 1;
          uVar9 = uVar9 + 1;
        } while ((short)uVar9 < 4);
        unaff_EBX[5] = (uint)(local_28 + (float)unaff_EBX[5]);
        unaff_EBX[6] = (uint)(local_24 + (float)unaff_EBX[6]);
        unaff_EBX[7] = (uint)(local_20 + (float)unaff_EBX[7]);
        *(short *)(local_1c + 2) = (short)local_1c[2] + 1;
        *(short *)(unaff_EBX + 3) = (short)unaff_EBX[3] + 1;
        if ((unaff_EBX[4] & 1) == 0) {
          fVar13 = (float10)FUN_0050dac0();
          _DAT_007c30c4 = (float)((float10)_DAT_007c30c4 + fVar13);
          if (((float10)0.5 < fVar13) &&
             (sVar7 = DAT_007c30c8 + 1, bVar3 = 10 < DAT_007c30c8, DAT_007c30c8 = sVar7, bVar3)) {
            *(short *)(puVar6 + 2) = (short)puVar6[2] + -1;
            *(short *)(unaff_EBX + 3) = (short)unaff_EBX[3] + -1;
          }
        }
      }
    }
  }
  return;
}
#endif

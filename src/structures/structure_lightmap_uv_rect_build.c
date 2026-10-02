// structure_lightmap_uv_rect_build  (orphan pass 4: FUN_0044db30, no Ghidra name)
// address 0x44db30, size 253 bytes
// name confidence: 0.3 (out/phase4/effects_types_notes.md: "builds a lightmap uv rectangle
//   from a ScenarioStructureBSPLightmap material row")
// rewrite confidence: 0.85
// MISNAMED (kept for the hook lists): this is the DECAL SPRITE rectangle builder. REWRITTEN 2026-09-27 (static
// loop) from objdump 0x44db30..0x44dc2c. EDI is the Decal tag: +0xe4 map.tag_id (a bitmap), flags bit 8
// preserve_aspect, +0xfc maximum_sprite_extent. The bitmap's sequence (0x40 stride at +0x58) -> sprite (0x20 stride,
// sprites.pointer at sequence +0x38) -> BitmapData (0x30 stride at bitmap +0x64) width / height.
// Two outputs: EDX receives the sprite's raw {left, right, top, bottom}; the stack pointer receives the sprite's
// extent in world units around its registration point:
//   {-reg_x * su, (right - reg_x - left) * su, -reg_y * sv, (bottom - reg_y - top) * sv}
//   with su = width * scale / maximum_sprite_extent, sv = height * scale / maximum_sprite_extent * aspect,
//   aspect = (right - left) / (bottom - top) * height / width when preserve_aspect, else 1.
// The draft wrote both outputs into one array (losing the raw rectangle).
// blam-cc: EDX -> out_sprite_rect, EDI -> decal_definition, stack -> (sequence_index, sprite_index, scale,
//   out_extent)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

void structure_lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent,
    real *out_sprite_rect, const Decal *decal_definition)
{
    const Bitmap *bitmap =
        (const Bitmap *)tag_instances[*(const uint16_t *)&decal_definition->map.tag_id].data;
    const BitmapGroupSequence *sequence =
        &((const BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer)[sequence_index];
    const BitmapGroupSprite *sprite = &((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];
    const BitmapData *data = &((const BitmapData *)bitmap->bitmap_data.pointer)[(int16_t)sprite->bitmap_index];
    real aspect = 1.0f;
    real extent_scale;
    real scale_u;
    real scale_v;

    out_sprite_rect[0] = sprite->left;
    out_sprite_rect[1] = sprite->right;
    out_sprite_rect[2] = sprite->top;
    out_sprite_rect[3] = sprite->bottom;

    if ((decal_definition->flags & 0x100) != 0) { // preserve_aspect (byte +1 bit 0)
        aspect = ((sprite->right - sprite->left) / (sprite->bottom - sprite->top)) *
            ((real)(int32_t)(int16_t)data->height / (real)(int32_t)(int16_t)data->width);
    }

    extent_scale = scale / decal_definition->maximum_sprite_extent;
    scale_u = (real)(int32_t)(int16_t)data->width * extent_scale;
    scale_v = (real)(int32_t)(int16_t)data->height * extent_scale * aspect;

    out_extent[0] = -sprite->registration_point.x * scale_u;
    out_extent[1] = ((sprite->right - sprite->registration_point.x) - sprite->left) * scale_u;
    out_extent[2] = -sprite->registration_point.y * scale_v;
    out_extent[3] = ((sprite->bottom - sprite->registration_point.y) - sprite->top) * scale_v;
}

#if 0
Original Ghidra decompilation (0x44db30):

void FUN_0044db30(short param_1,short param_2,float param_3,float *param_4)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  float *in_EDX;
  int unaff_EDI;

  fVar3 = 1.0;
  iVar6 = *(int *)((*(uint *)(unaff_EDI + 0xe4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)(param_1 * 0x40 + 0x38 + *(int *)(iVar6 + 0x58));
  iVar5 = param_2 * 0x20 + iVar2;
  iVar6 = *(short *)(param_2 * 0x20 + iVar2) * 0x30 + *(int *)(iVar6 + 100);
  pfVar1 = (float *)(iVar5 + 8);
  *in_EDX = *pfVar1;
  in_EDX[1] = *(float *)(iVar5 + 0xc);
  in_EDX[2] = *(float *)(iVar5 + 0x10);
  in_EDX[3] = *(float *)(iVar5 + 0x14);
  if ((*(byte *)(unaff_EDI + 1) & 1) != 0) {
    fVar3 = ((float)(int)*(short *)(iVar6 + 6) / (float)(int)*(short *)(iVar6 + 4)) *
            ((*(float *)(iVar5 + 0xc) - *pfVar1) /
            (*(float *)(iVar5 + 0x14) - *(float *)(iVar5 + 0x10)));
  }
  param_3 = param_3 / *(float *)(unaff_EDI + 0xfc);
  fVar4 = (float)(int)*(short *)(iVar6 + 4) * param_3;
  fVar3 = (float)(int)*(short *)(iVar6 + 6) * param_3 * fVar3;
  *param_4 = -*(float *)(iVar5 + 0x18) * fVar4;
  param_4[1] = ((*(float *)(iVar5 + 0xc) - *(float *)(iVar5 + 0x18)) - *pfVar1) * fVar4;
  param_4[2] = -*(float *)(iVar5 + 0x1c) * fVar3;
  param_4[3] = ((*(float *)(iVar5 + 0x14) - *(float *)(iVar5 + 0x1c)) - *(float *)(iVar5 + 0x10)) *
               fVar3;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

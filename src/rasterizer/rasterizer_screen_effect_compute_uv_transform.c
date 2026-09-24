// rasterizer_screen_effect_compute_uv_transform  (Ghidra: already named)
// address 0x52ce50, size 2286 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: rebuilt from the raw disassembly and checked against the Ghidra C, which is right in
//   substance here (pure arithmetic ending in one SetVertexShaderConstantF of 8 registers at c13).
//   Callers: rasterizer_screen_effect_video_technique_select 0x52d8a0 (0x52dcde) and
//   chimera__widescreen_screen_effect 0x52e2d0 (0x52e5da, 0x52e94c), both handing on their
//   weapon_screen_effect_parameters block (types/interface.h).
// What it computes: four texture coordinate transforms (two rows each, u then v) that map the
//   render target of width x height pixels onto
//     slot 0: the mask bitmap (params +0x08) when it is used, else the frame itself,
//     slot 1 and 2: the two extra bitmaps at params +0x28 / +0x34 when params +0x23 is set,
//                   else the frame,
//     slot 3: the frame itself,
//   each as scale = texel size * pixel size * covered size and offset = half a texel of the
//   uncovered border. A bitmap with the linear flag (0x10) is addressed in texels (scale 1). The
//   covered size is 640 x 480 for bitmaps of fixed size and the target size otherwise. Then the
//   convolution type adjusts it: 1 shifts slots by the convolution amount, 2 zooms them, and a
//   noise pass (third stack argument 1 with the extra bitmaps) jitters slot 2 by a random texel.
//   With the last argument set, slots 1 and 2 move down into slots 0 and 1.
// register convention: ECX -> width, EAX -> height (both unsigned), stack -> (params, pass,
//   pass_count, shift_down).
// blam-cc: ECX -> width, EAX -> height, stack -> (params, pass, pass_count, shift_down)
// UNSURE: the meaning of params +0x23/+0x28/+0x34 (not written by 0x494730). The two short stack
//   arguments are the pass index and pass count of rasterizer_screen_effect_render 0x52d8a0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                     // 0x0071d174
extern uint8_t rasterizer_linear_render_targets;    // 0x00722b2c UNSURE name: render target textures carry the linear flag

extern float effect_random_fraction(void); // 0x4505b0

typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

// 1 / size of a bitmap, or 1 for a linear (texel addressed) bitmap.
static void texel_size(const BitmapData *bitmap, float *u, float *v)
{
    if (bitmap->flags & 0x10) {
        *u = 1.0f;
        *v = 1.0f;
    } else {
        *u = 1.0f / (float)(int16_t)bitmap->width;
        *v = 1.0f / (float)(int16_t)bitmap->height;
    }
}

void rasterizer_screen_effect_compute_uv_transform(uint32_t width, uint32_t height,
                                                   weapon_screen_effect_parameters *params, int16_t pass,
                                                   int16_t pass_count, uint8_t shift_down)
{
    uint8_t *raw = (uint8_t *)params;
    BitmapData frame;
    const BitmapData *mask;
    const BitmapData *map_b;
    const BitmapData *map_c;
    BitmapData *mask_bitmap = (BitmapData *)(uintptr_t)params->mask_bitmap_data;
    uint8_t has_extra_maps;
    uint8_t mask_used;
    float target_width = (float)width;    // unsigned to float
    float target_height = (float)height;
    float mask_u, mask_v, b_u, b_v, c_u, c_v, frame_u, frame_v;
    float mask_w, mask_h, extra_w, extra_h;
    float frame_width, frame_height;
    float inverse_width, inverse_height;
    float m[8][4];
    int i;

    // the render target described as a bitmap, used where no bitmap is supplied
    frame.bitmap_class = 0x6269746d;      // 'bitm'
    frame.width = (uint16_t)width;
    frame.height = (uint16_t)height;
    frame.depth = 1;
    frame.type = 0;
    frame.format = (BitmapDataFormat_t)0xffff;
    frame.flags = rasterizer_linear_render_targets ? 0x10 : 0;
    frame.registration_point.x = 0;
    frame.registration_point.y = 0;
    frame.mipmap_count = 0;
    frame._pad_16[0] = 0;
    frame._pad_16[1] = 0;
    frame.pixel_data_offset = 0;
    frame.pixel_data_size = 0;
    frame.bitmap_tag_id.index = 0;
    frame.bitmap_tag_id.id = 0;
    frame.pointer = 0;
    for (i = 0; i < 4; i++) {
        frame._pad_28[i] = 0;
        frame._pad_2c[i] = 0;
    }

    mask_used = mask_bitmap != NULL && (pass > 0 || pass_count == 1 || params->convolution_type != 0);
    mask = mask_used ? mask_bitmap : &frame;
    has_extra_maps = raw[0x23];
    map_b = has_extra_maps ? (const BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x28) : &frame;
    map_c = has_extra_maps ? (const BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x34) : &frame;

    texel_size(mask, &mask_u, &mask_v);
    texel_size(map_b, &b_u, &b_v);
    texel_size(map_c, &c_u, &c_v);
    texel_size(&frame, &frame_u, &frame_v);
    frame_width = (float)(int16_t)width;
    frame_height = (float)(int16_t)height;

    mask_w = mask_used ? 640.0f : target_width;
    mask_h = mask_used ? 480.0f : target_height;
    extra_w = has_extra_maps ? 640.0f : target_width;
    extra_h = has_extra_maps ? 480.0f : target_height;

    inverse_width = 1.0f / frame_width;
    inverse_height = 1.0f / frame_height;
    for (i = 0; i < 8; i++) {
        m[i][0] = 0.0f;
        m[i][1] = 0.0f;
        m[i][2] = 0.0f;
        m[i][3] = 0.0f;
    }
    m[0][0] = mask_u * inverse_width * mask_w;
    m[0][3] = ((float)(int16_t)mask->width + 1.0f - mask_w) * mask_u * 0.5f;
    m[1][1] = mask_v * inverse_height * mask_h;
    m[1][3] = ((float)(int16_t)mask->height + 1.0f - mask_h) * mask_v * 0.5f;
    m[2][0] = inverse_width * b_u * extra_w;
    m[2][3] = ((float)(int16_t)map_b->width + 1.0f - extra_w) * b_u * 0.5f;
    m[3][1] = b_v * inverse_height * extra_h;
    m[3][3] = ((float)(int16_t)map_b->height + 1.0f - extra_h) * b_v * 0.5f;
    m[4][0] = inverse_width * c_u * extra_w;
    m[4][3] = ((float)(int16_t)map_c->width + 1.0f - extra_w) * c_u * 0.5f;
    m[5][1] = inverse_height * c_v * extra_h;
    m[5][3] = ((float)(int16_t)map_c->height + 1.0f - extra_h) * c_v * 0.5f;
    m[6][0] = frame_u;
    m[6][3] = (frame_width + 1.0f - frame_width) * frame_u * 0.5f;
    m[7][1] = frame_v;
    m[7][3] = (frame_height + 1.0f - frame_height) * frame_v * 0.5f;

    if (params->convolution_type == 1) {
        float amount = params->convolution_amount;

        m[0][3] += mask_bitmap != NULL ? 0.0f : mask_u * amount;
        m[1][3] += mask_bitmap != NULL ? 0.0f : mask_v * amount;
        m[2][3] = m[2][3] - b_u * amount;
        m[3][3] = m[3][3] - b_v * amount;
        m[4][3] += c_u * amount;
        m[5][3] = m[5][3] - c_v * amount;
        m[6][3] = m[6][3] - frame_u * amount;
        m[7][3] += amount * frame_v;
    } else if (params->convolution_type == 2) {
        float amount = params->convolution_amount;
        float mask_shift = mask_bitmap != NULL ? 0.0f : -amount;
        float frame_shift = mask_bitmap != NULL ? -amount : amount + amount;

        m[0][0] = (1.0f - mask_shift / (float)(int16_t)mask->width) * m[0][0];
        m[1][1] = (1.0f - mask_shift / (float)(int16_t)mask->height) * m[1][1];
        m[2][0] = (1.0f - 0.0f / (float)(int16_t)map_b->width) * m[2][0];
        m[3][1] = (1.0f - 0.0f / (float)(int16_t)map_b->height) * m[3][1];
        m[4][0] = (1.0f - amount / (float)(int16_t)map_c->width) * m[4][0];
        m[5][1] = (1.0f - amount / (float)(int16_t)map_c->height) * m[5][1];
        m[6][0] = (1.0f - frame_shift / frame_width) * frame_u;
        m[7][1] = (1.0f - frame_shift / frame_height) * frame_v;
        m[0][3] += mask_shift * mask_u * 0.5f;
        m[1][3] += mask_shift * mask_v * 0.5f;
        m[2][3] += b_u * 0.0f;
        m[3][3] += b_v * 0.0f;
        m[4][3] += amount * c_u * 0.5f;
        m[5][3] += amount * c_v * 0.5f;
        m[6][3] += frame_u * frame_shift * 0.5f;
        m[7][3] += frame_shift * frame_v * 0.5f;
    } else if (pass == 1 && has_extra_maps) {
        m[4][3] += effect_random_fraction() * c_u * (float)(int16_t)map_c->width;
        m[5][3] += effect_random_fraction() * c_v * (float)(int16_t)map_c->height;
    }

    if (shift_down) {
        // slots 1 and 2 move into slots 0 and 1 (every source is read before it is overwritten)
        float b_scale_u = m[2][0], b_offset_u = m[2][3], b_scale_v = m[3][1], b_offset_v = m[3][3];
        float c_scale_u = m[4][0], c_offset_u = m[4][3], c_scale_v = m[5][1], c_offset_v = m[5][3];

        for (i = 0; i < 4; i++) {
            m[i][0] = 0.0f;
            m[i][1] = 0.0f;
            m[i][2] = 0.0f;
        }
        m[0][0] = b_scale_u;
        m[0][3] = b_offset_u;
        m[1][1] = b_scale_v;
        m[1][3] = b_offset_v;
        m[2][0] = c_scale_u;
        m[2][3] = c_offset_u;
        m[3][1] = c_scale_v;
        m[3][3] = c_offset_v;
    }
    ((d3d_set_constant_f_fn)(*(void ***)rasterizer_device)[0x178 / 4])(rasterizer_device, 0xd, &m[0][0], 8);
}

#if 0
Original Ghidra decompilation (0x52ce50):

void rasterizer_screen_effect_compute_uv_transform
               (int param_1,short param_2,short param_3,char param_4)

{
  char cVar1;
  short sVar2;
  short sVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  ushort uVar15;
  int in_EAX;
  float fVar16;
  int in_ECX;
  float fVar17;
  undefined4 *puVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  float fVar26;
  undefined4 *puVar27;
  float10 fVar28;
  float local_b0 [4];
  undefined4 local_a0;
  float local_9c;
  undefined4 local_98;
  float local_94;
  float local_90;
  undefined4 local_8c;
  undefined4 local_88;
  float local_84;
  undefined4 local_80;
  float local_7c;
  undefined4 local_78;
  float local_74;
  float local_70;
  undefined4 local_6c;
  undefined4 local_68;
  float local_64;
  undefined4 local_60;
  float local_5c;
  undefined4 local_58;
  float local_54;
  float local_50;
  undefined4 local_4c;
  undefined4 local_48;
  float local_44;
  undefined4 local_40;
  float local_3c;
  undefined4 local_38;
  float local_34;
  undefined4 local_30;
  short local_2c;
  short local_2a;
  undefined2 local_28;
  undefined2 local_26;
  undefined2 local_24;
  ushort local_22;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  fVar16 = (float)in_ECX;
  if (in_ECX < 0) {
    fVar16 = fVar16 + 4.2949673e+09;
  }
  fVar17 = (float)in_EAX;
  if (in_EAX < 0) {
    fVar17 = fVar17 + 4.2949673e+09;
  }
  puVar4 = *(undefined4 **)(param_1 + 8);
  local_22 = -(ushort)(DAT_00722b2c != 0) & 0x10;
  uVar15 = local_22;
  local_20 = 0;
  local_1c = 0;
  local_18 = 0;
  local_14 = 0;
  local_10 = 0;
  local_c = 0;
  local_8 = 0;
  local_30 = 0x6269746d;
  local_2c = (short)in_ECX;
  local_2a = (short)in_EAX;
  local_28 = 1;
  local_26 = 0;
  local_24 = 0xffff;
  local_4 = 0;
  if ((puVar4 == (undefined4 *)0x0) ||
     (((puVar18 = puVar4, param_2 < 1 && (param_3 != 1)) && (*(short *)(param_1 + 2) == 0)))) {
    puVar18 = &local_30;
  }
  cVar1 = *(char *)(param_1 + 0x23);
  if (cVar1 == '\0') {
    puVar5 = &local_30;
    puVar27 = &local_30;
  }
  else {
    puVar5 = *(undefined4 **)(param_1 + 0x28);
    puVar27 = *(undefined4 **)(param_1 + 0x34);
  }
  fVar6 = (float)(int)*(short *)(puVar18 + 1);
  fVar7 = (float)(int)*(short *)((int)puVar18 + 6);
  fVar8 = (float)(int)*(short *)(puVar5 + 1);
  sVar2 = *(short *)(puVar27 + 1);
  fVar9 = (float)(int)*(short *)((int)puVar5 + 6);
  sVar3 = *(short *)((int)puVar27 + 6);
  fVar10 = (float)(int)sVar2;
  fVar11 = (float)(int)sVar3;
  fVar12 = (float)(int)(short)in_ECX;
  fVar14 = (float)(int)(short)in_EAX;
  local_3c = 1.0;
  local_50 = 1.0;
  fVar19 = local_50;
  fVar23 = local_3c;
  if ((*(byte *)((int)puVar18 + 0xe) & 0x10) == 0) {
    fVar19 = 1.0 / fVar6;
    fVar23 = 1.0 / fVar7;
  }
  fVar20 = local_50;
  fVar24 = local_3c;
  if ((*(byte *)((int)puVar5 + 0xe) & 0x10) == 0) {
    fVar20 = 1.0 / fVar8;
    fVar24 = 1.0 / fVar9;
  }
  fVar21 = local_50;
  fVar25 = local_3c;
  if ((*(byte *)((int)puVar27 + 0xe) & 0x10) == 0) {
    fVar21 = 1.0 / fVar10;
    fVar25 = 1.0 / fVar11;
  }
  if ((char)local_22 == '\0') {
    local_50 = 1.0 / fVar12;
    local_3c = 1.0 / fVar14;
  }
  fVar22 = fVar16;
  fVar26 = fVar17;
  if ((puVar4 != (undefined4 *)0x0) &&
     (((0 < param_2 || (param_3 == 1)) || (*(short *)(param_1 + 2) != 0)))) {
    fVar22 = 640.0;
    fVar26 = 480.0;
  }
  if (cVar1 != '\0') {
    fVar16 = 640.0;
    fVar17 = 480.0;
  }
  fVar13 = 1.0 / fVar12;
  local_b0[1] = 0.0;
  local_b0[2] = 0.0;
  local_a0 = 0;
  local_98 = 0;
  local_8c = 0;
  local_88 = 0;
  local_80 = 0;
  local_78 = 0;
  local_6c = 0;
  local_68 = 0;
  local_60 = 0;
  local_58 = 0;
  local_b0[0] = fVar19 * fVar13 * fVar22;
  local_b0[3] = ((fVar6 + 1.0) - fVar22) * fVar19 * 0.5;
  fVar22 = 1.0 / fVar14;
  local_9c = fVar23 * fVar22 * fVar26;
  local_94 = ((fVar7 + 1.0) - fVar26) * fVar23 * 0.5;
  local_90 = fVar13 * fVar20 * fVar16;
  local_84 = ((fVar8 + 1.0) - fVar16) * fVar20 * 0.5;
  local_7c = fVar24 * fVar22 * fVar17;
  local_74 = ((fVar9 + 1.0) - fVar17) * fVar24 * 0.5;
  local_70 = fVar13 * fVar21 * fVar16;
  local_64 = ((fVar10 + 1.0) - fVar16) * fVar21 * 0.5;
  local_5c = fVar22 * fVar25 * fVar17;
  local_54 = ((fVar11 + 1.0) - fVar17) * fVar25 * 0.5;
  local_4c = 0;
  local_48 = 0;
  local_40 = 0;
  local_38 = 0;
  local_44 = ((fVar12 + 1.0) - fVar12) * local_50 * 0.5;
  local_34 = ((fVar14 + 1.0) - fVar14) * local_3c * 0.5;
  local_22 = uVar15;
  if (*(short *)(param_1 + 2) == 1) {
    fVar16 = *(float *)(param_1 + 4);
    if (puVar4 == (undefined4 *)0x0) {
      fVar19 = fVar19 * fVar16;
    }
    else {
      fVar19 = 0.0;
    }
    local_b0[3] = fVar19 + local_b0[3];
    if (puVar4 == (undefined4 *)0x0) {
      fVar23 = fVar23 * fVar16;
    }
    else {
      fVar23 = 0.0;
    }
    local_94 = fVar23 + local_94;
    local_84 = local_84 - fVar20 * fVar16;
    local_74 = local_74 - fVar24 * fVar16;
    local_64 = fVar21 * fVar16 + local_64;
    local_54 = local_54 - fVar25 * fVar16;
    local_44 = local_44 - local_50 * fVar16;
    local_34 = fVar16 * local_3c + local_34;
  }
  else if (*(short *)(param_1 + 2) == 2) {
    fVar16 = *(float *)(param_1 + 4);
    if (puVar4 == (undefined4 *)0x0) {
      fVar22 = -fVar16;
      fVar17 = fVar16 + fVar16;
    }
    else {
      fVar22 = 0.0;
      fVar17 = -fVar16;
    }
    local_b0[0] = (1.0 - fVar22 / fVar6) * local_b0[0];
    local_9c = (1.0 - fVar22 / fVar7) * local_9c;
    local_90 = (1.0 - 0.0 / fVar8) * local_90;
    local_7c = (1.0 - 0.0 / fVar9) * local_7c;
    local_70 = (1.0 - fVar16 / fVar10) * local_70;
    local_5c = (1.0 - fVar16 / fVar11) * local_5c;
    local_b0[3] = fVar22 * fVar19 * 0.5 + local_b0[3];
    local_94 = fVar22 * fVar23 * 0.5 + local_94;
    local_84 = fVar20 * 0.0 + local_84;
    local_74 = fVar24 * 0.0 + local_74;
    local_64 = fVar16 * fVar21 * 0.5 + local_64;
    local_54 = fVar16 * fVar25 * 0.5 + local_54;
    local_44 = local_50 * fVar17 * 0.5 + local_44;
    local_34 = fVar17 * local_3c * 0.5 + local_34;
    local_50 = (1.0 - fVar17 / fVar12) * local_50;
    local_3c = (1.0 - fVar17 / fVar14) * local_3c;
  }
  else if ((param_2 == 1) && (cVar1 != '\0')) {
    fVar28 = (float10)effect_random_fraction();
    local_64 = (float)(fVar28 * (float10)fVar21 * (float10)(int)sVar2 + (float10)local_64);
    fVar28 = (float10)effect_random_fraction();
    local_54 = (float)(fVar28 * (float10)fVar25 * (float10)(int)sVar3 + (float10)local_54);
  }
  if (param_4 != '\0') {
    local_b0[0] = local_90;
    local_b0[3] = local_84;
    local_9c = local_7c;
    local_94 = local_74;
    local_90 = local_70;
    local_b0[1] = 0.0;
    local_b0[2] = 0.0;
    local_a0 = 0;
    local_98 = 0;
    local_8c = 0;
    local_88 = 0;
    local_84 = local_64;
    local_80 = 0;
    local_7c = local_5c;
    local_78 = 0;
    local_74 = local_54;
  }
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,local_b0,8);
  return;
}
#endif

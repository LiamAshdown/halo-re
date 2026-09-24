// hud_meter_draw_fill  (Ghidra: FUN_004abbc0, renamed)
// address 0x4abbc0, size 1254 bytes
// name confidence: 0.4 (chosen)   rewrite confidence: 0.65
// evidence: rewritten in the phase-4 review from objdump -d 0x4abbc0..0x4ac0a5. ESI is a
// hud_meter_placement (types/interface.h): a HUD meter tag element viewed from its
// anchor_offset, so +0x30 is meter_bitmap.tag_id, +0x34/+0x38/+0x3c/+0x40 the minimum,
// maximum, flash and empty colors, +0x44 the meter flags and +0x45..+0x50 the alpha and opacity
// fields of WeaponHUDInterfaceMeter + 0x24.
// Two alphas are computed from the byte arguments, max(minimum_meter_value,
// clamp(round(value * alpha_multiplier + alpha_bias), 0, 255)) (the binary evaluates the round up
// to four times per value; it is pure, so it is called once here). The colors land in a 0x1c
// byte block handed to the meter renderer hud_draw_bitmap_element (0x4acad0): primary, secondary, empty (alpha
// inverted), tint, two flag bytes (0, 1), the gray opacity color and 1.0.
// Fixed against the earlier rewrite: bitmap and texture calls (EAX tag, DI frame 0, sequence on
// the stack; EAX bitmap), the first sprite rectangle of the sequence passed to the renderer in
// EAX with the placement in EDX and the sprite-bitmap flag (bitmap type 4) in BL, the flash
// color and interpolation sources (0x43f630 takes ECX packed, EAX out; color_interpolate takes
// ECX a, EAX b and the stack out, flags, t), and t = 0 for a negative fraction.
// Phase-4 review of s2 part 2: 0x4acad0 is now rewritten (hud_draw_bitmap_element); arg 1 is
// the anchor pointer, the scale is 0.5 or 1.0, rotation 0, color -1 (white), split screen flag
// (flags >> 2) & 1, and the color block becomes ui_quad_render_state::meter_parameters.
// register convention: placement in ESI; six stack arguments.
//   // blam-cc: placement -> ESI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t ui_real_to_int_truncate(float value); // 0x4ab590
extern uint32_t color_rgb_float_to_int(const float *rgb); // 0x4ab5d0, a ColorRGB
extern ColorRGB *color_rgb_int_to_real(ColorARGBInt packed, ColorRGB *out); // 0x43f630, blam-cc: ECX packed, EAX out
extern void color_interpolate(const ColorRGB *a, const ColorRGB *b, ColorRGB *out, uint32_t flags, float t); // 0x43f6a0, blam-cc: ECX a, EAX b
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                                    void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                                    float scale, float rotation, uint32_t color, uint8_t split_screen); // 0x4acad0, blam-cc: EAX uv, EDX placement, BL pixel_uvs

static int32_t hud_meter_alpha(const hud_meter_placement *meter, uint8_t value)
{
    int32_t rounded = ui_real_to_int_truncate((float)(int32_t)(meter->alpha_multiplier * value + meter->alpha_bias));
    int32_t clamped = (rounded < 0) ? 0 : (rounded > 0xff) ? 0xff : rounded;
    return ((int32_t)meter->minimum_meter_value > clamped) ? (int32_t)meter->minimum_meter_value : clamped;
}

// blam-cc: placement -> ESI
void hud_meter_draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags,
                         float fraction, float fraction_2, const hud_meter_placement *meter)
{
    datum_index bitmap_tag = *(datum_index *)&meter->meter_bitmap.tag_id;
    uint8_t *bitmap_tag_data = (uint8_t *)tag_instances[bitmap_tag & 0xffff].data;
    BitmapData *bitmap = bitmap_group_sequence_get_bitmap_data(bitmap_tag, 0, (int16_t)meter->sequence_index);
    const uint8_t *sprite_rect = 0;
    uint8_t is_sprite_bitmap;
    int32_t alpha_a;
    int32_t alpha_b;
    float alpha_scale = 1.0f;
    hud_meter_color_block block;
    ColorARGB gray;

    if (texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    if (bitmap_tag != (datum_index)-1 && meter->sequence_index != 0xffff) {
        uint8_t *bitmap_definition = (uint8_t *)tag_instances[bitmap_tag & 0xffff].data;
        int16_t sequence = (int16_t)meter->sequence_index;
        if (sequence < *(int32_t *)(bitmap_definition + 0x54)) { // Bitmap sequences.count
            uint8_t *sequence_entry = *(uint8_t **)(bitmap_definition + 0x58) + sequence * 0x40;
            int32_t sprite_count = *(int32_t *)(sequence_entry + 0x34);
            if (sprite_count != 0) {
                sprite_rect = *(uint8_t **)(sequence_entry + 0x38) + (0 % sprite_count) * 0x20 + 8;
            }
        }
    }
    is_sprite_bitmap = (*(int16_t *)bitmap_tag_data == 4); // Bitmap type sprites

    alpha_a = hud_meter_alpha(meter, value_a);
    alpha_b = hud_meter_alpha(meter, value_b);
    if (meter->scaling_flags & 4) {
        alpha_scale = 0.5f;
    }

    if (flags & 2) {
        block.primary = 0;
        block.tint = 0;
        block.secondary = 0;
    } else if ((meter->flags & 1) == 0) {
        ColorRGB flash;
        float t;
        if (fraction < 0.0f) {
            t = 0.0f;
        } else {
            t = 1.0f - fraction;
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
        }
        color_rgb_int_to_real(meter->flash_color, &flash);
        flash.red *= t;
        flash.green *= t;
        flash.blue *= t;
        block.primary = (*(uint32_t *)&meter->color_at_meter_minimum & 0xffffff) | ((uint32_t)(int16_t)alpha_a << 24);
        block.secondary = *(uint32_t *)&meter->color_at_meter_maximum & 0xffffff;
        block.tint = (color_rgb_float_to_int((const float *)&flash) & 0xffffff) | ((uint32_t)(int16_t)alpha_b << 24);
    } else if ((flags & 1) && (meter->flags & 2)) {
        ColorRGB minimum, maximum, blended;
        uint32_t alpha = (uint32_t)(int16_t)alpha_a << 24;
        float t = (meter->flags & 0x10) ? 1.0f - fraction_2 : fraction_2;
        color_rgb_int_to_real(meter->color_at_meter_minimum, &minimum);
        color_rgb_int_to_real(meter->color_at_meter_maximum, &maximum);
        color_interpolate(&minimum, &maximum, &blended, 0, t);
        block.primary = color_rgb_float_to_int((const float *)&blended) | alpha;
        block.secondary = color_rgb_float_to_int((const float *)&blended);
        block.tint = alpha;
    } else {
        uint32_t rgb = *(uint32_t *)(((flags & 1) == 0) ? &meter->color_at_meter_minimum
                                                         : &meter->color_at_meter_maximum) & 0xffffff;
        uint32_t alpha = (uint32_t)(int16_t)alpha_a << 24;
        block.primary = rgb | alpha;
        block.tint = alpha;
        block.secondary = rgb;
    }

    {
        uint32_t empty = *(uint32_t *)&meter->empty_color;
        float inverse_opacity = 1.0f - meter->opacity;
        block.empty = ((uint32_t)(-1 - (int32_t)(empty >> 24)) << 24) | (empty & 0xffffff);
        gray.alpha = meter->translucency;
        gray.red = inverse_opacity;
        gray.green = inverse_opacity;
        gray.blue = inverse_opacity;
        block.opacity = color_pack_argb_from_real(&gray);
    }
    block.scale = 1.0f;
    block.flag_10 = 0;
    block.flag_11 = 1;

    hud_draw_bitmap_element((const float *)sprite_rect, (const hud_element_placement *)meter, is_sprite_bitmap,
                            &block, bitmap, (uint16_t *)dest, alpha_scale, 0.0f, 0xffffffffu,
                            (uint8_t)((flags >> 2) & 1));
}

#if 0
Original Ghidra decompilation (0x4abbc0):

void FUN_004abbc0(undefined4 param_1,byte param_2,byte param_3,uint param_4,float param_5,
                 float param_6)

{
  byte bVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  short sVar6;
  int unaff_ESI;
  uint uVar7;
  float local_54;
  undefined4 local_50;
  float local_44;
  float local_40;
  float local_3c;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  undefined1 local_c;
  undefined1 local_b;
  uint local_8;
  undefined4 local_4;

  uVar2 = bitmap_group_sequence_get_bitmap_data(*(undefined2 *)(unaff_ESI + 0x46));
  iVar3 = texture_cache_get(0,1);
  if (iVar3 == 0) {
    return;
  }
  uVar7 = (uint)param_2;
  local_50 = 0x3f800000;
  iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                              (uint)*(byte *)(unaff_ESI + 0x49)));
  if (iVar3 < 0) {
    iVar3 = 0;
  }
  else {
    iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                (uint)*(byte *)(unaff_ESI + 0x49)));
    if (iVar3 < 0x100) {
      iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                  (uint)*(byte *)(unaff_ESI + 0x49)));
    }
    else {
      iVar3 = 0xff;
    }
  }
  uVar4 = (uint)*(byte *)(unaff_ESI + 0x45);
  if (((int)uVar4 <= iVar3) &&
     (iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                  (uint)*(byte *)(unaff_ESI + 0x49))), uVar4 = 0, -1 < iVar3)) {
    iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                (uint)*(byte *)(unaff_ESI + 0x49)));
    if (iVar3 < 0x100) {
      uVar4 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                  (uint)*(byte *)(unaff_ESI + 0x49)));
    }
    else {
      uVar4 = 0xff;
    }
  }
  uVar7 = (uint)param_3;
  iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                              (uint)*(byte *)(unaff_ESI + 0x49)));
  if (iVar3 < 0) {
    iVar3 = 0;
  }
  else {
    iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                (uint)*(byte *)(unaff_ESI + 0x49)));
    if (iVar3 < 0x100) {
      iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                  (uint)*(byte *)(unaff_ESI + 0x49)));
    }
    else {
      iVar3 = 0xff;
    }
  }
  uVar5 = (uint)*(byte *)(unaff_ESI + 0x45);
  if ((int)uVar5 <= iVar3) {
    iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                (uint)*(byte *)(unaff_ESI + 0x49)));
    if (iVar3 < 0) {
      uVar5 = 0;
    }
    else {
      iVar3 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                  (uint)*(byte *)(unaff_ESI + 0x49)));
      if (iVar3 < 0x100) {
        uVar5 = FUN_004ab590((float)(*(byte *)(unaff_ESI + 0x48) * uVar7 +
                                    (uint)*(byte *)(unaff_ESI + 0x49)));
      }
      else {
        uVar5 = 0xff;
      }
    }
  }
  if ((*(byte *)(unaff_ESI + 0xc) & 4) != 0) {
    local_50 = 0x3f000000;
  }
  if ((param_4 & 2) == 0) {
    bVar1 = *(byte *)(unaff_ESI + 0x44);
    sVar6 = (short)uVar4;
    if ((bVar1 & 1) == 0) {
      if ((param_5 < 0.0) || (local_54 = 1.0 - param_5, local_54 < 0.0)) {
        local_54 = 0.0;
      }
      else if (1.0 < local_54) {
        local_54 = 1.0;
      }
      color_rgb_int_to_real();
      local_44 = local_44 * local_54;
      local_40 = local_40 * local_54;
      local_1c = *(uint *)(unaff_ESI + 0x34) & 0xffffff | (int)sVar6 << 0x18;
      local_18 = *(uint *)(unaff_ESI + 0x38) & 0xffffff;
      local_3c = local_3c * local_54;
      uVar7 = color_rgb_float_to_int(&local_44);
      local_10 = uVar7 & 0xffffff | (int)(short)uVar5 << 0x18;
    }
    else {
      if ((param_4 & 1) == 0) {
        local_18 = *(uint *)(unaff_ESI + 0x34);
      }
      else {
        if ((bVar1 & 2) != 0) {
          color_rgb_int_to_real();
          color_rgb_int_to_real();
          if ((bVar1 & 0x10) == 0) {
            local_54 = param_6;
          }
          else {
            local_54 = 1.0 - param_6;
          }
          color_interpolate(&local_44,0,local_54);
          local_1c = color_rgb_float_to_int(&local_44);
          local_1c = local_1c | (int)sVar6 << 0x18;
          local_18 = color_rgb_float_to_int(&local_44);
          local_10 = (int)sVar6 << 0x18;
          goto LAB_004ac00e;
        }
        local_18 = *(uint *)(unaff_ESI + 0x38);
      }
      local_18 = local_18 & 0xffffff;
      local_10 = (int)sVar6 << 0x18;
      local_1c = local_18 | local_10;
    }
  }
  else {
    local_18 = 0;
    local_1c = 0;
    local_10 = 0;
  }
LAB_004ac00e:
  local_28 = 1.0 - *(float *)(unaff_ESI + 0x4c);
  local_14 = (-1 - (*(uint *)(unaff_ESI + 0x40) >> 0x18)) * 0x1000000 |
             *(uint *)(unaff_ESI + 0x40) & 0xffffff;
  local_2c = *(float *)(unaff_ESI + 0x50);
  local_24 = local_28;
  local_20 = local_28;
  local_8 = color_pack_argb_from_real(&local_2c);
  local_4 = 0x3f800000;
  local_c = 0;
  local_b = 1;
  FUN_004acad0(&local_1c,uVar2,param_1,local_50,0,0xffffffff,param_4 >> 2 & 0xffffff01);
  return;
}
#endif

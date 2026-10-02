// hud_draw_static_element  (Ghidra: FUN_004ac6f0, renamed in the phase-4 review)
// address 0x4ac6f0, size 595 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4ac6f0..0x4ac942 in the phase-4 review. Argument 3 is a
// HUD static element viewed from its anchor_offset (WeaponHUDInterfaceStaticElement + 0x24):
// bitmap tag id +0x30, flash parameters +0x34, disabled color +0x4c, sequence index +0x54 and
// the multitexture overlay block +0x58/+0x5c (stride 0x1e0), now hud_static_element_placement
// in types/interface.h (the first rewrite carried it as a TYPES-GAP typedef with an invented
// segment count). The sprite rectangle is sprite 0 of the sequence (0 % count), the bitmap
// type 4 (interface bitmaps) test selects pixel uvs, and the draw flags are bit 0 flash,
// bit 1 disabled color, bit 2 split screen. The first rewrite passed texture_cache_get and
// 0x4acad0 no real arguments and lost the local player index that the overlays need.
// register convention: plain cdecl, five stack arguments.

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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time); // 0x4ab980, blam-cc: ESI flash, EDI start_time
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector); // 0x4ab690, blam-cc: AL has_scale, EDX offset, ECX child placement (selector)
extern void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                                    void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                                    float scale, float rotation, uint32_t color, uint8_t split_screen); // 0x4acad0, blam-cc: EAX uv, EDX placement, BL pixel_uvs
extern void hud_bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv,
                                      float *out_extents, int16_t anchor); // 0x4acc50, blam-cc: CL, ESI, EDX, EAX
extern void hud_draw_multitexture_overlay(const float *scale, const HUDInterfaceMultitextureOverlay *overlay,
                                          int16_t local_player_index, const Point2DInt *screen_position,
                                          const float *uv, const float *extents, float rotation,
                                          uint32_t color); // 0x4acfe0, blam-cc: EAX scale

// Draws one HUD static element (its bitmap sprite plus every multitexture overlay attached to
// it) at the element anchor. draw_flags: bit 0 blend in the flashing color from
// flash_start_time, bit 1 use the disabled color, bit 2 split screen.
void hud_draw_static_element(int16_t local_player_index, uint16_t *anchor,
                             const hud_static_element_placement *element, uint32_t draw_flags,
                             int32_t flash_start_time)
{
    Bitmap *bitmap_tag;
    BitmapData *bitmap;
    const float *uv;
    uint32_t color;
    uint8_t pixel_uvs;
    float scale;
    datum_index tag_id;
    int16_t i;

    tag_id = *(const datum_index *)&element->interface_bitmap.tag_id;
    bitmap_tag = (Bitmap *)tag_instances[tag_id & 0xffff].data; // read before the -1 test
    bitmap = bitmap_group_sequence_get_bitmap_data(tag_id, 0, (int16_t)element->sequence_index);
    if (texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    uv = 0;
    if (tag_id != (datum_index)-1 && element->sequence_index != 0xffff) {
        Bitmap *tag = (Bitmap *)tag_instances[tag_id & 0xffff].data;
        if ((int32_t)(int16_t)element->sequence_index < (int32_t)tag->bitmap_group_sequence.count) {
            BitmapGroupSequence *sequence =
                (BitmapGroupSequence *)tag->bitmap_group_sequence.pointer + (int16_t)element->sequence_index;
            int32_t sprite_count = (int32_t)sequence->sprites.count;
            if (sprite_count != 0) {
                BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer + 0 % sprite_count;
                uv = &sprite->left;
            }
        }
    }

    if ((draw_flags & 2) != 0) {
        color = *(const uint32_t *)&element->disabled_color;
    } else if ((draw_flags & 1) != 0) {
        color = hud_meter_flash_color_blend(&element->flash, flash_start_time);
    } else {
        color = *(const uint32_t *)&element->flash.default_color;
    }

    pixel_uvs = bitmap_tag->type == 4; // bitmaptype_interface_bitmaps
    scale = (*(const uint8_t *)&element->scaling_flags & 4) != 0 ? 0.5f : 1.0f;
    hud_draw_bitmap_element(uv, (const hud_element_placement *)element, pixel_uvs, 0, bitmap, anchor,
                            scale, 0.0f, color, (uint8_t)((draw_flags >> 2) & 1));

    if ((int32_t)element->multitexture_overlays.count > 0) {
        float default_uv[4];

        default_uv[0] = 0.0f;
        default_uv[2] = 0.0f;
        i = 0;
        do {
            const HUDInterfaceMultitextureOverlay *overlay =
                (const HUDInterfaceMultitextureOverlay *)element->multitexture_overlays.pointer + i;
            float element_scale[2];
            float extents[4];
            Point2DInt screen_position;
            uint8_t scale_offset;

            default_uv[1] = 1.0f;
            default_uv[3] = 1.0f;
            if (pixel_uvs != 0) {
                default_uv[1] = (float)(int32_t)(int16_t)bitmap->width;
                default_uv[3] = (float)(int32_t)(int16_t)bitmap->height;
            }
            if (uv == 0) {
                uv = default_uv;
            }
            element_scale[0] = scale * element->width_scale;
            element_scale[1] = scale * element->height_scale;
            scale_offset = 0;
            if ((int16_t)(draw_flags & 4) != 0 && (*(const uint8_t *)&element->scaling_flags & 1) == 0) {
                scale_offset = 1;
            }
            hud_anchor_offset_to_screen_position(anchor, scale_offset, 0.0f, &element->anchor_offset.x,
                                                 &screen_position.x, 0);
            hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, (int16_t)*anchor);
            hud_draw_multitexture_overlay(element_scale, overlay, local_player_index, &screen_position, uv,
                                          extents, 0.0f, color);
            i++;
        } while ((int32_t)i < (int32_t)element->multitexture_overlays.count);
    }
}

#if 0
Original Ghidra decompilation (0x4ac6f0):

void FUN_004ac6f0(undefined4 param_1,undefined2 *param_2,int param_3,uint param_4)

{
  short sVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  float local_48;
  undefined4 local_44;
  undefined4 *local_40;
  undefined4 *local_3c;
  undefined1 local_30 [4];
  int local_2c;
  float local_28;
  float local_24;
  undefined4 local_20;
  float local_1c;
  undefined4 local_18;
  float local_14;
  undefined1 local_10 [16];

  psVar2 = *(short **)((*(uint *)(param_3 + 0x30) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = bitmap_group_sequence_get_bitmap_data(*(undefined2 *)(param_3 + 0x54));
  iVar4 = FUN_00444550(0,1);
  if (iVar4 != 0) {
    sVar1 = *(short *)(param_3 + 0x54);
    local_40 = (undefined4 *)0x0;
    if ((*(uint *)(param_3 + 0x30) != 0xffffffff) && (sVar1 != -1)) {
      iVar4 = *(int *)((*(uint *)(param_3 + 0x30) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((int)sVar1 < *(int *)(iVar4 + 0x54)) {
        iVar5 = sVar1 * 0x40 + *(int *)(iVar4 + 0x58);
        iVar4 = *(int *)(iVar5 + 0x34);
        if (iVar4 != 0) {
          local_40 = (undefined4 *)((int)(0 % (longlong)iVar4) * 0x20 + 8 + *(int *)(iVar5 + 0x38));
        }
      }
    }
    local_3c = local_40;
    if ((param_4 & 2) == 0) {
      if ((param_4 & 1) == 0) {
        local_44 = *(undefined4 *)(param_3 + 0x34);
      }
      else {
        local_44 = FUN_004ab980();
      }
    }
    else {
      local_44 = *(undefined4 *)(param_3 + 0x4c);
    }
    sVar1 = *psVar2;
    local_48 = 1.0;
    if ((*(byte *)(param_3 + 0xc) & 4) != 0) {
      local_48 = 0.5;
    }
    FUN_004acad0(0,iVar3,param_2,local_48,0,local_44,param_4 >> 2 & 0xffffff01);
    sVar6 = 0;
    if (0 < *(int *)(param_3 + 0x58)) {
      iVar4 = 0;
      local_20 = 0;
      local_18 = 0;
      do {
        local_2c = iVar4 * 0x1e0 + *(int *)(param_3 + 0x5c);
        local_1c = 1.0;
        local_14 = 1.0;
        if (sVar1 == 4) {
          local_1c = (float)(int)*(short *)(iVar3 + 4);
          local_14 = (float)(int)*(short *)(iVar3 + 6);
        }
        if (local_3c == (undefined4 *)0x0) {
          local_3c = &local_20;
        }
        local_28 = local_48 * *(float *)(param_3 + 4);
        local_24 = local_48 * *(float *)(param_3 + 8);
        FUN_004ab690(param_2,0,local_30);
        FUN_004acc50(*param_2);
        FUN_004acfe0(local_2c,param_1,local_30,local_3c,local_10,0,local_44);
        sVar6 = sVar6 + 1;
        iVar4 = (int)sVar6;
      } while (iVar4 < *(int *)(param_3 + 0x58));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

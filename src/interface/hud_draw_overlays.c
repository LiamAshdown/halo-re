// hud_draw_overlays  (Ghidra: FUN_004ac950, renamed in the phase-4 review)
// address 0x4ac950, size 372 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4ac950..0x4acac3 in the phase-4 review. Argument 2 is a
// WeaponHUDInterfaceOverlayElement viewed from +0x24 (bitmap tag id +0x0c, overlays block
// +0x10/+0x14, stride 0x88), now hud_overlay_list in types/interface.h. Per overlay: skipped
// when flags bit 1 is set or type & type_mask is 0; flashing color and an animated frame
// (((game_time - start) / frame_rate) / 30 % sprite count) only when overlay flags bit 0 and
// draw flags bit 0 are both set; hud_meter_resolve_bitmap_frame gives the bitmap data and the
// sprite uv; drawn through 0x4acad0 at scale 1.0 with normalized uvs. The first rewrite passed
// NULL to texture_cache_get and 0x4acad0, lost the frame and the flash start time.
// register convention: plain cdecl, six stack arguments.

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
extern game_time_globals *game_time; // 0x006f1d6c

extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time); // 0x4ab980, blam-cc: ESI flash, EDI start_time
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index
extern void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                                    void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                                    float scale, float rotation, uint32_t color, uint8_t split_screen); // 0x4acad0, blam-cc: EAX uv, EDX placement, BL pixel_uvs

// Draws the overlays of one HUD overlay element whose type bits (show on flashing, empty,
// reload/overheat, default, always) intersect type_mask. draw_flags bit 0 allows flashing and
// frame animation from flash_start_time; split_screen is forwarded to 0x4acad0.
void hud_draw_overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask,
                       int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen)
{
    int32_t i;

    for (i = 0; i < (int32_t)list->overlays.count; i++) {
        const WeaponHUDInterfaceOverlay *overlay = (const WeaponHUDInterfaceOverlay *)list->overlays.pointer + i;
        uint32_t overlay_flags = *(const uint32_t *)&overlay->flags; // dword read at +0x4c
        datum_index tag_id = *(const datum_index *)&list->overlay_bitmap.tag_id;
        const BitmapGroupSequence *sequence;
        uint32_t color;
        int32_t frame;
        BitmapData *bitmap;
        int32_t sprite_uv;

        if ((overlay_flags & 2) != 0 || (type_mask & (uint32_t)(int32_t)(int16_t)overlay->type) == 0) {
            continue;
        }
        sequence = (const BitmapGroupSequence *)((Bitmap *)tag_instances[tag_id & 0xffff].data)
                       ->bitmap_group_sequence.pointer + (int16_t)overlay->sequence_index;

        if ((overlay_flags & 1) != 0 && (draw_flags & 1) != 0) {
            color = hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color,
                                                flash_start_time);
        } else {
            color = *(const uint32_t *)&overlay->default_color;
        }

        if ((*(const uint8_t *)&overlay->flags & 1) != 0 && (draw_flags & 1) != 0 && overlay->frame_rate > 0) {
            frame = ((game_time->game_time - flash_start_time) / overlay->frame_rate) / 30 %
                    (int32_t)sequence->sprites.count;
        } else {
            frame = 0;
        }

        bitmap = 0;
        sprite_uv = 0;
        hud_meter_resolve_bitmap_frame(tag_id, (int16_t)overlay->sequence_index, (uint16_t)frame,
                                       (void **)&bitmap, &sprite_uv);
        if (bitmap != 0 && texture_cache_get(bitmap, 0, 1) != 0) {
            hud_draw_bitmap_element((const float *)sprite_uv, (const hud_element_placement *)overlay, 0, 0,
                                    bitmap, anchor, 1.0f, 0.0f, color, split_screen);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ac950):

void FUN_004ac950(undefined4 param_1,int param_2,uint param_3,undefined4 param_4,byte param_5,
                 undefined4 param_6)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int local_14;
  int local_10;
  undefined4 local_c;
  undefined4 local_8;
  int local_4;

  local_4 = 0;
  if (0 < *(int *)(param_2 + 0x10)) {
    local_14 = 0;
    do {
      uVar1 = *(uint *)(*(int *)(param_2 + 0x14) + 0x4c + local_14);
      iVar3 = *(int *)(param_2 + 0x14) + local_14;
      if (((uVar1 & 2) == 0) && ((param_3 & (int)*(short *)(iVar3 + 0x4a)) != 0)) {
        if (((uVar1 & 1) == 0) || ((param_5 & 1) == 0)) {
          local_c = *(undefined4 *)(iVar3 + 0x24);
        }
        else {
          local_c = FUN_004ab980();
        }
        local_10 = 0;
        local_8 = 0;
        FUN_004ab8d0(*(undefined4 *)(param_2 + 0xc),*(undefined2 *)(iVar3 + 0x48),&local_10,&local_8
                    );
        iVar3 = local_10;
        if ((local_10 != 0) && (iVar2 = FUN_00444550(0,1), iVar2 != 0)) {
          FUN_004acad0(0,iVar3,param_1,0x3f800000,0,local_c,param_6);
        }
      }
      local_4 = local_4 + 1;
      local_14 = local_14 + 0x88;
    } while (local_4 < *(int *)(param_2 + 0x10));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

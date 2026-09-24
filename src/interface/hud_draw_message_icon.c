// hud_draw_message_icon  (Ghidra: FUN_004ad970, renamed in the phase-4 review)
// address 0x4ad970, size 447 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: objdump 0x4ad970..0x4adb2e; the only caller is hud_messaging_update (0x4ae550),
// for a message element that references an icon argument. ESI is a hud_messaging_information
// block (WeaponHUDInterface +0x13c layout: sequence +0x0, width offset +0x2, offset +0x4/+0x6,
// override color +0x8, frame rate +0xc, flags +0xd), not an ammo state. The icon comes from
// HUDGlobals icon_bitmap (+0xb0 tag id), frame game_time / frame_rate, scale 0.75 in split
// screen; it is drawn with hud_draw_bitmap_at at anchor 2 (bottom left) with its bottom on the
// cursor bottom, and the cursor left edge moves past it. The first rewrite passed no
// rectangle, called the truncating helper 0x6391b4 without arguments and lost the cursor
// update.
// register convention: ESI information block; two stack arguments.
//   // blam-cc: information -> ESI
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern game_time_globals *game_time;          // 0x006f1d6c
extern player_globals *local_player_globals;  // 0x0087a478

extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color); // 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs

// blam-cc: information -> ESI
// Draws the icon of one HUD message icon argument at the text cursor and advances the cursor.
void hud_draw_message_icon(const hud_messaging_information *information, Rectangle2D *cursor, uint32_t color)
{
    BitmapData *bitmap;
    int32_t uv_offset;
    const float *uv;
    int32_t frame;
    float scale;
    int16_t x;
    Point2DInt position;

    bitmap = 0;
    uv_offset = 0;
    frame = 0;
    if (information->frame_rate != 0) {
        frame = game_time->game_time / (int32_t)information->frame_rate;
    }
    hud_meter_resolve_bitmap_frame(*(datum_index *)&hud_globals_tag_data->icon_bitmap.tag_id,
                                   (int16_t)information->sequence_index, (uint16_t)frame, (void **)&bitmap,
                                   &uv_offset);
    if (bitmap == 0 || texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    uv = (const float *)uv_offset;

    scale = 0.75f;
    if (local_player_globals->local_player_count <= 1) {
        scale = 1.0f;
    }
    x = (int16_t)__ftol((double)((float)information->offset.x * scale + (float)cursor->left));
    position.x = x;
    position.y = (int16_t)__ftol((double)((float)cursor->bottom - (float)information->offset.y * scale));
    if ((information->flags & 2) != 0) {
        color = *(const uint32_t *)&information->override_icon_color;
    }
    hud_draw_bitmap_at(uv, bitmap, 0, 2, &position, scale, 0.0f, color);

    if ((information->flags & 4) != 0) {
        cursor->left = (int16_t)__ftol((double)((float)information->width_offset * scale + (float)x));
    } else if (uv != 0) {
        cursor->left = (int16_t)__ftol((double)(((uv[1] - uv[0]) * (float)(int16_t)bitmap->width +
                                                 (float)information->width_offset) * scale + (float)x));
    } else {
        cursor->left = (int16_t)__ftol((double)((float)((int16_t)bitmap->width + information->width_offset) * scale +
                                                (float)x));
    }
}

#if 0
Original Ghidra decompilation (0x4ad970):

void FUN_004ad970(int param_1,undefined4 param_2)

{
  short sVar1;
  undefined2 uVar2;
  int iVar3;
  undefined2 *unaff_ESI;
  int local_10;
  undefined4 local_c;
  int local_8;
  int local_4;

  local_10 = 0;
  local_8 = 0;
  FUN_004ab8d0(*(undefined4 *)(DAT_0071941c + 0xb0),*unaff_ESI,&local_10,&local_8);
  if ((local_10 != 0) && (iVar3 = FUN_00444550(0,1), iVar3 != 0)) {
    local_10 = 0x3f400000;
    if (*(short *)(DAT_0087a478 + 0xc) < 2) {
      local_10 = 0x3f800000;
    }
    local_c = (int)*(short *)(param_1 + 2);
    sVar1 = FUN_006391b4();
    local_4 = (int)(short)unaff_ESI[3];
    local_c = CONCAT22(local_c._2_2_,sVar1);
    uVar2 = FUN_006391b4();
    iVar3 = local_8;
    local_c = CONCAT22(uVar2,(undefined2)local_c);
    if ((*(byte *)((int)unaff_ESI + 0xd) & 2) != 0) {
      param_2 = *(undefined4 *)(unaff_ESI + 4);
    }
    FUN_004acbb0(2,&local_c,local_10,0,param_2);
    if ((*(byte *)((int)unaff_ESI + 0xd) & 4) != 0) {
      local_4 = (int)sVar1;
      uVar2 = FUN_006391b4();
      *(undefined2 *)(param_1 + 2) = uVar2;
      return;
    }
    if (iVar3 != 0) {
      local_4 = (int)sVar1;
      uVar2 = FUN_006391b4();
      *(undefined2 *)(param_1 + 2) = uVar2;
      return;
    }
    local_4 = (int)sVar1;
    uVar2 = FUN_006391b4();
    *(undefined2 *)(param_1 + 2) = uVar2;
  }
  return;
}
#endif

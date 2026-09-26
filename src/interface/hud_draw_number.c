// hud_draw_number  (Ghidra: FUN_004ac0b0, renamed; earlier hud_draw_ammo_digit)
// address 0x4ac0b0, size 1566 bytes; real extent 0x4ac0b0..0x4ac6ce (1566 bytes; Ghidra stopped at the jump
// table and reported 402)
// name confidence: 0.55 (chosen)   rewrite confidence: 0.6
// evidence: rewritten in the phase-4 review from objdump -d 0x4ac0b0..0x4ac6ce and the jump
// table at 0x4ac6d0 (anchor 0 and 2, 1 and 3, 4). The routine draws a whole HUD number right to
// left with the Globals interface_bitmaps hud_digits_definition (HUDNumber): glyphs 0..9 are the
// digits, 0xa the decimal point, 0xc the minus sign, 0xd the trailing m and 0xe its km form. The
// element is a hud_number_placement (types/interface.h). With flags bit 2 (draw a trailing m) a
// value over 999 is shown in thousands with the remainder as fraction digits.
// Each glyph goes through hud_meter_resolve_bitmap_frame @0x4ab8d0 (EAX glyph, stack digits
// bitmap tag, sequence 0, out bitmap data, out sprite rect) and hud_draw_bitmap_at 0x4acbb0 (EAX sprite rect, EDX
// bitmap data, CL sprite-bitmap flag, stack anchor value, position, scale, 0, color), then the pen
// moves left by screen_digit_width times the scale (_ftol truncation at every step).
// UNSURE: the first stack argument is never read. 0x4acbb0 is hud_draw_bitmap_at (rewritten in
// the s2 part 2 review); the 0 pushed as its fourth stack argument is the rotation.
// register convention: __cdecl, eight stack arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern Globals *global_globals;     // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out,
                                                 int32_t selector); // 0x4ab690, blam-cc: AL has_scale, EDX offset, ECX selector
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time); // 0x4ab980, blam-cc: ESI flash, EDI start_time
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color); // 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs

typedef struct hud_number_pen {
    datum_index digits_bitmap;
    uint8_t is_sprite_bitmap;
    uint16_t anchor;
    int16_t x;
    int16_t y;
    float advance;   // screen_digit_width * scale
    float scale;
    uint32_t color;
} hud_number_pen;

// Draws one glyph at the pen and moves it one digit to the left.
static void hud_number_draw_glyph(hud_number_pen *pen, uint16_t glyph, uint8_t advance)
{
    void *bitmap_data = 0;
    int32_t sprite_rect = 0;
    Point2DInt position;

    position.x = pen->x;
    position.y = pen->y;
    hud_meter_resolve_bitmap_frame(pen->digits_bitmap, 0, glyph, &bitmap_data, &sprite_rect);
    hud_draw_bitmap_at((const float *)sprite_rect, (BitmapData *)bitmap_data, pen->is_sprite_bitmap, (int16_t)pen->anchor,
                       &position, pen->scale, 0.0f, pen->color);
    if (advance) {
        pen->x = (int16_t)(int32_t)((float)pen->x - pen->advance);
    }
}

void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                     int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale)
{
    GlobalsInterfaceBitmaps *interface_bitmaps = (global_globals->interface_bitmaps.count != 0)
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    datum_index digits_tag = *(datum_index *)&interface_bitmaps->hud_digits_definition.tag_id;
    HUDNumber *digits;
    uint8_t *digits_bitmap_data;
    BitmapData *bitmap;
    uint8_t thousands;
    uint8_t negative;
    float width_digits;
    float width_decimal;
    int32_t magnitude;
    int32_t fraction_value = fraction;
    Point2DInt origin;
    hud_number_pen pen;

    (void)unused;
    if (digits_tag == (datum_index)-1) {
        return;
    }
    digits = (HUDNumber *)tag_instances[digits_tag & 0xffff].data;
    pen.digits_bitmap = *(datum_index *)&digits->digits_bitmap.tag_id;
    digits_bitmap_data = (uint8_t *)tag_instances[pen.digits_bitmap & 0xffff].data;
    bitmap = bitmap_group_sequence_get_bitmap_data(pen.digits_bitmap, 0, 0);
    thousands = (value > 999);
    if (texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    negative = (value < 0);

    {
        int32_t extra = 0;
        if (placement->number_of_fractional_digits != 0 && fraction != -1) {
            extra = ((placement->number_of_fractional_digits > 4) ? 4 : placement->number_of_fractional_digits) + 1;
        }
        width_digits = (float)(placement->maximum_number_of_digits + extra);
        width_decimal = (float)((placement->number_of_fractional_digits != 0) ? digits->decimal_point_width : 0);
    }
    pen.scale = (scale > 0.0f) ? scale : 1.0f;
    if (placement->scaling_flags & 4) {
        pen.scale = pen.scale * 0.5f;
    }
    if (placement->flags & 4) {
        width_digits = width_digits + 1.0f;
        if (thousands) {
            fraction_value = value * 10;
            value = (int16_t)(value / 1000);
        }
    }
    magnitude = (value < 0) ? -value : value;

    hud_anchor_offset_to_screen_position(anchor, (uint8_t)((flags >> 2) & 1), 0.0f,
                                         (const int16_t *)&placement->anchor_offset, (int16_t *)&origin, 0);
    switch (*anchor) {
    case 0:
    case 2:
        pen.x = (int16_t)(int32_t)(((width_digits - 2.0f) * digits->screen_digit_width + width_decimal) * pen.scale + origin.x);
        break;
    case 4:
        pen.x = (int16_t)(int32_t)(((width_digits - 1.0f) * digits->screen_digit_width + width_decimal) * pen.scale * 0.5f + origin.x);
        break;
    default: // 1 and 3, read as the jump table targets; other values are out of the table
        pen.x = origin.x;
        break;
    }
    if (bitmap == 0) {
        return;
    }

    if (flags & 2) {
        pen.color = *(uint32_t *)&placement->disabled_color;
    } else if (flags & 1) {
        pen.color = hud_meter_flash_color_blend(&placement->flash, flash_start_time);
    } else {
        pen.color = *(uint32_t *)&placement->flash.default_color;
    }
    pen.y = origin.y;
    pen.anchor = *anchor;
    pen.is_sprite_bitmap = (*(int16_t *)digits_bitmap_data == 4);
    pen.advance = (float)digits->screen_digit_width * pen.scale;

    if (placement->flags & 4) {
        hud_number_draw_glyph(&pen, (uint16_t)(0xd + (thousands != 0)), 1);
    }

    if (placement->number_of_fractional_digits != 0 && (int16_t)fraction_value >= 0) {
        int16_t count = (placement->number_of_fractional_digits > 4) ? 4 : placement->number_of_fractional_digits;
        int16_t i;
        if (count < 4) {
            for (i = (int16_t)(4 - count); i != 0; i--) {
                fraction_value = (int16_t)fraction_value / 10;
            }
        }
        for (i = count; i > 0; i--) {
            int16_t digit = (int16_t)((int16_t)fraction_value % 10);
            fraction_value = (int16_t)fraction_value / 10;
            hud_number_draw_glyph(&pen, (uint16_t)digit, 1);
        }
        {
            int16_t right = (int16_t)(int32_t)(pen.advance + (float)pen.x);
            pen.x = (int16_t)(int32_t)((float)right - (float)digits->decimal_point_width * pen.scale);
        }
        hud_number_draw_glyph(&pen, 0xa, 1);
    }

    if (placement->maximum_number_of_digits > 0) {
        int16_t i;
        for (i = 0; i < placement->maximum_number_of_digits; i++) {
            int16_t digit = (int16_t)((int16_t)magnitude % 10);
            int32_t quotient = (int16_t)magnitude / 10;
            if ((int16_t)magnitude == 0 && (placement->flags & 1) == 0) {
                break;
            }
            hud_number_draw_glyph(&pen, (uint16_t)digit, 1);
            magnitude = quotient;
        }
    }

    if (negative) {
        hud_number_draw_glyph(&pen, 0xc, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4ac0b0):

void FUN_004ac0b0(undefined4 param_1,short *param_2,int param_3,undefined4 param_4,short param_5)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int local_20;
  undefined4 local_1c;
  float local_18;
  float local_14;
  int local_c;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = *(int *)(DAT_00746fa0 + 0x144);
  }
  if (*(uint *)(iVar2 + 0xbc) != 0xffffffff) {
    iVar2 = *(int *)((*(uint *)(iVar2 + 0xbc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_1c = *(undefined4 *)((*(uint *)(iVar2 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    bitmap_group_sequence_get_bitmap_data(0);
    iVar3 = texture_cache_get(0,1);
    if (iVar3 != 0) {
      cVar1 = *(char *)(param_3 + 0x46);
      if ((cVar1 == '\0') || (param_5 == -1)) {
        local_c = 0;
      }
      else {
        local_c = 4;
        if (cVar1 < '\x05') {
          local_c = (int)cVar1;
        }
        local_c = local_c + 1;
      }
      local_c = *(char *)(param_3 + 0x44) + local_c;
      local_18 = (float)local_c;
      if (cVar1 == '\0') {
        local_20 = 0;
      }
      else {
        local_20 = (int)*(char *)(iVar2 + 0x14);
      }
      local_14 = (float)local_20;
      if ((*(byte *)(param_3 + 0x45) & 4) != 0) {
        local_18 = local_18 + 1.0;
      }
      FUN_004ab690(param_2,0,&local_20);
                    /* WARNING: Could not recover jumptable at 0x004ac234. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      (*(code *)(&PTR_LAB_004ac6d0)[*param_2])();
      return;
    }
  }
  return;
}
#endif

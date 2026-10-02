// ui_draw_trouble_brewing_indicator  (Ghidra: ui_draw_trouble_brewing_indicator, already named)
// address 0x49c870, size 128 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// phase-4 review: checked against objdump 0x49c870..0x49c8ef. The quad is drawn into the rect
// {top 0x196, left 0x236, bottom 0x1d6, right 0x276} (ECX, no source rect in EAX); without the
// tag or its bitmap the same rect is filled with 0x80ff0000 (0x449780: EAX color, ECX rect).
// The UNSURE notes below are resolved by this.
// evidence: matches the given name; functions.md: "Draws the 'trouble brewing' network-wait
// indicator bitmap while a network operation is pending, falling back to a hide/cleanup call if
// the bitmap tag cannot be resolved." types/interface.h names 0x006927c4 as
// ui_network_wait_start_time; 0x006927c8 is the cached bitmap tag for this indicator (TYPES-GAP).
// register convention: none (void).
// UNSURE: bitmap_group_sequence_get_bitmap_data's EAX/EDI (bitmap/frame) and ui_draw_filled_rectangle's two
// arguments are all read here from unresolved registers Ghidra shows no visible source for;
// modeled as the cached bitmap tag, frame 0, and (0, NULL) respectively.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ui_network_wait_start_time;      // 0x006927c4, -1 when no wait is running
extern datum_index trouble_brewing_bitmap_tag;  // 0x006927c8, TYPES-GAP

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550; blam-cc: group in EDI
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill, blam-cc: EAX packed_color, ECX rect
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                int16_t *clip_rect, uint32_t vertex_color); // 0x498b20, blam-cc: EAX source_rect, ECX dest_rect

// While a network wait is pending, resolves and draws the "trouble brewing" indicator bitmap;
// if the tag or its bitmap data cannot be resolved, falls back to ui_draw_filled_rectangle instead.
void ui_draw_trouble_brewing_indicator(void)
{
    if (ui_network_wait_start_time != -1) {
        Rectangle2D rect;

        rect.top = 0x196;
        rect.left = 0x236;
        rect.bottom = 0x1d6;
        rect.right = 0x276;
        trouble_brewing_bitmap_tag = tag_lookup(0x6269746d /* 'bitm' */,
                                                 (char *)"ui\\shell\\bitmaps\\trouble_brewing");
        if (trouble_brewing_bitmap_tag != (datum_index)-1) {
            BitmapData *bitmap_data = bitmap_group_sequence_get_bitmap_data(trouble_brewing_bitmap_tag, 0, 0);

            if (bitmap_data != 0) {
                ui_draw_screen_quad(0, (int16_t *)&rect, (int32_t)bitmap_data, 0, 0xffffffff);
                return;
            }
        }
        ui_draw_filled_rectangle(0x80ff0000, &rect); // half transparent red box in the same rect
    }
}

#if 0
Original Ghidra decompilation (0x49c870):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void ui_draw_trouble_brewing_indicator(void)

{
  int iVar1;

  if (DAT_006927c4 != -1) {
    _DAT_006927c8 = tag_lookup("ui\\shell\\bitmaps\\trouble_brewing");
    if (_DAT_006927c8 != -1) {
      iVar1 = bitmap_group_sequence_get_bitmap_data(0);
      if (iVar1 != 0) {
        FUN_00498b20(iVar1,0,0xffffffff);
        return;
      }
    }
    FUN_00449780();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

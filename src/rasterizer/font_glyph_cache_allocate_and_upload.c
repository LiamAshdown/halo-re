// font_glyph_cache_allocate_and_upload  (Ghidra: font_glyph_cache_allocate_and_upload, already named)
// address 0x514ed0, size 745 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: types/rasterizer.h font_glyph_cache (0x006d8828: oldest_slot 0x006d882a, next_slot
//   0x006d882c, cursor_x 0x006d882e, cursor_y 0x006d8830, row_height 0x006d8832, atlas 0x006d8834,
//   entries 0x006d8838 stride 8); FontCharacter bitmap_width +4, bitmap_height +6,
//   hardware_character_index +0xc, frame stamp +0xe, pixels_offset +0x10; Font.pixels.pointer +0x94.
//   Control flow and every int16 compare checked against the raw code (0x514ed0..0x5151b9).
// register convention: __cdecl (font, character) on the stack. Callee
//   bitmap_data_get_row_address 0x43f8e0 takes the bitmap in EDI and the mip level in EAX (always
//   0 here) plus (x, y) on the stack; rasterizer_bitmap_upload_cubemap_mipmaps 0x524270 takes the
//   bitmap in EBX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern font_glyph_cache g_font_glyph_cache;   // 0x006d8828
extern int32_t rasterizer_frame_index;        // 0x0069c694

// blam-cc: bitmap in EDI, mip level in EAX, x and y on the stack
extern uint16_t *bitmap_data_get_row_address(BitmapData *bitmap, int32_t mip_level, int32_t x, int32_t y); // 0x43f8e0


// Drops the glyph held by the oldest ring slot (if any) and advances the ring read index.
static void font_glyph_cache_evict_oldest(void)
{
    font_glyph_cache_entry *entry = &g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot];

    if (entry->character != 0) {
        ((FontCharacter *)entry->character)->hardware_character_index = 0xffff;
        entry->character = 0;
    }
    g_font_glyph_cache.oldest_slot = (uint16_t)((g_font_glyph_cache.oldest_slot + 1) & 0x1ff);
}

// Makes sure `character` has a slot in the 512x512 glyph atlas: packs it at the row cursor
// (starting a new row, or wrapping to the top and evicting what is in the way), copies its 8 bit
// coverage into the atlas as A4R4G4B4-style texels (alpha from the glyph, color 0xfff) with a one
// texel border, and re-uploads the atlas texture. A glyph that already has a slot is left alone.
void font_glyph_cache_allocate_and_upload(Font *font, FontCharacter *character)
{
    font_glyph_cache_entry *entry;
    BitmapData *atlas;
    uint8_t *pixels;
    uint16_t *texel;
    int16_t slot;
    int16_t row;
    int16_t column;

    if ((int16_t)character->hardware_character_index != -1) {
        return;
    }
    *(int16_t *)((uint8_t *)character + 0xe) = (int16_t)rasterizer_frame_index;

    // does not fit on the current row: start the next one
    if (character->bitmap_width + g_font_glyph_cache.cursor_x + 2 > 0x200) {
        g_font_glyph_cache.cursor_y = (int16_t)(g_font_glyph_cache.cursor_y + g_font_glyph_cache.row_height);
        g_font_glyph_cache.cursor_x = 0;
        g_font_glyph_cache.row_height = 0;
    }

    // does not fit below either: wrap to the top and evict every glyph that is not on row 0
    if (character->bitmap_height + g_font_glyph_cache.cursor_y + 2 >= 0x200) {
        g_font_glyph_cache.cursor_y = 0;
        g_font_glyph_cache.cursor_x = 0;
        g_font_glyph_cache.row_height = 0;
        while (g_font_glyph_cache.oldest_slot != g_font_glyph_cache.next_slot) {
            if (g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot].y <= 0) {
                break;
            }
            font_glyph_cache_evict_oldest();
        }
    }

    // the glyph makes the current row taller: evict the oldest glyphs that overlap the new band
    if (character->bitmap_height + 2 >= g_font_glyph_cache.row_height) {
        int16_t band_top = (int16_t)(g_font_glyph_cache.cursor_y + g_font_glyph_cache.row_height);
        int16_t band_bottom = (int16_t)(character->bitmap_height + g_font_glyph_cache.cursor_y + 2);

        while (g_font_glyph_cache.oldest_slot != g_font_glyph_cache.next_slot) {
            int16_t y = g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot].y;

            if (y < band_top || y >= band_bottom) {
                break;
            }
            font_glyph_cache_evict_oldest();
        }
        g_font_glyph_cache.row_height = (int16_t)(character->bitmap_height + 2);
    }

    // ring full: make room for one more slot
    if (((g_font_glyph_cache.next_slot + 1) & 0x1ff) == g_font_glyph_cache.oldest_slot) {
        font_glyph_cache_evict_oldest();
    }

    slot = (int16_t)g_font_glyph_cache.next_slot;
    character->hardware_character_index = (uint16_t)slot;
    entry = &g_font_glyph_cache.entries[slot];
    entry->character = (uint32_t)character;
    entry->x = g_font_glyph_cache.cursor_x;
    entry->y = g_font_glyph_cache.cursor_y;

    pixels = (uint8_t *)(font->pixels.pointer + character->pixels_offset);
    for (row = 0; row < character->bitmap_height + 2; row++) {
        atlas = (BitmapData *)g_font_glyph_cache.atlas;
        texel = bitmap_data_get_row_address(atlas, 0, (uint16_t)entry->x, (uint16_t)(entry->y + row));
        for (column = 0; column < character->bitmap_width + 2; column++) {
            if (row < 1 || row > character->bitmap_height ||
                column < 1 || column > character->bitmap_width) {
                *texel = 0x0fff;                             // transparent border texel
            } else {
                *texel = (uint16_t)((*pixels << 8) | 0x0fff);
                pixels++;
            }
            texel++;
        }
    }

    // the entry remembers the glyph's interior, inside the one texel border
    entry->x++;
    entry->y++;

    atlas = (BitmapData *)g_font_glyph_cache.atlas;
    switch (atlas->type) {
    case 0:
        rasterizer_bitmap_upload_2d_mipmaps(atlas);
        break;
    case 1:
        rasterizer_bitmap_upload_cubemap_mipmaps(atlas);   // UNSURE: type 1 is a 3D bitmap; see README
        break;
    case 2:
        rasterizer_bitmap_upload_cubemap_mipmaps_by_face(atlas);
        break;
    }

    g_font_glyph_cache.cursor_x = (int16_t)(g_font_glyph_cache.cursor_x + character->bitmap_width + 2);
    g_font_glyph_cache.next_slot = (uint16_t)((g_font_glyph_cache.next_slot + 1) & 0x1ff);
}

#if 0
Original Ghidra decompilation (0x514ed0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void font_glyph_cache_allocate_and_upload(int param_1,byte *param_2)

{
  short *psVar1;
  int iVar2;
  int iVar3;
  ushort *puVar4;
  short sVar5;
  int iVar6;
  short sVar7;
  ushort uVar8;
  short sVar9;
  ushort uVar10;
  bool bVar11;
  
  iVar3 = (int)param_2;
  if (*(short *)((int)param_2 + 0xc) == -1) {
    *(undefined2 *)((int)param_2 + 0xe) = DAT_0069c694;
    uVar8 = (ushort)DAT_006d8830;
    if (0x200 < *(short *)((int)param_2 + 4) + 2 + (int)_DAT_006d882e) {
      uVar8 = (ushort)DAT_006d8830 + DAT_006d8830._2_2_;
      _DAT_006d882c = _DAT_006d882c & 0xffff;
      DAT_006d8830 = (uint)uVar8;
    }
    uVar10 = DAT_006d882c;
    if (0x1ff < (short)uVar8 + 2 + (int)*(short *)((int)param_2 + 6)) {
      uVar8 = 0;
      bVar11 = DAT_006d882a != DAT_006d882c;
      _DAT_006d882c = _DAT_006d882c & 0xffff;
      DAT_006d8830 = 0;
      if (bVar11) {
        do {
          iVar6 = (int)(short)DAT_006d882a;
          if (*(short *)(&DAT_006d883e + iVar6 * 8) < 1) break;
          iVar2 = (&DAT_006d8838)[iVar6 * 2];
          if (iVar2 != 0) {
            *(undefined2 *)(iVar2 + 0xc) = 0xffff;
            (&DAT_006d8838)[iVar6 * 2] = 0;
            uVar8 = (ushort)DAT_006d8830;
            uVar10 = DAT_006d882c;
          }
          DAT_006d882a = DAT_006d882a + 1 & 0x1ff;
        } while (DAT_006d882a != uVar10);
      }
    }
    sVar7 = *(short *)((int)param_2 + 6);
    if ((int)DAT_006d8830._2_2_ <= sVar7 + 2) {
      sVar9 = uVar8 + DAT_006d8830._2_2_;
      sVar5 = (short)DAT_006d8830;
      if (DAT_006d882a != uVar10) {
        do {
          iVar6 = (int)(short)DAT_006d882a;
          if ((*(short *)(&DAT_006d883e + iVar6 * 8) < sVar9) ||
             ((short)(sVar7 + 2 + sVar5) <= *(short *)(&DAT_006d883e + iVar6 * 8))) break;
          iVar2 = (&DAT_006d8838)[iVar6 * 2];
          if (iVar2 != 0) {
            *(undefined2 *)(iVar2 + 0xc) = 0xffff;
            (&DAT_006d8838)[iVar6 * 2] = 0;
            uVar10 = DAT_006d882c;
          }
          DAT_006d882a = DAT_006d882a + 1 & 0x1ff;
        } while (DAT_006d882a != uVar10);
      }
      DAT_006d8830 = CONCAT22(*(short *)((int)param_2 + 6) + 2,(ushort)DAT_006d8830);
    }
    if (((short)_DAT_006d882c + 1U & 0x1ff) == DAT_006d882a) {
      iVar6 = (int)(short)DAT_006d882a;
      if ((&DAT_006d8838)[iVar6 * 2] != 0) {
        *(undefined2 *)((&DAT_006d8838)[iVar6 * 2] + 0xc) = 0xffff;
        (&DAT_006d8838)[iVar6 * 2] = 0;
        uVar10 = DAT_006d882c;
      }
      DAT_006d882a = DAT_006d882a + 1 & 0x1ff;
    }
    *(ushort *)((int)param_2 + 0xc) = uVar10;
    iVar6 = (short)uVar10 * 8;
    (&DAT_006d8838)[(short)uVar10 * 2] = param_2;
    *(short *)(&DAT_006d883c + iVar6) = _DAT_006d882e;
    *(ushort *)(&DAT_006d883e + iVar6) = (ushort)DAT_006d8830;
    psVar1 = (short *)((int)param_2 + 6);
    sVar7 = 0;
    param_2 = (byte *)(*(int *)(param_1 + 0x94) + *(int *)((int)param_2 + 0x10));
    if (*psVar1 != -2 && -1 < *psVar1 + 2) {
      do {
        puVar4 = (ushort *)
                 bitmap_data_get_row_address
                           (*(undefined2 *)(&DAT_006d883c + iVar6),
                            *(short *)(&DAT_006d883e + iVar6) + sVar7);
        sVar5 = 0;
        if (0 < *(short *)(iVar3 + 4) + 2) {
          do {
            if ((((sVar7 < 1) || (*(short *)(iVar3 + 6) < sVar7)) || (sVar5 < 1)) ||
               (*(short *)(iVar3 + 4) < sVar5)) {
              *puVar4 = 0xfff;
            }
            else {
              *puVar4 = (ushort)*param_2 << 8 | 0xfff;
              param_2 = param_2 + 1;
            }
            puVar4 = puVar4 + 1;
            sVar5 = sVar5 + 1;
          } while ((int)sVar5 < *(short *)(iVar3 + 4) + 2);
        }
        sVar7 = sVar7 + 1;
      } while ((int)sVar7 < *(short *)(iVar3 + 6) + 2);
    }
    *(short *)(&DAT_006d883c + iVar6) = *(short *)(&DAT_006d883c + iVar6) + 1;
    *(short *)(&DAT_006d883e + iVar6) = *(short *)(&DAT_006d883e + iVar6) + 1;
    sVar7 = *(short *)(DAT_006d8834 + 10);
    if (sVar7 == 0) {
      FUN_00524100(DAT_006d8834);
    }
    else if (sVar7 == 1) {
      FUN_00524270();
    }
    else if (sVar7 == 2) {
      FUN_005243c0(DAT_006d8834);
    }
    _DAT_006d882e = _DAT_006d882e + *(short *)(iVar3 + 4) + 2;
    _DAT_006d882c = CONCAT22(_DAT_006d882e,DAT_006d882c + 1) & 0xffff01ff;
  }
  return;
}
#endif

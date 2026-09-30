// screenshot_render  (Ghidra: screenshot_render, already named)
// address 0x4ca1a0, size 754 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: matches the given name (cea-pdb string match). out/phase4/main_types_notes.md pins
// the BitmapData this function builds (0x30 bytes, signature 'bitm', width/height/depth/type/
// format/flags, pixel buffer pointer +0x2c) and the file_reference_record it builds to export
// each tile (same 0x43-dword-memset shape as src/main/movie_capture_frame_export.c). game_window_
// rect (0x0069c634/0x0069c638, Rectangle2D pair) reuses src/main/viewport_split_rect_compute.c's
// derivation; screenshot_scale (0x00696568, clamped to 1..3) and screenshot_index (0x00719722)
// are main.h's own documented globals. rasterizer_capture_and_present's (FUN_00518180) own file
// header explicitly names this exact call site: "screenshot_render 0x4ca33d (EAX = the tile
// being rendered)" -- confirming EAX is a pointer to the current sub-tile's {x, y} indices, not
// the bitmap. render_frame (Ghidra: render_views_draw_all) reuses src/render/render_frame.c's
// signature: the 1-tile call passes a NULL screenshot_page, the N x N call passes &{col, row}.
// register convention: cdecl, one parameter (views, forwarded straight into render_frame).
// rewrite confidence raised to 0.7 after the review.
// phase 4 review (disassembly 0x4ca1a0..0x4ca493): 0x0065512c is the empty string and the
// print passes AL = 1 (clear the console first), not 0; the single tile call passes NULL to
// both render_frame (EBX) and the capture (EAX), the tiled call passes &tile to both and &page
// on the stack (the phase 3 file passed NULL as screenshot_tile and always a tile to the
// capture); 0x43f880 takes the bitmap in ESI.
// UNSURE: the file_reference_record's path-field backslash-insertion preamble (objdump-confirmed
// dead code exactly like src/main/movie_capture_frame_export.c's flags-bit-0 check: the record
// is always freshly zeroed immediately before, so its path field is always empty at this point)
// is simplified to the single strncpy it always reduces to, per that same file's precedent.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "rasterizer.h"
#include "render.h"
#include "networking.h"
#include "saved_games.h"
#include "hs.h"
#include "main.h"
#include "fn_main.h"
#include <stdio.h>
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern int16_t screenshot_scale;       // 0x00696568, foreign (rasterizer module)
extern Rectangle2D game_window_top_left;   // 0x0069c634, foreign (rasterizer module)

extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, this module

extern void render_frame(Point2DInt *screenshot_tile, render_view *views, int16_t count,
    Point2DInt *screenshot_page, float time_since_tick, float time_since_frame); // 0x50bea0, foreign (render module)
extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); // 0x518180, foreign (rasterizer module)
extern void directory_create_recursive(char *path); // 0x449250, foreign
extern void path_remove_last_component(uint8_t *path); // 0x555f80, foreign (game module)
extern char * targa_export(BitmapData *bitmap, file_reference_record *destination); // 0x43fe60, foreign
    // blam-cc: EAX -> bitmap, EBX -> destination (0x4ca432 mov eax,esi ; lea ebx,[esp+0x18])
extern uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap); // 0x43fb70, foreign
extern void bitmap_data_free(BitmapData *bitmap); // 0x43f880, foreign; blam-cc: ESI -> bitmap (it tests esi
    // first and releases +0x24 / +0x28 / +0x2c; src/rasterizer declares it (void))

// Renders each active viewport tiled n by n (n = screenshot_scale, clamped to 1..3) into a
// freshly allocated bitmap sized to the full game window at that scale, and saves each n by n
// page as its own numbered "screenshots\\NNscreenshotROWCOL.tga" file. Clears
// main_globals.screenshot_tile_count when the bitmap allocation fails; otherwise renders every
// tile/page combination, exports each page, advances screenshot_index, and frees the bitmap
// group.
void screenshot_render(render_view *views)
{
    int16_t width;
    int16_t height;
    BitmapData *bitmap;
    int16_t original_scale = screenshot_scale;
    int16_t page_row, page_col;

    if (original_scale < 1) {
        screenshot_scale = 1;
    } else {
        screenshot_scale = 3;
        if (original_scale < 4) {
            screenshot_scale = original_scale;
        }
    }

    height = (int16_t)((game_window_top_left.bottom - game_window_top_left.top) * screenshot_scale);
    width = (int16_t)((game_window_top_left.right - game_window_top_left.left) * screenshot_scale);

    bitmap = (BitmapData *)GlobalAlloc(0, 0x30);
    if (bitmap == 0) {
        main_globals_data.screenshot_tile_count = 0;
        return;
    }
    memset(bitmap, 0, 0x30);

    bitmap->bitmap_class = 0x6269746d; // 'bitm'
    bitmap->width = width;
    bitmap->height = height;
    bitmap->depth = 1;
    bitmap->type = 0;
    bitmap->format = 10;
    bitmap->flags = 0x40;
    if (((int32_t)width & ((int32_t)width - 1)) == 0 &&
        ((int32_t)height & ((int32_t)height - 1)) == 0) {
        bitmap->flags = 0x41;
    }

    *(void **)&((struct BitmapData *)bitmap)->pixel_base = GlobalAlloc(0, bitmap_data_calculate_pixel_data_size(bitmap));

    if (*(void **)&((struct BitmapData *)bitmap)->pixel_base != 0) {
        console_print_error_va(1, "");   // AL = 1: clear the console, 0x0065512c is ""
        console_deactivate();

        for (page_row = 0; page_row < main_globals_data.screenshot_tile_count; page_row++) {
            for (page_col = 0; page_col < main_globals_data.screenshot_tile_count; page_col++) {
                int16_t sub_row, sub_col;
                char filename[512];
                file_reference_record request;
                Point2DInt page;             // [esp+0x10] {col, row}
                Point2DInt tile;             // [esp+0x14] {sub_col, sub_row}

                page.x = page_col;
                page.y = page_row;
                for (sub_row = 0; sub_row < screenshot_scale; sub_row++) {
                    for (sub_col = 0; sub_col < screenshot_scale; sub_col++) {
                        tile.x = sub_col;
                        tile.y = sub_row;
                        if (main_globals_data.screenshot_tile_count < 2 && screenshot_scale < 2) {
                            render_frame(0, views, 1, 0, 0.0f, 0.0f);          // EBX = 0
                            rasterizer_capture_and_present(0, bitmap);          // EAX = 0
                        } else {
                            render_frame(&tile, views, 1, &page, 0.0f, 0.0f);  // EBX = &tile
                            rasterizer_capture_and_present(&tile.x, bitmap);    // EAX = EBX
                        }
                    }
                }

                sprintf(filename, "%s\\%dscreenshot%d%d.tga", "screenshots",
                        (uint32_t)main_globals_data.screenshot_index, (int32_t)page_row,
                        (int32_t)page_col);
                directory_create_recursive("screenshots");

                memset(&request, 0, sizeof(request));
                request.signature = 0x66696c6f; // 'filo'
                request.location = -1;
                if ((request.flags & 1) != 0) {
                    path_remove_last_component((uint8_t *)&request.path); // see file header UNSURE
                }
                if (filename[0] != 0) {
                    strncpy(request.path, filename, 0xff);   // path is empty: no separator added
                    request.path[0xff] = 0;
                }
                request.flags = request.flags | 1;

                targa_export(bitmap, &request);
            }
        }

        main_globals_data.screenshot_index = main_globals_data.screenshot_index + 1;
        bitmap_data_free(bitmap);
    }
    main_globals_data.screenshot_tile_count = 0;
}

#if 0
Original Ghidra decompilation (0x4ca1a0):

void screenshot_render(undefined4 param_1)

{
  char cVar1;
  undefined4 uVar2;
  short sVar3;
  undefined4 *puVar4;
  SIZE_T dwBytes;
  HGLOBAL pvVar5;
  char *pcVar6;
  int iVar7;
  char *_Dest;
  short sVar8;
  short sVar9;
  undefined4 *puVar10;
  short local_318;
  short local_316;
  short local_314;
  short local_312;
  undefined4 local_310;
  byte local_30c;
  undefined2 local_30a;
  char local_308 [255];
  undefined1 local_209;
  char local_200 [512];

  uVar2 = DAT_00696568;
  sVar8 = (short)DAT_00696568;
  if ((short)DAT_00696568 < 1) {
    DAT_00696568 = CONCAT22(DAT_00696568._2_2_,1);
  }
  else {
    DAT_00696568 = CONCAT22(DAT_00696568._2_2_,3);
    if (sVar8 < 4) {
      DAT_00696568 = uVar2;
    }
  }
  sVar9 = ((short)DAT_0069c638 - (short)DAT_0069c634) * (short)DAT_00696568;
  sVar8 = (DAT_0069c638._2_2_ - DAT_0069c634._2_2_) * (short)DAT_00696568;
  puVar4 = GlobalAlloc(0,0x30);
  if (puVar4 == (undefined4 *)0x0) {
    DAT_00719aac = 0;
    return;
  }
  puVar10 = puVar4;
  for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar10 = 0;
    puVar10 = puVar10 + 1;
  }
  *puVar4 = 0x6269746d;
  *(short *)(puVar4 + 1) = sVar8;
  *(short *)((int)puVar4 + 6) = sVar9;
  *(undefined2 *)(puVar4 + 2) = 1;
  *(undefined2 *)((int)puVar4 + 10) = 0;
  *(undefined2 *)(puVar4 + 3) = 10;
  *(undefined2 *)((int)puVar4 + 0xe) = 0x40;
  *(undefined2 *)(puVar4 + 5) = 0;
  if ((((int)sVar8 & (int)sVar8 - 1U) == 0) && (((int)sVar9 & (int)sVar9 - 1U) == 0)) {
    *(undefined2 *)((int)puVar4 + 0xe) = 0x41;
  }
  dwBytes = bitmap_data_calculate_pixel_data_size();
  pvVar5 = GlobalAlloc(0,dwBytes);
  puVar4[0xb] = pvVar5;
  if (pvVar5 != (HGLOBAL)0x0) {
    console_print_error_va(&DAT_0065512c);
    console_deactivate();
    local_316 = 0;
    sVar8 = DAT_00719aac;
    if (0 < DAT_00719aac) {
      do {
        local_318 = 0;
        if (0 < sVar8) {
          do {
            local_312 = 0;
            sVar8 = (short)DAT_00696568;
            if (0 < (short)DAT_00696568) {
              do {
                sVar9 = local_312;
                local_314 = 0;
                if (0 < sVar8) {
                  do {
                    sVar3 = local_314;
                    if ((DAT_00719aac < 2) && (sVar8 < 2)) {
                      render_views_draw_all(param_1,1,0,0,0);
                    }
                    else {
                      render_views_draw_all(param_1,1,&local_318,0,0);
                    }
                    FUN_00518180(puVar4);
                    local_314 = sVar3 + 1;
                    sVar8 = (short)DAT_00696568;
                  } while (local_314 < (short)DAT_00696568);
                }
                local_312 = sVar9 + 1;
              } while (local_312 < sVar8);
            }
            _sprintf(local_200,"%s\\%dscreenshot%d%d.tga","screenshots",(uint)DAT_00719722,
                     (int)local_316,(int)local_318);
            directory_create_recursive("screenshots");
            puVar10 = &local_310;
            for (iVar7 = 0x43; iVar7 != 0; iVar7 = iVar7 + -1) {
              *puVar10 = 0;
              puVar10 = puVar10 + 1;
            }
            local_310 = 0x66696c6f;
            local_30a = 0xffff;
            if ((local_30c & 1) != 0) {
              path_remove_last_component();
            }
            if (local_200[0] != '\0') {
              pcVar6 = local_308;
              do {
                _Dest = pcVar6;
                pcVar6 = _Dest + 1;
              } while (*_Dest != '\0');
              if (_Dest != local_308) {
                *_Dest = '\\';
                *pcVar6 = '\0';
                _Dest = pcVar6;
              }
              pcVar6 = local_308;
              do {
                cVar1 = *pcVar6;
                pcVar6 = pcVar6 + 1;
              } while (cVar1 != '\0');
              _strncpy(_Dest,local_200,0xff - ((int)pcVar6 - (int)(local_308 + 1)));
              local_209 = 0;
            }
            local_30c = local_30c | 1;
            targa_export();
            local_318 = local_318 + 1;
            sVar8 = DAT_00719aac;
          } while (local_318 < DAT_00719aac);
        }
        local_316 = local_316 + 1;
      } while (local_316 < sVar8);
    }
    DAT_00719722 = DAT_00719722 + 1;
    bitmap_group_free();
  }
  DAT_00719aac = 0;
  return;
}
#endif

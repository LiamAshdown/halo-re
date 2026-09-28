// viewport_split_rect_compute  (Ghidra: viewport_split_rect_compute, already named)
// address 0x4c8da0, size 379 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "viewport_split_rect_compute 0x4c8da0: EAX = view count, EDX = view index, ECX = window rect
// (render_view +0x8c), one stack argument = viewport rect (+0x84) (0x4c9326..0x4c9332)".
// Confirmed against objdump -d -M intel bin/halo.exe at the call site 0x4c9320..0x4c9337:
// `lea ebx,[esi+0x84]; lea ecx,[ebx+8]; push ebx; ... call 0x4c8da0` (ecx = esi+0x8c = window,
// the pushed stack arg = esi+0x84 = viewport, matching render_view's field offsets exactly).
// Two distinct Rectangle2D globals are read: 0x0069c634 (8 bytes: top/left/bottom/right ending
// exactly at 0x0069c63c) and 0x0069c63c (8 bytes: top/left/bottom/right ending at 0x0069c644) --
// this matches main.h's "0x0069c634 / 0x0069c63c game window and screen rectangles (rasterizer)"
// exactly as two adjacent structs, not one. The first is used only to clamp the computed grid
// cell to the true window edge on boundary rows/columns; the second is what the row/column grid
// is actually divided out of. Named game_window_top_left / game_screen_rect accordingly.
// register convention: EAX -> view_count, EDX -> view_index, ECX -> window, stack -> out_viewport.
// phase 4 review (disassembly 0x4c8da0..0x4c8f1a: grid search, rounding and every rect store match; no drift.
// UNSURE: the exact intent of the four "extra_blank_flag" adjustments (added/subtracted from
// `window`, a margin of 4 units per row/column index when view_count >= 2) is not confirmed
// beyond what the disassembly shows; likely a split-screen divider gap. Preserved exactly.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"

extern Rectangle2D game_window_top_left; // 0x0069c634, foreign (rasterizer module)
extern Rectangle2D game_screen_rect; // 0x0069c63c, foreign (rasterizer module)

// blam-cc: EAX -> view_count, EDX -> view_index, ECX -> window, stack -> out_viewport
// Fits `view_count` viewports into as square a grid as possible, then computes the `view_index`th
// cell of that grid, dividing game_screen_rect's width across the grid's columns and its height
// across its rows. Writes the raw grid cell into `window` (with a small margin applied per row/
// column when splitting more than one view), and a copy of the pre-margin cell into
// `out_viewport`, except that a cell touching the outer edge of the grid instead gets clamped to
// game_window_top_left's matching edge on that side.
void viewport_split_rect_compute(int32_t view_count, int32_t view_index, Rectangle2D *window,
                                  Rectangle2D *out_viewport)
{
    int32_t columns;
    int32_t rows;
    int32_t candidate_rows;
    uint8_t center_single_column;
    uint8_t one_fewer_row;
    int16_t extra_blank_flag;
    int32_t row_index;
    int32_t col_index;
    int16_t cell_width;
    int16_t cell_height;

    columns = 1;
    center_single_column = 0;
    one_fewer_row = 0;
    extra_blank_flag = (view_count < 2) ? 0 : 4;
    candidate_rows = 1;
    rows = 1;
    if (view_count > 1) {
        do {
            if (columns < candidate_rows) {
                columns = columns + 1;
            } else {
                columns = 1;
                candidate_rows = candidate_rows + 1;
            }
            rows = candidate_rows;
        } while (candidate_rows * columns < view_count);
    }

    if (rows * columns - view_count != 0 && view_count <= rows * columns) {
        if (view_index == 0) {
            center_single_column = 1;
            one_fewer_row = 1;
        } else {
            view_index = view_index + 1;
        }
    }

    row_index = view_index / columns;
    col_index = view_index - columns * row_index;

    cell_width = (int16_t)((game_screen_rect.right - game_screen_rect.left) / columns) *
                 (center_single_column + 1);
    window->left = cell_width * (int16_t)col_index + game_screen_rect.left;
    window->right = cell_width * ((int16_t)col_index + 1) + game_screen_rect.left;

    cell_height = (int16_t)((game_screen_rect.bottom - game_screen_rect.top) / rows);
    window->top = cell_height * (int16_t)row_index + game_screen_rect.top;
    window->bottom = cell_height * ((int16_t)row_index + 1) + game_screen_rect.top;

    *out_viewport = *window;

    window->left = window->left + (int16_t)col_index * extra_blank_flag;
    window->right = window->right - (int16_t)((col_index == 0) * extra_blank_flag);
    window->top = window->top + (int16_t)row_index * extra_blank_flag;
    window->bottom = window->bottom - (int16_t)((row_index == 0) * extra_blank_flag);

    if (col_index == 0) {
        out_viewport->left = game_window_top_left.left;
    }
    if (one_fewer_row + 1 + col_index == columns) {
        out_viewport->right = game_window_top_left.right;
    }
    if (row_index == 0) {
        out_viewport->top = game_window_top_left.top;
    }
    if (row_index + 1 == rows) {
        out_viewport->bottom = game_window_top_left.bottom;
    }
}

#if 0
Original Ghidra decompilation (0x4c8da0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void viewport_split_rect_compute(undefined4 *param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  byte bVar4;
  byte bVar5;
  undefined4 uVar6;
  ushort uVar7;
  int in_EAX;
  int iVar8;
  int iVar9;
  short *in_ECX;
  int in_EDX;
  int iVar10;
  int iVar11;
  int iVar12;
  int local_c;

  iVar11 = 1;
  bVar4 = 0;
  bVar5 = 0;
  uVar7 = (in_EAX < 2) - 1 & 4;
  iVar8 = 1;
  local_c = 1;
  if (1 < in_EAX) {
    do {
      if (iVar11 < iVar8) {
        iVar11 = iVar11 + 1;
      }
      else {
        iVar11 = 1;
        iVar8 = iVar8 + 1;
      }
      local_c = iVar8;
    } while (iVar8 * iVar11 < in_EAX);
  }
  if (local_c * iVar11 - in_EAX != 0 && in_EAX <= local_c * iVar11) {
    if (in_EDX == 0) {
      bVar4 = 1;
      bVar5 = 1;
    }
    else {
      in_EDX = in_EDX + 1;
    }
  }
  iVar8 = in_EDX / iVar11;
  iVar10 = (int)DAT_0069c63c;
  iVar12 = in_EDX - iVar11 * iVar8;
  iVar9 = (int)DAT_0069c640;
  sVar3 = DAT_0069c63e;
  sVar1 = (short)(((int)DAT_0069c642 - (int)DAT_0069c63e) / iVar11) * (bVar4 + 1);
  sVar2 = (short)iVar12;
  in_ECX[1] = sVar1 * sVar2 + DAT_0069c63e;
  uVar6 = _DAT_0069c63c;
  in_ECX[3] = (sVar2 + 1) * sVar1 + sVar3;
  sVar1 = (short)iVar8;
  sVar3 = (short)((iVar9 - iVar10) / local_c);
  *in_ECX = sVar1 * sVar3 + (short)uVar6;
  in_ECX[2] = (sVar1 + 1) * sVar3 + (short)uVar6;
  *param_1 = *(undefined4 *)in_ECX;
  param_1[1] = *(undefined4 *)(in_ECX + 2);
  in_ECX[1] = in_ECX[1] + sVar2 * uVar7;
  in_ECX[3] = in_ECX[3] - (iVar12 == 0) * uVar7;
  *in_ECX = *in_ECX + sVar1 * uVar7;
  in_ECX[2] = in_ECX[2] - (iVar8 == 0) * uVar7;
  if (iVar12 == 0) {
    *(undefined2 *)((int)param_1 + 2) = DAT_0069c634._2_2_;
  }
  if (bVar5 + 1 + iVar12 == iVar11) {
    *(undefined2 *)((int)param_1 + 6) = DAT_0069c638._2_2_;
  }
  if (iVar8 == 0) {
    *(undefined2 *)param_1 = (undefined2)DAT_0069c634;
  }
  if (iVar8 + 1 == local_c) {
    *(undefined2 *)(param_1 + 1) = (undefined2)DAT_0069c638;
  }
  return;
}
#endif

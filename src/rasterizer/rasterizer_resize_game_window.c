// rasterizer_resize_game_window  (Ghidra: rasterizer_resize_game_window, already named)
// address 0x515b20, size 271 bytes
// name confidence: 0.55  rewrite confidence: 0.65
// evidence: recomputes a centered, style-adjusted window rect (same shape as
//   rasterizer_create_game_window.c) and MoveWindow's the existing window only if it changed,
//   then updates the cached client-area globals game_window_top_left/game_window_bottom_right
//   (0x0069c634/0x0069c638, types/rasterizer.h) plus four further mouse-bound style globals the
//   header does not document.
// register convention: height in in_EAX, width in in_ECX. // blam-cc: EAX -> height, ECX -> width
// UNSURE: the exact (x,y) semantics of game_window_top_left/bottom_right's two int16 halves --
//   preserved as raw low/high half writes rather than asserted field names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"


extern void *rasterizer_hwnd;            // 0x007461c4
extern uint32_t rasterizer_window_style; // 0x0069c6a4
extern uint32_t game_window_top_left;    // 0x0069c634
extern uint32_t game_window_bottom_right; // 0x0069c638
extern int16_t unknown_0069c640; // 0x0069c640 UNSURE: mouse-bound style global
extern int16_t unknown_0069c642; // 0x0069c642 UNSURE
extern int16_t unknown_0069c63e; // 0x0069c63e UNSURE
extern int16_t unknown_0069c63c; // 0x0069c63c UNSURE
extern int32_t unknown_0069c648;                                    // 0x0069c648 UNSURE
extern int32_t unknown_0069c64c;                                    // 0x0069c64c UNSURE

extern void *GetDesktopWindow(void);
extern int32_t GetWindowRect(void *hwnd, win32_rect *rect);
extern int32_t AdjustWindowRect(win32_rect *rect, uint32_t style, int32_t menu);
extern int32_t MoveWindow(void *hwnd, int32_t x, int32_t y, int32_t w, int32_t h, int32_t repaint);
extern int32_t ShowWindow(void *hwnd, int32_t cmd_show);

// blam-cc: EAX -> height, ECX -> width
// Repositions/resizes the game window to a centered `width` by `height` client area if that
// differs from its current adjusted rect, then updates the cached client-area/mouse-bound
// globals.
void rasterizer_resize_game_window(int32_t height, int32_t width)
{
    win32_rect current;
    win32_rect target;

    GetWindowRect(rasterizer_hwnd, &current);
    GetWindowRect(GetDesktopWindow(), &target);

    target.left = (uint32_t)((target.right - target.left) - width) >> 1;
    target.right = target.left + width;
    target.top = (uint32_t)((target.bottom - target.top) - height) >> 1;
    target.bottom = target.top + height;
    AdjustWindowRect(&target, rasterizer_window_style, 0);

    if (current.top != target.top || current.bottom != target.bottom ||
        current.left != target.left || current.right != target.right) {
        MoveWindow(rasterizer_hwnd, target.left, target.top, target.right - target.left,
                   target.bottom - target.top, 1);
        ShowWindow(rasterizer_hwnd, 5); // SW_SHOW
    }

    game_window_bottom_right = (uint16_t)(int16_t)height | ((uint32_t)(uint16_t)(int16_t)width << 16);
    unknown_0069c640 = (int16_t)height - 8;
    unknown_0069c642 = (int16_t)width - 8;
    game_window_top_left = 0;
    unknown_0069c63e = 8;
    unknown_0069c63c = 8;
    unknown_0069c648 = 1;
    unknown_0069c64c = 0;
}

#if 0
Original Ghidra decompilation (0x515b20):

void rasterizer_resize_game_window(void)

{
  HWND hWnd;
  int in_EAX;
  HWND hWnd_00;
  int in_ECX;
  tagRECT *lpRect;
  tagRECT local_20;
  tagRECT local_10;

  hWnd = DAT_007461c4;
  GetWindowRect(DAT_007461c4,&local_10);
  lpRect = &local_20;
  hWnd_00 = GetDesktopWindow();
  GetWindowRect(hWnd_00,lpRect);
  local_20.left = (uint)((local_20.right - local_20.left) - in_ECX) >> 1;
  local_20.right = local_20.left + in_ECX;
  local_20.top = (uint)((local_20.bottom - local_20.top) - in_EAX) >> 1;
  local_20.bottom = local_20.top + in_EAX;
  AdjustWindowRect(&local_20,DAT_0069c6a4,0);
  if ((((local_10.top != local_20.top) || (local_10.bottom != local_20.bottom)) ||
      (local_10.left != local_20.left)) || (local_10.right != local_20.right)) {
    MoveWindow(hWnd,local_20.left,local_20.top,local_20.right - local_20.left,
               local_20.bottom - local_20.top,1);
    ShowWindow(hWnd,5);
  }
  DAT_0069c638._2_2_ = (short)in_ECX;
  DAT_0069c638._0_2_ = (short)in_EAX;
  DAT_0069c640 = (short)in_EAX + -8;
  DAT_0069c642 = (short)in_ECX + -8;
  DAT_0069c634._0_2_ = 0;
  DAT_0069c634._2_2_ = 0;
  DAT_0069c63e = 8;
  DAT_0069c63c = 8;
  DAT_0069c648 = 1;
  DAT_0069c64c = 0;
  return;
}
#endif

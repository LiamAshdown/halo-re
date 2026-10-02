// dialog_center_on_screen  (Ghidra: dialog_center_on_screen, already named)
// address 0x542f00, size 160 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: WM_INITDIALOG (0x110) handler that centers hwnd over GetDesktopWindow's client rect.
// register convention: __stdcall DLGPROC (hwnd, message, wparam, lparam): ret 0x10 at 0x542f13.
//   Review fix: was declared __cdecl with two parameters. The crash reporter 0x542fa0 passes it to
//   CreateDialogIndirectParamA (push 0x542f00 at 0x54323a).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


// WM_INITDIALOG handler snippet that centers the given dialog/window over the desktop.
int32_t __stdcall dialog_center_on_screen(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    win32_rect window_rect;
    win32_rect desktop_rect;

    if (message != 0x110 /* WM_INITDIALOG */) {
        return 0;
    }

    GetWindowRect((HWND)hwnd, &window_rect);
    GetClientRect(GetDesktopWindow(), &desktop_rect);
    MoveWindow((HWND)hwnd,
               (desktop_rect.right - desktop_rect.left) / 2 - (window_rect.right - window_rect.left) / 2,
               (desktop_rect.bottom - desktop_rect.top) / 2 - (window_rect.bottom - window_rect.top) / 2,
               window_rect.right - window_rect.left,
               window_rect.bottom - window_rect.top,
               1);
    return 1;
}

#if 0
Original Ghidra decompilation (0x542f00):


undefined4 dialog_center_on_screen(HWND param_1,int param_2)

{
  HWND hWnd;
  tagRECT *lpRect;
  tagRECT local_20;
  tagRECT local_10;
  
  if (param_2 != 0x110) {
    return 0;
  }
  GetWindowRect(param_1,&local_10);
  lpRect = &local_20;
  hWnd = GetDesktopWindow();
  GetClientRect(hWnd,lpRect);
  MoveWindow(param_1,(local_20.right - local_20.left) / 2 - (local_10.right - local_10.left) / 2,
             (local_20.bottom - local_20.top) / 2 - (local_10.bottom - local_10.top) / 2,
             local_10.right - local_10.left,local_10.bottom - local_10.top,1);
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

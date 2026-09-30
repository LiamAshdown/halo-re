// dialog_static_hyperlink_subclass_proc  (Ghidra: dialog_static_hyperlink_subclass_proc, already
// named)
// address 0x57e350, size 363 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (phase-4 review: matched instruction by instruction to objdump)
// evidence: out/phase2/results/dialogs_00.json "Retrieves the saved original window procedure
//   via GetPropA(\"Old_Proc\") and forwards via CallWindowProcA (standard subclass pattern); on
//   WM_DESTROY (2) it unsubclasses and restores the font/props saved by FUN_0057e4c0
//   (dialog_static_hyperlink_install); on WM_SETCURSOR (0x20) it sets a hand-style cursor; on
//   WM_MOUSEMOVE (0x200) it tracks mouse capture and calls InvalidateRect for hover-state
//   redraw." Property key strings ("Old_Proc", "Old_Font", "Static") and message/cursor
//   constants match types/dialogs.h's dialogs_constants exactly. The RECT/POINT locals are
//   win32_rect/win32_point (types/interface.h); the two locals Ghidra decompiles here
//   (tagPOINT local_18, POINT pt) are combined into one cursor_point, same value in, same value
//   passed to PtInRect, no behaviour change.
// evidence for the global: out/phase4/dialogs_types_notes.md "global 0x00722bc8
//   dialog_hyperlink_hovered (owned). Written only by dialog_static_hyperlink_subclass_proc
//   0x57e350. It is set to 1 when WM_MOUSEMOVE arrives without capture, and then SetCapture
//   runs. It is set to 0 (the PtInRect result) when the cursor leaves the window rect, and then
//   ReleaseCapture runs."
// register convention: out/phase4/dialogs_types_notes.md "0x57e350 and 0x57e2a0 are __stdcall
//   WNDPROCs (ret 0x10)." All four parameters are Ghidra-recognized stack arguments already, no
//   register-passed arguments to remap.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "dialogs.h"
#include "fn_dialogs.h"


extern int32_t dialog_hyperlink_hovered; // 0x00722bc8, this module; written only here, read by
                                         // dialog_static_hyperlink_parent_proc 0x57e2a0 (WM_CTLCOLORSTATIC)

// Window-subclass procedure for a hyperlink-style static control (installed by
// dialog_static_hyperlink_install 0x57e4c0). On WM_DESTROY, unsubclasses the control, restores
// its original font and removes the properties this module set. On WM_SETCURSOR, shows the hand
// cursor (falling back to the arrow if IDC_HAND is unavailable) and eats the message. On
// WM_MOUSEMOVE, tracks mouse capture: takes capture and marks the control hovered when the mouse
// enters without capture, releases it and clears hovered once the cursor leaves the control's
// screen rect, invalidating the control on both transitions so the parent's WM_CTLCOLORSTATIC
// handler repaints it in the new colour. Every message, handled or not, is then forwarded to the
// saved original window procedure and its result returned.
int32_t __stdcall dialog_static_hyperlink_subclass_proc(void *hwnd, uint32_t message, uint32_t wparam,
                                                          int32_t lparam)
{
    void *old_wnd_proc;
    void *saved_value;
    void *capture_window;
    void *cursor;
    win32_rect window_rect;
    win32_point cursor_point;

    old_wnd_proc = GetPropA(hwnd, "Old_Proc");

    if (message == k_dialog_message_destroy) {
        SetWindowLongA(hwnd, k_dialog_window_long_wndproc, (int32_t)old_wnd_proc);
        RemovePropA(hwnd, "Old_Proc");
        saved_value = GetPropA(hwnd, "Old_Font");
        SendMessageA(hwnd, k_dialog_message_set_font, (uint32_t)saved_value, 0);
        RemovePropA(hwnd, "Old_Font");
        saved_value = GetPropA(hwnd, "Font");
        DeleteObject(saved_value);
        RemovePropA(hwnd, "Font");
        RemovePropA(hwnd, "Static");
    } else if (message == k_dialog_message_set_cursor) {
        cursor = LoadCursorA(0, (const char *)k_dialog_cursor_hand);
        if (cursor == 0) {
            cursor = LoadCursorA(0, (const char *)k_dialog_cursor_arrow);
        }
        SetCursor(cursor);
        return 1;
    } else if (message == k_dialog_message_mouse_move) {
        capture_window = GetCapture();
        if (capture_window == hwnd) {
            GetWindowRect(hwnd, &window_rect);
            cursor_point.x = (int32_t)(lparam & 0xffff);
            cursor_point.y = (int32_t)((uint32_t)lparam >> 0x10);
            ClientToScreen(hwnd, &cursor_point);
            if (!PtInRect(&window_rect, cursor_point)) {
                dialog_hyperlink_hovered = 0;
                InvalidateRect(hwnd, (win32_rect *)0, 0);
                ReleaseCapture();
            }
        } else {
            dialog_hyperlink_hovered = 1;
            InvalidateRect(hwnd, (win32_rect *)0, 0);
            SetCapture(hwnd);
        }
    }

    return CallWindowProcA(old_wnd_proc, hwnd, message, wparam, lparam);
}

#if 0
Original Ghidra decompilation (0x57e350):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

LRESULT dialog_static_hyperlink_subclass_proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)

{
  POINT pt;
  WNDPROC lpPrevWndFunc;
  HWND pHVar1;
  BOOL BVar2;
  HCURSOR hCursor;
  HANDLE pvVar3;
  LRESULT LVar4;
  tagPOINT local_18;
  tagRECT local_10;

  lpPrevWndFunc = GetPropA(hwnd,"Old_Proc");
  if (msg == 2) {
    SetWindowLongA(hwnd,-4,(LONG)lpPrevWndFunc);
    RemovePropA(hwnd,"Old_Proc");
    pvVar3 = GetPropA(hwnd,"Old_Font");
    SendMessageA(hwnd,0x30,(WPARAM)pvVar3,0);
    RemovePropA(hwnd,"Old_Font");
    pvVar3 = GetPropA(hwnd,"Font");
    DeleteObject(pvVar3);
    RemovePropA(hwnd,"Font");
    RemovePropA(hwnd,"Static");
  }
  else {
    if (msg == 0x20) {
      hCursor = LoadCursorA((HINSTANCE)0x0,&DAT_00007f89);
      if (hCursor == (HCURSOR)0x0) {
        hCursor = LoadCursorA((HINSTANCE)0x0,&DAT_00007f00);
      }
      SetCursor(hCursor);
      return 1;
    }
    if (msg == 0x200) {
      pHVar1 = GetCapture();
      if (pHVar1 == hwnd) {
        GetWindowRect(hwnd,&local_10);
        local_18.x = lParam & 0xffff;
        local_18.y = (uint)lParam >> 0x10;
        ClientToScreen(hwnd,&local_18);
        pt.y = local_18.y;
        pt.x = local_18.x;
        BVar2 = PtInRect(&local_10,pt);
        if (BVar2 == 0) {
          _DAT_00722bc8 = BVar2;
          InvalidateRect(hwnd,(RECT *)0x0,0);
          ReleaseCapture();
        }
      }
      else {
        _DAT_00722bc8 = 1;
        InvalidateRect(hwnd,(RECT *)0x0,0);
        SetCapture(hwnd);
      }
    }
  }
  LVar4 = CallWindowProcA(lpPrevWndFunc,hwnd,msg,wParam,lParam);
  return LVar4;
}
#endif

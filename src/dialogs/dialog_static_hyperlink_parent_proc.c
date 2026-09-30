// dialog_static_hyperlink_parent_proc  (Ghidra: missed_57e2a0; named directly in types/dialogs.h:
// "dialog_static_hyperlink_parent_proc 0x57e2a0, NOT a Ghidra function")
// address 0x57e2a0, size 176 bytes
// name confidence: 0.85 (already the settled name in types/dialogs.h and this module's other
// source files)   rewrite confidence: 0.9
// evidence: out/phase4/dialogs_types_notes.md "0x57e2a0 dialog_static_hyperlink_parent_proc is
// real code with no Ghidra function. It is the parent-dialog subclass proc installed by 0x57e4c0
// (SetWindowLongA(parent, GWL_WNDPROC, 0x57e2a0), guarded by cmp eax,0x57e2a0). It handles:
// WM_DESTROY: restores Old_Proc and removes the property. WM_CTLCOLORSTATIC: when lParam carries
// the Static property, it forwards to the old proc for the brush, then calls
// SetTextColor(wParam, hovered ? 0xe0 : 0xc00000). Everything else goes to CallWindowProcA."
// types/dialogs.h's dialogs_constants (k_dialog_message_destroy, k_dialog_message_ctl_color_static,
// k_dialog_window_long_wndproc, k_dialog_hyperlink_color_hover/_normal) and the property-name
// strings ("Old_Proc", "Static") match exactly; global 0x00722bc8 dialog_hyperlink_hovered is
// owned by dialog_static_hyperlink_subclass_proc 0x57e350 and read here on WM_CTLCOLORSTATIC.
// register convention: out/phase4/dialogs_types_notes.md "0x57e350 and 0x57e2a0 are __stdcall
// WNDPROCs (ret 0x10)." All four parameters are ordinary stack arguments; objdump confirms the
// prologue reads [esp+0x10] (hwnd), [esp+0x18] (message), [esp+0x1c]/[esp+0x20] (wParam/lParam,
// offsets after the four register pushes) with no register-passed values.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "dialogs.h"
#include "fn_dialogs.h"


extern int32_t dialog_hyperlink_hovered; // 0x00722bc8, this module; owned by
                                         // dialog_static_hyperlink_subclass_proc 0x57e350

// Window-subclass procedure for a hyperlink-style static control's PARENT dialog (installed by
// dialog_static_hyperlink_install 0x57e4c0). On WM_DESTROY, unsubclasses the parent and removes
// its "Old_Proc" property. On WM_CTLCOLORSTATIC, when the control (lParam) carries the "Static"
// marker property, forwards the message to the saved original window procedure first (to get its
// brush), then colours the text via SetTextColor on the HDC (wParam): the hover colour while
// dialog_hyperlink_hovered is set, the normal colour otherwise, returning the forwarded brush
// result. Every other message, and WM_CTLCOLORSTATIC for a control without the marker, is simply
// forwarded to the saved original window procedure and its result returned.
int32_t __stdcall dialog_static_hyperlink_parent_proc(void *hwnd, uint32_t message, uint32_t wparam,
                                                        int32_t lparam)
{
    void *old_wnd_proc;
    void *static_marker;
    int32_t forwarded_result;

    old_wnd_proc = GetPropA(hwnd, "Old_Proc");

    if (message == k_dialog_message_destroy) {
        SetWindowLongA(hwnd, k_dialog_window_long_wndproc, (int32_t)old_wnd_proc);
        RemovePropA(hwnd, "Old_Proc");
    } else if (message == k_dialog_message_ctl_color_static) {
        static_marker = GetPropA((void *)lparam, "Static");
        if (static_marker != (void *)0) {
            forwarded_result = CallWindowProcA(old_wnd_proc, hwnd, message, wparam, lparam);
            if (dialog_hyperlink_hovered != 0) {
                SetTextColor((void *)wparam, k_dialog_hyperlink_color_hover);
            } else {
                SetTextColor((void *)wparam, k_dialog_hyperlink_color_normal);
            }
            return forwarded_result;
        }
    }

    return CallWindowProcA(old_wnd_proc, hwnd, message, wparam, lparam);
}

#if 0
Original Ghidra decompilation (0x57e2a0):

LRESULT missed_57e2a0(HWND param_1,UINT param_2,HDC param_3,HWND param_4)

{
  WNDPROC lpPrevWndFunc;
  HANDLE pvVar1;
  LRESULT LVar2;

  lpPrevWndFunc = GetPropA(param_1,"Old_Proc");
  if (param_2 == 2) {
    SetWindowLongA(param_1,-4,(LONG)lpPrevWndFunc);
    RemovePropA(param_1,"Old_Proc");
  }
  else if (param_2 == 0x138) {
    pvVar1 = GetPropA(param_4,"Static");
    if (pvVar1 != (HANDLE)0x0) {
      LVar2 = CallWindowProcA(lpPrevWndFunc,param_1,0x138,(WPARAM)param_3,(LPARAM)param_4);
      if (dialog_hyperlink_hovered != 0) {
        SetTextColor(param_3,0xe0);
        return LVar2;
      }
      SetTextColor(param_3,0xc00000);
      return LVar2;
    }
  }
  LVar2 = CallWindowProcA(lpPrevWndFunc,param_1,param_2,(WPARAM)param_3,(LPARAM)param_4);
  return LVar2;
}
#endif

// dialog_static_hyperlink_install  (Ghidra: dialog_static_hyperlink_install, already named)
// address 0x57e4c0, size 209 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: out/phase4/dialogs_functions.md / dialogs_types_notes.md: the only function that
//   sets the "Old_Proc"/"Old_Font"/"Font"/"Static" window properties dialog_static_hyperlink_
//   subclass_proc 0x57e350 and the parent proc 0x57e2a0 consume; subclasses the control itself
//   (installing 0x57e350 as its WNDPROC) and, once per parent dialog, subclasses the parent
//   (installing 0x57e2a0, guarded by a `!= &DAT_0057e2a0` check so it is not stacked twice);
//   ORs SS_NOTIFY into the control's style so it receives WM_SETCURSOR/mouse messages; and
//   swaps in a copy of the control's current font with lfUnderline set, matching the
//   win32_logfonta layout confirmed by GetObjectA's size argument (0x3c) and offset 0x15.
// register convention: control HWND in ESI (unaff_ESI), no stack arguments, always returns 1.
//   The `push esi` at 0x57e4c6 is the GetParent call argument, not a saved register -- the
//   epilogue pops only edi/ebp/ebx. Confirmed by objdump; the only call site is 0x57e6a9 in the
//   shell fatal error dialog proc, with ESI = GetDlgItem(dlg, 0x3ef).
// module note: out/phase4/dialogs_types_notes.md judges this function (and 0x57e350) most
//   likely belongs to the shell translation unit rather than a standalone "dialogs" module.
//   Not skipped here: that is a module-placement note, not a library-code misattribution. See
//   dialog_static_hyperlink_subclass_proc.c for the same note.

#include "tags.h"
#include "dialogs.h"

extern void *__stdcall GetParent(void *hwnd);                                           // import 0x63a32c
extern int32_t __stdcall GetWindowLongA(void *hwnd, int32_t index);                     // import 0x63a348
extern int32_t __stdcall SetWindowLongA(void *hwnd, int32_t index, int32_t value);      // import 0x63a3f4
extern int32_t __stdcall SetPropA(void *hwnd, const char *name, void *data);            // import 0x63a340
extern int32_t __stdcall SendMessageA(void *hwnd, uint32_t message, uint32_t wparam,
                                       int32_t lparam);                                  // import 0x63a334
extern int32_t __stdcall GetObjectA(void *object, int32_t buffer_size, void *buffer);   // import 0x63a074
extern void *__stdcall CreateFontIndirectA(const win32_logfonta *logfont);              // import 0x63a06c

// The two subclass procs are referenced only as values (push 0x57e350 at 0x57e526,
// push 0x57e2a0 at 0x57e4f8, cmp eax,0x57e2a0 at 0x57e4e8), declared here so their addresses
// can be passed to SetWindowLongA without raw immediates.
extern int32_t __stdcall dialog_static_hyperlink_subclass_proc(void *window, uint32_t message, uint32_t wparam,
                                                                 int32_t lparam); // 0x57e350, this module
// Not a Ghidra function (out/phase4/dialogs_types_notes.md), so not rewritten yet. Its address is
// the immediates above; its __stdcall WNDPROC shape is confirmed by objdump at 0x57e2a0 (hwnd read
// at [esp+0x10] after four pushes, every exit is ret 0x10).
extern int32_t __stdcall dialog_static_hyperlink_parent_proc(void *window, uint32_t message, uint32_t wparam,
                                                               int32_t lparam); // 0x57e2a0, not a Ghidra function

// Converts a static text control (`control`) into a clickable, underlined hyperlink-style
// control: subclasses the control's parent dialog with dialog_static_hyperlink_parent_proc,
// unless it is already subclassed; ORs SS_NOTIFY into the control's window style so it starts
// receiving WM_SETCURSOR/WM_MOUSEMOVE; subclasses the control itself with dialog_static_
// hyperlink_subclass_proc, saving the previous WNDPROC under "Old_Proc"; fetches the control's
// current font (WM_GETFONT), saves it under "Old_Font", copies its LOGFONTA, sets lfUnderline,
// creates the underlined font and installs it (WM_SETFONT), saving the new font handle under
// "Font"; and marks the control with the "Static" property so the parent proc knows to colour
// it. Always returns 1.
int32_t dialog_static_hyperlink_install(void *control)
{
    void *parent;
    void *previous_wnd_proc;
    int32_t style;
    void *previous_font;
    void *underlined_font;
    win32_logfonta logfont;

    parent = GetParent(control);
    if (parent != 0) {
        previous_wnd_proc = (void *)GetWindowLongA(parent, k_dialog_window_long_wndproc);
        if (previous_wnd_proc != (void *)dialog_static_hyperlink_parent_proc) {
            SetPropA(parent, "Old_Proc", previous_wnd_proc);
            SetWindowLongA(parent, k_dialog_window_long_wndproc, (int32_t)dialog_static_hyperlink_parent_proc);
        }
    }

    style = GetWindowLongA(control, k_dialog_window_long_style);
    SetWindowLongA(control, k_dialog_window_long_style, style | k_dialog_static_style_notify);

    previous_wnd_proc = (void *)GetWindowLongA(control, k_dialog_window_long_wndproc);
    SetPropA(control, "Old_Proc", previous_wnd_proc);
    SetWindowLongA(control, k_dialog_window_long_wndproc, (int32_t)dialog_static_hyperlink_subclass_proc);

    previous_font = (void *)SendMessageA(control, k_dialog_message_get_font, 0, 0);
    SetPropA(control, "Old_Font", previous_font);
    GetObjectA(previous_font, k_dialog_logfont_size, &logfont);
    logfont.underline = 1;
    underlined_font = CreateFontIndirectA(&logfont);
    SetPropA(control, "Font", underlined_font);
    SendMessageA(control, k_dialog_message_set_font, (uint32_t)underlined_font, 0);
    SetPropA(control, "Static", (void *)1);

    return 1;
}

#if 0
Original Ghidra decompilation (0x57e4c0):

undefined4 dialog_static_hyperlink_install(void)

{
  HWND hWnd;
  undefined *hData;
  uint uVar1;
  HANDLE pvVar2;
  HFONT hData_00;
  HWND unaff_ESI;
  LOGFONTA local_3c;

  hWnd = GetParent(unaff_ESI);
  if (hWnd != (HWND)0x0) {
    hData = (undefined *)GetWindowLongA(hWnd,-4);
    if (hData != &DAT_0057e2a0) {
      SetPropA(hWnd,"Old_Proc",hData);
      SetWindowLongA(hWnd,-4,0x57e2a0);
    }
  }
  uVar1 = GetWindowLongA(unaff_ESI,-0x10);
  SetWindowLongA(unaff_ESI,-0x10,uVar1 | 0x100);
  pvVar2 = (HANDLE)GetWindowLongA(unaff_ESI,-4);
  SetPropA(unaff_ESI,"Old_Proc",pvVar2);
  SetWindowLongA(unaff_ESI,-4,0x57e350);
  pvVar2 = (HANDLE)SendMessageA(unaff_ESI,0x31,0,0);
  SetPropA(unaff_ESI,"Old_Font",pvVar2);
  GetObjectA(pvVar2,0x3c,&local_3c);
  local_3c.lfUnderline = '\x01';
  hData_00 = CreateFontIndirectA(&local_3c);
  SetPropA(unaff_ESI,"Font",hData_00);
  SendMessageA(unaff_ESI,0x30,(WPARAM)hData_00,0);
  SetPropA(unaff_ESI,"Static",(HANDLE)0x1);
  return 1;
}
#endif

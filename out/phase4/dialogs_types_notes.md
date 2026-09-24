# dialogs module: type notes (types/dialogs.h)

Checked with `gcc -fsyntax-only -Wall -I types out/phase4/dialogs_smoke.c` (host and -m32): clean.
The smoke file includes tags.h, memory.h, dialogs.h and asserts the LOGFONTA size and offsets.

## Summary

No engine struct is addressed by any of the three functions. Everything they touch is Win32:
window properties keyed by four .rdata strings, one module global and a LOGFONTA local. The
header therefore holds one struct (`win32_logfonta`), one enum of constants, one function
pointer typedef (`dialog_window_proc_fn`), the prototypes as comments, and one owned global.

## win32_logfonta (0x3c)

| field | offset | evidence |
|---|---|---|
| (whole struct) | size 0x3c | `GetObjectA(font, 0x3c, &local)` at 0x57e54f in dialog_static_hyperlink_install 0x57e4c0 |
| underline | 0x15 | `mov byte ptr [esp+0x25],1` at 0x57e55d. The local is at esp+0xc after 3 saved registers, esp+0x10 after the `push ecx` argument, so the byte is local + 0x15 |
| all other fields | 0x00..0x3b | Win32 SDK LOGFONTA layout. The binary pins only the size and +0x15, and these agree with the SDK |

No unresolved offsets.

Not redefined: the RECT and POINT locals of 0x57e350 (GetWindowRect / ClientToScreen /
PtInRect) are `win32_rect` / `win32_point` in types/interface.h.

## Globals

- **0x00722bc8 `int32_t dialog_hyperlink_hovered`** (owned). Written only by
  dialog_static_hyperlink_subclass_proc 0x57e350. It is set to 1 when WM_MOUSEMOVE arrives
  without capture, and then SetCapture runs. It is set to 0 (the PtInRect result) when the
  cursor leaves the window rect, and then ReleaseCapture runs. The parent subclass proc
  0x57e2a0 reads it on WM_CTLCOLORSTATIC to choose the text colour: 0xe0 when hovered,
  0xc00000 when not. There is one flag for all hyperlink controls.
- 0x0069ff20 shell_language_id (shell, read only). The low word is the language passed to
  FindResourceExA (`xor eax,eax; mov ax,[0x69ff20]`). The 0x409 compare reads the full dword.
- 0x00722bb8 strings_module (shell). The caller passes it in ESI as the module.
- **Correction to out/phase4/shell_types_notes.md**: 0x00722bc0 and 0x00722c58 are NOT used
  by the three dialogs functions. Both are used by the shell fatal error dialog proc 0x57e5a0
  (not a Ghidra function):
  - 0x00722bc0 is the IsDlgButtonChecked(0x3e9) state. It is stored before EndDialog at
    0x57e773 / 0x57e7a3, and shell_display_fatal_error_dialog reads it at 0x57ee7c.
  - 0x00722c58 is the sprintf "%dMHz, %dMB, ..." system specs text set into control 0x3f1.

  They belong in shell.h.
- 0x00672a14 is not a global. Ghidra shows it as DAT_00672a14, but it is the string "Font".
  The other property keys are "Old_Font" 0x00672a1c, "Static" 0x00672a28 and "Old_Proc"
  0x00672a30.

## Register conventions (objdump)

- dialog_box_show_localized 0x57e1f0 passes the module (HINSTANCE) in ESI and the DLGPROC in
  EBX. The stack holds (template name or MAKEINTRESOURCE id, parent HWND), and the caller
  pops them. It returns the INT_PTR DialogBox result. The only call site is 0x57ee2a in
  shell_display_fatal_error_dialog, with ESI = [0x722bb8], EBX = 0x57e5a0, id 0x66,
  parent = the window. The caller compares the result with 2 (IDCANCEL). Ghidra's
  `void (LPCSTR, HWND)` signature drops the return and both register arguments. The existing
  src/shell/shell_display_fatal_error_dialog.c extern declaration is consistent with this.
- dialog_static_hyperlink_install 0x57e4c0 takes the control HWND in ESI and has no stack
  arguments. It returns 1. The `push esi` at 0x57e4c6 is the GetParent argument, not a
  saved register (the epilogue pops only edi/ebp/ebx). The only call site is 0x57e6a9 in the
  fatal error dialog proc, with ESI = GetDlgItem(dlg, 0x3ef).
- 0x57e350 and 0x57e2a0 are __stdcall WNDPROCs (ret 0x10).

## Misattributed / missing functions

- All three functions most likely belong to the **shell** translation unit, not to a separate
  "dialogs" module:
  - They sit between shell_load_localized_string 0x57e1a0 and the shell fatal error dialog
    proc 0x57e5a0 / 0x57e850.
  - Their only callers are shell code.
  - modules.json gives them only 0.4 confidence.

  The types are defined in dialogs.h anyway, because shell.h does not define them. If the
  module is folded into shell, move the header contents to shell.h rather than duplicating
  them.
- **0x57e2a0 dialog_static_hyperlink_parent_proc** is real code with no Ghidra function. It
  is the parent-dialog subclass proc installed by 0x57e4c0 (SetWindowLongA(parent, GWL_WNDPROC,
  0x57e2a0), guarded by `cmp eax,0x57e2a0`). It handles:
  - WM_DESTROY: restores Old_Proc and removes the property.
  - WM_CTLCOLORSTATIC: when lParam carries the "Static" property, it forwards to the old proc
    for the brush, then calls SetTextColor(wParam, hovered ? 0xe0 : 0xc00000).
  - Everything else goes to CallWindowProcA.

  A Ghidra function should be created there before it is rewritten.
- 0x57e5a0 (fatal error dialog proc, shell) is also missing as a Ghidra function. Its types
  are not defined here.
- No library code in the range.

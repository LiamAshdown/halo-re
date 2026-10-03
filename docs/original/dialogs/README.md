# `dialogs` — localized dialog boxes and the hyperlink static control

Retail Halo PC `halo.exe` 1.0.10, `0x57e1f0 .. 0x57e590` (3 Ghidra functions, 737 bytes, plus
one 174-byte code entry at `0x57e2a0` that Ghidra has no function for), plain C / MSVC 7.1 / x86.
Each file here holds one function. It is rewritten from the Ghidra decompile against
`types/dialogs.h`, and the original decompile is kept verbatim at the bottom of the file inside
`#if 0 ... #endif`.

Gate: `python tools/build_check.py dialogs` → **3 ok, 0 failed**.

## What the module contains

Two small Win32 helpers. They touch no engine structs: only window properties, one global and
a `LOGFONTA` local.

| Family | Range | What it is |
|---|---|---|
| localized dialog | `0x57e1f0`–`0x57e294` | `DialogBox` from an `RT_DIALOG` resource. It tries the current UI language (`shell_language_id`), then en-US `0x409`, then a plain `DialogBoxParamA` |
| hyperlink static | `0x57e2a0`–`0x57e590` | Turns a static control into a clickable, underlined link. The control and its parent dialog are subclassed, and the parent proc colours the text red while hovered and blue otherwise |

The hyperlink state is kept in window properties keyed by strings in `.rdata`:

| Key (address) | Window | Value |
|---|---|---|
| `"Old_Proc"` `0x672a30` | control and parent | the WNDPROC that was replaced |
| `"Old_Font"` `0x672a1c` | control | the original `HFONT` (`WM_GETFONT`), restored on `WM_DESTROY` |
| `"Font"` `0x672a14` | control | the underlined `HFONT`, passed to `DeleteObject` on `WM_DESTROY` (Ghidra shows it as `DAT_00672a14`) |
| `"Static"` `0x672a28` | control | marker `1`. The parent proc colours only children that carry it |

**Ownership.** These functions almost certainly belong to the `shell` translation unit. They sit
between `shell_load_localized_string 0x57e1a0` and the fatal error dialog proc `0x57e5a0`, and
their only callers are shell code (`0x57ee2a` in `shell_display_fatal_error_dialog`, `0x57e6a9`
in `0x57e5a0`). They are kept as a module only because `modules.json` says so. If they are folded
into shell, move `types/dialogs.h` into `shell.h` rather than duplicating it.

## Struct layouts

### `win32_logfonta` — size `0x3c` (`types/dialogs.h`)

The binary fixes only the size (`GetObjectA(font, 0x3c, &lf)` at `0x57e54f`) and `+0x15`
(`mov byte ptr [esp+0x25],1` at `0x57e55d`, where the local is at `esp+0x10`). The other offsets
follow the Win32 SDK layout, which agrees with both.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `height` |
| `0x04` | `int32_t` | `width` |
| `0x08` | `int32_t` | `escapement` |
| `0x0c` | `int32_t` | `orientation` |
| `0x10` | `int32_t` | `weight` |
| `0x14` | `uint8_t` | `italic` |
| `0x15` | `uint8_t` | `underline`, the only field written (set to 1) |
| `0x16` | `uint8_t` | `strike_out` |
| `0x17` | `uint8_t` | `char_set` |
| `0x18` | `uint8_t` | `out_precision` |
| `0x19` | `uint8_t` | `clip_precision` |
| `0x1a` | `uint8_t` | `quality` |
| `0x1b` | `uint8_t` | `pitch_and_family` |
| `0x1c` | `char[0x20]` | `face_name` |

`win32_rect` (0x10) and `win32_point` (0x08) are reused from `types/interface.h` and are not
redefined here.

### Globals

| Address | Type | Name | Owner | Use |
|---|---|---|---|---|
| `0x722bc8` | `int32_t` | `dialog_hyperlink_hovered` | dialogs | Written 1/0 by `0x57e350` around `SetCapture`/`ReleaseCapture`. Read by `0x57e2a0` on `WM_CTLCOLORSTATIC`. One flag is shared by every hyperlink control |
| `0x69ff20` | `uint32_t` | `shell_language_id` | shell | The low word is the `FindResourceExA` language. The `0x409` check compares the full dword |
| `0x722bb8` | `void *` | `strings_module` | shell | The caller passes it in ESI as the module |

## Register conventions (LTCG, confirmed in objdump)

| Function | Registers | Stack | Return |
|---|---|---|---|
| `dialog_box_show_localized` | ESI = module, EBX = DLGPROC | template name or id, parent HWND (caller pops) | EAX = DialogBox result |
| `dialog_static_hyperlink_install` | ESI = control HWND | none | EAX = 1 |
| `dialog_static_hyperlink_subclass_proc` | none | `__stdcall` WNDPROC, `ret 0x10` | LRESULT |
| `dialog_static_hyperlink_parent_proc` | none | `__stdcall` WNDPROC, `ret 0x10` | LRESULT |

## Functions

| Address | Name | Size | Name conf. | Rewrite conf. | File |
|---|---|---|---|---|---|
| `0x57e1f0` | `dialog_box_show_localized` | 165 | 0.7 | 0.9 | `dialog_box_show_localized.c` |
| `0x57e2a0` | `dialog_static_hyperlink_parent_proc` | 174 | 0.8 | see cleanup pass 1 | `dialog_static_hyperlink_parent_proc.c` |
| `0x57e350` | `dialog_static_hyperlink_subclass_proc` | 363 | 0.6 | 0.9 | `dialog_static_hyperlink_subclass_proc.c` |
| `0x57e4c0` | `dialog_static_hyperlink_install` | 209 | 0.65 | 0.9 | `dialog_static_hyperlink_install.c` |

The review checked all three rewrites instruction by instruction against the objdump of
`0x57e1f0..0x57e590` and found no semantic drift. It covered argument order, the zero-extended
low word of the language id against the full-dword `0x409` compare, the `movzx`/`shr` extraction
of the unsigned `lParam` point, `lfUnderline` at `+0x15`, and the early `return 1` on
`WM_SETCURSOR`.

## Known gaps

- **`0x57e2a0`** is now written (`dialog_static_hyperlink_parent_proc.c`, cleanup pass 1). Its
  behaviour, from objdump:
  - It reads `"Old_Proc"`.
  - On `WM_DESTROY` it restores the old proc with `SetWindowLongA(GWL_WNDPROC)` and removes the
    property.
  - On `WM_CTLCOLORSTATIC`, if `lParam` (the child HWND) carries `"Static"`, it calls
    `CallWindowProcA(old, hwnd, 0x138, wParam, lParam)` for the brush. It then calls
    `SetTextColor(wParam, dialog_hyperlink_hovered ? 0xe0 : 0xc00000)` and returns the brush.
  - Every other case, including `WM_DESTROY` after the restore, falls through to
    `CallWindowProcA(old, ...)`.
- The fatal error dialog proc `0x57e5a0` is written here as `fatal_error_dialog_proc.c` (cleanup pass 1).
- **Extern mismatch with shell.** `src/shell/shell_display_fatal_error_dialog.c` declares
  `dialog_box_show_localized(void *dialog_proc, void *module, uint32_t dialog_id, void *parent)`.
  This module defines it as `(dialog_window_proc_fn, void *, const char *, void *)`. The two are
  ABI-identical: the id is `MAKEINTRESOURCE(0x66)`. The shell side was left alone because it is
  outside this module. Align it when the modules are merged.
- `k_dialog_language_english` (0x409) duplicates shell's `k_shell_language_default`.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x57e2a0` | `dialog_static_hyperlink_parent_proc` | 176 | 0.85 | 0.9 | 0 |  |
| `0x57e5a0` | `fatal_error_dialog_proc` | 687 | 0.8 | 0.7 | 3 | review: LOWORD(wParam) zero-extended; hyperlink install noted as ESI |

Gate: `python tools/build_check.py dialogs` clean after the cleanup review.

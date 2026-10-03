#pragma once
// Blam "dialogs" module (halo.exe 1.0.10 retail, 0x57e1f0..0x57e590, 3 Ghidra functions plus
// one code entry Ghidra has no function for). Two small Win32 helpers:
//   - dialog_box_show_localized 0x57e1f0: DialogBox by resource name or id, trying the
//     RT_DIALOG resource in the current UI language (shell_language_id 0x0069ff20), then in
//     en-US (0x409), then a plain DialogBoxParamA lookup;
//   - the static hyperlink control: dialog_static_hyperlink_install 0x57e4c0 subclasses a
//     static control (dialog_static_hyperlink_subclass_proc 0x57e350) and its parent dialog
//     (dialog_static_hyperlink_parent_proc 0x57e2a0, NOT a Ghidra function: only the
//     immediates push 0x57e2a0 / cmp eax,0x57e2a0 in the installer reference it), swaps in an
//     underlined copy of its font and keeps the saved state in window properties.
//
// Ownership: these functions almost certainly belong to the shell translation unit. They sit
// between shell functions (shell_load_localized_string 0x57e1a0 before, the fatal error dialog
// proc 0x57e5a0 after), and their only callers are shell code: shell_display_fatal_error_dialog
// 0x57ea70 (call at 0x57ee2a) and the fatal error dialog proc 0x57e5a0 (WM_INITDIALOG arm, call
// at 0x57e6a9). types/shell.h does not define the types below, so they are defined here and
// shell.h must not repeat them. See out/phase4/dialogs_types_notes.md.
//
// No engine struct is addressed by these functions. Everything they touch is Win32 state:
// window properties, one module global (the hover flag) and a LOGFONTA local. Where the binary
// carries the layout it is used:
//   - win32_logfonta (0x3c) is the GetObjectA(font, 0x3c, &local) size in 0x57e4c0, and the
//     only field written, lfUnderline, is the byte store at local + 0x15
//     (mov byte ptr [esp+0x25],1 with the local at esp+0x10 after the push of the argument).
//     The other field offsets are the Win32 SDK LOGFONTA layout; the 0x3c total pins them.
//   - RECT / POINT locals of 0x57e350 are win32_rect / win32_point from types/interface.h
//     and are not redefined; nothing below refers to them, so interface.h is not needed.
//
// Register conventions (LTCG, confirmed in objdump at the callee prologues and call sites):
//   dialog_box_show_localized 0x57e1f0: module (HINSTANCE) in ESI, dialog proc (DLGPROC) in EBX,
//     stack (template name or MAKEINTRESOURCE id, parent HWND), caller pops (add esp,8 at
//     0x57ee2f). Returns the DialogBox INT_PTR result in EAX. The caller loads ESI from
//     strings_module 0x00722bb8 and EBX = 0x57e5a0.
//   dialog_static_hyperlink_install 0x57e4c0: control HWND in ESI, no stack arguments (the
//     push esi at 0x57e4c6 is the GetParent argument, not a saved register), returns 1.
//   dialog_static_hyperlink_subclass_proc 0x57e350 and dialog_static_hyperlink_parent_proc
//     0x57e2a0: plain __stdcall WNDPROCs (ret 0x10).
//
// Pointer convention: as in shell.h, handles and pointers are void * in prototypes; the one
// struct here holds no pointers.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants (immediates in the three functions and the parent proc)
// ---------------------------------------------------------------------------
typedef enum dialogs_constants {
    k_dialog_resource_type_dialog = 5,              // RT_DIALOG, FindResourceExA type in 0x57e1f0
    k_dialog_language_english = 0x409,              // en-US fallback; compared against the full
                                                    // dword of shell_language_id, but passed to
                                                    // FindResourceExA as its low 16 bits
    k_dialog_box_failed = -1,                       // DialogBoxIndirectParamA failure result

    k_dialog_window_long_wndproc = -4,              // GWL_WNDPROC
    k_dialog_window_long_style = -16,               // GWL_STYLE
    k_dialog_static_style_notify = 0x100,           // SS_NOTIFY, or-ed into the control style

    k_dialog_message_destroy = 0x2,                 // WM_DESTROY: unsubclass, restore, remove props
    k_dialog_message_set_cursor = 0x20,             // WM_SETCURSOR: hand cursor, return 1
    k_dialog_message_set_font = 0x30,               // WM_SETFONT
    k_dialog_message_get_font = 0x31,               // WM_GETFONT
    k_dialog_message_ctl_color_static = 0x138,      // WM_CTLCOLORSTATIC, handled by the parent proc
    k_dialog_message_mouse_move = 0x200,            // WM_MOUSEMOVE: hover tracking with capture

    k_dialog_cursor_hand = 0x7f89,                  // IDC_HAND (LoadCursorA(NULL, ...))
    k_dialog_cursor_arrow = 0x7f00,                 // IDC_ARROW, used when IDC_HAND is missing

    k_dialog_hyperlink_color_hover = 0x0000e0,      // COLORREF RGB(0xe0,0,0), SetTextColor at 0x57e2f6
    k_dialog_hyperlink_color_normal = 0xc00000,     // COLORREF RGB(0,0,0xc0), SetTextColor at 0x57e30b

    k_dialog_logfont_size = 0x3c,                   // GetObjectA size in 0x57e4c0
    k_dialog_logfont_face_name_length = 0x20        // LF_FACESIZE
} dialogs_constants;

// Window property names (string literals in .rdata, SetPropA / GetPropA / RemovePropA keys):
// string 0x00672a30: Old_Proc   saved WNDPROC, set on both the control and its parent
// string 0x00672a1c: Old_Font   control font before install (WM_GETFONT result), restored
//                               with WM_SETFONT on WM_DESTROY
// string 0x00672a14: Font       the underlined HFONT made by CreateFontIndirectA; DeleteObject
//                               on WM_DESTROY (Ghidra shows it as DAT_00672a14, it is a string)
// string 0x00672a28: Static     marker value 1; the parent proc colours only children that
//                               carry it

// ---------------------------------------------------------------------------
// LOGFONTA  (GetObjectA / CreateFontIndirectA local in dialog_static_hyperlink_install)
// ---------------------------------------------------------------------------
typedef struct win32_logfonta {
    int32_t height;                                 // 0x00 lfHeight
    int32_t width;                                  // 0x04 lfWidth
    int32_t escapement;                             // 0x08 lfEscapement
    int32_t orientation;                            // 0x0c lfOrientation
    int32_t weight;                                 // 0x10 lfWeight
    uint8_t italic;                                 // 0x14 lfItalic
    uint8_t underline;                              // 0x15 lfUnderline, set to 1 at 0x57e55d
    uint8_t strike_out;                             // 0x16 lfStrikeOut
    uint8_t char_set;                               // 0x17 lfCharSet
    uint8_t out_precision;                          // 0x18 lfOutPrecision
    uint8_t clip_precision;                         // 0x19 lfClipPrecision
    uint8_t quality;                                // 0x1a lfQuality
    uint8_t pitch_and_family;                       // 0x1b lfPitchAndFamily
    char face_name[0x20];                           // 0x1c lfFaceName
} win32_logfonta;                                   // size 0x3c

// ---------------------------------------------------------------------------
// function types
// ---------------------------------------------------------------------------
// WNDPROC / DLGPROC shape of 0x57e350, 0x57e2a0 and the DLGPROC passed in EBX to 0x57e1f0.
typedef int32_t (__stdcall *dialog_window_proc_fn)(void *window, uint32_t message, uint32_t wparam,
                                                   int32_t lparam);

// Prototypes (comment only, as in the other headers; the src/ files declare them extern):
// int32_t dialog_box_show_localized(dialog_window_proc_fn dialog_proc, void *module,
//     const char *template_name, void *parent_window);          0x57e1f0, blam-cc: dialog_proc
//     in EBX, module in ESI; template_name is a resource name or MAKEINTRESOURCE id (0x66, the
//     fatal error dialog, from the only caller)
// int32_t dialog_static_hyperlink_install(void *control);       0x57e4c0, blam-cc: control in
//     ESI; always returns 1
// int32_t __stdcall dialog_static_hyperlink_subclass_proc(void *window, uint32_t message,
//     uint32_t wparam, int32_t lparam);                         0x57e350, control subclass proc
// int32_t __stdcall dialog_static_hyperlink_parent_proc(void *window, uint32_t message,
//     uint32_t wparam, int32_t lparam);                         0x57e2a0, parent subclass proc
//     (no Ghidra function)

// ===========================================================================
// globals owned by this module
// ===========================================================================
// global 0x00722bc8: int32_t dialog_hyperlink_hovered            1 while the mouse is over a
//                    hyperlink control (set when 0x57e350 takes capture on WM_MOUSEMOVE, 0 when
//                    PtInRect fails and capture is released); read by the parent proc on
//                    WM_CTLCOLORSTATIC to pick hover / normal text colour. One flag shared by
//                    every hyperlink control.
//
// ---------------------------------------------------------------------------
// globals these functions read but do not own
// ---------------------------------------------------------------------------
// 0x0069ff20  uint32_t shell_language_id (shell)   low word is the FindResourceExA language
// 0x00722bb8  void *strings_module (shell)         the HINSTANCE the caller passes in ESI
// Not used by these functions (the shell notes attributed them here by mistake): 0x00722bc0
// (fatal error dialog checkbox 0x3e9 state, written by dialog proc 0x57e5a0 before EndDialog)
// and 0x00722c58 (the system specs text of control 0x3f1, sprintf in 0x57e5a0). Both belong to
// the shell fatal error dialog proc 0x57e5a0.

#pragma pack(pop)

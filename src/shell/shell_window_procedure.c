// shell_window_procedure  (Ghidra: no function created; the phase-4 types agent carved the
//   stub name "missed_541b30" here from the vtable/callsite evidence)
// address 0x541b30, size 2544 bytes (0x9f0, 0x541b30..0x54251f, last instruction the jmp at 0x54251b: Ghidra's own function-boundary
//   metadata says 2100/0x834 bytes, ending at 0x542364, but that is stale -- objdump shows the
//   real control flow (every jcc/jmp inside this body) continuing past that point through the
//   second dispatch table at 0x54217b and the Keystone block at 0x542440, down to the last
//   `ret 0x10` at 0x54250f/0x5424d4, with 0x542520 the start of the jump-table data the two
//   `jmp DWORD PTR [reg*4+...]` sites (0x541b8d, 0x542197) index into. Ghidra's own decompile
//   (matching this rewrite) already reconstructs that whole range as one function, so the size
//   field alone is what is wrong.
// name confidence 0.85, rewrite confidence 0.55 (see the UNSURE notes below)
// evidence: shell_winmain.c stores 0x00541b30 in shell_window_proc ("the window procedure (no
//   Ghidra function; see the notes)") right before RegisterClassExA; out/phase4/shell_types_
//   notes.md calls this "the window procedure (0x541b30, which Ghidra has no function for; the
//   entry 0x542380 is one of its tails)"; out/phase4/shell_functions.md's summary of the
//   (misattributed) entry 0x542380 -- "The game's main window procedure: intercepts the
//   Windows-key chord to pause/minimize the game, routes Enter/Escape to the chat box, forwards
//   Keystone UI accelerator translation, and defers everything else" -- matches this body
//   exactly. Every global is named in types/shell.h except the ones this file's comments trace
//   into main.h (main_globals), rasterizer.h and sound.h (all "foreign" per that convention).
// register convention: __stdcall WNDPROC(hwnd, message, wparam, lparam), `ret 0x10` at every
//   exit (confirmed by objdump).
// UNSURE: 0x00542141, also in this cleanup pass's function list, is not a second real function.
//   objdump shows it is reached only by unconditional jumps and one fallthrough from inside this
//   function's own body (never by a `call`), it shares this function's own prologue's register
//   set on the way out (pops edi/esi/ebp/ebx, `ret 0x10` -- the exact mirror of this function's
//   own `push ebx/push ebp/push esi/push edi`), and its whole 28-byte body
//   (0x542141..0x54215d) is exactly `return DefWindowProcA(hwnd, message, wparam, lparam);`,
//   reproduced inline below at every place Ghidra's decompile of this function shows a tail call
//   to "missed_542141(unaff_ESI,unaff_EBP,unaff_EBX)". It is left unwritten as its own file; see
//   the pass summary for the full argument.
// UNSURE: the Keystone dispatch block (0x542440..0x54251b, message == the values that miss both
//   dispatch tables) calls input_record_windows_key_message with no visible arguments in
//   Ghidra's own decompile of this function, but objdump shows EAX = wparam and ECX = message
//   loaded immediately before both call sites (0x5424c0/0x542512), matching the EAX=key_or_char/
//   ECX=message convention console_process_input_events.c already established for this callee;
//   called that way here.
// UNSURE: rasterizer_capture_and_present is called with only one visible argument in Ghidra's
//   decompile ("rasterizer_capture_and_present(0)"); objdump shows `xor eax,eax` / `push 0`
//   before both call sites, i.e. both parameters are NULL, matching the EAX=tile/stack=bitmap
//   convention rasterizer_render_loading_screen.c already established for this callee.
// Cleanup-pass review (objdump, line by line): WM_ENTERSIZEMOVE (0x231) does not set
//   shell_window_minimized (the draft did); FUN_005410d0 takes BL = (wparam == 0);
//   input_key_block_timer_set takes the key in EDI (0x38 and 0x66 after chat submit, 0 on
//   Escape), which the draft dropped; Win32 message / SC_ names in the comments corrected
//   against the two dispatch tables (0x542520/0x54254c and 0x5425cc/0x5425d8).
// UNSURE: FUN_005410d0 (0x5410d0, WM_ACTIVATEAPP) and FUN_005481a0 (0x5481a0, the sound-resume
//   companion of sound_pause) are both in this module's address range but outside this pass's
//   function list; referenced here by their Ghidra names, not renamed.

// VERIFIED against disassembly 0x541b30..0x542520 (2026-09-30): both dispatch tables (0x542520/0x54254c, 0x5425cc/0x5425d8) and every case body compared: SIZE/FOCUS/PAINT/SETCURSOR/ACTIVATEAPP/DISPLAYCHANGE/SYSCOMMAND/ENTERSIZEMOVE/INITMENUPOPUP/POWERBROADCAST/keyboard/Keystone/chat paths, StretchBlt argument order, stdcall ret 0x10
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "sound.h"
#include "interface.h"
#include "main.h"
#include "shell.h"


// Minimal GDI BITMAP layout: only the fields GetObjectA's caller reads are named (bmWidth,
// bmHeight); the rest is exactly the 0x18-byte buffer size the call site passes.
typedef struct win32_bitmap {
    int32_t type;            // 0x00 bmType
    int32_t width;            // 0x04 bmWidth
    int32_t height;           // 0x08 bmHeight
    uint8_t unknown_0c[0xc]; // 0x0c bmWidthBytes/bmPlanes/bmBitsPixel/bmBits, unread here
} win32_bitmap;               // size 0x18


extern uint32_t time_query_performance_counter_ms(void);       // 0x449210, foreign (math)
extern void input_directinput_acquire_devices(void);           // 0x490620, foreign (input)
extern void input_directinput_unacquire_devices(void);         // 0x4906e0, foreign (input)
extern void input_reset_state_and_axis_configs(void);          // 0x490aa0, foreign (input)
extern void input_record_windows_key_message(int32_t key_or_char, int32_t message); // 0x490d10, foreign (input);
                                                                 // blam-cc: EAX -> key_or_char, ECX -> message
extern void input_key_block_timer_set(int16_t key, int32_t duration_ms); // 0x490bf0, foreign (input);
                                                                 // blam-cc: EDI -> key, stack -> duration_ms
extern void chat_close(void);                                  // 0x4aa900, foreign (interface)
extern void chat_submit_input(void);                            // 0x4aa9b0, foreign (interface)
extern int32_t render_device_is_ready(void);                   // 0x511d80, foreign (render)
extern void rasterizer_capture_and_present(const int16_t *tile, void *bitmap); // 0x518180, foreign
                                                                 // (rasterizer); EAX = tile, push = bitmap
extern void shell_handle_activate_app(uint8_t inactive);         // 0x5410d0, blam-cc: BL -> inactive;
                                                                 // blam-cc: BL -> inactive (sete bl at 0x541f63)
extern void sound_pause(void);                                  // 0x548170, foreign (sound)
extern void sound_resume(void);                                  // 0x5481a0

extern uint8_t shell_window_proc_bypass;    // 0x00721e8d
extern uint8_t shell_application_inactive;  // 0x00721e8c
extern void *shell_window;                  // 0x007461c4
extern uint8_t shell_window_minimized;      // 0x00746254
extern uint8_t shell_window_maximized;      // 0x00746255
extern void *shell_arrow_cursor;            // 0x006e35bc
extern int32_t nowindowskey;                // 0x007196f8
extern uint8_t split_screen_quit_prompt_string[4]; // 0x00719754, foreign (networking); byte 3 (offset
                                             // +3 = 0x00719757) is main_globals.return_to_main_menu's
                                             // twin -- both names are attested for this dword by
                                             // different modules; kept as the networking module's own
                                             // extern here since only the raw byte is touched

extern main_globals main_globals_data;      // 0x00719700, foreign (main); .return_to_main_menu is
                                             // +0x057 (byte 3 of this same dword), .quit is +0x05b
extern int32_t movie_playback_abort;        // 0x007196d4, foreign, UNSURE owner (see types/main.h)
extern int32_t game_time_force_single_tick; // 0x007196d8, foreign (game)

extern int32_t rasterizer_window_requested; // 0x0071d1a8, foreign (rasterizer), the -window flag
extern uint8_t rasterizer_fullscreen;       // 0x0071d16c, foreign (rasterizer)
extern void *rasterizer_device;             // 0x0071d174, foreign (rasterizer)
extern void *rasterizer_window_icon_dc;     // 0x0071d184, foreign (rasterizer), HDC
extern void *rasterizer_window_icon_bitmap; // 0x0071d188, foreign (rasterizer), HBITMAP

extern uint8_t sound_paused;                // 0x00725202, foreign (sound)
extern sound_driver *current_sound_driver;  // 0x00725208, foreign (sound)
extern int32_t sound_time;                  // 0x0072520c, foreign (sound)

extern uint8_t chat_dialog_open;            // 0x006b3858, foreign (interface)

extern void *keystone_module;               // 0x00721e9c
extern void *chat_gui_root_handle;                 // 0x00721ea4
extern keystone_dispatch_message_fn keystone_dispatch_message; // 0x00721ec0
extern chat_gui_release_fn chat_gui_release;            // 0x00721ec8

// Leaves the window in the "suspended" state used whenever the app loses input focus: pauses
// sound (or tells Keystone to pause when the device is not fullscreen-exclusive yet), releases
// DirectInput, resets input state, minimizes the window when it is fullscreen and closes chat.
static void shell_window_suspend_focus(void)
{
    if (shell_application_inactive != 1) {
        shell_application_inactive = 1;
        if (rasterizer_fullscreen == 0 || rasterizer_device == 0) {
            if (sound_paused != 1) {
                sound_paused = 1;
                if (current_sound_driver != 0) {
                    current_sound_driver->set_paused(1);
                }
            }
        } else {
            sound_pause();
        }
        input_directinput_unacquire_devices();
        input_reset_state_and_axis_configs();
        if (shell_window != 0 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
            ShowWindow((HWND)shell_window, 6 /* SW_MINIMIZE */);
        }
        chat_close();
    }
}

// The game's main window procedure (registered by shell_winmain / RegisterClassExA). Handles
// suspend/resume around focus loss, WM_CLOSE/WM_DESTROY quitting, the splash-bitmap WM_PAINT
// fallback before the device exists, WM_SYSCOMMAND screensaver/monitor-power suppression while
// the device is running, the Windows-key WM_SYSKEYDOWN chord (minimizes like WM_ACTIVATE),
// Keystone UI accelerator translation and Enter/Escape chat routing for keyboard messages, and
// defers everything else to DefWindowProcA.
int32_t __stdcall shell_window_procedure(HWND hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    HDC dc;
    win32_rect client_rect;
    win32_bitmap splash_bitmap_info;
    HWND desktop;
    int32_t handled;
    void *released_window;

    if (shell_window_proc_bypass != 0) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message <= 0x84) {
        if (message == 0x84) { // WM_NCHITTEST
            if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                return 0;
            }
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
        switch (message) {
        case 2:    // WM_DESTROY
        case 0x10: // WM_CLOSE
            PostQuitMessage(0);
            main_globals_data.return_to_main_menu = 0;
            main_globals_data.quit = 1;
            movie_playback_abort = 1;
            return DefWindowProcA(hwnd, message, wparam, lparam);

        case 5: // WM_SIZE
            if (wparam == 1) { // SIZE_MINIMIZED
                shell_window_suspend_focus();
                shell_window_minimized = 1;
            size_restored_tail: // == LAB_00541d8d
                shell_window_maximized = 0;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (wparam == 2) { // SIZE_MAXIMIZED
                if (shell_window_minimized != 0 && shell_application_inactive != 0) {
                    shell_application_inactive = 0;
                    input_directinput_acquire_devices();
                    input_reset_state_and_axis_configs();
                    if (shell_window != 0) {
                        ShowWindow((HWND)shell_window, 9 /* SW_RESTORE */);
                    }
                    if (shell_window_proc_bypass == 0) {
                        if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                            sound_resume();
                            shell_window_minimized = 0;
                            shell_window_maximized = 1;
                            return DefWindowProcA(hwnd, message, wparam, lparam);
                        }
                        if (sound_paused != 0) {
                            sound_paused = 0;
                            if (current_sound_driver != 0) {
                                current_sound_driver->set_paused(0);
                            }
                            sound_time = time_query_performance_counter_ms();
                        }
                    }
                }
                shell_window_minimized = 0;
                shell_window_maximized = 1;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (wparam != 0) {
                break;
            }
            // SIZE_RESTORED
            if (shell_window_maximized != 0) {
                goto size_restored_tail; // clears shell_window_maximized only, no minimized write
            }
            if (shell_window_minimized != 0) {
                if (shell_application_inactive != 0) {
                    shell_application_inactive = 0;
                    input_directinput_acquire_devices();
                    input_reset_state_and_axis_configs();
                    if (shell_window != 0) {
                        ShowWindow((HWND)shell_window, 9 /* SW_RESTORE */);
                    }
                    if (shell_window_proc_bypass == 0) {
                        if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                            sound_resume();
                            shell_window_minimized = 0;
                            return DefWindowProcA(hwnd, message, wparam, lparam);
                        }
                        if (sound_paused != 0) {
                            sound_paused = 0;
                            if (current_sound_driver != 0) {
                                current_sound_driver->set_paused(0);
                            }
                            sound_time = time_query_performance_counter_ms();
                        }
                    }
                }
                shell_window_minimized = 0;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (render_device_is_ready() == 0) {
                break;
            }
            goto resume_focus_fast_path; // == LAB_00541f30, skips the gate below entirely

        case 7: // WM_SETFOCUS
            if (shell_window == 0) {
                break;
            }
        restore_from_suspend: // == LAB_005420bb
            if (shell_application_inactive != 0) {
                shell_application_inactive = 0;
                input_directinput_acquire_devices();
                input_reset_state_and_axis_configs();
                if (shell_window != 0) {
                    ShowWindow((HWND)shell_window, 9 /* SW_RESTORE */);
                }
                if (shell_window_proc_bypass == 0) {
                    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                    resume_focus_fast_path: // == LAB_00541f30
                        sound_resume();
                        return DefWindowProcA(hwnd, message, wparam, lparam);
                    }
                    if (sound_paused != 0) {
                        sound_paused = 0;
                        if (current_sound_driver != 0) {
                            current_sound_driver->set_paused(0);
                        }
                        sound_time = time_query_performance_counter_ms();
                    }
                }
            }
            break;

        case 8: // WM_KILLFOCUS
            if (shell_window != 0 && shell_application_inactive != 1) {
                shell_window_suspend_focus();
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0xf: // WM_PAINT
            if (render_device_is_ready() == 0 && rasterizer_window_requested == 0) {
                if (rasterizer_device == 0) {
                    if (rasterizer_window_icon_bitmap != 0) {
                        dc = GetDC(hwnd);
                        GetClientRect(hwnd, &client_rect);
                        GetObjectA(rasterizer_window_icon_bitmap, 0x18, &splash_bitmap_info);
                        StretchBlt(dc, 0, 0, client_rect.right - client_rect.left,
                                   client_rect.bottom - client_rect.top,
                                   (HDC)rasterizer_window_icon_dc, 0, 0,
                                   splash_bitmap_info.width, splash_bitmap_info.height,
                                   0xcc0020 /* SRCCOPY */);
                        ReleaseDC(hwnd, dc);
                    }
                    ValidateRect(hwnd, (win32_rect *)0);
                    return 0;
                }
                rasterizer_capture_and_present((const int16_t *)0, (void *)0); // EAX = 0, push 0
                ValidateRect(hwnd, (win32_rect *)0);
                return 0;
            }
            if (rasterizer_device != 0) {
                rasterizer_capture_and_present((const int16_t *)0, (void *)0); // EAX = 0, push 0
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0x14: // WM_ERASEBKGND
            return 1;

        case 0x1c: // WM_ACTIVATEAPP
            if (rasterizer_window_requested == 0 && game_time_force_single_tick == 0 && shell_window != 0) {
                shell_handle_activate_app(wparam == 0); // BL = the application is being deactivated
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0x20: // WM_SETCURSOR
            if (render_device_is_ready() == 0 && rasterizer_window_requested == 0) {
                if ((int16_t)lparam == 1 && GetForegroundWindow() == hwnd) {
                    SetCursor((HCURSOR)0);
                    return 1;
                }
                SetCursor((HCURSOR)shell_arrow_cursor);
            }
            return 1;

        case 0x51: // WM_INPUTLANGCHANGE: shares the Windows-key dispatch below
            goto keyboard_message;

        case 0x7e: // WM_DISPLAYCHANGE (wparam = bits per pixel; 32 is left alone)
            if (shell_window != 0 && wparam != 0x20) {
                shell_window_suspend_focus();
                ShowWindow((HWND)shell_window, 6 /* SW_MINIMIZE */);
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        default:
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message == 0x112) { // WM_SYSCOMMAND
        // SC_SIZE (0xf000), SC_MOVE (0xf010), SC_MAXIMIZE (0xf030), SC_MONITORPOWER (0xf170) and
        // SC_KEYMENU (0xf100) are swallowed (return 0); both render_device_is_ready() outcomes
        // return 0 (0x5421ce and 0x5424cb). Every other command goes to DefWindowProcA.
        if (wparam < 0xf031) {
            if (wparam != 0xf030 && wparam != 0xf000 && wparam != 0xf010) {
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else {
            if (wparam == 0xf100) { // SC_KEYMENU
                return 0;
            }
            if (wparam != 0xf170) {
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        }
        if (render_device_is_ready() != 1) {
            return 0;
        }
        return 0;
    }

    if (message < 0x113) {
        switch (message) {
        case 0x100: // WM_KEYDOWN
        case 0x101: // WM_KEYUP
        case 0x106: // WM_SYSCHAR
        case 0x10d: // WM_IME_STARTCOMPOSITION
        case 0x10e: // WM_IME_ENDCOMPOSITION
        case 0x10f: // WM_IME_COMPOSITION
            goto keyboard_message;
        case 0x102: // WM_CHAR
        case 0x104: // WM_SYSKEYDOWN (0x103 WM_DEADCHAR, 0x105 WM_SYSKEYUP, 0x107..0x10c: default)
            goto keystone_dispatch;
        default:
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
    }

    if (message < 0x232) {
        if (message == 0x231) { // WM_ENTERSIZEMOVE
            if (shell_application_inactive != 1) {
                shell_window_suspend_focus(); // no shell_window_minimized write here (0x5422d9..0x54235d)
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else if (message == 0x117) { // WM_INITMENUPOPUP; high word of lparam (fSystemMenu) == 1
            if ((uint32_t)lparam >> 0x10 == 1 && shell_application_inactive != 1) {
                shell_window_suspend_focus();
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else if (message == 0x218) { // WM_POWERBROADCAST
            if (wparam == 0) { // PBT_APMQUERYSUSPEND
                sound_pause();
                return 1;
            }
            if (wparam == 7) { // PBT_APMRESUMESUSPEND
                sound_resume();
                return 1;
            }
        }
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message == 0x232) { // WM_EXITSIZEMOVE (skips the shell_window test of WM_SETFOCUS)
        goto restore_from_suspend;
    }

    if (message < 0x281 || message > 0x284) { // WM_IME_SETCONTEXT..WM_IME_COMPOSITIONFULL fall through
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

keyboard_message:
    // reached for WM_INPUTLANGCHANGE (0x51), WM_KEYDOWN/WM_KEYUP, WM_SYSCHAR, WM_IME_START/END/
    // COMPOSITION (0x10d..0x10f) and 0x281..0x284: VK_LWIN / VK_RWIN suspend and hand the
    // foreground to the desktop, anything else goes to the Keystone dispatch.
    if ((wparam == 0x5b || wparam == 0x5c) && nowindowskey == 0 && shell_window != 0) {
        if (shell_application_inactive != 1) {
            shell_window_suspend_focus();
        }
        desktop = GetDesktopWindow();
        SetForegroundWindow(desktop);
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

keystone_dispatch:
    if (chat_gui_root_handle != 0 && keystone_module != 0) {
        handled = 1;
        released_window = keystone_dispatch_message(chat_gui_root_handle, message, wparam, lparam, &handled);
        if (released_window != 0) {
            chat_gui_release(released_window);
        }
        if (message == 0x100) { // WM_KEYDOWN
            if (wparam == 0xd) { // VK_RETURN
                if (chat_dialog_open != 0) {
                    chat_submit_input();
                    input_key_block_timer_set(0x38, 200); // EDI = 0x38 (0x5424ea)
                    input_key_block_timer_set(0x66, 200); // EDI = 0x66 (0x5424f9)
                    return 0;
                }
            } else if (wparam == 0x1b) { // VK_ESCAPE
                if (chat_dialog_open != 0) {
                    input_key_block_timer_set(0, 0xfa); // EDI = 0 (0x5424a7)
                }
                chat_close();
            }
        }
        if (handled == 0) {
            input_record_windows_key_message(wparam, message);
            return 0;
        }
    }
    input_record_windows_key_message(wparam, message);
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

#if 0
Original Ghidra decompilation (0x541b30), before the phase-4 rewrite (parameters/globals renamed
above; case 0x51/0x7e and the two jump-table dispatches were expanded from switchD_00541b8d_caseD_51
/ switchD_00542197_caseD_102 by reading objdump 0x541b30..0x54251b, since Ghidra's own function
boundary (2100 bytes) stops short of that code even though its own decompile below already
includes it):

LRESULT missed_541b30(HWND param_1,uint param_2,uint param_3,uint param_4)

{
  uint wParam;
  LRESULT LVar1;
  HDC hdcDest;
  int iVar2;
  HWND pHVar3;
  undefined4 unaff_EBX;
  undefined4 unaff_EBP;
  undefined4 unaff_ESI;
  tagRECT local_28;
  undefined1 local_18 [4];
  int local_14;
  int local_10;

  wParam = param_3;
  if (DAT_00721e8d != '\0') {
    LVar1 = DefWindowProcA(param_1,param_2,param_3,param_4);
    return LVar1;
  }
  if (0x84 < param_2) {
    if (param_2 < 0x113) {
      if (param_2 == 0x112) {
        if (param_3 < 0xf031) {
          if (((param_3 != 0xf030) && (param_3 != 0xf000)) && (param_3 != 0xf010)) {
            LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
            return LVar1;
          }
        }
        else {
          if (param_3 == 0xf100) {
            return 0;
          }
          if (param_3 != 0xf170) goto code_r0x00542141;
        }
        iVar2 = render_device_is_ready();
        if ((char)iVar2 != '\x01') {
          return 0;
        }
        return 0;
      }
      switch(param_2) {
      case 0x100:
      case 0x101:
      case 0x106:
      case 0x10d:
      case 0x10e:
      case 0x10f:
        goto switchD_00541b8d_caseD_51;
      case 0x102:
      case 0x104:
        goto switchD_00542197_caseD_102;
      default:
        goto code_r0x00542141;
      }
    }
    if (param_2 < 0x232) {
      if (param_2 == 0x231) {
        if (DAT_00721e8c != '\x01') {
          DAT_00721e8c = 1;
          if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
            if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
              (**(code **)(DAT_00725208 + 0x28))(1);
            }
          }
          else {
            sound_pause();
          }
          input_directinput_unacquire_devices();
          input_reset_state_and_axis_configs();
          if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
            ShowWindow(DAT_007461c4,6);
          }
          chat_close();
          LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
          return LVar1;
        }
      }
      else if (param_2 == 0x117) {
        if ((param_4 >> 0x10 == 1) && (DAT_00721e8c != '\x01')) {
          DAT_00721e8c = 1;
          if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
            if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
              (**(code **)(DAT_00725208 + 0x28))(1);
            }
          }
          else {
            sound_pause();
          }
          input_directinput_unacquire_devices();
          input_reset_state_and_axis_configs();
          if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
            ShowWindow(DAT_007461c4,6);
          }
          chat_close();
          LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
          return LVar1;
        }
      }
      else if (param_2 == 0x218) {
        if (param_3 == 0) {
          sound_pause();
          return 1;
        }
        if (param_3 == 7) {
          sound_resume();
          return 1;
        }
      }
      goto code_r0x00542141;
    }
    if (param_2 == 0x232) goto LAB_005420bb;
    if ((param_2 < 0x281) || (0x284 < param_2)) goto code_r0x00542141;
switchD_00541b8d_caseD_51:
    if (((param_3 == 0x5b) || (param_3 == 0x5c)) &&
       ((DAT_007196f8 == 0 && (DAT_007461c4 != (HWND)0x0)))) {
      if (DAT_00721e8c != '\x01') {
        DAT_00721e8c = '\x01';
        if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
          if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
            (**(code **)(DAT_00725208 + 0x28))(1);
          }
        }
        else {
          sound_pause();
        }
        input_directinput_unacquire_devices();
        input_reset_state_and_axis_configs();
        if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
          ShowWindow(DAT_007461c4,6);
        }
        chat_close();
      }
      pHVar3 = GetDesktopWindow();
      SetForegroundWindow(pHVar3);
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
switchD_00542197_caseD_102:
    if ((DAT_00721ea4 != 0) && (DAT_00721e9c != 0)) {
      param_3 = 1;
      iVar2 = (*DAT_00721ec0)(DAT_00721ea4,param_2,wParam,param_4,&param_3);
      if (iVar2 != 0) {
        (*DAT_00721ec8)(iVar2);
      }
      if (param_2 == 0x100) {
        if (wParam == 0xd) {
          if (DAT_006b3858 != '\0') {
            chat_submit_input();
            input_key_block_timer_set(200);
            input_key_block_timer_set(200);
            return 0;
          }
        }
        else if (wParam == 0x1b) {
          if (DAT_006b3858 != '\0') {
            input_key_block_timer_set(0xfa);
          }
          chat_close();
        }
      }
      if (param_3 == 0) {
        input_record_windows_key_message();
        return 0;
      }
    }
    input_record_windows_key_message();
    LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
    return LVar1;
  }
  if (param_2 == 0x84) {
    if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
      return 0;
    }
    goto code_r0x00542141;
  }
  switch(param_2) {
  case 2:
  case 0x10:
    PostQuitMessage(0);
    DAT_00719754._3_1_ = 0;
    DAT_0071975b = 1;
    DAT_007196d4 = 1;
    LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
    return LVar1;
  case 5:
    if (param_3 == 1) {
      if (DAT_00721e8c != '\x01') {
        DAT_00721e8c = '\x01';
        if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
          if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
            (**(code **)(DAT_00725208 + 0x28))(1);
          }
        }
        else {
          sound_pause();
        }
        input_directinput_unacquire_devices();
        input_reset_state_and_axis_configs();
        if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
          ShowWindow(DAT_007461c4,6);
        }
        chat_close();
      }
      DAT_00746254 = '\x01';
LAB_00541d8d:
      DAT_00746255 = 0;
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    if (param_3 == 2) {
      if ((DAT_00746254 != '\0') && (DAT_00721e8c != '\0')) {
        DAT_00721e8c = '\0';
        input_directinput_acquire_devices();
        input_reset_state_and_axis_configs();
        if (DAT_007461c4 != (HWND)0x0) {
          ShowWindow(DAT_007461c4,9);
        }
        if (DAT_00721e8d == '\0') {
          if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
            sound_resume();
            DAT_00746254 = 0;
            DAT_00746255 = 1;
            LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
            return LVar1;
          }
          if (DAT_00725202 != '\0') {
            DAT_00725202 = '\0';
            if (DAT_00725208 != 0) {
              (**(code **)(DAT_00725208 + 0x28))(0);
            }
            DAT_0072520c = time_query_performance_counter_ms();
          }
        }
      }
      DAT_00746254 = 0;
      DAT_00746255 = 1;
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    if (param_3 != 0) break;
    if (DAT_00746255 != '\0') goto LAB_00541d8d;
    if (DAT_00746254 != '\0') {
      if (DAT_00721e8c != '\0') {
        DAT_00721e8c = '\0';
        input_directinput_acquire_devices();
        input_reset_state_and_axis_configs();
        if (DAT_007461c4 != (HWND)0x0) {
          ShowWindow(DAT_007461c4,9);
        }
        if (DAT_00721e8d == '\0') {
          if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
            sound_resume();
            DAT_00746254 = 0;
            LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
            return LVar1;
          }
          if (DAT_00725202 != '\0') {
            DAT_00725202 = '\0';
            if (DAT_00725208 != 0) {
              (**(code **)(DAT_00725208 + 0x28))(0);
            }
            DAT_0072520c = time_query_performance_counter_ms();
          }
        }
      }
      DAT_00746254 = 0;
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    iVar2 = render_device_is_ready();
    if ((char)iVar2 == '\0') break;
    goto LAB_00541f30;
  case 7:
    if (DAT_007461c4 == (HWND)0x0) break;
LAB_005420bb:
    if (DAT_00721e8c != '\0') {
      DAT_00721e8c = '\0';
      input_directinput_acquire_devices();
      input_reset_state_and_axis_configs();
      if (DAT_007461c4 != (HWND)0x0) {
        ShowWindow(DAT_007461c4,9);
      }
      if (DAT_00721e8d == '\0') {
        if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
LAB_00541f30:
          sound_resume();
          LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
          return LVar1;
        }
        if (DAT_00725202 != '\0') {
          DAT_00725202 = '\0';
          if (DAT_00725208 != 0) {
            (**(code **)(DAT_00725208 + 0x28))(0);
          }
          DAT_0072520c = time_query_performance_counter_ms();
        }
      }
    }
    break;
  case 8:
    if ((DAT_007461c4 != (HWND)0x0) && (DAT_00721e8c != '\x01')) {
      DAT_00721e8c = 1;
      if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
        if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
          (**(code **)(DAT_00725208 + 0x28))(1);
        }
      }
      else {
        sound_pause();
      }
      input_directinput_unacquire_devices();
      input_reset_state_and_axis_configs();
      if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
        ShowWindow(DAT_007461c4,6);
      }
      chat_close();
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    break;
  case 0xf:
    iVar2 = render_device_is_ready();
    if (((char)iVar2 == '\0') && (DAT_0071d1a8 == 0)) {
      if (DAT_0071d174 == 0) {
        if (DAT_0071d188 != (HANDLE)0x0) {
          hdcDest = GetDC(param_1);
          GetClientRect(param_1,&local_28);
          GetObjectA(DAT_0071d188,0x18,local_18);
          StretchBlt(hdcDest,0,0,local_28.right - local_28.left,local_28.bottom - local_28.top,
                     DAT_0071d184,0,0,local_14,local_10,0xcc0020);
          ReleaseDC(param_1,hdcDest);
        }
        ValidateRect(param_1,(RECT *)0x0);
        return 0;
      }
      rasterizer_capture_and_present(0);
      ValidateRect(param_1,(RECT *)0x0);
      return 0;
    }
    if (DAT_0071d174 != 0) {
      rasterizer_capture_and_present(0);
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    break;
  case 0x14:
    return 1;
  case 0x1c:
    if (((DAT_0071d1a8 == 0) && (DAT_007196d8 == 0)) && (DAT_007461c4 != (HWND)0x0)) {
      FUN_005410d0();
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
    break;
  case 0x20:
    iVar2 = render_device_is_ready();
    if (((char)iVar2 == '\0') && (DAT_0071d1a8 == 0)) {
      if (((short)param_4 == 1) && (pHVar3 = GetForegroundWindow(), pHVar3 == param_1)) {
        SetCursor((HCURSOR)0x0);
        return 1;
      }
      SetCursor(DAT_006e35bc);
    }
    return 1;
  case 0x51:
    goto switchD_00541b8d_caseD_51;
  case 0x7e:
    if ((DAT_007461c4 != (HWND)0x0) && (param_3 != 0x20)) {
      if (DAT_00721e8c != '\x01') {
        DAT_00721e8c = '\x01';
        if ((DAT_0071d16c == '\0') || (DAT_0071d174 == 0)) {
          if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
            (**(code **)(DAT_00725208 + 0x28))(1);
          }
        }
        else {
          sound_pause();
        }
        input_directinput_unacquire_devices();
        input_reset_state_and_axis_configs();
        if (((DAT_007461c4 != (HWND)0x0) && (DAT_0071d16c != '\0')) && (DAT_0071d174 != 0)) {
          ShowWindow(DAT_007461c4,6);
        }
        chat_close();
      }
      ShowWindow(DAT_007461c4,6);
      LVar1 = missed_542141(unaff_ESI,unaff_EBP,unaff_EBX);
      return LVar1;
    }
  }
code_r0x00542141:
  LVar1 = DefWindowProcA(param_1,param_2,wParam,param_4);
  return LVar1;
}

The 0x542141 tail itself (objdump, not a Ghidra function; reproduced inline above at every exit
that reached it):

00542141:
  mov edx,[esp+0x48]   ; lparam
  mov eax,[esp+0x3c]   ; hwnd
  push edx
  push ebp             ; wparam
  push edi             ; message
  push eax
  call DefWindowProcA
  pop edi
  pop esi
  pop ebp
  pop ebx
  add esp,0x28
  ret 0x10
#endif

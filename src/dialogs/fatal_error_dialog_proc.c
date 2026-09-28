// fatal_error_dialog_proc  (Ghidra: missed_57e5a0; named per src/shell/shell_display_fatal_error_dialog.c,
// which already declares "extern int32_t __stdcall fatal_error_dialog_proc(void *dialog, uint32_t message,
// uint32_t wparam, int32_t lparam); // 0x57e5a0, the DLGPROC for this dialog")
// address 0x57e5a0, size 687 bytes
// name confidence: 0.8 (matches the existing caller-side extern)   rewrite confidence: 0.7
// evidence: out/phase4/dialogs_types_notes.md "0x57e5a0 (fatal error dialog proc, shell) is also
// missing as a Ghidra function. Its types are not defined here" -- the notes explicitly hand the
// two registry-choice globals to this function: "0x00722bc0 is the IsDlgButtonChecked(0x3e9)
// state. It is stored before EndDialog at 0x57e773 / 0x57e7a3, and shell_display_fatal_error_dialog
// reads it at 0x57ee7c" and "0x00722c58 is the sprintf \"%dMHz, %dMB, ...\" system specs text set
// into control 0x3f1." types/shell.h documents every other global this function touches
// (fatal_error_text/_title/_help_file/_is_fatal, graphics_vendor_name/_device_name/_device_id,
// physical_memory, cpu_speed, video_memory, shell_window) and the two format strings match
// 0x006729e0/0x006729e8/0x006729f4 by address order in the strings list.
// register convention: plain __stdcall DLGPROC (dialog, message, wparam, lparam), all on the
// stack, ret 0x10; confirmed at every call site by the stack offsets after the callee's own
// pushes (objdump). No register-passed (in_EAX-style) arguments. Ghidra narrowed its own
// param_3 to ushort because only LOWORD(wParam) (the WM_COMMAND control id) is ever read; the
// full uint32_t wparam is kept here to match the DLGPROC shape the caller already declares.
// UNSURE: dialog control ids (0x3e9 checkbox, 0x3ec/2/3 the three "done" buttons, 0x3ee the
// message static, 0x3ef the hyperlink static, 0x3f1 the system-specs static) have no resource
// script in this repo to name them from; kept as commented literals. The outer
// "message == 0x3ed" arm (only reachable for message > 0x111, i.e. not a normal window message)
// and the WM_CLOSE (0x10) arm run byte-identical duplicated code (DestroyWindow(shell_window) +
// EndDialog(dialog, 2)); this is the compiler's own duplication (two separate code blocks in the
// binary, not a shared subroutine) and is reproduced as two identical blocks here rather than
// factored into a helper, to keep control flow 1:1 with the binary.
// UNSURE: button ids 0x3ec (EndDialog result 0) and 3 (EndDialog result 1) both save the
// checkbox 0x3e9 state to fatal_error_remember_choice first; which button is "Continue" vs
// "Safe Mode" is not recoverable from this function alone.
// reconciled: R15 0x00722bc0 / 0x00722c58 are shell.h globals (int32_t fatal_error_remember_choice, char fatal_error_system_specs[0x100])

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "interface.h"

extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT

extern int32_t dialog_static_hyperlink_install(void *control); // 0x57e4c0, this module; blam-cc: ESI -> control

extern char fatal_error_text[k_shell_fatal_error_text_length];       // 0x006effe8
extern char fatal_error_help_file[k_shell_fatal_error_readme_length]; // 0x006f0470
extern char fatal_error_title[k_shell_fatal_error_title_length];     // 0x006f03f0
extern int32_t fatal_error_is_fatal;       // 0x006f03ec
extern char *graphics_vendor_name;         // 0x00722b90
extern char *graphics_device_name;         // 0x00722b94
extern uint32_t graphics_device_id;        // 0x00722b98
extern uint32_t cpu_speed;                 // 0x00722bac
extern uint32_t physical_memory;           // 0x00722ba8
extern uint32_t video_memory;              // 0x00722bb0
extern void *shell_window;                 // 0x007461c4
extern int32_t fatal_error_remember_choice; // 0x00722bc0, types/shell.h (R15)
extern char fatal_error_system_specs[0x100]; // 0x00722c58, types/shell.h (R15); sprintf'd here, read by SetDlgItemTextA(0x3f1);
                                        // 0x100 bytes: the next referenced global is 0x00722d58

// The fatal-error dialog's DLGPROC. On WM_INITDIALOG, centers the dialog on the desktop, sets
// the title and the main message static (0x3ee), disables the checkbox and the two "done"
// buttons (0x3e9, 0x3ec, 3) while fatal_error_is_fatal is set, installs the hyperlink subclass on
// the help-file static (0x3ef), and formats the system-specs static (0x3f1) from the graphics /
// cpu / memory globals. On WM_COMMAND, the checkbox+button combos (0x3ec -> EndDialog 0, 3 ->
// EndDialog 1) latch the checkbox into fatal_error_remember_choice first; the hyperlink button
// (0x3ef) opens fatal_error_help_file with ShellExecuteA; the two "quit" ids (2, 0x3ed) and
// WM_CLOSE / the message-id-0x3ed arm all destroy shell_window (if any) and EndDialog(2).
// Unhandled messages return 0 (the dialog manager default).
int32_t __stdcall fatal_error_dialog_proc(void *dialog, uint32_t message, uint32_t wparam, int32_t lparam)
{
    win32_rect dialog_rect;
    win32_rect desktop_rect;
    void *desktop_window;
    void *hyperlink_control;
    int32_t control_id;
    int32_t checked;

    if (message < 0x111 /* WM_COMMAND */) {
        if (message == 0x10 /* WM_CLOSE */) {
            if (shell_window != 0) {
                DestroyWindow(shell_window);
            }
            EndDialog(dialog, 2);
            return 1;
        }
        if (message == 0x110 /* WM_INITDIALOG */) {
            GetWindowRect(dialog, &dialog_rect);
            desktop_window = GetDesktopWindow();
            GetClientRect(desktop_window, &desktop_rect);
            MoveWindow(dialog,
                       (desktop_rect.right - desktop_rect.left) / 2 - (dialog_rect.right - dialog_rect.left) / 2,
                       (desktop_rect.bottom - desktop_rect.top) / 2 - (dialog_rect.bottom - dialog_rect.top) / 2,
                       dialog_rect.right - dialog_rect.left, dialog_rect.bottom - dialog_rect.top, 1);
            SetWindowTextA(dialog, fatal_error_title);
            SetDlgItemTextA(dialog, 0x3ee /* message static */, fatal_error_text);
            if (fatal_error_is_fatal != 0) {
                EnableWindow(GetDlgItem(dialog, 0x3e9 /* "remember my choice" checkbox */), 0);
                EnableWindow(GetDlgItem(dialog, 0x3ec /* "done" button, result 0 */), 0);
                EnableWindow(GetDlgItem(dialog, 3 /* "done" button, result 1 */), 0);
            }
            hyperlink_control = GetDlgItem(dialog, 0x3ef /* help-file hyperlink static */);
            dialog_static_hyperlink_install(hyperlink_control);
            if (graphics_device_id == 0) {
                if (cpu_speed == 0 || physical_memory == 0) {
                    fatal_error_system_specs[0] = 0;
                } else {
                    sprintf(fatal_error_system_specs, "%dMHz, %dMB", cpu_speed, physical_memory);
                }
            } else {
                sprintf(fatal_error_system_specs, "%dMHz, %dMB, %dM %s %s (0x%04x)", cpu_speed, physical_memory,
                        video_memory >> 0x14, graphics_vendor_name, graphics_device_name, graphics_device_id);
            }
            SetDlgItemTextA(dialog, 0x3f1 /* system specs static */, fatal_error_system_specs);
            return 1;
        }
        return 0;
    }

    if (message == 0x111 /* WM_COMMAND */) {
        control_id = (int32_t)(uint16_t)wparam; // LOWORD(wParam), movzx at 0x57e73c

        if (control_id <= 0x3ec) {
            if (control_id == 0x3ec) {
                checked = IsDlgButtonChecked(dialog, 0x3e9);
                fatal_error_remember_choice = (checked != 0);
                EndDialog(dialog, 0);
                return 1;
            }
            if (control_id == 2) {
                if (shell_window != 0) {
                    DestroyWindow(shell_window);
                }
                EndDialog(dialog, 2);
                return 1;
            }
            if (control_id == 3) {
                checked = IsDlgButtonChecked(dialog, 0x3e9);
                fatal_error_remember_choice = (checked != 0);
                EndDialog(dialog, 1);
                return 1;
            }
        } else {
            if (control_id == 0x3ed) {
                if (shell_window != 0) {
                    DestroyWindow(shell_window);
                }
                EndDialog(dialog, 2);
                return 1;
            }
            if (control_id == 0x3ef /* hyperlink button */) {
                ShellExecuteA(dialog, "open", fatal_error_help_file, (const char *)0, (const char *)0, 1);
                return 1;
            }
        }
        return 0;
    }

    if (message > 0x111 && message == 0x3ed) {
        if (shell_window != 0) {
            DestroyWindow(shell_window);
        }
        EndDialog(dialog, 2);
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x57e5a0):

undefined4 missed_57e5a0(HWND param_1,uint param_2,ushort param_3)

{
  HWND pHVar1;
  UINT UVar2;
  tagRECT *lpRect;
  BOOL BVar3;
  tagRECT local_20;
  tagRECT local_10;

  if (param_2 < 0x112) {
    if (param_2 == 0x111) {
      if (param_3 < 0x3ed) {
        if (param_3 == 0x3ec) {
          UVar2 = IsDlgButtonChecked(param_1,0x3e9);
          DAT_00722bc0 = (uint)(UVar2 != 0);
          EndDialog(param_1,0);
          return 1;
        }
        if (param_3 == 2) {
LAB_0057e7ed:
          if (DAT_007461c4 != (HWND)0x0) {
            DestroyWindow(DAT_007461c4);
          }
          EndDialog(param_1,2);
          return 1;
        }
        if (param_3 == 3) {
          UVar2 = IsDlgButtonChecked(param_1,0x3e9);
          DAT_00722bc0 = (uint)(UVar2 != 0);
          EndDialog(param_1,1);
          return 1;
        }
      }
      else {
        if (param_3 == 0x3ed) goto LAB_0057e7ed;
        if (param_3 == 0x3ef) {
          ShellExecuteA(param_1,"open",&DAT_006f0470,(LPCSTR)0x0,(LPCSTR)0x0,1);
          return 1;
        }
      }
    }
    else {
      if (param_2 == 0x10) goto LAB_0057e826;
      if (param_2 == 0x110) {
        GetWindowRect(param_1,&local_10);
        lpRect = &local_20;
        pHVar1 = GetDesktopWindow();
        GetClientRect(pHVar1,lpRect);
        MoveWindow(param_1,(local_20.right - local_20.left) / 2 -
                           (local_10.right - local_10.left) / 2,
                   (local_20.bottom - local_20.top) / 2 - (local_10.bottom - local_10.top) / 2,
                   local_10.right - local_10.left,local_10.bottom - local_10.top,1);
        SetWindowTextA(param_1,&DAT_006f03f0);
        SetDlgItemTextA(param_1,0x3ee,(LPCSTR)&DAT_006effe8);
        if (DAT_006f03ec != 0) {
          BVar3 = 0;
          pHVar1 = GetDlgItem(param_1,0x3e9);
          EnableWindow(pHVar1,BVar3);
          BVar3 = 0;
          pHVar1 = GetDlgItem(param_1,0x3ec);
          EnableWindow(pHVar1,BVar3);
          BVar3 = 0;
          pHVar1 = GetDlgItem(param_1,3);
          EnableWindow(pHVar1,BVar3);
        }
        GetDlgItem(param_1,0x3ef);
        dialog_static_hyperlink_install();
        if (DAT_00722b98 == 0) {
          if ((DAT_00722bac == 0) || (DAT_00722ba8 == 0)) {
            DAT_00722c58 = 0;
          }
          else {
            _sprintf(&DAT_00722c58,"%dMHz, %dMB",DAT_00722bac,DAT_00722ba8);
          }
        }
        else {
          _sprintf(&DAT_00722c58,"%dMHz, %dMB, %dM %s %s (0x%04x)",DAT_00722bac,DAT_00722ba8,
                   DAT_00722bb0 >> 0x14,DAT_00722b90,DAT_00722b94,DAT_00722b98);
        }
        SetDlgItemTextA(param_1,0x3f1,&DAT_00722c58);
        return 1;
      }
    }
  }
  else if (param_2 == 0x3ed) {
LAB_0057e826:
    if (DAT_007461c4 != (HWND)0x0) {
      DestroyWindow(DAT_007461c4);
    }
    EndDialog(param_1,2);
    return 1;
  }
  return 0;
}
#endif

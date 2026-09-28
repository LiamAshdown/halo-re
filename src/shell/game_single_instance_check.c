// game_single_instance_check  (Ghidra: game_single_instance_check, already named)
// address 0x542d70, size 382 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: CreateMutexA over shell_instance_mutex_names, ERROR_ALREADY_EXISTS (0xb7) handling,
// then FindWindowA("Halo","Halo") + SetForegroundWindow/ShowWindow fallback; matches
// shell_instance_mode / shell_instance_index in shell.h exactly.
// register convention: __cdecl, mode is the recognized parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t shell_instance_mode_value;    // 0x0069eab4, UNSURE: named _value -- shell.h's shell_instance_mode enum typedef occupies that identifier
extern int32_t shell_instance_index;         // 0x00721f04
extern void *shell_instance_mutex;           // 0x00721f00
extern char *shell_instance_mutex_names[9];  // 0x0069eab8

extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal); // 0x0057ea70
extern void _exit(int32_t code);

extern int32_t __stdcall GetVersionExA(os_version_info_a *info);
extern void *__stdcall CreateMutexA(void *security_attributes, int32_t initial_owner, const char *name);
extern uint32_t __stdcall GetLastError(void);
extern void __stdcall CloseHandle(void *handle);
extern void *__stdcall FindWindowA(const char *class_name, const char *window_name);
extern int32_t __stdcall GetWindowPlacement(void *hwnd, window_placement *placement);
extern int32_t __stdcall SetForegroundWindow(void *hwnd);
extern int32_t __stdcall ShowWindow(void *hwnd, int32_t cmd);

// Enforces a single running instance of Halo by taking a named mutex; if another copy already
// holds it, brings that window to the foreground and terminates the current process.
void game_single_instance_check(int32_t mode)
{
    os_version_info_a version;
    window_placement placement;
    void *window;
    int32_t first_index;
    int32_t last_index;
    int32_t i;
    uint32_t last_error;
    const char *name;

    shell_instance_mode_value = mode;
    shell_instance_index = -1;

    version.size = 0x94;
    GetVersionExA(&version);

    if (mode == k_shell_instance_mode_multiple) {
        first_index = 1;
        last_index = k_shell_instance_mutex_multi_last;
    } else {
        first_index = 0;
        last_index = 0;
    }

    last_error = 1;
    for (i = first_index; i <= last_index; i++) {
        name = shell_instance_mutex_names[i];
        if (version.major_version < 5) {
            name += 7; // skip the "Global\" namespace prefix, pre-Windows 2000
        }
        shell_instance_mutex = CreateMutexA(0, 1, name);
        last_error = GetLastError();
        if (last_error == 0xb7 /* ERROR_ALREADY_EXISTS */) {
            if (shell_instance_mutex != 0) {
                CloseHandle(shell_instance_mutex);
            }
        } else if (shell_instance_mutex != 0) {
            shell_instance_index = i - first_index;
            break;
        }
    }

    if (last_error != 0xb7) {
        if (shell_instance_mutex == 0) {
            goto find_running_instance;
        }
        if (shell_instance_index != -1) {
            return;
        }
    }

    if (shell_instance_mutex != 0) {
        CloseHandle(shell_instance_mutex);
        shell_instance_mutex = 0;
        shell_instance_mode_value = -1;
        shell_instance_index = -1;
    }

find_running_instance:
    if (mode != k_shell_instance_mode_multiple) {
        window = FindWindowA("Halo", "Halo");
        if (window != 0) {
            placement.length = 0x2c;
            GetWindowPlacement(window, &placement);
            SetForegroundWindow(window);
            if (placement.show_command == 2 /* SW_SHOWMINIMIZED */) {
                ShowWindow(window, 9 /* SW_RESTORE */);
            }
            _exit(1);
        }
    }
    shell_display_fatal_error_dialog(0x92, (uint32_t)((const char *)0x7e), 1);
}

#if 0
Original Ghidra decompilation (0x542d70):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_single_instance_check(int mode)

{
  DWORD DVar1;
  LPCSTR lpName;
  DWORD DVar2;
  HWND hWnd;
  DWORD local_c8;
  int local_c4;
  WINDOWPLACEMENT local_c0;
  _OSVERSIONINFOA local_94;
  
  _DAT_0069eab4 = mode;
  DAT_00721f04 = -1;
  local_94.dwOSVersionInfoSize = 0x94;
  GetVersionExA(&local_94);
  if (mode == 1) {
    local_c8 = mode;
    local_c4 = 8;
    DVar1 = 1;
    DVar2 = 1;
  }
  else {
    local_c8 = 0;
    local_c4 = 0;
    DVar1 = local_c8;
    DVar2 = local_c8;
  }
  for (; (int)DVar1 <= local_c4; DVar1 = DVar1 + 1) {
    lpName = (&PTR_s_Global__cff2f1a0_500e_4fae_a789__0069eab8)[DVar1];
    if (local_94.dwMajorVersion < 5) {
      lpName = lpName + 7;
    }
    DAT_00721f00 = CreateMutexA((LPSECURITY_ATTRIBUTES)0x0,1,lpName);
    DVar2 = GetLastError();
    if (DVar2 == 0xb7) {
      if (DAT_00721f00 != (HANDLE)0x0) {
        CloseHandle(DAT_00721f00);
      }
    }
    else if (DAT_00721f00 != (HANDLE)0x0) {
      DAT_00721f04 = DVar1 - local_c8;
      break;
    }
  }
  if (DVar2 != 0xb7) {
    if (DAT_00721f00 == (HANDLE)0x0) goto LAB_00542e89;
    if (DAT_00721f04 != -1) {
      return;
    }
  }
  if (DAT_00721f00 != (HANDLE)0x0) {
    CloseHandle(DAT_00721f00);
    DAT_00721f00 = (HANDLE)0x0;
    _DAT_0069eab4 = -1;
    DAT_00721f04 = -1;
  }
LAB_00542e89:
  if ((mode != 1) && (hWnd = FindWindowA("Halo","Halo"), hWnd != (HWND)0x0)) {
    local_c0.length = 0x2c;
    GetWindowPlacement(hWnd,&local_c0);
    SetForegroundWindow(hWnd);
    if (local_c0.showCmd == 2) {
      ShowWindow(hWnd,9);
    }
                    /* WARNING: Subroutine does not return */
    _exit(1);
  }
  shell_display_fatal_error_dialog(0x92,0x7e,1);
  return;
}
#endif

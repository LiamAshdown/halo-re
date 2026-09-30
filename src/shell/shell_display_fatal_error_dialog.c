// shell_display_fatal_error_dialog  (Ghidra: shell_display_fatal_error_dialog, already named)
// address 0x57ea70, size 1300 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Builds and displays
//   Halo's fatal-error dialog with a formatted error/exception message, updates crash-tracking
//   registry state, and terminates the process." Every global matches types/shell.h's fatal_
//   error_* / graphics_* field list; the "already showed this exact error, skip it" registry
//   check under HKCU\...\Halo keyed by "%s %s (0x%04x):%d" (vendor, device, device id, resource
//   id) is confirmed twice (once silent, once writing) by objdump.
// register convention: this function's 3 parameters (resource_id, help_text_id_or_string,
//   is_fatal) are already ordinary recognized stack parameters in Ghidra's own decompile;
//   objdump confirms every use. dialog_box_show_localized 0x57e1f0 (a "dialogs" module function,
//   not in the function list) takes its DLGPROC in EBX and hInstance in ESI, with the dialog
//   resource id and parent HWND on the stack.
// blam-cc: resource_id, help_text (string when resource_id == -1, else a second string-resource
//   id), is_fatal (all three on the stack, matching Ghidra's recognized parameters).
// Review fix: the shutdown calls before ExitProcess (chimera__registry_check_3 0x5226c0, the
//   rasterizer's 0x5180d0, sound_stop_all 0x54adb0, keystone_library_unload) run inside
//   __try / __except(EXCEPTION_EXECUTE_HANDLER) (scope table 0x00672f00, filter 0x57ef78), so a
//   fault while tearing down still reaches ExitProcess(1). Callee names now match the other modules.
// Review note: shell_load_string_resource 0x57e110 is called with the id in EAX, the full language
//   dword in ECX, the capacity in EBX, the module in EDI and the buffer on the stack.
// UNSURE: the byte-by-byte " (" / ")" splice around rasterizer_shader_file_name is Ghidra's decompile of
//   an inlined strcat; rewritten here as three explicit strcat calls with identical observable
//   effect (fatal_error_text = fatal_error_text + " (" + rasterizer_shader_file_name + ")").
// global 0x00722bc0 fatal_error_remember_choice is now in types/shell.h's globals list (R15):
//   written by the dialog proc 0x57e5a0, read here at 0x57ee7c.
// reconciled: R15 0x00722bc0 fatal_error_remember_choice added to shell.h as int32_t; extern retyped uint32 -> int32

#include "win32.h"
#include "tags.h"
#include "dialogs.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "interface.h"
#include "fn_rasterizer.h"
#include "fn_sound.h"
#include "fn_dialogs.h"

extern int32_t shell_load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module,
                                           char *buffer); // 0x57e110
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
extern char *strcat(char *dst, const char *src);                // CRT, statically linked
extern void shell_registry_set_exit_flag_clean(void); // 0x57ea10

                                                                 // in ESI, dialog_id/parent on the stack; "dialogs"
                                                                 // module function, not in the function list


extern void keystone_library_unload(void); // 0x542cf0, below this module's rewrite range

extern void *shell_module_handle;               // 0x00722bb8
extern char fatal_error_text[k_shell_fatal_error_text_length];       // 0x006effe8
extern char fatal_error_help_file[k_shell_fatal_error_readme_length]; // 0x006f0470
extern char fatal_error_title[k_shell_fatal_error_title_length];     // 0x006f03f0
extern int32_t fatal_error_is_fatal;       // 0x006f03ec
extern char *rasterizer_shader_file_name;         // 0x00722bbc
extern char *graphics_vendor_name;         // 0x00722b90
extern char *graphics_device_name;         // 0x00722b94
extern uint32_t graphics_device_id;        // 0x00722b98
extern void *shell_window;                 // 0x007461c4
extern void *shell_instance;               // 0x007461c0
extern uint32_t shell_language_id;         // 0x0069ff20
extern uint8_t shell_window_proc_bypass;   // 0x00721e8d
extern int32_t safe_mode;                  // 0x007196f4 (32 bit BOOL)
extern int32_t fatal_error_remember_choice; // 0x00722bc0, types/shell.h (R15)


                                                                    // "dialogs" module function, not in the
                                                                    // function list -- referenced only as an
                                                                    // opaque callback pointer

// Builds and displays Halo's fatal-error dialog. `resource_id` of 0xffffffff means `help_text` is
// a raw C string to copy directly into fatal_error_text (and the help file defaults to
// "readme.rtf"); any other `resource_id` loads fatal_error_text from that string resource (with a
// "Missing error string %d" fallback) and treats `help_text` as a SECOND string-resource id for
// the help file name. `is_fatal` selects the dialog title ("...Continue?" vs "...Fatal Error")
// and, when true, skips the "remember my choice" registry short-circuit and always terminates the
// process afterward (running engine shutdown and ExitProcess). Returns the user's dialog result
// (0/1/2), or a remembered 0/1 answer read back from the registry without showing anything.
// FIXED: the second argument is dual-use (0x57eaa1 reads it as the text to copy when resource_id is -1, otherwise it is
// the help-file string's resource id); every caller passes the id form, so it is declared as an integer here
int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    const char *help_text = (const char *)help_text_or_id;
    void *module;
    int32_t loaded;
    win32_wndclassexa wndclass;
    void *window;
    int32_t result;
    char registry_value_name[256];
    void *key;
    uint32_t data_size;
    uint8_t remembered[16];
    char digit_text[16];
    uint32_t digit_length;

    if (resource_id == 0xffffffff) {
        const char *source = help_text;
        char *dest = fatal_error_text;
        do {
            *dest++ = *source;
        } while (*source++ != 0);
        sprintf(fatal_error_help_file, "readme.rtf");
    } else {
        module = shell_module_handle;
        loaded = shell_load_string_resource(resource_id, (uint16_t)shell_language_id, k_shell_fatal_error_text_length,
                                             module, fatal_error_text);
        if (loaded == 0 &&
            (shell_language_id == k_shell_language_default ||
             (loaded = shell_load_string_resource(resource_id, k_shell_language_default,
                                                   k_shell_fatal_error_text_length, module, fatal_error_text),
              loaded == 0)) &&
            (loaded = LoadStringA(module, resource_id, fatal_error_text, k_shell_fatal_error_text_length),
             loaded == 0)) {
            sprintf(fatal_error_text, "Missing error string %d", resource_id);
        }

        module = shell_module_handle;
        loaded = shell_load_string_resource((uint32_t)help_text, (uint16_t)shell_language_id,
                                             k_shell_fatal_error_readme_length, module, fatal_error_help_file);
        if (loaded == 0 &&
            (shell_language_id == k_shell_language_default ||
             (loaded = shell_load_string_resource((uint32_t)help_text, k_shell_language_default,
                                                   k_shell_fatal_error_readme_length, module, fatal_error_help_file),
              loaded == 0)) &&
            (loaded = LoadStringA(module, (uint32_t)help_text, fatal_error_help_file,
                                   k_shell_fatal_error_readme_length),
             loaded == 0)) {
            sprintf(fatal_error_help_file, "readme.rtf");
        }
    }

    module = shell_module_handle;
    loaded = shell_load_string_resource(0x7f + (is_fatal != 0), (uint16_t)shell_language_id,
                                         k_shell_fatal_error_title_length, module, fatal_error_title);
    if (loaded == 0 &&
        (shell_language_id == k_shell_language_default ||
         (loaded = shell_load_string_resource(0x7f + (is_fatal != 0), k_shell_language_default,
                                               k_shell_fatal_error_title_length, module, fatal_error_title),
          loaded == 0)) &&
        (loaded = LoadStringA(module, 0x7f + (is_fatal != 0), fatal_error_title, k_shell_fatal_error_title_length),
         loaded == 0)) {
        sprintf(fatal_error_title, "Halo - Error");
    }

    fatal_error_is_fatal = is_fatal;

    if (rasterizer_shader_file_name != 0) {
        strcat(fatal_error_text, " (");
        strcat(fatal_error_text, rasterizer_shader_file_name);
        strcat(fatal_error_text, ")");
        rasterizer_shader_file_name = 0;
    }

    if (is_fatal == 0) {
        sprintf(registry_value_name, "%s %s (0x%04x):%d", graphics_vendor_name, graphics_device_name,
                graphics_device_id, resource_id);
        RegOpenKeyExA((void *)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                      0x20019, (PHKEY)&key);
        data_size = 0x10;
        remembered[0] = 0;
        RegQueryValueExA(key, registry_value_name, 0, 0, remembered, &data_size);
        RegCloseKey(key);
        if (remembered[0] == 'y') {
            result = remembered[1] - '0';
            safe_mode = result;
            return result;
        }
    }

    window = shell_window;
    if (shell_window == 0) {
        wndclass.size = 0;
        wndclass.style = 0;
        wndclass.window_procedure = 0;
        wndclass.class_extra = 0;
        wndclass.window_extra = 0;
        wndclass.instance = 0;
        wndclass.icon = 0;
        wndclass.cursor = 0;
        wndclass.background_brush = 0;
        wndclass.menu_name = 0;
        wndclass.class_name = 0;
        wndclass.small_icon = 0;
        wndclass.size = 0x30;
        wndclass.window_procedure = (uint32_t)DefWindowProcA;
        wndclass.instance = (uint32_t)shell_instance;
        wndclass.icon = (uint32_t)LoadIconA(shell_instance, (const char *)0x66);
        wndclass.cursor = (uint32_t)LoadCursorA(0, (const char *)0x7f00);
        wndclass.class_name = (uint32_t)"Halo";
        RegisterClassExA((const WNDCLASSEXA *)&wndclass);
        window = CreateWindowExA(0, "Halo", "Halo", 0x80000000, -0x80000000, -0x80000000, -0x80000000, -0x80000000,
                                  0, 0, shell_instance, 0);
        ShowWindow(window, 5);
    }

    ShowCursor(1);
    result = dialog_box_show_localized((dialog_window_proc_fn)fatal_error_dialog_proc, shell_module_handle, (const char *)0x66, window); // 0x57ee1d
    ShowCursor(0);

    if (is_fatal != 0 || result == 2) {
        shell_registry_set_exit_flag_clean();
#if defined(_MSC_VER)
        __try {
#endif
            shell_window_proc_bypass = 1;
            chimera__registry_check_3();
            rasterizer_service_deferred_windowed_ops();
            sound_stop_all();
            keystone_library_unload();
#if defined(_MSC_VER)
        } __except (1) {
        }
#endif
        ExitProcess(1);
    }

    if (shell_window == 0) {
        DestroyWindow(window);
        UnregisterClassA("Halo", shell_instance);
    } else {
        ShowWindow(shell_window, 5);
    }

    if (fatal_error_remember_choice != 0) {
        sprintf(registry_value_name, "%s %s (0x%04x):%d", graphics_vendor_name, graphics_device_name,
                graphics_device_id, resource_id);
        RegCreateKeyExA((void *)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                         0, 0x20006, 0, 0, (PHKEY)&key, 0);
        digit_text[0] = 'y';
        digit_text[1] = (char)(result + '0');
        digit_text[2] = 0;
        digit_length = 0;
        while (digit_text[digit_length] != 0) {
            digit_length++;
        }
        RegSetValueExA(key, registry_value_name, 0, 1 /* REG_SZ */, (const uint8_t *)digit_text, digit_length + 1);
        RegCloseKey(key);
        if (digit_text[0] != 0) {
            result = digit_text[1] - '0';
            safe_mode = result;
            return result;
        }
    }

    if (result != 0) {
        safe_mode = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x57ea70):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int shell_display_fatal_error_dialog(UINT param_1,char *param_2,int param_3)

{
  char cVar1;
  BYTE BVar2;
  HINSTANCE pHVar3;
  char *pcVar4;
  HWND hWnd;
  BYTE *pBVar5;
  uint uVar6;
  int iVar7;
  char *pcVar8;
  undefined2 *puVar9;
  char *pcVar10;
  undefined2 *puVar11;
  WNDCLASSEXA *pWVar12;
  char local_27c [256];
  char local_17c [256];
  WNDCLASSEXA local_7c;
  BYTE local_4c;
  char local_4b;
  BYTE local_3c [16];
  DWORD local_2c;
  HINSTANCE local_28;
  HKEY local_24;
  HKEY local_20;
  undefined1 *local_1c;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;

  pHVar3 = DAT_00722bb8;
  local_8 = 0xffffffff;
  puStack_c = &DAT_00672f00;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  local_1c = &stack0xfffffd78;
  if (param_1 == 0xffffffff) {
    iVar7 = (int)&DAT_006effe8 - (int)param_2;
    ExceptionList = &local_14;
    do {
      cVar1 = *param_2;
      param_2[iVar7] = cVar1;
      param_2 = param_2 + 1;
    } while (cVar1 != '\0');
    DAT_006f0470 = 'r';
    DAT_006f0470_1._0_1_ = 'e';
    DAT_006f0470_1._1_1_ = 'a';
    DAT_006f0470_1._2_1_ = 'd';
    DAT_006f0474 = 'm';
    DAT_006f0474_1._0_1_ = 'e';
    DAT_006f0474_1._1_1_ = '.';
    DAT_006f0474_1._2_1_ = 'r';
    _DAT_006f0478 = 0x6674;
    DAT_006f047a = 0;
    local_1c = &stack0xfffffd78;
  }
  else {
    ExceptionList = &local_14;
    iVar7 = shell_load_string_resource(&DAT_006effe8);
    if ((iVar7 == 0) &&
       (((DAT_0069ff20 == 0x409 || (iVar7 = shell_load_string_resource(&DAT_006effe8), iVar7 == 0))
        && (iVar7 = LoadStringA(pHVar3,param_1,(LPSTR)&DAT_006effe8,0x400), iVar7 == 0)))) {
      _sprintf((char *)&DAT_006effe8,"Missing error string %d",param_1);
    }
    pHVar3 = DAT_00722bb8;
    iVar7 = shell_load_string_resource(&DAT_006f0470);
    if (((iVar7 == 0) &&
        ((DAT_0069ff20 == 0x409 || (iVar7 = shell_load_string_resource(&DAT_006f0470), iVar7 == 0)))
        ) && (iVar7 = LoadStringA(pHVar3,(UINT)param_2,&DAT_006f0470,0x200), iVar7 == 0)) {
      _sprintf(&DAT_006f0470,"readme.rtf");
    }
  }
  local_28 = DAT_00722bb8;
  iVar7 = shell_load_string_resource(&DAT_006f03f0);
  if ((iVar7 == 0) &&
     ((DAT_0069ff20 == 0x409 || (iVar7 = shell_load_string_resource(&DAT_006f03f0), iVar7 == 0)))) {
    iVar7 = LoadStringA(local_28,(param_3 != 0) + 0x7f,&DAT_006f03f0,0x80);
  }
  if (iVar7 == 0) {
    _sprintf(&DAT_006f03f0,"Halo - Error");
  }
  DAT_006f03ec = param_3;
  if (DAT_00722bbc != (char *)0x0) {
    puVar11 = (undefined2 *)0x6effe7;
    do {
      puVar9 = puVar11;
      puVar11 = (undefined2 *)((int)puVar9 + 1);
    } while (*(char *)((int)puVar9 + 1) != '\0');
    *(undefined2 *)((int)puVar9 + 1) = 0x2820;
    *(undefined1 *)((int)puVar9 + 3) = 0;
    pcVar4 = DAT_00722bbc;
    do {
      cVar1 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar1 != '\0');
    pcVar10 = (char *)0x6effe7;
    do {
      pcVar8 = pcVar10 + 1;
      pcVar10 = pcVar10 + 1;
    } while (*pcVar8 != '\0');
    pcVar8 = DAT_00722bbc;
    for (uVar6 = (uint)((int)pcVar4 - (int)DAT_00722bbc) >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pcVar10 = *(undefined4 *)pcVar8;
      pcVar8 = pcVar8 + 4;
      pcVar10 = pcVar10 + 4;
    }
    for (uVar6 = (int)pcVar4 - (int)DAT_00722bbc & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar10 = *pcVar8;
      pcVar8 = pcVar8 + 1;
      pcVar10 = pcVar10 + 1;
    }
    puVar11 = (undefined2 *)0x6effe7;
    do {
      pcVar4 = (char *)((int)puVar11 + 1);
      puVar11 = (undefined2 *)((int)puVar11 + 1);
    } while (*pcVar4 != '\0');
    *puVar11 = 0x29;
    DAT_00722bbc = (char *)0x0;
  }
  if (param_3 == 0) {
    _sprintf(local_17c,"%s %s (0x%04x):%d",DAT_00722b90,DAT_00722b94,DAT_00722b98,param_1);
    RegOpenKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                  &local_20);
    local_2c = 0x10;
    local_4c = '\0';
    RegQueryValueExA(local_20,local_17c,(LPDWORD)0x0,(LPDWORD)0x0,&local_4c,&local_2c);
    RegCloseKey(local_20);
    local_3c[1] = local_4b;
    if (local_4c == 'y') goto LAB_0057ed5f;
  }
  hWnd = DAT_007461c4;
  if (DAT_007461c4 == (HWND)0x0) {
    pWVar12 = &local_7c;
    for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
      pWVar12->cbSize = 0;
      pWVar12 = (WNDCLASSEXA *)&pWVar12->style;
    }
    local_7c.cbSize = 0x30;
    local_7c.lpfnWndProc = DefWindowProcA_exref;
    local_7c.hInstance = DAT_007461c0;
    local_7c.hIcon = LoadIconA(DAT_007461c0,&DAT_00000066);
    local_7c.hCursor = LoadCursorA((HINSTANCE)0x0,&DAT_00007f00);
    local_7c.lpszClassName = "Halo";
    RegisterClassExA(&local_7c);
    hWnd = CreateWindowExA(0,"Halo","Halo",0x80000000,-0x80000000,-0x80000000,-0x80000000,
                           -0x80000000,(HWND)0x0,(HMENU)0x0,DAT_007461c0,(LPVOID)0x0);
    ShowWindow(hWnd,5);
  }
  ShowCursor(1);
  iVar7 = dialog_box_show_localized(0x66,hWnd);
  ShowCursor(0);
  if ((param_3 != 0) || (iVar7 == 2)) {
    shell_registry_set_exit_flag_clean();
    local_8 = 0;
    DAT_00721e8d = 1;
    chimera__registry_check_3();
    FUN_005180d0();
    FUN_0054adb0();
    keystone_library_unload();
    local_8 = 0xffffffff;
                    /* WARNING: Subroutine does not return */
    ExitProcess(1);
  }
  if (DAT_007461c4 == (HWND)0x0) {
    DestroyWindow(hWnd);
    UnregisterClassA("Halo",DAT_007461c0);
  }
  else {
    ShowWindow(DAT_007461c4,5);
  }
  if (DAT_00722bc0 != 0) {
    _sprintf(local_27c,"%s %s (0x%04x):%d",DAT_00722b90,DAT_00722b94,DAT_00722b98,param_1);
    RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                    0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_24,(LPDWORD)0x0);
    local_3c[0] = 'y';
    local_3c[1] = (char)iVar7 + '0';
    local_3c[2] = 0;
    pBVar5 = local_3c;
    do {
      BVar2 = *pBVar5;
      pBVar5 = pBVar5 + 1;
    } while (BVar2 != '\0');
    RegSetValueExA(local_24,local_27c,0,1,local_3c,(DWORD)(pBVar5 + (1 - (int)(local_3c + 1))));
    RegCloseKey(local_24);
    if (local_3c[0] != '\0') {
LAB_0057ed5f:
      DAT_007196f4 = (char)local_3c[1] + -0x30;
      ExceptionList = local_14;
      return (char)local_3c[1] + -0x30;
    }
  }
  if (iVar7 != 0) {
    DAT_007196f4 = 1;
  }
  ExceptionList = local_14;
  return iVar7;
}
#endif

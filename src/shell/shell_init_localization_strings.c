// shell_init_localization_strings  (Ghidra: shell_init_localization_strings, already named)
// address 0x57efa0, size 765 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Loads strings.dll
//   (fatally erroring out if missing), determines the active language from the registry, and
//   pre-loads a set of localized diagnostic/UI strings with built-in fallbacks." Every default
//   string ("Exception!", "Gathering Exception Data...", "eula.rtf", "Invalid / missing
//   strings.dll") and every global address matches types/shell.h's field list (string ids
//   0x77/0x78/0x84/0x88 documented there).
// register convention: plain __cdecl, no parameters. Each shell_load_string_resource call uses
//   the same register convention established in shell_display_fatal_error_dialog.c (id in EAX,
//   language in CX, buffer size in EBX, module in EDI, buffer on the stack).
// blam-cc: (no arguments)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t shell_load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module,
                                           char *buffer); // 0x57e110
extern char *strcat(char *dst, const char *src); // CRT, statically linked

extern void *shell_module_handle;          // 0x00722bb8
extern uint32_t shell_language_id;    // 0x0069ff20
extern char exception_title[0x100];             // 0x006f0770, string 0x77, "Exception!"
extern char exception_gathering_text[0x100];    // 0x006f0670, string 0x78, "Gathering Exception Data..."
extern char eula_file_name[k_shell_eula_name_length]; // 0x006f0870, string 0x84, "eula.rtf"
extern char strings_dll_invalid_text[k_shell_strings_dll_error_length]; // 0x006f0890, string 0x88
extern uint32_t shell_startup_tick_count; // 0x006f03e8

// Loads strings.dll (fatally MessageBox-and-exits if it's missing), determines the active
// language from HKCU\...\Halo\LangID (falling back to en-US, 0x409, if it isn't set or its
// sublanguage is unspecified), and pre-loads four localized diagnostic/UI strings -- each tried
// first in the active language, then in English, then via the plain Win32 LoadStringA, and
// finally falling back to a hardcoded default if every attempt fails. Records the tick count at
// which this finished into shell_startup_tick_count.
void shell_init_localization_strings(void)
{
    char current_directory[260];
    void *key;
    uint32_t value_type;
    uint32_t language_id;
    uint32_t value_size;
    int32_t loaded;

    shell_module_handle = LoadLibraryA("strings.dll");
    if (shell_module_handle == 0) {
        GetCurrentDirectoryA(0x104, current_directory);
        strcat(current_directory, "\\strings.dll is missing.");
        MessageBoxA(0, current_directory, "Error!", 0);
        ExitProcess(1);
    }

    shell_language_id = k_shell_language_default;
    if (RegOpenKeyExA((void *)0x80000002 /* HKEY_LOCAL_MACHINE */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                      0x20019, (PHKEY)&key) == 0) {
        value_type = 4;
        value_size = 4;
        if (RegQueryValueExA(key, "LangID", 0, &value_type, (uint8_t *)&language_id, &value_size) == 0) {
            if ((language_id & 0xfc00) == 0) {
                shell_language_id = (language_id & 0xffff) | 0x400;
            } else {
                shell_language_id = language_id;
            }
        }
        RegCloseKey(key);
    }

    loaded = shell_load_string_resource(0x77, (uint16_t)shell_language_id, 0x100, shell_module_handle, exception_title);
    if (loaded == 0 &&
        (shell_language_id == k_shell_language_default ||
         (loaded = shell_load_string_resource(0x77, k_shell_language_default, 0x100, shell_module_handle,
                                               exception_title),
          loaded == 0)) &&
        (loaded = LoadStringA(shell_module_handle, 0x77, exception_title, 0x100), loaded == 0)) {
        exception_title[0] = 0;
        strcat(exception_title, "Exception!");
    }

    loaded = shell_load_string_resource(0x78, (uint16_t)shell_language_id, 0x100, shell_module_handle,
                                         exception_gathering_text);
    if (loaded == 0 &&
        (shell_language_id == k_shell_language_default ||
         (loaded = shell_load_string_resource(0x78, k_shell_language_default, 0x100, shell_module_handle,
                                               exception_gathering_text),
          loaded == 0)) &&
        (loaded = LoadStringA(shell_module_handle, 0x78, exception_gathering_text, 0x100), loaded == 0)) {
        exception_gathering_text[0] = 0;
        strcat(exception_gathering_text, "Gathering Exception Data...");
    }

    loaded = shell_load_string_resource(0x84, (uint16_t)shell_language_id, k_shell_eula_name_length, shell_module_handle,
                                         eula_file_name);
    if (loaded == 0 &&
        (shell_language_id == k_shell_language_default ||
         (loaded = shell_load_string_resource(0x84, k_shell_language_default, k_shell_eula_name_length,
                                               shell_module_handle, eula_file_name),
          loaded == 0)) &&
        (loaded = LoadStringA(shell_module_handle, 0x84, eula_file_name, k_shell_eula_name_length), loaded == 0)) {
        eula_file_name[0] = 0;
        strcat(eula_file_name, "eula.rtf");
    }

    loaded = shell_load_string_resource(0x88, (uint16_t)shell_language_id, k_shell_strings_dll_error_length,
                                         shell_module_handle, strings_dll_invalid_text);
    if (loaded == 0 &&
        (shell_language_id == k_shell_language_default ||
         (loaded = shell_load_string_resource(0x88, k_shell_language_default, k_shell_strings_dll_error_length,
                                               shell_module_handle, strings_dll_invalid_text),
          loaded == 0))) {
        loaded = LoadStringA(shell_module_handle, 0x88, strings_dll_invalid_text, k_shell_strings_dll_error_length);
    }
    if (loaded == 0) {
        strings_dll_invalid_text[0] = 0;
        strcat(strings_dll_invalid_text, "Invalid / missing strings.dll");
    }

    shell_startup_tick_count = GetTickCount();
}

#if 0
Original Ghidra decompilation (0x57efa0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void shell_init_localization_strings(void)

{
  HMODULE pHVar1;
  LSTATUS LVar2;
  int iVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 *puVar6;
  HKEY local_114;
  DWORD local_110;
  uint local_10c;
  undefined4 local_108;
  CHAR local_104 [260];

  DAT_00722bb8 = LoadLibraryA("strings.dll");
  if (DAT_00722bb8 == (HMODULE)0x0) {
    GetCurrentDirectoryA(0x104,local_104);
    pcVar5 = (char *)((int)&local_108 + 3);
    do {
      pcVar4 = pcVar5 + 1;
      pcVar5 = pcVar5 + 1;
    } while (*pcVar4 != '\0');
    pcVar4 = "\\strings.dll is missing.";
    for (iVar3 = 6; iVar3 != 0; iVar3 = iVar3 + -1) {
      *(undefined4 *)pcVar5 = *(undefined4 *)pcVar4;
      pcVar4 = pcVar4 + 4;
      pcVar5 = pcVar5 + 4;
    }
    *pcVar5 = *pcVar4;
    MessageBoxA((HWND)0x0,local_104,"Error!",0);
                    /* WARNING: Subroutine does not return */
    ExitProcess(1);
  }
  DAT_0069ff20 = 0x409;
  LVar2 = RegOpenKeyExA((HKEY)&DAT_80000002,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                        &local_114);
  if (LVar2 == 0) {
    local_110 = 4;
    local_108 = 4;
    LVar2 = RegQueryValueExA(local_114,"LangID",(LPDWORD)0x0,&local_110,(LPBYTE)&local_10c,
                             &local_108);
    if (LVar2 == 0) {
      if ((local_10c & 0xfc00) == 0) {
        DAT_0069ff20 = local_10c & 0xffff | 0x400;
      }
      else {
        DAT_0069ff20 = local_10c;
      }
    }
    RegCloseKey(local_114);
  }
  pHVar1 = DAT_00722bb8;
  iVar3 = shell_load_string_resource(&DAT_006f0770);
  if (iVar3 == 0) {
    if (DAT_0069ff20 != 0x409) {
      iVar3 = shell_load_string_resource(&DAT_006f0770);
      if (iVar3 != 0) goto LAB_0057f11e;
    }
    iVar3 = LoadStringA(pHVar1,0x77,&DAT_006f0770,0x100);
    if (iVar3 == 0) {
      DAT_006f0770 = 'E';
      DAT_006f0770_1._0_1_ = 'x';
      DAT_006f0770_1._1_1_ = 'c';
      DAT_006f0770_1._2_1_ = 'e';
      DAT_006f0774 = 'p';
      DAT_006f0774_1._0_1_ = 't';
      DAT_006f0774_1._1_1_ = 'i';
      DAT_006f0774_1._2_1_ = 'o';
      _DAT_006f0778 = 0x216e;
      DAT_006f077a = 0;
    }
  }
LAB_0057f11e:
  pHVar1 = DAT_00722bb8;
  iVar3 = shell_load_string_resource(&DAT_006f0670);
  if (iVar3 == 0) {
    if (DAT_0069ff20 != 0x409) {
      iVar3 = shell_load_string_resource(&DAT_006f0670);
      if (iVar3 != 0) goto LAB_0057f18f;
    }
    iVar3 = LoadStringA(pHVar1,0x78,(LPSTR)&DAT_006f0670,0x100);
    if (iVar3 == 0) {
      pcVar5 = "Gathering Exception Data...";
      puVar6 = &DAT_006f0670;
      for (iVar3 = 7; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *(undefined4 *)pcVar5;
        pcVar5 = pcVar5 + 4;
        puVar6 = puVar6 + 1;
      }
    }
  }
LAB_0057f18f:
  pHVar1 = DAT_00722bb8;
  iVar3 = shell_load_string_resource(&DAT_006f0870);
  if (iVar3 == 0) {
    if (DAT_0069ff20 != 0x409) {
      iVar3 = shell_load_string_resource(&DAT_006f0870);
      if (iVar3 != 0) goto LAB_0057f211;
    }
    iVar3 = LoadStringA(pHVar1,0x84,&DAT_006f0870,0x20);
    if (iVar3 == 0) {
      _DAT_006f0870 = 0x616c7565;
      _DAT_006f0874 = 0x6674722e;
      DAT_006f0878 = 0;
    }
  }
LAB_0057f211:
  pHVar1 = DAT_00722bb8;
  iVar3 = shell_load_string_resource(&DAT_006f0890);
  if (iVar3 == 0) {
    if (DAT_0069ff20 != 0x409) {
      iVar3 = shell_load_string_resource(&DAT_006f0890);
      if (iVar3 != 0) goto LAB_0057f270;
    }
    iVar3 = LoadStringA(pHVar1,0x88,(LPSTR)&DAT_006f0890,0x400);
  }
LAB_0057f270:
  if (iVar3 == 0) {
    pcVar5 = "Invalid / missing strings.dll";
    puVar6 = &DAT_006f0890;
    for (iVar3 = 7; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *(undefined4 *)pcVar5;
      pcVar5 = pcVar5 + 4;
      puVar6 = puVar6 + 1;
    }
    *(undefined2 *)puVar6 = *(undefined2 *)pcVar5;
  }
  DAT_006f03e8 = GetTickCount();
  return;
}
#endif

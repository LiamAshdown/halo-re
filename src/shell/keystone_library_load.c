// keystone_library_load  (Ghidra: keystone_library_load, already named)
// address 0x542ad0, size 536 bytes
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: LoadLibraryA("keystone.dll") followed by GetProcAddress of every Call_Ks*/Call_KW*/
// Call_KC* export, matching every keystone_* function pointer global shell.h documents at
// 0x00721ea0..0x00721edc.
// register convention: __cdecl, no arguments, no return value.
// UNSURE: DAT_00670f90 (2 bytes) and PTR_DAT_00670f8c (a wsprintf format string) were not
// recovered as literals; by usage they are almost certainly the default C-locale name "C" and a
// ".%d" codepage format, matching the standard "switch off the C locale before mbstowcs" idiom.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "interface.h"
#include "shell.h"

extern uint8_t default_locale_name[2]; // 0x00670f90, UNSURE: "C"
extern char *locale_codepage_format;   // 0x00670f8c, UNSURE: ".%d"

extern void *keystone_module;                 // 0x00721e9c
extern keystone_create_fn keystone_create;                                // 0x00721ea0
extern keystone_translate_accelerator_fn keystone_translate_accelerator;  // 0x00721eb0
extern keystone_create_window_fn keystone_create_window;                  // 0x00721eb4
extern chat_gui_find_object_fn keystone_get_window;                        // 0x00721eb8
extern keystone_update_fn keystone_update;                                  // 0x00721ebc
extern keystone_dispatch_message_fn keystone_dispatch_message;               // 0x00721ec0
extern chat_gui_release_fn keystone_window_release;                          // 0x00721ec8
extern keystone_unknown_fn keystone_set_focus_window;                        // 0x00721ec4
extern chat_gui_find_child_fn keystone_window_get_control;                   // 0x00721ecc
extern chat_gui_get_property_string_fn keystone_control_get_attribute;       // 0x00721ee0
extern chat_gui_set_property_string_fn keystone_control_set_attribute;       // 0x00721ee4
extern chat_gui_set_property_int_fn keystone_control_send_message;           // 0x00721ee8
extern chat_gui_finalize_fn keystone_window_relayout;                        // 0x00721ed0
extern chat_gui_set_focus_fn keystone_window_set_focus_control;              // 0x00721ed4
extern keystone_unknown_fn keystone_window_add_dirty_control;                // 0x00721ed8
extern keystone_release_fn keystone_release;                                 // 0x00721eac
extern chat_gui_set_state_fn keystone_window_show;                           // 0x00721edc

extern uint16_t *keystone_current_directory; // 0x00721ea8
extern int32_t safe_mode;                    // 0x007196f4 (32 bit BOOL)

extern char *_setlocale(int32_t category, const char *locale);
extern uint32_t _mbstowcs(uint16_t *dest, const char *src, uint32_t count);

// Loads the Keystone UI middleware DLL and resolves all of its Call_Ks*/Call_KW*/Call_KC*
// entry points into globals, unless networking-only mode (safe_mode) disables the UI.
void keystone_library_load(void)
{
    uint8_t *current_locale;
    int32_t compare;
    int32_t i;
    uint8_t less_than;
    uint8_t equal;
    char codepage_locale[16];
    uint32_t current_directory_size;
    char *current_directory;
    uint32_t wide_length;

    current_locale = (uint8_t *)_setlocale(2 /* LC_CTYPE */, 0);
    compare = 0;
    less_than = 0;
    equal = 1;
    for (i = 0; i < 2; i++) {
        less_than = default_locale_name[i] < current_locale[i];
        equal = default_locale_name[i] == current_locale[i];
        if (!equal) break;
    }
    if (!equal) {
        compare = (1 - less_than) - (less_than != 0);
    }
    if (compare == 0) {
        wsprintfA(codepage_locale, locale_codepage_format, GetACP());
        _setlocale(2, codepage_locale);
    }

    current_directory_size = GetCurrentDirectoryA(0, 0);
    current_directory = (char *)GlobalAlloc(0, current_directory_size);
    GetCurrentDirectoryA(current_directory_size, current_directory);

    wide_length = _mbstowcs(0, current_directory, 0);
    keystone_current_directory = (uint16_t *)GlobalAlloc(0, (wide_length + 1) * 2);
    _mbstowcs(keystone_current_directory, current_directory, wide_length + 1);
    GlobalFree(current_directory);

    if (safe_mode != 0) {
        keystone_module = 0;
        return;
    }

    keystone_module = LoadLibraryA("keystone.dll");
    if (keystone_module != 0) {
        keystone_create = (keystone_create_fn)GetProcAddress(keystone_module, "KeystoneCreate");
        keystone_translate_accelerator = (keystone_translate_accelerator_fn)GetProcAddress(
            keystone_module, "Call_KsTranslateAccelerator");
        keystone_create_window = (keystone_create_window_fn)GetProcAddress(keystone_module, "Call_KsCreateWindow");
        keystone_get_window = (chat_gui_find_object_fn)GetProcAddress(keystone_module, "Call_KsGetWindow");
        keystone_update = (keystone_update_fn)GetProcAddress(keystone_module, "Call_KsUpdate");
        keystone_dispatch_message = (keystone_dispatch_message_fn)GetProcAddress(keystone_module,
                                                                                  "Call_KsDispatchMessage");
        keystone_window_release = (chat_gui_release_fn)GetProcAddress(keystone_module, "Call_KW_Release");
        keystone_set_focus_window = (keystone_unknown_fn)GetProcAddress(keystone_module, "Call_KsSetFocusWindow");
        keystone_window_get_control = (chat_gui_find_child_fn)GetProcAddress(keystone_module,
                                                                              "Call_KW_GetControlByID");
        keystone_control_get_attribute = (chat_gui_get_property_string_fn)GetProcAddress(keystone_module,
                                                                                           "Call_KC_GetAttribute");
        keystone_control_set_attribute = (chat_gui_set_property_string_fn)GetProcAddress(keystone_module,
                                                                                           "Call_KC_SetAttribute");
        keystone_control_send_message = (chat_gui_set_property_int_fn)GetProcAddress(keystone_module,
                                                                                       "Call_KC_SendMessage");
        keystone_window_relayout = (chat_gui_finalize_fn)GetProcAddress(keystone_module, "Call_KW_ReLayout");
        keystone_window_set_focus_control = (chat_gui_set_focus_fn)GetProcAddress(keystone_module,
                                                                                    "Call_KW_SetFocusControl");
        keystone_window_add_dirty_control = (keystone_unknown_fn)GetProcAddress(keystone_module,
                                                                                  "Call_KW_AddDirtyControl");
        keystone_release = (keystone_release_fn)GetProcAddress(keystone_module, "Call_KsRelease");
        keystone_window_show = (chat_gui_set_state_fn)GetProcAddress(keystone_module, "Call_KW_ShowWindow");
    }
}

#if 0
Original Ghidra decompilation (0x542ad0):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl keystone_library_load(void)

{
  byte *pbVar1;
  int iVar2;
  UINT UVar3;
  DWORD dwBytes;
  LPSTR lpBuffer;
  size_t sVar4;
  int iVar5;
  byte *pbVar6;
  bool bVar7;
  bool bVar8;
  CHAR local_10 [16];
  
  pbVar1 = (byte *)_setlocale(2,(char *)0x0);
  iVar5 = 2;
  bVar7 = false;
  iVar2 = 0;
  bVar8 = true;
  pbVar6 = &DAT_00670f90;
  do {
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    bVar7 = *pbVar6 < *pbVar1;
    bVar8 = *pbVar6 == *pbVar1;
    pbVar6 = pbVar6 + 1;
    pbVar1 = pbVar1 + 1;
  } while (bVar8);
  if (!bVar8) {
    iVar2 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
  }
  if (iVar2 == 0) {
    UVar3 = GetACP();
    wsprintfA(local_10,(LPCSTR)&PTR_DAT_00670f8c,UVar3);
    _setlocale(2,local_10);
  }
  dwBytes = GetCurrentDirectoryA(0,(LPSTR)0x0);
  lpBuffer = GlobalAlloc(0,dwBytes);
  GetCurrentDirectoryA(dwBytes,lpBuffer);
  sVar4 = _mbstowcs((wchar_t *)0x0,lpBuffer,0);
  DAT_00721ea8 = GlobalAlloc(0,(sVar4 + 1) * 2);
  _mbstowcs(DAT_00721ea8,lpBuffer,sVar4 + 1);
  GlobalFree(lpBuffer);
  if (DAT_007196f4 != 0) {
    DAT_00721e9c = (HMODULE)0x0;
    return;
  }
  DAT_00721e9c = LoadLibraryA("keystone.dll");
  if (DAT_00721e9c != (HMODULE)0x0) {
    DAT_00721ea0 = GetProcAddress(DAT_00721e9c,"KeystoneCreate");
    DAT_00721eb0 = GetProcAddress(DAT_00721e9c,"Call_KsTranslateAccelerator");
    DAT_00721eb4 = GetProcAddress(DAT_00721e9c,"Call_KsCreateWindow");
    DAT_00721eb8 = GetProcAddress(DAT_00721e9c,"Call_KsGetWindow");
    DAT_00721ebc = GetProcAddress(DAT_00721e9c,"Call_KsUpdate");
    DAT_00721ec0 = GetProcAddress(DAT_00721e9c,"Call_KsDispatchMessage");
    DAT_00721ec8 = GetProcAddress(DAT_00721e9c,"Call_KW_Release");
    _DAT_00721ec4 = GetProcAddress(DAT_00721e9c,"Call_KsSetFocusWindow");
    DAT_00721ecc = GetProcAddress(DAT_00721e9c,"Call_KW_GetControlByID");
    DAT_00721ee0 = GetProcAddress(DAT_00721e9c,"Call_KC_GetAttribute");
    DAT_00721ee4 = GetProcAddress(DAT_00721e9c,"Call_KC_SetAttribute");
    DAT_00721ee8 = GetProcAddress(DAT_00721e9c,"Call_KC_SendMessage");
    DAT_00721ed0 = GetProcAddress(DAT_00721e9c,"Call_KW_ReLayout");
    DAT_00721ed4 = GetProcAddress(DAT_00721e9c,"Call_KW_SetFocusControl");
    _DAT_00721ed8 = GetProcAddress(DAT_00721e9c,"Call_KW_AddDirtyControl");
    DAT_00721eac = GetProcAddress(DAT_00721e9c,"Call_KsRelease");
    DAT_00721edc = GetProcAddress(DAT_00721e9c,"Call_KW_ShowWindow");
  }
  return;
}
#endif

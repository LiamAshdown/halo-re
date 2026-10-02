// keystone_library_unload  (Ghidra: keystone_library_unload, already named)
// address 0x542cf0, size 123 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: FreeLibrary of keystone_module plus a full clear of every Keystone function pointer
// keystone_library_load resolves.
// register convention: __cdecl, no arguments, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "interface.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *keystone_module;                 // 0x00721e9c
extern keystone_translate_accelerator_fn keystone_translate_accelerator;  // 0x00721eb0
extern keystone_create_window_fn keystone_create_window;                  // 0x00721eb4
extern chat_gui_find_object_fn chat_gui_find_object;                        // 0x00721eb8
extern keystone_update_fn keystone_update;                                  // 0x00721ebc
extern keystone_dispatch_message_fn keystone_dispatch_message;               // 0x00721ec0
extern chat_gui_release_fn chat_gui_release;                          // 0x00721ec8
extern keystone_unknown_fn keystone_set_focus_window;                        // 0x00721ec4
extern chat_gui_find_child_fn chat_gui_find_child;                   // 0x00721ecc
extern chat_gui_get_property_string_fn keystone_control_get_attribute;       // 0x00721ee0
extern chat_gui_set_property_string_fn keystone_control_set_attribute;       // 0x00721ee4
extern chat_gui_set_property_int_fn chat_gui_set_property_int;           // 0x00721ee8
extern chat_gui_finalize_fn chat_gui_finalize;                        // 0x00721ed0
extern chat_gui_set_focus_fn chat_gui_set_focus;              // 0x00721ed4
extern keystone_unknown_fn keystone_window_add_dirty_control;                // 0x00721ed8
extern keystone_release_fn keystone_release;                                 // 0x00721eac
extern chat_gui_set_state_fn chat_gui_set_state;                           // 0x00721edc


// Unloads keystone.dll and clears all of its cached UI entry-point function pointers.
void keystone_library_unload(void)
{
    if (keystone_module != 0) {
        FreeLibrary((HMODULE)keystone_module);
        keystone_module = 0;
    }
    keystone_translate_accelerator = 0;
    keystone_create_window = 0;
    chat_gui_find_object = 0;
    keystone_update = 0;
    keystone_dispatch_message = 0;
    chat_gui_release = 0;
    keystone_set_focus_window = 0;
    chat_gui_find_child = 0;
    keystone_control_get_attribute = 0;
    keystone_control_set_attribute = 0;
    chat_gui_set_property_int = 0;
    chat_gui_finalize = 0;
    chat_gui_set_focus = 0;
    keystone_window_add_dirty_control = 0;
    keystone_release = 0;
    chat_gui_set_state = 0;
}

#if 0
Original Ghidra decompilation (0x542cf0):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl keystone_library_unload(void)

{
  if (DAT_00721e9c != (HMODULE)0x0) {
    FreeLibrary(DAT_00721e9c);
    DAT_00721e9c = (HMODULE)0x0;
  }
  DAT_00721eb0 = 0;
  DAT_00721eb4 = 0;
  DAT_00721eb8 = 0;
  DAT_00721ebc = 0;
  DAT_00721ec0 = 0;
  DAT_00721ec8 = 0;
  _DAT_00721ec4 = 0;
  DAT_00721ecc = 0;
  DAT_00721ee0 = 0;
  DAT_00721ee4 = 0;
  DAT_00721ee8 = 0;
  DAT_00721ed0 = 0;
  DAT_00721ed4 = 0;
  _DAT_00721ed8 = 0;
  DAT_00721eac = 0;
  DAT_00721edc = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

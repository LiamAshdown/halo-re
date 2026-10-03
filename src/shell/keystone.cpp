#include "halo/shell/window.hpp"
#include "interface.h"

extern "C" {
extern uint8_t default_locale_name[2];
extern char *locale_codepage_format;

extern void *keystone_module;
extern keystone_create_fn keystone_create;
extern keystone_translate_accelerator_fn keystone_translate_accelerator;
extern keystone_create_window_fn keystone_create_window;
extern chat_gui_find_object_fn chat_gui_find_object;
extern keystone_update_fn keystone_update;
extern keystone_dispatch_message_fn keystone_dispatch_message;
extern chat_gui_release_fn chat_gui_release;
extern keystone_unknown_fn keystone_set_focus_window;
extern chat_gui_find_child_fn chat_gui_find_child;
extern chat_gui_get_property_string_fn keystone_control_get_attribute;
extern chat_gui_set_property_string_fn keystone_control_set_attribute;
extern chat_gui_set_property_int_fn chat_gui_set_property_int;
extern chat_gui_finalize_fn chat_gui_finalize;
extern chat_gui_set_focus_fn chat_gui_set_focus;
extern keystone_unknown_fn keystone_window_add_dirty_control;
extern keystone_release_fn keystone_release;
extern chat_gui_set_state_fn chat_gui_set_state;

extern uint16_t *keystone_current_directory;
extern int32_t safe_mode;
}

namespace halo::shell {

/**
 * Stores the current directory, converted to UTF-16, in keystone_current_directory (a GlobalAlloc'd
 * buffer) for the library to find its UI files.
 */
void KeystoneLibrary::capture_current_directory()
{
    uint32_t current_directory_size;
    char *current_directory;
    uint32_t wide_length;

    current_directory_size = GetCurrentDirectoryA(0, 0);
    current_directory = (char *)GlobalAlloc(0, current_directory_size);
    GetCurrentDirectoryA(current_directory_size, current_directory);

    wide_length = mbstowcs(0, current_directory, 0);
    keystone_current_directory = (uint16_t *)GlobalAlloc(0, (wide_length + 1) * 2);
    mbstowcs((wchar_t *)keystone_current_directory, current_directory, wide_length + 1);
    GlobalFree(current_directory);
}

/**
 * Switches the C runtime's character type locale to the system code page when it is still the default
 * "C" locale, so the multibyte conversions below see the user's code page.
 */
void KeystoneLibrary::use_codepage_locale()
{
    uint8_t *current_locale;
    int32_t compare;
    int32_t i;
    uint8_t less_than;
    uint8_t equal;
    char codepage_locale[16];

    current_locale = (uint8_t *)setlocale(2, 0);
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
        setlocale(2, codepage_locale);
    }
}

/**
 * Resolves every Keystone export the game uses into its global call slot, in the order the library
 * documents them.
 */
void KeystoneLibrary::resolve_exports()
{
    struct Export {
        const char *name;
        void **slot;
    };
    const Export exports[] = {
        {"KeystoneCreate", (void **)&keystone_create},
        {"Call_KsTranslateAccelerator", (void **)&keystone_translate_accelerator},
        {"Call_KsCreateWindow", (void **)&keystone_create_window},
        {"Call_KsGetWindow", (void **)&chat_gui_find_object},
        {"Call_KsUpdate", (void **)&keystone_update},
        {"Call_KsDispatchMessage", (void **)&keystone_dispatch_message},
        {"Call_KW_Release", (void **)&chat_gui_release},
        {"Call_KsSetFocusWindow", (void **)&keystone_set_focus_window},
        {"Call_KW_GetControlByID", (void **)&chat_gui_find_child},
        {"Call_KC_GetAttribute", (void **)&keystone_control_get_attribute},
        {"Call_KC_SetAttribute", (void **)&keystone_control_set_attribute},
        {"Call_KC_SendMessage", (void **)&chat_gui_set_property_int},
        {"Call_KW_ReLayout", (void **)&chat_gui_finalize},
        {"Call_KW_SetFocusControl", (void **)&chat_gui_set_focus},
        {"Call_KW_AddDirtyControl", (void **)&keystone_window_add_dirty_control},
        {"Call_KsRelease", (void **)&keystone_release},
        {"Call_KW_ShowWindow", (void **)&chat_gui_set_state},
    };
    uint32_t i;

    for (i = 0; i < sizeof(exports) / sizeof(exports[0]); i++) {
        *exports[i].slot = (void *)GetProcAddress((HMODULE)keystone_module, exports[i].name);
    }
}

/**
 * Loads the Keystone UI middleware and resolves its entry points into the global call slots, unless
 * safe mode disables the UI. The current directory is captured first.
 *
 * @address 0x542ad0
 */
void KeystoneLibrary::load()
{
    use_codepage_locale();
    capture_current_directory();

    if (safe_mode != 0) {
        keystone_module = 0;
        return;
    }

    keystone_module = LoadLibraryA("keystone.dll");
    if (keystone_module != 0) {
        resolve_exports();
    }
}

/**
 * Unloads keystone.dll and clears the cached entry points (the create slot is left alone).
 *
 * @address 0x542cf0
 */
void KeystoneLibrary::unload()
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

}

#include "halo/shell/window.hpp"
#include "interface.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shell/api.hpp"

static auto &default_locale_name = halo::link::ref<uint8_t [2]>(halo::shell::vars().default_locale_name);
static auto &locale_codepage_format = halo::link::ref<char [4]>(halo::shell::vars().locale_codepage_format);
static auto &keystone_module = halo::link::ref<void *>(halo::shell::vars().keystone_module);
static auto &keystone_create = halo::link::ref<keystone_create_fn>(halo::shell::vars().keystone_create);
static auto &keystone_translate_accelerator = halo::link::ref<keystone_translate_accelerator_fn>(halo::shell::vars().keystone_translate_accelerator);
static auto &keystone_create_window = halo::link::ref<keystone_create_window_fn>(halo::shell::vars().keystone_create_window);
static auto &chat_gui_find_object = halo::link::ref<chat_gui_find_object_fn>(halo::ui::vars().chat_gui_find_object);
static auto &keystone_update = halo::link::ref<keystone_update_fn>(halo::shell::vars().keystone_update);
static auto &keystone_dispatch_message = halo::link::ref<keystone_dispatch_message_fn>(halo::shell::vars().keystone_dispatch_message);
static auto &chat_gui_release = halo::link::ref<chat_gui_release_fn>(halo::ui::vars().chat_gui_release);
static auto &keystone_set_focus_window = halo::link::ref<keystone_unknown_fn>(halo::shell::vars().keystone_set_focus_window);
static auto &chat_gui_find_child = halo::link::ref<chat_gui_find_child_fn>(halo::ui::vars().chat_gui_find_child);
static auto &keystone_control_get_attribute = halo::link::ref<chat_gui_get_property_string_fn>(halo::ui::vars().keystone_control_get_attribute);
static auto &keystone_control_set_attribute = halo::link::ref<chat_gui_set_property_string_fn>(halo::ui::vars().keystone_control_set_attribute);
static auto &chat_gui_set_property_int = halo::link::ref<chat_gui_set_property_int_fn>(halo::ui::vars().chat_gui_set_property_int);
static auto &chat_gui_finalize = halo::link::ref<chat_gui_finalize_fn>(halo::ui::vars().chat_gui_finalize);
static auto &chat_gui_set_focus = halo::link::ref<chat_gui_set_focus_fn>(halo::ui::vars().chat_gui_set_focus);
static auto &keystone_window_add_dirty_control = halo::link::ref<keystone_unknown_fn>(halo::shell::vars().keystone_window_add_dirty_control);
static auto &keystone_release = halo::link::ref<keystone_release_fn>(halo::shell::vars().keystone_release);
static auto &chat_gui_set_state = halo::link::ref<chat_gui_set_state_fn>(halo::ui::vars().chat_gui_set_state);
static auto &keystone_current_directory = halo::link::ref<uint16_t *>(halo::rasterizer::vars().keystone_current_directory);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);

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

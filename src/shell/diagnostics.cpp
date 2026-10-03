#include "halo/shell/diagnostics.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"
#include "dialogs.h"
#include "interface.h"
#include "halo/sound/api.hpp"
#include "halo/dialogs/api.hpp"
#include "halo/rasterizer/api.hpp"

extern "C" {
extern void *shell_module_handle;
extern char fatal_error_text[k_shell_fatal_error_text_length];
extern char fatal_error_help_file[k_shell_fatal_error_readme_length];
extern char fatal_error_title[k_shell_fatal_error_title_length];
extern int32_t fatal_error_is_fatal;
extern char *graphics_vendor_name;
extern char *graphics_device_name;
extern uint32_t graphics_device_id;
extern void *shell_window;
extern void *shell_instance;
extern uint32_t shell_language_id;
extern uint8_t shell_window_proc_bypass;
extern int32_t safe_mode;
extern int32_t fatal_error_remember_choice;
extern char exception_title[k_shell_exception_string_length];
extern char exception_gathering_text[k_shell_exception_string_length];
extern char eula_file_name[k_shell_eula_name_length];
extern char strings_dll_invalid_text[k_shell_strings_dll_error_length];
extern uint32_t shell_startup_tick_count;
}

namespace halo::shell {

namespace {

/**
 * Loads string id into buffer through the localized lookup; when every lookup fails the buffer holds
 * the built-in fallback text.
 */
void load_string_or_default(uint32_t id, uint32_t capacity, char *buffer, const char *fallback)
{
    if (Localization::load_localized_string(capacity, shell_module_handle, buffer, id) == 0) {
        buffer[0] = 0;
        strcat(buffer, fallback);
    }
}

}

/**
 * Loads Win32 string table entry id (block (id >> 4) + 1, index id & 0xf) from module for the given
 * language and converts it to ANSI into buffer. Returns the converted character count (without the
 * terminator), or 0 on any failure.
 *
 * @address 0x57e110
 */
int32_t Localization::load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module,
                                           char *buffer)
{
    void *resource_info;
    void *resource_data;
    const uint16_t *entry;
    uint32_t index;
    uint16_t entry_length;
    int32_t converted;
    int32_t terminator_index;

    resource_info = FindResourceExA((HMODULE)module, (const char *)6, (const char *)(uint32_t)((id >> 4) + 1), language);
    if (resource_info == 0) {
        return 0;
    }
    resource_data = LoadResource((HMODULE)module, (HRSRC)resource_info);
    if (resource_data == 0) {
        return 0;
    }
    entry = (const uint16_t *)LockResource(resource_data);
    if (entry == 0) {
        return 0;
    }

    index = 0;
    do {
        entry_length = *entry;
        entry++;
        if (entry_length != 0 && index == (id & 0xf)) {
            converted = WideCharToMultiByte(0, 0, (LPCWCH)entry, entry_length, buffer, buffer_capacity, 0, 0);
            if (converted == 0) {
                return 0;
            }
            terminator_index = (int32_t)buffer_capacity - 1;
            if (converted < terminator_index) {
                terminator_index = converted;
            }
            buffer[terminator_index] = 0;
            return converted;
        }
        index++;
        entry += entry_length;
    } while (index < 0x10);

    return 0;
}

/**
 * Loads a localized UI string: the current language first, then (when that is not already English)
 * en-US, and finally the plain LoadStringA. Returns the character count, 0 when nothing was found.
 *
 * @address 0x57e1a0
 */
int32_t Localization::load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id)
{
    int32_t loaded;

    loaded = load_string_resource(id, (uint16_t)shell_language_id, buffer_capacity, module, buffer);
    if (loaded != 0) {
        return loaded;
    }
    if (shell_language_id != k_shell_language_default) {
        loaded = load_string_resource(id, (uint16_t)k_shell_language_default, buffer_capacity, module, buffer);
        if (loaded != 0) {
            return loaded;
        }
    }
    return LoadStringA((HINSTANCE)module, id, buffer, buffer_capacity);
}

/**
 * Loads strings.dll (a message box and exit if it is missing), determines the language from the LangID
 * setting (en-US when unset; a bare primary language gets the default sublanguage) and caches the
 * exception title, the gathering text, the EULA file name and the invalid strings.dll text, each with
 * a built-in fallback. Records the tick count at the end.
 *
 * @address 0x57efa0
 */
void Localization::initialize()
{
    char current_directory[260];
    uint32_t value_type;
    uint32_t language_id;
    uint32_t value_size;

    shell_module_handle = LoadLibraryA("strings.dll");
    if (shell_module_handle == 0) {
        GetCurrentDirectoryA(0x104, current_directory);
        strcat(current_directory, "\\strings.dll is missing.");
        MessageBoxA(0, current_directory, "Error!", 0);
        ExitProcess(1);
    }

    shell_language_id = k_shell_language_default;
    value_type = 4;
    value_size = 4;
    if (SettingsStore::current().read_value(SettingsScope::machine, "LangID", &value_type, &language_id, &value_size)) {
        if ((language_id & 0xfc00) == 0) {
            shell_language_id = (language_id & 0xffff) | 0x400;
        } else {
            shell_language_id = language_id;
        }
    }

    load_string_or_default(0x77, k_shell_exception_string_length, exception_title, "Exception!");
    load_string_or_default(0x78, k_shell_exception_string_length, exception_gathering_text, "Gathering Exception Data...");
    load_string_or_default(0x84, k_shell_eula_name_length, eula_file_name, "eula.rtf");
    load_string_or_default(0x88, k_shell_strings_dll_error_length, strings_dll_invalid_text, "Invalid / missing strings.dll");

    shell_startup_tick_count = GetTickCount();
}

/**
 * Loads the dialog text and the help file name into the shared fatal error buffers. A resource id of
 * 0xffffffff means the second argument is a raw message and the help file defaults to readme.rtf;
 * otherwise the text comes from string resource resource_id and the help file name from string resource
 * help_text_or_id.
 */
void FatalError::load_text(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    const char *help_text = (const char *)help_text_or_id;
    void *module;
    int32_t loaded;

    if (resource_id == 0xffffffff) {
        const char *source = help_text;
        char *dest = fatal_error_text;
        do {
            *dest++ = *source;
        } while (*source++ != 0);
        sprintf(fatal_error_help_file, "readme.rtf");
    } else {
        module = shell_module_handle;
        loaded = Localization::load_localized_string(k_shell_fatal_error_text_length, module, fatal_error_text, resource_id);
        if (loaded == 0) {
            sprintf(fatal_error_text, "Missing error string %d", resource_id);
        }

        module = shell_module_handle;
        loaded = Localization::load_localized_string(k_shell_fatal_error_readme_length, module, fatal_error_help_file,
                                                     (uint32_t)help_text);
        if (loaded == 0) {
            sprintf(fatal_error_help_file, "readme.rtf");
        }
    }

    module = shell_module_handle;
    loaded = Localization::load_localized_string(k_shell_fatal_error_title_length, module, fatal_error_title,
                                                 0x7f + (is_fatal != 0));
    if (loaded == 0) {
        sprintf(fatal_error_title, "Halo - Error");
    }
}

/**
 * Formats the setting name under which the player's answer to this error on this graphics device is
 * remembered.
 */
void FatalError::make_remembered_name(char *name, uint32_t resource_id)
{
    sprintf(name, "%s %s (0x%04x):%d", graphics_vendor_name, graphics_device_name, graphics_device_id, resource_id);
}

/**
 * Stops the services that would still be running when the process is about to end: window procedure
 * bypass, gamma ramp, deferred windowed operations, sound and the Keystone library. Faults are ignored.
 */
void FatalError::shut_down_engine_services()
{
    __try {
        shell_window_proc_bypass = 1;
        halo::rasterizer::chimera__registry_check_3();
        halo::rasterizer::rasterizer_service_deferred_windowed_ops();
        halo::sound::sound_stop_all();
        KeystoneLibrary::unload();
    } __except (1) {
    }
}

/**
 * Builds and shows the error dialog. Resource id 0xffffffff means help_text_or_id is a raw message;
 * any other id loads the text from strings.dll and treats help_text_or_id as the id of the help file
 * name. A non fatal error first looks up a remembered answer for this graphics device; a fatal one (or
 * a "quit" answer) runs the shutdown services and ends the process. Returns the dialog result, or the
 * remembered answer.
 *
 * @address 0x57ea70
 */
int32_t FatalError::show(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    win32_wndclassexa wndclass;
    void *window;
    int32_t result;
    char registry_value_name[256];
    uint32_t data_size;
    uint8_t remembered[16];
    char digit_text[16];
    uint32_t digit_length;

    load_text(resource_id, help_text_or_id, is_fatal);

    fatal_error_is_fatal = is_fatal;

    if (halo::rasterizer::globals().shader_file_name != 0) {
        strcat(fatal_error_text, " (");
        strcat(fatal_error_text, halo::rasterizer::globals().shader_file_name);
        strcat(fatal_error_text, ")");
        halo::rasterizer::globals().shader_file_name = 0;
    }

    if (is_fatal == 0) {
        make_remembered_name(registry_value_name, resource_id);
        data_size = 0x10;
        remembered[0] = 0;
        SettingsStore::current().read_value(SettingsScope::user, registry_value_name, 0, remembered, &data_size);
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
        wndclass.icon = (uint32_t)LoadIconA((HINSTANCE)shell_instance, (const char *)0x66);
        wndclass.cursor = (uint32_t)LoadCursorA(0, (const char *)0x7f00);
        wndclass.class_name = (uint32_t)"Halo";
        RegisterClassExA((const WNDCLASSEXA *)&wndclass);
        window = CreateWindowExA(0, "Halo", "Halo", 0x80000000, -0x80000000, -0x80000000, -0x80000000, -0x80000000,
                                  0, 0, (HINSTANCE)shell_instance, 0);
        ShowWindow((HWND)window, 5);
    }

    ShowCursor(1);
    result = halo::dialogs::dialog_box_show_localized((dialog_window_proc_fn)halo::dialogs::fatal_error_dialog_proc, shell_module_handle, (const char *)0x66, window);
    ShowCursor(0);

    if (is_fatal != 0 || result == 2) {
        ExitFlag::set_clean();
        shut_down_engine_services();
        ExitProcess(1);
    }

    if (shell_window == 0) {
        DestroyWindow((HWND)window);
        UnregisterClassA("Halo", (HINSTANCE)shell_instance);
    } else {
        ShowWindow((HWND)shell_window, 5);
    }

    if (fatal_error_remember_choice != 0) {
        make_remembered_name(registry_value_name, resource_id);
        digit_text[0] = 'y';
        digit_text[1] = (char)(result + '0');
        digit_text[2] = 0;
        digit_length = 0;
        while (digit_text[digit_length] != 0) {
            digit_length++;
        }
        SettingsStore::current().write_string(SettingsScope::user, registry_value_name, digit_text, digit_length + 1);
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

/**
 * WM_INITDIALOG handler that centers the dialog on the desktop; every other message is left to the
 * default dialog processing.
 *
 * @address 0x542f00
 */
int32_t __stdcall DialogCentering::procedure(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    win32_rect window_rect;
    win32_rect desktop_rect;

    if (message != 0x110) {
        return 0;
    }

    GetWindowRect((HWND)hwnd, &window_rect);
    GetClientRect(GetDesktopWindow(), &desktop_rect);
    MoveWindow((HWND)hwnd,
               (desktop_rect.right - desktop_rect.left) / 2 - (window_rect.right - window_rect.left) / 2,
               (desktop_rect.bottom - desktop_rect.top) / 2 - (window_rect.bottom - window_rect.top) / 2,
               window_rect.right - window_rect.left,
               window_rect.bottom - window_rect.top,
               1);
    return 1;
}

}

#include "halo/shell/diagnostics.hpp"
#include "halo/shell/layout.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"
#include "interface.h"
#include "halo/sound/api.hpp"
#include "halo/shell/messages.hpp"
#include "halo/shell/standalone.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/core/link.hpp"
#include "halo/main/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/main/api.hpp"
#include "halo/shell/api.hpp"

static auto &shell_module_handle = halo::link::ref<void *>(halo::shell::vars().shell_module_handle);
static auto &fatal_error_text = halo::link::ref<char [k_shell_fatal_error_text_length]>(halo::shell::vars().fatal_error_text);
static auto &fatal_error_help_file = halo::link::ref<char [k_shell_fatal_error_readme_length]>(halo::shell::vars().fatal_error_help_file);
static auto &fatal_error_title = halo::link::ref<char [k_shell_fatal_error_title_length]>(halo::shell::vars().fatal_error_title);
static auto &fatal_error_is_fatal = halo::link::ref<int32_t>(halo::shell::vars().fatal_error_is_fatal);
static auto &graphics_vendor_name = halo::link::ref<char *>(halo::shell::vars().graphics_vendor_name);
static auto &graphics_device_name = halo::link::ref<char *>(halo::shell::vars().graphics_device_name);
static auto &graphics_device_id = halo::link::ref<uint32_t>(halo::shell::vars().graphics_device_id);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &shell_instance = halo::link::ref<void *>(halo::shell::vars().shell_instance);
static auto &shell_language_id = halo::link::ref<uint32_t>(halo::shell::vars().shell_language_id);
static auto &shell_window_proc_bypass = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_proc_bypass);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &fatal_error_remember_choice = halo::link::ref<int32_t>(halo::shell::vars().fatal_error_remember_choice);
static auto &exception_title = halo::link::ref<char [k_shell_exception_string_length]>(halo::shell::vars().exception_title);
static auto &exception_gathering_text = halo::link::ref<char [k_shell_exception_string_length]>(halo::shell::vars().exception_gathering_text);
static auto &strings_dll_invalid_text = halo::link::ref<char [k_shell_strings_dll_error_length]>(halo::shell::vars().strings_dll_invalid_text);
static auto &shell_startup_tick_count = halo::link::ref<uint32_t>(halo::main::vars().shell_startup_tick_count);

namespace halo::shell {

/**
 * Copies the English text of message `id` into buffer (truncated to capacity) and returns its length, or 0 when the id
 * has no text. The module argument is unused; it stays so existing callers keep their signature.
 */
int32_t Localization::load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id)
{
    const char *text = shell_message(id);
    uint32_t length;

    (void)module;
    if (text == nullptr || buffer_capacity == 0) {
        return 0;
    }
    length = static_cast<uint32_t>(strlen(text));
    if (length >= buffer_capacity) {
        length = buffer_capacity - 1;
    }
    memcpy(buffer, text, length);
    buffer[length] = 0;
    return static_cast<int32_t>(length);
}

/**
 * Fills the cached exception title and gathering text and records the start-up tick count.
 *
 * @address 0x57efa0
 */
void Localization::initialize()
{
    Localization::load_localized_string(k_shell_exception_string_length, nullptr, exception_title, k_string_exception_title);
    Localization::load_localized_string(k_shell_exception_string_length, nullptr, exception_gathering_text, k_string_exception_gathering);

    shell_startup_tick_count = GetTickCount();
}

/**
 * Loads the dialog text and title into the shared fatal error buffers. A resource id of 0xffffffff means the second
 * argument is a raw message; otherwise the text is the English message for the id.
 */
void FatalError::load_text(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    const char *text = shell_message(resource_id);

    if (resource_id == k_dword_none) {
        strncpy(fatal_error_text, (const char *)help_text_or_id, k_shell_fatal_error_text_length - 1);
        fatal_error_text[k_shell_fatal_error_text_length - 1] = 0;
    } else if (text != nullptr) {
        strncpy(fatal_error_text, text, k_shell_fatal_error_text_length - 1);
        fatal_error_text[k_shell_fatal_error_text_length - 1] = 0;
    } else {
        sprintf(fatal_error_text, "Missing error string %d", resource_id);
    }

    strcpy(fatal_error_title, shell_message(k_string_error_title_base + (is_fatal != 0)));
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
 * Shows the error message box. Resource id 0xffffffff means help_text_or_id is a raw message; any other id is looked up
 * in the shell messages. A fatal error (or a "quit" answer) runs the shutdown services and ends the process. A non
 * fatal error offers to continue, to restart in safe mode or to quit. Returns 0 to continue, 1 for safe mode.
 *
 * @address 0x57ea70
 */
int32_t FatalError::show(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    char message[k_shell_fatal_error_text_length + 160];
    char registry_value_name[256];
    char remembered_answer[16];
    uint32_t remembered_size;
    int32_t answer;
    int32_t result;

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
        remembered_size = sizeof(remembered_answer);
        remembered_answer[0] = 0;
        SettingsStore::current().read_value(SettingsScope::user, registry_value_name, 0, remembered_answer, &remembered_size);
        if (remembered_answer[0] == 'y') {
            result = remembered_answer[1] - '0';
            safe_mode = result;
            return result;
        }
    }

    halo::shell::standalone_log("error message id=0x%x fatal=%d: %s", resource_id, is_fatal, fatal_error_text);
    strcpy(message, fatal_error_text);
    if (is_fatal == 0) {
        strcat(message, "\n\nYes: continue (and do not ask again on this graphics device)\nNo: restart in safe mode\nCancel: quit");
    }

    ShowCursor(1);
    answer = MessageBoxA((HWND)shell_window, message, fatal_error_title,
                         is_fatal != 0 ? (halo::win32::k_mb_ok | halo::win32::k_mb_iconerror) : (halo::win32::k_mb_yesnocancel | halo::win32::k_mb_iconwarning));
    ShowCursor(0);

    if (is_fatal != 0 || answer == halo::win32::k_id_cancel) {
        ExitFlag::set_clean();
        shut_down_engine_services();
        ExitProcess(1);
    }

    result = (answer == halo::win32::k_id_no) ? 1 : 0;
    if (result == 0 && is_fatal == 0) {
        remembered_answer[0] = 'y';
        remembered_answer[1] = '0';
        remembered_answer[2] = 0;
        SettingsStore::current().write_string(SettingsScope::user, registry_value_name, remembered_answer, 3);
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

    if (message != k_wm_initdialog) {
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

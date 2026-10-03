#pragma once


#include <cstdint>

namespace halo::dialogs {

/**
 * Window messages the fatal error dialog procedure handles.
 */
enum class dialog_message : uint32_t {
    close = 0x10,
    command = 0x111,
    init_dialog = 0x110,
};

/**
 * Control identifiers of the fatal error dialog template.
 */
enum class fatal_error_control : int32_t {
    remember_choice_checkbox = 0x3e9,
    ignore_button = 0x3ec,
    quit_button = 0x3ed,
    message_text = 0x3ee,
    help_link = 0x3ef,
    system_specs_text = 0x3f1,
};

inline constexpr int32_t k_dialog_button_ok = 1;
inline constexpr int32_t k_dialog_button_cancel = 2;
inline constexpr int32_t k_dialog_button_abort = 3;
inline constexpr uint32_t k_low_word_mask = 0xffff;
inline constexpr uint32_t k_system_specs_capacity = 256;
inline constexpr uint32_t k_high_word_shift = 16;
inline constexpr uint32_t k_megabyte_shift = 20;

constexpr int32_t id_of(fatal_error_control control) noexcept { return static_cast<int32_t>(control); }
constexpr uint32_t message_id(dialog_message message) noexcept { return static_cast<uint32_t>(message); }

static_assert(sizeof(dialogs_constants) == 4);
static_assert(sizeof(win32_logfonta) == 60);

}

#include "halo/interface/chat_gui.hpp"
#include "halo/shell/window.hpp"
#include "sound.h"
#include "interface.h"
#include "main.h"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/main/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"
#include "halo/platform/window.hpp"

static auto &shell_window_proc_bypass = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_proc_bypass);
static auto &shell_application_inactive = halo::link::ref<uint8_t>(halo::main::vars().shell_application_inactive);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &shell_window_minimized = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_minimized);
static auto &shell_window_maximized = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_maximized);
static auto &nowindowskey = halo::link::ref<int32_t>(halo::shell::vars().nowindowskey);
static auto &sound_paused = halo::link::ref<uint8_t>(halo::shell::vars().sound_paused);

namespace halo::shell {

/**
 * Puts the window into the suspended state used whenever the application loses input focus: pauses
 * sound (through the driver when the device is not exclusive yet), releases DirectInput, resets input
 * state, minimizes a fullscreen window and closes chat.
 */
void GameWindow::suspend_focus()
{
    if (shell_application_inactive != 1) {
        shell_application_inactive = 1;
        if (halo::rasterizer::globals().fullscreen == 0 || halo::rasterizer::globals().device == 0) {
            if (sound_paused != 1) {
                sound_paused = 1;
                if (halo::sound::globals().current_driver != 0) {
                    halo::sound::globals().current_driver->set_paused(1);
                }
            }
        } else {
            halo::sound::sound_pause();
        }
        halo::input::DirectInput::directinput_unacquire_devices();
        halo::input::GameActions::reset_state_and_axis_configs();
        if (shell_window != 0 && halo::rasterizer::globals().fullscreen != 0 && halo::rasterizer::globals().device != 0) {
            halo::platform::window_show(shell_window, halo::platform::k_window_minimize);
        }
        halo::interface::chat_close();
    }
}

/**
 * Brings the game back from the suspended state: reacquires DirectInput, resets input state, restores the window and
 * resumes sound.
 */
void GameWindow::resume_focus()
{
    if (shell_application_inactive == 0) {
        return;
    }
    shell_application_inactive = 0;
    halo::input::DirectInput::directinput_acquire_devices();
    halo::input::GameActions::reset_state_and_axis_configs();
    if (shell_window != 0) {
        halo::platform::window_show(shell_window, halo::platform::k_window_restore);
    }
    if (shell_window_proc_bypass != 0) {
        return;
    }
    if (halo::rasterizer::globals().fullscreen != 0 && halo::rasterizer::globals().device != 0) {
        halo::sound::sound_resume();
    } else if (sound_paused != 0) {
        sound_paused = 0;
        if (halo::sound::globals().current_driver != 0) {
            halo::sound::globals().current_driver->set_paused(0);
        }
        halo::sound::globals().time = halo::cseries::time_query_performance_counter_ms();
    }
}

namespace {

bool loading_screen_showing()
{
    return halo::render::render_device_is_ready() == 0 && halo::rasterizer::globals().window_requested == 0;
}

/** Keyboard and IME messages: chat input, Enter submits and Escape closes the chat, then the DirectInput key record. */
bool key_message(uint32_t message, uint32_t key)
{
    bool consumed = halo::interface::ChatGui::get().handle_message(message, key);

    if (message == halo::platform::k_message_key_down) {
        if (key == halo::platform::k_key_return) {
            if (halo::interface::globals().chat_dialog_open != 0) {
                halo::interface::chat_submit_input();
                halo::input::DirectInput::key_block_timer_set(0x38, 200);
                halo::input::DirectInput::key_block_timer_set(0x66, 200);
                return true;
            }
        } else if (key == halo::platform::k_key_escape) {
            if (halo::interface::globals().chat_dialog_open != 0) {
                halo::input::DirectInput::key_block_timer_set(0, 0xfa);
            }
            halo::interface::chat_close();
        }
    }
    halo::input::DirectInput::record_windows_key_message(key, message);
    return consumed;
}

}  // namespace

/**
 * The engine's reactions to the game window's events (the original window procedure at 0x541b30, minus what only
 * Win32 had: hit testing, the system menu, size/move loops, power and display-depth messages): suspend and resume
 * around focus loss, quitting on close, the loading screen before the device exists, the Windows key chord and chat
 * keyboard routing.
 *
 * @address 0x541b30
 */
const halo::platform::window_events &GameWindow::events()
{
    static const halo::platform::window_events table = {
        []() { return shell_window_proc_bypass != 0; },
        []() {
            halo::main::globals().main_globals.return_to_main_menu = 0;
            halo::main::globals().main_globals.quit = 1;
            halo::main::globals().movie_playback_abort = 1;
        },
        []() {
            suspend_focus();
            shell_window_minimized = 1;
            shell_window_maximized = 0;
        },
        []() {
            if (shell_window_minimized != 0) {
                resume_focus();
            }
            shell_window_minimized = 0;
            shell_window_maximized = 1;
        },
        []() {
            if (shell_window_maximized != 0) {
                shell_window_maximized = 0;
            } else if (shell_window_minimized != 0) {
                resume_focus();
                shell_window_minimized = 0;
            } else if (halo::render::render_device_is_ready() != 0) {
                halo::sound::sound_resume();
            }
        },
        []() {
            if (shell_window != 0) {
                resume_focus();
            }
        },
        []() {
            if (shell_window != 0 && shell_application_inactive != 1) {
                suspend_focus();
            }
        },
        []() {
            if (loading_screen_showing()) {
                if (halo::rasterizer::globals().device == 0) {
                    return halo::platform::window_paint::splash;
                }
                halo::rasterizer::rasterizer_capture_and_present(nullptr, nullptr);
                return halo::platform::window_paint::painted;
            }
            if (halo::rasterizer::globals().device != 0) {
                halo::rasterizer::rasterizer_capture_and_present(nullptr, nullptr);
            }
            return halo::platform::window_paint::native;
        },
        [](bool inactive) {
            if (halo::rasterizer::globals().window_requested == 0 && halo::game::globals().time_force_single_tick == 0 && shell_window != 0) {
                handle_activate_app(inactive ? 1 : 0);
            }
        },
        loading_screen_showing,
        []() {
            if (nowindowskey != 0 || shell_window == 0) {
                return false;
            }
            if (shell_application_inactive != 1) {
                suspend_focus();
            }
            return true;
        },
        key_message,
    };

    return table;
}

/**
 * Reacts to the application losing or regaining focus: acquires or releases DirectInput, pauses or
 * resumes sound, minimizes or restores the window and closes chat when deactivated.
 *
 * @address 0x5410d0
 */
void GameWindow::handle_activate_app(uint8_t inactive)
{
    uint8_t fullscreen_device;

    if (shell_application_inactive == inactive) {
        return;
    }
    shell_application_inactive = inactive;

    if (inactive == 0) {
        halo::input::DirectInput::directinput_acquire_devices();
    } else {
        if (shell_window_proc_bypass == 0) {
            if (halo::rasterizer::globals().fullscreen != 0 && halo::rasterizer::globals().device != 0) {
                halo::sound::sound_pause();
            } else if (sound_paused != 1) {
                sound_paused = 1;
                if (halo::sound::globals().current_driver != 0) {
                    halo::sound::globals().current_driver->set_paused(1);
                }
            }
        }
        halo::input::DirectInput::directinput_unacquire_devices();
    }
    halo::input::GameActions::reset_state_and_axis_configs();

    if (shell_window != 0) {
        fullscreen_device = halo::rasterizer::globals().fullscreen != 0 && halo::rasterizer::globals().device != 0;
        if (fullscreen_device) {
            halo::platform::window_show(shell_window, inactive != 0 ? halo::platform::k_window_minimize : halo::platform::k_window_restore);
        } else if (inactive == 0) {
            halo::platform::window_show(shell_window, halo::platform::k_window_restore);
        }
    }
    if (inactive != 0) {
        halo::interface::chat_close();
        return;
    }

    if (shell_window_proc_bypass != 0) {
        return;
    }
    if (halo::rasterizer::globals().fullscreen != 0 && halo::rasterizer::globals().device != 0) {
        halo::sound::sound_resume();
        return;
    }
    if (sound_paused != 0) {
        sound_paused = 0;
        if (halo::sound::globals().current_driver != 0) {
            halo::sound::globals().current_driver->set_paused(0);
        }
        halo::sound::globals().time = halo::cseries::time_query_performance_counter_ms();
    }
}

/**
 * Drains the window message queue each frame through the normal translate and dispatch path.
 *
 * @address 0x541a20
 */
void GameWindow::pump_messages()
{
    halo::platform::pump_messages();
}


const halo::platform::window_events &game_window_events()
{
    return GameWindow::events();
}

}

#pragma once

#include "halo/shell/platform.hpp"
#include "halo/platform/window.hpp"

namespace halo::shell {

/**
 * The game's main window: the reactions to its messages (handed to halo::platform::window_create), the
 * activation handling that pauses and resumes sound and input, and the per frame message pump.
 */
class GameWindow {
public:
    static const halo::platform::window_events &events();
    static void handle_activate_app(uint8_t inactive);
    static void pump_messages();

private:
    static void suspend_focus();
    static void resume_focus();
};

/** Selects the system code page for the C runtime's multibyte conversions. */
class CodepageLocale {
public:
    static void apply();
};

}

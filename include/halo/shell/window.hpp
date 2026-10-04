#pragma once

#include "halo/shell/platform.hpp"

namespace halo::shell {

/**
 * The game's main window: the window procedure, the activation handling that pauses and resumes sound
 * and input, and the per frame message pump.
 */
class GameWindow {
public:
    static int32_t __stdcall procedure(HWND hwnd, uint32_t message, uint32_t wparam, int32_t lparam);
    static void handle_activate_app(uint8_t inactive);
    static void pump_messages();

private:
    static void suspend_focus();
};

/** Selects the system code page for the C runtime's multibyte conversions. */
class CodepageLocale {
public:
    static void apply();
};

}

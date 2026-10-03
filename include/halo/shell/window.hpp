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

/**
 * The Keystone UI middleware (keystone.dll): loads the library and resolves its exports into the
 * global call slots the chat interface uses, or clears them again.
 */
class KeystoneLibrary {
public:
    static void load();
    static void unload();

private:
    static void capture_current_directory();
    static void use_codepage_locale();
    static void resolve_exports();
};

}

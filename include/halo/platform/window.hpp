/**
 * @file include/halo/platform/window.hpp
 * The game window, its message loop, the cursor and the display gamma ramp, independent of the operating system.
 *
 * The engine creates the window with a table of handlers (window_events); the platform's window procedure turns
 * native window messages into those calls and does the native default handling itself. Show commands keep the Win32
 * SW_ values and key messages the Win32 WM_ ids and virtual keys, the encoding the engine already uses.
 * src/platform/window_sdl.cpp implements this on SDL2.
 */
#pragma once

#include <cstdint>

namespace halo::platform {

using window_handle = void *;

/** A rectangle with the layout of the engine's win32_rect (RECT). */
struct window_rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

/** window_show commands (SW_HIDE, SW_SHOW, SW_MINIMIZE, SW_RESTORE). */
inline constexpr uint32_t k_window_hide = 0;
inline constexpr uint32_t k_window_show = 5;
inline constexpr uint32_t k_window_minimize = 6;
inline constexpr uint32_t k_window_restore = 9;

/** Key message ids and virtual keys the engine looks at (WM_KEYDOWN, VK_RETURN, VK_ESCAPE). */
inline constexpr uint32_t k_message_key_down = 0x100;
inline constexpr uint32_t k_key_return = 0x0d;
inline constexpr uint32_t k_key_escape = 0x1b;

/** What window_events::paint asks the platform to do afterwards. */
enum class window_paint : uint8_t {
    native,   // the platform's default handling
    splash,   // draw the loading-screen bitmap over the client area; the window counts as painted
    painted,  // the engine presented a frame; the window counts as painted
};

/**
 * The engine's reactions to window messages. Every member must be set.
 */
struct window_events {
    bool (*bypass)();                       // true: the engine ignores window events
    void (*closed)();                       // the window was closed or destroyed
    void (*minimized)();
    void (*maximized)();
    void (*restored)();
    void (*focus_gained)();
    void (*focus_lost)();
    window_paint (*paint)();
    void (*activate_app)(bool inactive);    // the application was activated or deactivated
    bool (*splash_cursor)();                // true while the loading screen shows: no cursor over it when in front
    bool (*windows_key)();                  // a Windows key went down; true: hand the desktop the focus
    bool (*key_message)(uint32_t message, uint32_t key);  // keyboard / IME message (WM_ id, virtual key); true: consumed
};

/**
 * Creates the game window, width by height client pixels centred on the desktop, with the icon and loading-screen
 * bitmap resources of the given modules. Shows a message box and returns null on failure; on success brings the
 * window to the front.
 */
window_handle window_create(void *instance, void *resource_module, const char *class_name, const char *title, int32_t width, int32_t height,
    uint32_t icon_resource, uint32_t splash_resource, const window_events &events);
/** Hides and destroys the window and frees the loading-screen bitmap. */
void window_destroy(window_handle window);
void window_show(window_handle window, uint32_t command);
/** Gives a borderless fullscreen window its popup style. */
void window_set_fullscreen_style(window_handle window);
/** Centres the window on the desktop with a width by height client area, moving it only when that changes it. */
void window_center(window_handle window, int32_t width, int32_t height);
/** Brings forward (restoring when minimised) a running window of the given class and title; false when there is none. */
bool window_activate_existing(const char *class_name, const char *title);

/** The bounds of the display the window is on. */
void desktop_bounds(window_rect *rect);
uint32_t desktop_bits_per_pixel();

/** Runs the window procedure for every waiting message. */
void pump_messages();
/** Waits up to the given time for a window message or input. */
void wait_for_messages(uint32_t milliseconds);

/** Shows or hides the cursor; returns the new state (1 shown, 0 hidden). */
int32_t cursor_show(bool show);
void cursor_position(int32_t *x, int32_t *y);

/** The display's gamma ramp (3 x 256 16-bit entries) for the window's screen. */
bool gamma_ramp_get(window_handle window, void *ramp);
bool gamma_ramp_set(window_handle window, const void *ramp);

}  // namespace halo::platform

/**
 * @file src/platform/window_sdl.cpp
 * halo::platform window on SDL2: the game window, its events (SDL events turned into the engine's window_events, keyboard
 * events into the Win32 key messages the chat and DirectInput still read), cursor and gamma ramp. On Windows the
 * window handle the engine gets is the native HWND, so Direct3D 9, DirectInput and DirectSound keep working; a small
 * Windows-only part paints the loading-screen resource bitmap and hands the desktop the focus on the Windows key.
 */

#include "halo/platform/window.hpp"
#include "sdl_events.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#ifdef _WIN32
#endif

namespace halo::platform {

namespace {

constexpr uint32_t k_message_key_up = 0x101;
constexpr uint32_t k_message_char = 0x102;
constexpr uint32_t k_message_sys_key_down = 0x104;

window_events g_events;
SDL_Window *g_window;
void *g_native;
bool g_cursor_hidden_for_splash;

#ifdef _WIN32
HBITMAP g_splash_bitmap;
HDC g_splash_dc;
#endif

/** The Windows virtual key a key event would have carried (WM_KEYDOWN wparam), 0 for keys Windows has none for. */
uint32_t virtual_key(SDL_Scancode scancode)
{
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return 'A' + (scancode - SDL_SCANCODE_A);
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return '1' + (scancode - SDL_SCANCODE_1);
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return 0x70 + (scancode - SDL_SCANCODE_F1);
    }
    if (scancode >= SDL_SCANCODE_F13 && scancode <= SDL_SCANCODE_F24) {
        return 0x7c + (scancode - SDL_SCANCODE_F13);
    }
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) {
        return 0x61 + (scancode - SDL_SCANCODE_KP_1);
    }
    switch (scancode) {
    case SDL_SCANCODE_0: return '0';
    case SDL_SCANCODE_RETURN: return 0x0d;
    case SDL_SCANCODE_ESCAPE: return 0x1b;
    case SDL_SCANCODE_BACKSPACE: return 0x08;
    case SDL_SCANCODE_TAB: return 0x09;
    case SDL_SCANCODE_SPACE: return 0x20;
    case SDL_SCANCODE_MINUS: return 0xbd;
    case SDL_SCANCODE_EQUALS: return 0xbb;
    case SDL_SCANCODE_LEFTBRACKET: return 0xdb;
    case SDL_SCANCODE_RIGHTBRACKET: return 0xdd;
    case SDL_SCANCODE_BACKSLASH: return 0xdc;
    case SDL_SCANCODE_NONUSHASH: return 0xde;
    case SDL_SCANCODE_SEMICOLON: return 0xba;
    case SDL_SCANCODE_APOSTROPHE: return 0xde;
    case SDL_SCANCODE_GRAVE: return 0xc0;
    case SDL_SCANCODE_COMMA: return 0xbc;
    case SDL_SCANCODE_PERIOD: return 0xbe;
    case SDL_SCANCODE_SLASH: return 0xbf;
    case SDL_SCANCODE_CAPSLOCK: return 0x14;
    case SDL_SCANCODE_PRINTSCREEN: return 0x2c;
    case SDL_SCANCODE_SCROLLLOCK: return 0x91;
    case SDL_SCANCODE_PAUSE: return 0x13;
    case SDL_SCANCODE_INSERT: return 0x2d;
    case SDL_SCANCODE_HOME: return 0x24;
    case SDL_SCANCODE_PAGEUP: return 0x21;
    case SDL_SCANCODE_DELETE: return 0x2e;
    case SDL_SCANCODE_END: return 0x23;
    case SDL_SCANCODE_PAGEDOWN: return 0x22;
    case SDL_SCANCODE_RIGHT: return 0x27;
    case SDL_SCANCODE_LEFT: return 0x25;
    case SDL_SCANCODE_DOWN: return 0x28;
    case SDL_SCANCODE_UP: return 0x26;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x90;
    case SDL_SCANCODE_KP_DIVIDE: return 0x6f;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x6a;
    case SDL_SCANCODE_KP_MINUS: return 0x6d;
    case SDL_SCANCODE_KP_PLUS: return 0x6b;
    case SDL_SCANCODE_KP_ENTER: return 0x0d;
    case SDL_SCANCODE_KP_0: return 0x60;
    case SDL_SCANCODE_KP_PERIOD: return 0x6e;
    case SDL_SCANCODE_NONUSBACKSLASH: return 0xe2;
    case SDL_SCANCODE_APPLICATION: return 0x5d;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL: return 0x11;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT: return 0x10;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT: return 0x12;
    case SDL_SCANCODE_LGUI: return 0x5b;
    case SDL_SCANCODE_RGUI: return 0x5c;
    default: return 0;
    }
}

void key_event(const SDL_KeyboardEvent &key, bool down)
{
    uint32_t vk = virtual_key(key.keysym.scancode);
    bool alt = (key.keysym.mod & KMOD_ALT) != 0;

    if (vk == 0) {
        return;
    }
    if (down && (vk == 0x5b || vk == 0x5c)) {
        if (g_events.windows_key()) {
#ifdef _WIN32
            SetForegroundWindow(GetDesktopWindow());
#else
            SDL_MinimizeWindow(g_window);
#endif
        }
        return;
    }
    if (!down) {
        g_events.key_message(k_message_key_up, vk);
        return;
    }
    g_events.key_message(alt || vk == 0x79 ? k_message_sys_key_down : k_message_key_down, vk);
    // Windows' TranslateMessage follows these keys with a WM_CHAR; SDL's text input does not report them
    if (!alt && (vk == 0x0d || vk == 0x1b || vk == 0x08 || vk == 0x09)) {
        g_events.key_message(k_message_char, vk);
    }
}

/** Typed text arrives as UTF-8; the engine reads characters in the system ANSI code page (WM_CHAR of an ANSI window). */
void text_event(const char *text)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(text);

    while (*p != 0) {
        uint32_t code = *p++;
        int extra = code >= 0xf0 ? 3 : code >= 0xe0 ? 2 : code >= 0xc0 ? 1 : 0;

        code &= extra == 3 ? 0x07 : extra == 2 ? 0x0f : extra == 1 ? 0x1f : 0x7f;
        for (; extra > 0 && *p != 0; extra--) {
            code = (code << 6) | (*p++ & 0x3f);
        }
        if (code < 0x80) {
            g_events.key_message(k_message_char, code);
            continue;
        }
#ifdef _WIN32
        if (code <= 0xffff) {
            wchar_t wide = static_cast<wchar_t>(code);
            char narrow;

            if (WideCharToMultiByte(CP_ACP, 0, &wide, 1, &narrow, 1, nullptr, nullptr) == 1) {
                g_events.key_message(k_message_char, static_cast<uint8_t>(narrow));
            }
        }
#endif
    }
}

void paint()
{
    switch (g_events.paint()) {
    case window_paint::splash:
#ifdef _WIN32
        if (g_splash_bitmap != nullptr) {
            HWND window = static_cast<HWND>(g_native);
            HDC dc = GetDC(window);
            RECT client;
            BITMAP info;

            GetClientRect(window, &client);
            GetObjectA(g_splash_bitmap, sizeof(info), &info);
            StretchBlt(dc, 0, 0, client.right - client.left, client.bottom - client.top, g_splash_dc, 0, 0, info.bmWidth, info.bmHeight, SRCCOPY);
            ReleaseDC(window, dc);
        }
#endif
        break;
    default:
        break;
    }
}

void window_event(const SDL_WindowEvent &event)
{
    switch (event.event) {
    case SDL_WINDOWEVENT_CLOSE:
        g_events.closed();
        break;
    case SDL_WINDOWEVENT_MINIMIZED:
        g_events.minimized();
        break;
    case SDL_WINDOWEVENT_MAXIMIZED:
        g_events.maximized();
        break;
    case SDL_WINDOWEVENT_RESTORED:
        g_events.restored();
        break;
    case SDL_WINDOWEVENT_FOCUS_GAINED:
        g_events.activate_app(false);
        g_events.focus_gained();
        break;
    case SDL_WINDOWEVENT_FOCUS_LOST:
        g_events.activate_app(true);
        g_events.focus_lost();
        break;
    case SDL_WINDOWEVENT_EXPOSED:
        paint();
        break;
    default:
        break;
    }
}

/** Before the device exists the loading screen shows with no cursor over it while the window is in front. */
void update_splash_cursor()
{
    bool hide = g_events.splash_cursor() && (SDL_GetWindowFlags(g_window) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS)) ==
                                                (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);

    if (hide != g_cursor_hidden_for_splash) {
        SDL_ShowCursor(hide ? SDL_DISABLE : SDL_ENABLE);
        g_cursor_hidden_for_splash = hide;
    }
}

void dispatch(const SDL_Event &event)
{
    if (g_events.bypass()) {
        return;
    }
    switch (event.type) {
    case SDL_QUIT:
        g_events.closed();
        break;
    case SDL_WINDOWEVENT:
        window_event(event.window);
        break;
    case SDL_KEYDOWN:
        key_event(event.key, true);
        break;
    case SDL_KEYUP:
        key_event(event.key, false);
        break;
    case SDL_TEXTINPUT:
        text_event(event.text.text);
        break;
    default:
        break;
    }
}

SDL_Rect display_bounds()
{
    SDL_Rect bounds = {0, 0, 0, 0};

    SDL_GetDisplayBounds(g_window != nullptr ? SDL_GetWindowDisplayIndex(g_window) : 0, &bounds);
    return bounds;
}

}  // namespace

window_handle window_create(void *instance, void *resource_module, const char *class_name, const char *title, int32_t width, int32_t height,
    uint32_t icon_resource, uint32_t splash_resource, bool opengl, const window_events &events)
{
    SDL_SysWMinfo info;

    (void)icon_resource;
    g_events = events;
    SDL_SetMainReady();
#ifdef _WIN32
    // the window class keeps the game's name, which the single-instance check looks for
    SDL_RegisterApp(class_name, 0, instance);
#else
    (void)instance;
    (void)class_name;
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ERROR - failed to create window", SDL_GetError(), nullptr);
        return nullptr;
    }
    SDL_DisableScreenSaver();
    g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | (opengl ? SDL_WINDOW_OPENGL : 0));
    if (g_window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ERROR - failed to create window", SDL_GetError(), nullptr);
        return nullptr;
    }
    SDL_VERSION(&info.version);
    g_native = SDL_GetWindowWMInfo(g_window, &info) ? static_cast<void *>(info.info.win.window) : static_cast<void *>(g_window);

#ifdef _WIN32
    g_splash_bitmap = LoadBitmapA(static_cast<HINSTANCE>(resource_module), MAKEINTRESOURCEA(splash_resource));
    if (g_splash_bitmap != nullptr) {
        g_splash_dc = CreateCompatibleDC(GetDC(static_cast<HWND>(g_native)));
        SelectObject(g_splash_dc, g_splash_bitmap);
    }
#else
    (void)resource_module;
    (void)splash_resource;
#endif
    SDL_RaiseWindow(g_window);
    return g_native;
}

void window_destroy(window_handle window)
{
    (void)window;
#ifdef _WIN32
    if (g_splash_dc != nullptr) {
        DeleteDC(g_splash_dc);
        g_splash_dc = nullptr;
    }
    if (g_splash_bitmap != nullptr) {
        DeleteObject(g_splash_bitmap);
        g_splash_bitmap = nullptr;
    }
#endif
    if (g_window != nullptr) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
}

void window_show(window_handle window, uint32_t command)
{
    (void)window;
    switch (command) {
    case k_window_hide: SDL_HideWindow(g_window); break;
    case k_window_minimize: SDL_MinimizeWindow(g_window); break;
    case k_window_restore: SDL_RestoreWindow(g_window); break;
    default: SDL_ShowWindow(g_window); break;
    }
}

void window_set_fullscreen_style(window_handle window)
{
    (void)window;
    SDL_SetWindowBordered(g_window, SDL_FALSE);
}

void window_center(window_handle window, int32_t width, int32_t height)
{
    int current_width;
    int current_height;

    (void)window;
    SDL_GetWindowSize(g_window, &current_width, &current_height);
    if (current_width != width || current_height != height) {
        SDL_SetWindowSize(g_window, width, height);
        SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(g_window);
    }
}

bool window_activate_existing(const char *class_name, const char *title)
{
#ifdef _WIN32
    HWND window = FindWindowA(class_name, title);
    WINDOWPLACEMENT placement;

    if (window == nullptr) {
        return false;
    }
    placement.length = sizeof(placement);
    GetWindowPlacement(window, &placement);
    SetForegroundWindow(window);
    if (placement.showCmd == SW_SHOWMINIMIZED) {
        ShowWindow(window, SW_RESTORE);
    }
    return true;
#else
    (void)class_name;
    (void)title;
    return false;
#endif
}

void desktop_bounds(window_rect *rect)
{
    SDL_Rect bounds = display_bounds();

    rect->left = bounds.x;
    rect->top = bounds.y;
    rect->right = bounds.x + bounds.w;
    rect->bottom = bounds.y + bounds.h;
}

uint32_t desktop_bits_per_pixel()
{
    SDL_DisplayMode mode;

    if (SDL_GetDesktopDisplayMode(0, &mode) != 0) {
        return 32;
    }
    return SDL_BYTESPERPIXEL(mode.format) * 8;
}

void pump_messages()
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        sdl_input_event(event);
        dispatch(event);
    }
    if (g_window != nullptr) {
        update_splash_cursor();
    }
}

void wait_for_messages(uint32_t milliseconds)
{
    SDL_WaitEventTimeout(nullptr, static_cast<int>(milliseconds));
}

int32_t cursor_show(bool show)
{
    return SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void cursor_position(int32_t *x, int32_t *y)
{
    int global_x;
    int global_y;

    SDL_GetGlobalMouseState(&global_x, &global_y);
    *x = global_x;
    *y = global_y;
}

bool gl_context_create(window_handle window)
{
    SDL_GLContext context;

    (void)window;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    context = SDL_GL_CreateContext(g_window);
    return context != nullptr && SDL_GL_MakeCurrent(g_window, context) == 0;
}

void gl_drawable_size(window_handle window, uint32_t *width, uint32_t *height)
{
    int w = 0;
    int h = 0;

    (void)window;
    SDL_GL_GetDrawableSize(g_window, &w, &h);
    *width = (uint32_t)w;
    *height = (uint32_t)h;
}

void gl_swap(window_handle window)
{
    (void)window;
    SDL_GL_SwapWindow(g_window);
}

void *gl_proc_address(const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

bool gamma_ramp_get(window_handle window, void *ramp)
{
    uint16_t *entries = static_cast<uint16_t *>(ramp);

    (void)window;
    return SDL_GetWindowGammaRamp(g_window, entries, entries + 256, entries + 512) == 0;
}

bool gamma_ramp_set(window_handle window, const void *ramp)
{
    const uint16_t *entries = static_cast<const uint16_t *>(ramp);

    (void)window;
    return SDL_SetWindowGammaRamp(g_window, entries, entries + 256, entries + 512) == 0;
}

}  // namespace halo::platform

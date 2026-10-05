/**
 * @file src/platform/window_win32.cpp
 * halo::platform window on Windows: the game window class, its window procedure (native messages turned into the
 * engine's window_events, with the same dispatch and results as the original procedure at 0x541b30), the message
 * loop, cursor and gamma ramp.
 */

#include "halo/platform/window.hpp"

#include "win32.h"

namespace halo::platform {

namespace {

constexpr DWORD k_window_style = WS_OVERLAPPEDWINDOW;

window_events g_events;
HBITMAP g_splash_bitmap;
HDC g_splash_dc;
HCURSOR g_arrow_cursor;
const char *g_class_name;
HINSTANCE g_instance;

LRESULT paint_splash(HWND window)
{
    if (g_splash_bitmap != nullptr) {
        HDC dc = GetDC(window);
        RECT client;
        BITMAP info;

        GetClientRect(window, &client);
        GetObjectA(g_splash_bitmap, sizeof(info), &info);
        StretchBlt(dc, 0, 0, client.right - client.left, client.bottom - client.top, g_splash_dc, 0, 0, info.bmWidth, info.bmHeight, SRCCOPY);
        ReleaseDC(window, dc);
    }
    ValidateRect(window, nullptr);
    return 0;
}

LRESULT keyboard_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (wparam == VK_LWIN || wparam == VK_RWIN) {
        if (g_events.windows_key()) {
            SetForegroundWindow(GetDesktopWindow());
            return DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return g_events.key_message(message, static_cast<uint32_t>(wparam)) ? 0 : DefWindowProcA(window, message, wparam, lparam);
}

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (g_events.bypass == nullptr || g_events.bypass()) {
        return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message <= WM_NCHITTEST) {
        switch (message) {
        case WM_NCHITTEST:
            return g_events.exclusive_fullscreen() ? 0 : DefWindowProcA(window, message, wparam, lparam);
        case WM_DESTROY:
        case WM_CLOSE:
            PostQuitMessage(0);
            g_events.closed();
            break;
        case WM_SIZE:
            if (wparam == SIZE_MINIMIZED) {
                g_events.minimized();
            } else if (wparam == SIZE_MAXIMIZED) {
                g_events.maximized();
            } else if (wparam == SIZE_RESTORED) {
                g_events.restored();
            }
            break;
        case WM_SETFOCUS:
            g_events.focus_gained();
            break;
        case WM_KILLFOCUS:
            g_events.focus_lost();
            break;
        case WM_PAINT:
            switch (g_events.paint()) {
            case window_paint::splash:
                return paint_splash(window);
            case window_paint::painted:
                ValidateRect(window, nullptr);
                return 0;
            default:
                break;
            }
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_ACTIVATEAPP:
            g_events.activate_app(wparam == 0);
            break;
        case WM_SETCURSOR:
            if (g_events.splash_cursor()) {
                if (static_cast<int16_t>(lparam) == HTCLIENT && GetForegroundWindow() == window) {
                    SetCursor(nullptr);
                    return 1;
                }
                SetCursor(g_arrow_cursor);
            }
            return 1;
        case WM_INPUTLANGCHANGE:
            return keyboard_message(window, message, wparam, lparam);
        case WM_DISPLAYCHANGE:
            g_events.display_changed(static_cast<uint32_t>(wparam));
            break;
        default:
            break;
        }
        return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message == WM_SYSCOMMAND) {
        // the game window cannot be maximised, sized or moved, and keeps the screensaver, monitor power-down and
        // the Alt menu away
        if (wparam <= SC_MAXIMIZE) {
            if (wparam != SC_MAXIMIZE && wparam != SC_SIZE && wparam != SC_MOVE) {
                return DefWindowProcA(window, message, wparam, lparam);
            }
        } else if (wparam != SC_KEYMENU && wparam != SC_MONITORPOWER) {
            return DefWindowProcA(window, message, wparam, lparam);
        }
        return 0;
    }

    if (message < WM_TIMER) {
        switch (message) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSCHAR:
        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION:
            return keyboard_message(window, message, wparam, lparam);
        case WM_CHAR:
        case WM_SYSKEYDOWN:
            return g_events.key_message(message, static_cast<uint32_t>(wparam)) ? 0 : DefWindowProcA(window, message, wparam, lparam);
        default:
            return DefWindowProcA(window, message, wparam, lparam);
        }
    }

    if (message < WM_EXITSIZEMOVE) {
        if (message == WM_ENTERSIZEMOVE) {
            g_events.suspend();
        } else if (message == WM_INITMENUPOPUP) {
            if (static_cast<uint32_t>(lparam) >> 16 == 1) {
                g_events.suspend();
            }
        } else if (message == WM_POWERBROADCAST) {
            if (wparam == 0) {
                g_events.power(false);
                return 1;
            }
            if (wparam == PBT_APMRESUMESUSPEND) {
                g_events.power(true);
                return 1;
            }
        }
        return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message == WM_EXITSIZEMOVE) {
        g_events.resume();
        return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message < WM_IME_SETCONTEXT || message > WM_IME_COMPOSITIONFULL) {
        return DefWindowProcA(window, message, wparam, lparam);
    }
    return keyboard_message(window, message, wparam, lparam);
}

/** The window rectangle that gives a width by height client area centred on the desktop. */
RECT centered_rect(int32_t width, int32_t height)
{
    RECT rect;

    GetWindowRect(GetDesktopWindow(), &rect);
    rect.top = static_cast<LONG>(static_cast<uint32_t>((rect.bottom - rect.top) - height) >> 1);
    rect.bottom = rect.top + height;
    rect.left = static_cast<LONG>(static_cast<uint32_t>((rect.right - rect.left) - width) >> 1);
    rect.right = rect.left + width;
    AdjustWindowRect(&rect, k_window_style, FALSE);
    return rect;
}

}  // namespace

window_handle window_create(void *instance, void *resource_module, const char *class_name, const char *title, int32_t width, int32_t height,
    uint32_t icon_resource, uint32_t splash_resource, const window_events &events)
{
    WNDCLASSEXA window_class = {};
    RECT rect;
    HWND window;

    g_events = events;
    g_class_name = class_name;
    g_instance = static_cast<HINSTANCE>(instance);
    g_arrow_cursor = LoadCursorA(nullptr, IDC_ARROW);

    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = g_instance;
    window_class.hIcon = LoadIconA(g_instance, MAKEINTRESOURCEA(icon_resource));
    window_class.hIconSm = LoadIconA(g_instance, MAKEINTRESOURCEA(icon_resource));
    window_class.hCursor = g_arrow_cursor;
    window_class.lpszClassName = class_name;
    RegisterClassExA(&window_class);

    rect = centered_rect(width, height);
    window = CreateWindowExA(0, class_name, title, k_window_style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        GetDesktopWindow(), nullptr, g_instance, nullptr);
    if (window == nullptr) {
        char *message_buffer = nullptr;

        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);
        MessageBoxA(nullptr, message_buffer, "ERROR - failed to create window", MB_ICONINFORMATION);
        UnregisterClassA(class_name, g_instance);
        LocalFree(message_buffer);
        return nullptr;
    }

    g_splash_bitmap = LoadBitmapA(static_cast<HINSTANCE>(resource_module), MAKEINTRESOURCEA(splash_resource));
    if (g_splash_bitmap != nullptr) {
        g_splash_dc = CreateCompatibleDC(GetDC(window));
        SelectObject(g_splash_dc, g_splash_bitmap);
    }

    SetForegroundWindow(window);
    SetActiveWindow(window);
    SetFocus(window);
    ShowWindow(window, SW_SHOW);
    return window;
}

void window_destroy(window_handle window)
{
    if (g_splash_dc != nullptr) {
        ReleaseDC(static_cast<HWND>(window), g_splash_dc);
        g_splash_dc = nullptr;
    }
    if (g_splash_bitmap != nullptr) {
        DeleteObject(g_splash_bitmap);
        g_splash_bitmap = nullptr;
    }
    ShowWindow(static_cast<HWND>(window), SW_HIDE);
    DestroyWindow(static_cast<HWND>(window));
}

void window_show(window_handle window, uint32_t command)
{
    ShowWindow(static_cast<HWND>(window), static_cast<int>(command));
}

void window_set_fullscreen_style(window_handle window)
{
    SetWindowLongA(static_cast<HWND>(window), GWL_STYLE, static_cast<LONG>(WS_POPUP | WS_VISIBLE | WS_SYSMENU));
}

void window_center(window_handle window, int32_t width, int32_t height)
{
    RECT current;
    RECT target = centered_rect(width, height);

    GetWindowRect(static_cast<HWND>(window), &current);
    if (current.top != target.top || current.bottom != target.bottom || current.left != target.left || current.right != target.right) {
        MoveWindow(static_cast<HWND>(window), target.left, target.top, target.right - target.left, target.bottom - target.top, TRUE);
        ShowWindow(static_cast<HWND>(window), SW_SHOW);
    }
}

void window_bounds(window_handle window, window_rect *rect)
{
    GetWindowRect(static_cast<HWND>(window), reinterpret_cast<RECT *>(rect));
}

void window_move(window_handle window, int32_t x, int32_t y, int32_t width, int32_t height)
{
    MoveWindow(static_cast<HWND>(window), x, y, width, height, TRUE);
}

void window_focus_desktop()
{
    SetForegroundWindow(GetDesktopWindow());
}

bool window_activate_existing(const char *class_name, const char *title)
{
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
}

void desktop_bounds(window_rect *rect)
{
    GetWindowRect(GetDesktopWindow(), reinterpret_cast<RECT *>(rect));
}

void desktop_client_area(window_rect *rect)
{
    GetClientRect(GetDesktopWindow(), reinterpret_cast<RECT *>(rect));
}

uint32_t desktop_bits_per_pixel()
{
    HDC dc = GetDC(GetDesktopWindow());
    uint32_t bits = static_cast<uint32_t>(GetDeviceCaps(dc, BITSPIXEL));

    ReleaseDC(GetDesktopWindow(), dc);
    return bits;
}

void pump_messages()
{
    MSG message;

    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
}

void wait_for_messages(uint32_t milliseconds)
{
    MsgWaitForMultipleObjects(0, nullptr, FALSE, milliseconds, QS_ALLINPUT);
}

int32_t cursor_show(bool show)
{
    return ShowCursor(show ? TRUE : FALSE);
}

void cursor_position(int32_t *x, int32_t *y)
{
    POINT point;

    GetCursorPos(&point);
    *x = point.x;
    *y = point.y;
}

bool mouse_buttons_swapped()
{
    return GetSystemMetrics(SM_SWAPBUTTON) != 0;
}

bool gamma_ramp_get(window_handle window, void *ramp)
{
    HDC dc = GetDC(static_cast<HWND>(window));
    bool ok = GetDeviceGammaRamp(dc, ramp) != 0;

    ReleaseDC(static_cast<HWND>(window), dc);
    return ok;
}

bool gamma_ramp_set(window_handle window, const void *ramp)
{
    HDC dc = GetDC(static_cast<HWND>(window));
    bool ok = SetDeviceGammaRamp(dc, const_cast<void *>(ramp)) != 0;

    ReleaseDC(static_cast<HWND>(window), dc);
    return ok;
}

}  // namespace halo::platform

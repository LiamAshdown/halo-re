#include "halo/shell/window.hpp"
#include "sound.h"
#include "interface.h"
#include "main.h"
#include "halo/sound/api.hpp"

typedef struct win32_bitmap {
    int32_t type;
    int32_t width;
    int32_t height;
    uint8_t unknown_0c[0xc];
} win32_bitmap;

static_assert(sizeof(win32_bitmap) == 0x18, "win32_bitmap layout");

extern "C" {
extern uint32_t time_query_performance_counter_ms(void);
extern void input_directinput_acquire_devices(void);
extern void input_directinput_unacquire_devices(void);
extern void input_reset_state_and_axis_configs(void);
extern void input_record_windows_key_message(int32_t key_or_char, int32_t message);
extern void input_key_block_timer_set(int16_t key, int32_t duration_ms);
extern void chat_close(void);
extern void chat_submit_input(void);
extern int32_t render_device_is_ready(void);
extern void rasterizer_capture_and_present(const int16_t *tile, void *bitmap);

extern uint8_t shell_window_proc_bypass;
extern uint8_t shell_application_inactive;
extern void *shell_window;
extern uint8_t shell_window_minimized;
extern uint8_t shell_window_maximized;
extern void *shell_arrow_cursor;
extern int32_t nowindowskey;

extern main_globals main_globals_data;
extern int32_t movie_playback_abort;
extern int32_t game_time_force_single_tick;

extern int32_t rasterizer_window_requested;
extern uint8_t rasterizer_fullscreen;
extern void *rasterizer_device;
extern void *rasterizer_window_icon_dc;
extern void *rasterizer_window_icon_bitmap;

extern uint8_t sound_paused;

extern uint8_t chat_dialog_open;

extern void *keystone_module;
extern void *chat_gui_root_handle;
extern keystone_dispatch_message_fn keystone_dispatch_message;
extern chat_gui_release_fn chat_gui_release;
extern keystone_translate_accelerator_fn keystone_translate_accelerator;
}

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
        if (rasterizer_fullscreen == 0 || rasterizer_device == 0) {
            if (sound_paused != 1) {
                sound_paused = 1;
                if (halo::sound::globals().current_driver != 0) {
                    halo::sound::globals().current_driver->set_paused(1);
                }
            }
        } else {
            halo::sound::sound_pause();
        }
        input_directinput_unacquire_devices();
        input_reset_state_and_axis_configs();
        if (shell_window != 0 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
            ShowWindow((HWND)shell_window, 6);
        }
        chat_close();
    }
}

/**
 * The game's window procedure. Handles suspend and resume around focus loss, close and destroy
 * quitting, the splash bitmap paint before the device exists, screensaver and monitor power
 * suppression while the device runs, the Windows key chord, Keystone accelerator translation and
 * Enter/Escape chat routing for keyboard messages, and defers everything else to DefWindowProcA.
 *
 * @address 0x541b30
 */
int32_t __stdcall GameWindow::procedure(HWND hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    HDC dc;
    win32_rect client_rect;
    win32_bitmap splash_bitmap_info;
    HWND desktop;
    int32_t handled;
    void *released_window;

    if (shell_window_proc_bypass != 0) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message <= 0x84) {
        if (message == 0x84) {
            if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                return 0;
            }
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
        switch (message) {
        case 2:
        case 0x10:
            PostQuitMessage(0);
            main_globals_data.return_to_main_menu = 0;
            main_globals_data.quit = 1;
            movie_playback_abort = 1;
            return DefWindowProcA(hwnd, message, wparam, lparam);

        case 5:
            if (wparam == 1) {
                suspend_focus();
                shell_window_minimized = 1;
            size_restored_tail:
                shell_window_maximized = 0;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (wparam == 2) {
                if (shell_window_minimized != 0 && shell_application_inactive != 0) {
                    shell_application_inactive = 0;
                    input_directinput_acquire_devices();
                    input_reset_state_and_axis_configs();
                    if (shell_window != 0) {
                        ShowWindow((HWND)shell_window, 9);
                    }
                    if (shell_window_proc_bypass == 0) {
                        if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                            halo::sound::sound_resume();
                            shell_window_minimized = 0;
                            shell_window_maximized = 1;
                            return DefWindowProcA(hwnd, message, wparam, lparam);
                        }
                        if (sound_paused != 0) {
                            sound_paused = 0;
                            if (halo::sound::globals().current_driver != 0) {
                                halo::sound::globals().current_driver->set_paused(0);
                            }
                            halo::sound::globals().time = time_query_performance_counter_ms();
                        }
                    }
                }
                shell_window_minimized = 0;
                shell_window_maximized = 1;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (wparam != 0) {
                break;
            }
            if (shell_window_maximized != 0) {
                goto size_restored_tail;
            }
            if (shell_window_minimized != 0) {
                if (shell_application_inactive != 0) {
                    shell_application_inactive = 0;
                    input_directinput_acquire_devices();
                    input_reset_state_and_axis_configs();
                    if (shell_window != 0) {
                        ShowWindow((HWND)shell_window, 9);
                    }
                    if (shell_window_proc_bypass == 0) {
                        if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                            halo::sound::sound_resume();
                            shell_window_minimized = 0;
                            return DefWindowProcA(hwnd, message, wparam, lparam);
                        }
                        if (sound_paused != 0) {
                            sound_paused = 0;
                            if (halo::sound::globals().current_driver != 0) {
                                halo::sound::globals().current_driver->set_paused(0);
                            }
                            halo::sound::globals().time = time_query_performance_counter_ms();
                        }
                    }
                }
                shell_window_minimized = 0;
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            if (render_device_is_ready() == 0) {
                break;
            }
            goto resume_focus_fast_path;

        case 7:
            if (shell_window == 0) {
                break;
            }
        restore_from_suspend:
            if (shell_application_inactive != 0) {
                shell_application_inactive = 0;
                input_directinput_acquire_devices();
                input_reset_state_and_axis_configs();
                if (shell_window != 0) {
                    ShowWindow((HWND)shell_window, 9);
                }
                if (shell_window_proc_bypass == 0) {
                    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                    resume_focus_fast_path:
                        halo::sound::sound_resume();
                        return DefWindowProcA(hwnd, message, wparam, lparam);
                    }
                    if (sound_paused != 0) {
                        sound_paused = 0;
                        if (halo::sound::globals().current_driver != 0) {
                            halo::sound::globals().current_driver->set_paused(0);
                        }
                        halo::sound::globals().time = time_query_performance_counter_ms();
                    }
                }
            }
            break;

        case 8:
            if (shell_window != 0 && shell_application_inactive != 1) {
                suspend_focus();
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0xf:
            if (render_device_is_ready() == 0 && rasterizer_window_requested == 0) {
                if (rasterizer_device == 0) {
                    if (rasterizer_window_icon_bitmap != 0) {
                        dc = GetDC(hwnd);
                        GetClientRect(hwnd, &client_rect);
                        GetObjectA(rasterizer_window_icon_bitmap, 0x18, &splash_bitmap_info);
                        StretchBlt(dc, 0, 0, client_rect.right - client_rect.left,
                                   client_rect.bottom - client_rect.top,
                                   (HDC)rasterizer_window_icon_dc, 0, 0,
                                   splash_bitmap_info.width, splash_bitmap_info.height,
                                   0xcc0020);
                        ReleaseDC(hwnd, dc);
                    }
                    ValidateRect(hwnd, (win32_rect *)0);
                    return 0;
                }
                rasterizer_capture_and_present((const int16_t *)0, (void *)0);
                ValidateRect(hwnd, (win32_rect *)0);
                return 0;
            }
            if (rasterizer_device != 0) {
                rasterizer_capture_and_present((const int16_t *)0, (void *)0);
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0x14:
            return 1;

        case 0x1c:
            if (rasterizer_window_requested == 0 && game_time_force_single_tick == 0 && shell_window != 0) {
                handle_activate_app(wparam == 0);
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        case 0x20:
            if (render_device_is_ready() == 0 && rasterizer_window_requested == 0) {
                if ((int16_t)lparam == 1 && GetForegroundWindow() == hwnd) {
                    SetCursor((HCURSOR)0);
                    return 1;
                }
                SetCursor((HCURSOR)shell_arrow_cursor);
            }
            return 1;

        case 0x51:
            goto keyboard_message;

        case 0x7e:
            if (shell_window != 0 && wparam != 0x20) {
                suspend_focus();
                ShowWindow((HWND)shell_window, 6);
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
            break;

        default:
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message == 0x112) {
        if (wparam < 0xf031) {
            if (wparam != 0xf030 && wparam != 0xf000 && wparam != 0xf010) {
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else {
            if (wparam == 0xf100) {
                return 0;
            }
            if (wparam != 0xf170) {
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        }
        if (render_device_is_ready() != 1) {
            return 0;
        }
        return 0;
    }

    if (message < 0x113) {
        switch (message) {
        case 0x100:
        case 0x101:
        case 0x106:
        case 0x10d:
        case 0x10e:
        case 0x10f:
            goto keyboard_message;
        case 0x102:
        case 0x104:
            goto keystone_dispatch;
        default:
            return DefWindowProcA(hwnd, message, wparam, lparam);
        }
    }

    if (message < 0x232) {
        if (message == 0x231) {
            if (shell_application_inactive != 1) {
                suspend_focus();
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else if (message == 0x117) {
            if ((uint32_t)lparam >> 0x10 == 1 && shell_application_inactive != 1) {
                suspend_focus();
                return DefWindowProcA(hwnd, message, wparam, lparam);
            }
        } else if (message == 0x218) {
            if (wparam == 0) {
                halo::sound::sound_pause();
                return 1;
            }
            if (wparam == 7) {
                halo::sound::sound_resume();
                return 1;
            }
        }
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

    if (message == 0x232) {
        goto restore_from_suspend;
    }

    if (message < 0x281 || message > 0x284) {
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

keyboard_message:
    if ((wparam == 0x5b || wparam == 0x5c) && nowindowskey == 0 && shell_window != 0) {
        if (shell_application_inactive != 1) {
            suspend_focus();
        }
        desktop = GetDesktopWindow();
        SetForegroundWindow(desktop);
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }

keystone_dispatch:
    if (chat_gui_root_handle != 0 && keystone_module != 0) {
        handled = 1;
        released_window = keystone_dispatch_message(chat_gui_root_handle, message, wparam, lparam, &handled);
        if (released_window != 0) {
            chat_gui_release(released_window);
        }
        if (message == 0x100) {
            if (wparam == 0xd) {
                if (chat_dialog_open != 0) {
                    chat_submit_input();
                    input_key_block_timer_set(0x38, 200);
                    input_key_block_timer_set(0x66, 200);
                    return 0;
                }
            } else if (wparam == 0x1b) {
                if (chat_dialog_open != 0) {
                    input_key_block_timer_set(0, 0xfa);
                }
                chat_close();
            }
        }
        if (handled == 0) {
            input_record_windows_key_message(wparam, message);
            return 0;
        }
    }
    input_record_windows_key_message(wparam, message);
    return DefWindowProcA(hwnd, message, wparam, lparam);
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
        input_directinput_acquire_devices();
    } else {
        if (shell_window_proc_bypass == 0) {
            if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                halo::sound::sound_pause();
            } else if (sound_paused != 1) {
                sound_paused = 1;
                if (halo::sound::globals().current_driver != 0) {
                    halo::sound::globals().current_driver->set_paused(1);
                }
            }
        }
        input_directinput_unacquire_devices();
    }
    input_reset_state_and_axis_configs();

    if (shell_window != 0) {
        fullscreen_device = rasterizer_fullscreen != 0 && rasterizer_device != 0;
        if (fullscreen_device) {
            ShowWindow((HWND)shell_window, inactive != 0 ? 6 : 9);
        } else if (inactive == 0) {
            ShowWindow((HWND)shell_window, 9);
        }
    }
    if (inactive != 0) {
        chat_close();
        return;
    }

    if (shell_window_proc_bypass != 0) {
        return;
    }
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        halo::sound::sound_resume();
        return;
    }
    if (sound_paused != 0) {
        sound_paused = 0;
        if (halo::sound::globals().current_driver != 0) {
            halo::sound::globals().current_driver->set_paused(0);
        }
        halo::sound::globals().time = time_query_performance_counter_ms();
    }
}

/**
 * Drains the Win32 message queue each frame. While the Keystone UI is loaded, messages go through its
 * accelerator translator first; the rest take the normal translate and dispatch path.
 *
 * @address 0x541a20
 */
void GameWindow::pump_messages()
{
    uint8_t message[28];
    int32_t has_message;
    int32_t handled;

    has_message = PeekMessageA((LPMSG)message, 0, 0, 0, 1);
    while (has_message != 0) {
        if (chat_gui_root_handle == 0 || keystone_module == 0) {
            TranslateMessage((const MSG *)message);
            DispatchMessageA((const MSG *)message);
        } else {
            handled = (int32_t)keystone_translate_accelerator(chat_gui_root_handle, shell_window, 0, message);
            if (handled == 0) {
                TranslateMessage((const MSG *)message);
                DispatchMessageA((const MSG *)message);
            }
        }
        has_message = PeekMessageA((LPMSG)message, 0, 0, 0, 1);
    }
}

}

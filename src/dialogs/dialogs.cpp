#include "halo/dialogs/dialogs.hpp"
#include "halo/dialogs/api.hpp"
#include "halo/shell/api.hpp"

extern "C" {
extern int32_t dialog_hyperlink_hovered;
extern uint32_t shell_language_id;
extern int32_t sprintf(char *buffer, const char *format, ...);
extern char fatal_error_text[k_shell_fatal_error_text_length];
extern char fatal_error_help_file[k_shell_fatal_error_readme_length];
extern char fatal_error_title[k_shell_fatal_error_title_length];
extern int32_t fatal_error_is_fatal;
extern char *graphics_vendor_name;
extern char *graphics_device_name;
extern uint32_t graphics_device_id;
extern uint32_t cpu_speed;
extern uint32_t physical_memory;
extern uint32_t video_memory;
extern int32_t fatal_error_remember_choice;
extern char fatal_error_system_specs[halo::dialogs::k_system_specs_capacity];
}

namespace halo::dialogs {

/**
 * Converts a static text control (`control`) into a clickable, underlined hyperlink-style
 * control: subclasses the control's parent dialog with dialog_static_hyperlink_parent_proc,
 * unless it is already subclassed; ORs SS_NOTIFY into the control's window style so it starts
 * receiving WM_SETCURSOR/WM_MOUSEMOVE; subclasses the control itself with dialog_static_
 * hyperlink_subclass_proc, saving the previous WNDPROC under "Old_Proc".
 *
 * Register convention in the original: control HWND in ESI (unaff_ESI), no stack arguments,
 * always returns 1.
 *
 * @address 0x57e4c0
 */
int32_t StaticHyperlink::install()
{
    void *control = (void *)window;

    void *parent;
    void *previous_wnd_proc;
    int32_t style;
    void *previous_font;
    void *underlined_font;
    win32_logfonta logfont;

    parent = GetParent((HWND)control);
    if (parent != 0) {
        previous_wnd_proc = (void *)GetWindowLongA((HWND)parent, k_dialog_window_long_wndproc);
        if (previous_wnd_proc != (void *)halo::dialogs::dialog_static_hyperlink_parent_proc) {
            SetPropA((HWND)parent, "Old_Proc", previous_wnd_proc);
            SetWindowLongA((HWND)parent, k_dialog_window_long_wndproc, (int32_t)halo::dialogs::dialog_static_hyperlink_parent_proc);
        }
    }

    style = GetWindowLongA((HWND)control, k_dialog_window_long_style);
    SetWindowLongA((HWND)control, k_dialog_window_long_style, style | k_dialog_static_style_notify);

    previous_wnd_proc = (void *)GetWindowLongA((HWND)control, k_dialog_window_long_wndproc);
    SetPropA((HWND)control, "Old_Proc", previous_wnd_proc);
    SetWindowLongA((HWND)control, k_dialog_window_long_wndproc, (int32_t)halo::dialogs::dialog_static_hyperlink_subclass_proc);

    previous_font = (void *)SendMessageA((HWND)control, k_dialog_message_get_font, 0, 0);
    SetPropA((HWND)control, "Old_Font", previous_font);
    GetObjectA(previous_font, k_dialog_logfont_size, &logfont);
    logfont.underline = 1;
    underlined_font = CreateFontIndirectA((const LOGFONTA *)&logfont);
    SetPropA((HWND)control, "Font", underlined_font);
    SendMessageA((HWND)control, k_dialog_message_set_font, (uint32_t)underlined_font, 0);
    SetPropA((HWND)control, "Static", (void *)1);

    return 1;
}

/**
 * Window-subclass procedure for a hyperlink-style static control's PARENT dialog (installed by
 * dialog_static_hyperlink_install 0x57e4c0). On WM_DESTROY, unsubclasses the parent and
 * removes its "Old_Proc" property.
 *
 * Register convention in the original: out/phase4/dialogs_types_notes.md "0x57e350 and
 * 0x57e2a0 are __stdcall.
 *
 * @address 0x57e2a0
 */
int32_t __stdcall StaticHyperlink::parent_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    void *old_wnd_proc;
    void *static_marker;
    int32_t forwarded_result;

    old_wnd_proc = GetPropA((HWND)hwnd, "Old_Proc");

    if (message == k_dialog_message_destroy) {
        SetWindowLongA((HWND)hwnd, k_dialog_window_long_wndproc, (int32_t)old_wnd_proc);
        RemovePropA((HWND)hwnd, "Old_Proc");
    } else if (message == k_dialog_message_ctl_color_static) {
        static_marker = GetPropA((HWND)((void *)lparam), "Static");
        if (static_marker != (void *)0) {
            forwarded_result = CallWindowProcA((WNDPROC)old_wnd_proc, (HWND)hwnd, message, wparam, lparam);
            if (dialog_hyperlink_hovered != 0) {
                SetTextColor((HDC)((void *)wparam), k_dialog_hyperlink_color_hover);
            } else {
                SetTextColor((HDC)((void *)wparam), k_dialog_hyperlink_color_normal);
            }
            return forwarded_result;
        }
    }

    return CallWindowProcA((WNDPROC)old_wnd_proc, (HWND)hwnd, message, wparam, lparam);
}

/**
 * Window-subclass procedure for a hyperlink-style static control (installed by
 * dialog_static_hyperlink_install 0x57e4c0). On WM_DESTROY, unsubclasses the control, restores
 * its original font and removes the properties this module set. On WM_SETCURSOR, shows the
 * hand cursor (falling back to the arrow if IDC_HAND is unavailable) and eats the message.
 *
 * Register convention in the original: out/phase4/dialogs_types_notes.md "0x57e350 and
 * 0x57e2a0 are __stdcall WNDPROCs (ret 0x10)." All four parameters are Ghidra-recognized stack
 * arguments already, no register-passed arguments to remap.
 *
 * @address 0x57e350
 */
int32_t __stdcall StaticHyperlink::subclass_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    void *old_wnd_proc;
    void *saved_value;
    void *capture_window;
    void *cursor;
    win32_rect window_rect;
    win32_point cursor_point;

    old_wnd_proc = GetPropA((HWND)hwnd, "Old_Proc");

    if (message == k_dialog_message_destroy) {
        SetWindowLongA((HWND)hwnd, k_dialog_window_long_wndproc, (int32_t)old_wnd_proc);
        RemovePropA((HWND)hwnd, "Old_Proc");
        saved_value = GetPropA((HWND)hwnd, "Old_Font");
        SendMessageA((HWND)hwnd, k_dialog_message_set_font, (uint32_t)saved_value, 0);
        RemovePropA((HWND)hwnd, "Old_Font");
        saved_value = GetPropA((HWND)hwnd, "Font");
        DeleteObject(saved_value);
        RemovePropA((HWND)hwnd, "Font");
        RemovePropA((HWND)hwnd, "Static");
    } else if (message == k_dialog_message_set_cursor) {
        cursor = LoadCursorA(0, (const char *)k_dialog_cursor_hand);
        if (cursor == 0) {
            cursor = LoadCursorA(0, (const char *)k_dialog_cursor_arrow);
        }
        SetCursor((HCURSOR)cursor);
        return 1;
    } else if (message == k_dialog_message_mouse_move) {
        capture_window = GetCapture();
        if (capture_window == hwnd) {
            GetWindowRect((HWND)hwnd, &window_rect);
            cursor_point.x = (int32_t)((uint32_t)lparam & k_low_word_mask);
            cursor_point.y = (int32_t)((uint32_t)lparam >> k_high_word_shift);
            ClientToScreen((HWND)hwnd, &cursor_point);
            if (!PtInRect(&window_rect, cursor_point)) {
                dialog_hyperlink_hovered = 0;
                InvalidateRect((HWND)hwnd, (win32_rect *)0, 0);
                ReleaseCapture();
            }
        } else {
            dialog_hyperlink_hovered = 1;
            InvalidateRect((HWND)hwnd, (win32_rect *)0, 0);
            SetCapture((HWND)hwnd);
        }
    }

    return CallWindowProcA((WNDPROC)old_wnd_proc, (HWND)hwnd, message, wparam, lparam);
}

/**
 * Shows a modal dialog box built from `template_name` (a resource name, or a MAKEINTRESOURCE
 * id such as the fatal-error dialog's 0x66). Tries the RT_DIALOG resource in the current UI
 * language first, then in English (0x409) if that differs, and finally falls back to a plain
 * DialogBoxParamA lookup of `template_name` by the module's default language. Returns the
 * DialogBox result (an INT_PTR) of whichever attempt actually ran the dialog. blam-cc: EBX ->
 * dialog_proc, ESI -> module, stack -> template_name, parent_window
 *
 * @address 0x57e1f0
 */
int32_t DialogLoader::show_localized(dialog_window_proc_fn dialog_proc, void *module, const char *template_name, void *parent_window)
{
    void *resource_info;
    void *resource_data;
    const void *dialog_template;
    int32_t result;

    resource_info = FindResourceExA((HMODULE)module, (const char *)k_dialog_resource_type_dialog, template_name,
                                     (uint16_t)shell_language_id);
    if (resource_info != 0) {
        resource_data = LoadResource((HMODULE)module, (HRSRC)resource_info);
        if (resource_data != 0) {
            dialog_template = LockResource(resource_data);
            if (dialog_template != 0) {
                result = DialogBoxIndirectParamA((HINSTANCE)module, (LPCDLGTEMPLATEA)dialog_template, (HWND)parent_window, (DLGPROC)dialog_proc, 0);
                if (result != k_dialog_box_failed) {
                    return result;
                }
            }
        }
    }

    if (shell_language_id != k_dialog_language_english) {
        resource_info = FindResourceExA((HMODULE)module, (const char *)k_dialog_resource_type_dialog, template_name,
                                         k_dialog_language_english);
        if (resource_info != 0) {
            resource_data = LoadResource((HMODULE)module, (HRSRC)resource_info);
            if (resource_data != 0) {
                dialog_template = LockResource(resource_data);
                if (dialog_template != 0) {
                    result = DialogBoxIndirectParamA((HINSTANCE)module, (LPCDLGTEMPLATEA)dialog_template, (HWND)parent_window, (DLGPROC)dialog_proc, 0);
                    if (result != k_dialog_box_failed) {
                        return result;
                    }
                }
            }
        }
    }

    result = DialogBoxParamA((HINSTANCE)module, template_name, (HWND)parent_window, (DLGPROC)dialog_proc, 0);
    return result;
}

/**
 * The fatal-error dialog's DLGPROC. On WM_INITDIALOG, centers the dialog on the desktop, sets
 * the title and the main message static (0x3ee), disables the checkbox and the two "done"
 * buttons (0x3e9, 0x3ec, 3) while fatal_error_is_fatal is set, installs the hyperlink subclass
 * on the help-file static (0x3ef), and formats the system-specs static (0x3f1) from the
 * graphics / cpu / memory globals.
 *
 * Register convention in the original: plain __stdcall DLGPROC (dialog, message, wparam,
 * lparam), all on the.
 *
 * @address 0x57e5a0
 */
int32_t __stdcall FatalErrorDialog::proc(void *dialog, uint32_t message, uint32_t wparam, int32_t lparam)
{
    win32_rect dialog_rect;
    win32_rect desktop_rect;
    void *desktop_window;
    void *hyperlink_control;
    int32_t control_id;
    int32_t checked;

    if (message < message_id(dialog_message::command)) {
        if (message == message_id(dialog_message::close)) {
            if (halo::shell::globals().window != 0) {
                DestroyWindow((HWND)halo::shell::globals().window);
            }
            EndDialog((HWND)dialog, k_dialog_button_cancel);
            return 1;
        }
        if (message == message_id(dialog_message::init_dialog)) {
            GetWindowRect((HWND)dialog, &dialog_rect);
            desktop_window = GetDesktopWindow();
            GetClientRect((HWND)desktop_window, &desktop_rect);
            MoveWindow((HWND)dialog,
                       (desktop_rect.right - desktop_rect.left) / 2 - (dialog_rect.right - dialog_rect.left) / 2,
                       (desktop_rect.bottom - desktop_rect.top) / 2 - (dialog_rect.bottom - dialog_rect.top) / 2,
                       dialog_rect.right - dialog_rect.left, dialog_rect.bottom - dialog_rect.top, 1);
            SetWindowTextA((HWND)dialog, fatal_error_title);
            SetDlgItemTextA((HWND)dialog, id_of(fatal_error_control::message_text), fatal_error_text);
            if (fatal_error_is_fatal != 0) {
                EnableWindow(GetDlgItem((HWND)dialog, id_of(fatal_error_control::remember_choice_checkbox)), 0);
                EnableWindow(GetDlgItem((HWND)dialog, id_of(fatal_error_control::ignore_button)), 0);
                EnableWindow(GetDlgItem((HWND)dialog, k_dialog_button_abort), 0);
            }
            hyperlink_control = GetDlgItem((HWND)dialog, id_of(fatal_error_control::help_link));
            halo::dialogs::dialog_static_hyperlink_install(hyperlink_control);
            if (graphics_device_id == 0) {
                if (cpu_speed == 0 || physical_memory == 0) {
                    fatal_error_system_specs[0] = 0;
                } else {
                    sprintf(fatal_error_system_specs, "%dMHz, %dMB", cpu_speed, physical_memory);
                }
            } else {
                sprintf(fatal_error_system_specs, "%dMHz, %dMB, %dM %s %s (0x%04x)", cpu_speed, physical_memory,
                        video_memory >> k_megabyte_shift, graphics_vendor_name, graphics_device_name, graphics_device_id);
            }
            SetDlgItemTextA((HWND)dialog, id_of(fatal_error_control::system_specs_text), fatal_error_system_specs);
            return 1;
        }
        return 0;
    }

    if (message == message_id(dialog_message::command)) {
        control_id = (int32_t)(uint16_t)wparam; 

        if (control_id <= id_of(fatal_error_control::ignore_button)) {
            if (control_id == id_of(fatal_error_control::ignore_button)) {
                checked = IsDlgButtonChecked((HWND)dialog, id_of(fatal_error_control::remember_choice_checkbox));
                fatal_error_remember_choice = (checked != 0);
                EndDialog((HWND)dialog, 0);
                return 1;
            }
            if (control_id == k_dialog_button_cancel) {
                if (halo::shell::globals().window != 0) {
                    DestroyWindow((HWND)halo::shell::globals().window);
                }
                EndDialog((HWND)dialog, k_dialog_button_cancel);
                return 1;
            }
            if (control_id == k_dialog_button_abort) {
                checked = IsDlgButtonChecked((HWND)dialog, id_of(fatal_error_control::remember_choice_checkbox));
                fatal_error_remember_choice = (checked != 0);
                EndDialog((HWND)dialog, 1);
                return 1;
            }
        } else {
            if (control_id == id_of(fatal_error_control::quit_button)) {
                if (halo::shell::globals().window != 0) {
                    DestroyWindow((HWND)halo::shell::globals().window);
                }
                EndDialog((HWND)dialog, k_dialog_button_cancel);
                return 1;
            }
            if (control_id == id_of(fatal_error_control::help_link)) {
                ShellExecuteA((HWND)dialog, "open", fatal_error_help_file, (const char *)0, (const char *)0, 1);
                return 1;
            }
        }
        return 0;
    }

    if (message > message_id(dialog_message::command) && message == (uint32_t)id_of(fatal_error_control::quit_button)) {
        if (halo::shell::globals().window != 0) {
            DestroyWindow((HWND)halo::shell::globals().window);
        }
        EndDialog((HWND)dialog, k_dialog_button_cancel);
        return 1;
    }

    return 0;
}

}

namespace halo::dialogs {

int32_t dialog_static_hyperlink_install(void *control)
{
    return halo::dialogs::StaticHyperlink(control).install();
}

int32_t __stdcall dialog_static_hyperlink_parent_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    return halo::dialogs::StaticHyperlink::parent_proc(hwnd, message, wparam, lparam);
}

int32_t __stdcall dialog_static_hyperlink_subclass_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    return halo::dialogs::StaticHyperlink::subclass_proc(hwnd, message, wparam, lparam);
}

int32_t dialog_box_show_localized(dialog_window_proc_fn dialog_proc, void *module, const char *template_name, void *parent_window)
{
    return halo::dialogs::DialogLoader::show_localized(dialog_proc, module, template_name, parent_window);
}

int32_t __stdcall fatal_error_dialog_proc(void *dialog, uint32_t message, uint32_t wparam, int32_t lparam)
{
    return halo::dialogs::FatalErrorDialog::proc(dialog, message, wparam, lparam);
}

}

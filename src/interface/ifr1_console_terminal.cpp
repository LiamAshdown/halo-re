#include "halo/interface/ifr1_console_terminal.hpp"
#include "halo/interface/engine_state.hpp"
#include <stdarg.h>
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/text/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/main/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"

extern "C" {
extern char console_echo_prefix[];
extern datum_index console_message_new(void);
extern void chimera__console_out_copy(char *text);
extern uint8_t console_rcon_out_reentrant_guard;
extern void *console_output_handle;
extern void chimera__rcon_out(int32_t rcon_handle);
extern void console_clear_bottom_line(int32_t clear_all);
extern void console_draw_input_line(void);
extern void string_replace_all_in_place(char *buffer, char *search, char *replacement);
extern char console_window_title[0x20];
extern void console_position_cursor(void);
extern Globals *global_globals;
extern uint8_t console_caret_visible;
extern uint8_t console_show_messages;
extern uint16_t hud_text_draw_color_or_flags;
extern int32_t hud_text_draw_font_tag_id;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern uint32_t text_tab_stops;
extern uint32_t hud_text_draw_box_field_474e;
extern int16_t render_viewport_top[6];
extern void console_message_delete(datum_index message);
extern void widget_text_edit_clamp_selection(text_edit_state *state);
extern void console_restore_cursor(void);
extern uint32_t strlen(const char *s);
extern void *console_input_handle;
extern uint8_t controls_input_capture_flags;
extern int16_t key_event_read_index;
extern int16_t key_event_count;
extern ui_key_event key_events[];
extern void widget_text_edit_process_key(text_edit_state *state, ui_key_event *event);
extern char console_last_line[0x100];
extern int32_t console_last_cursor_column;
}

namespace halo::interface {

/**
 * Formats `format` with vsnprintf into a fresh console_message (defaulting its color to (1.0, 0.7, 0.7, 0.7)
 * when `color` is NULL), marks it as a command echo if its text contains the console's echo prefix, and
 * mirrors it out via chimera__console_out_copy. Does nothing if the terminal has not been initialized.
 * blam-cc: EAX -> color, stack -> format, ...
 *
 * @address 0x496b50
 */
void ConsoleTerminal::out(ColorARGB *color, char *format, va_list args)
{
    static const ColorARGB k_default_color = { 1.0f, 0.7f, 0.7f, 0.7f };
    datum_index message_handle;
    console_message *message;

    if (halo::main::globals().terminal_initialized == 0) {
        return;
    }

    message_handle = console_message_new();
    if (message_handle == (datum_index)0xffffffff) {
        return;
    }

    message = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                   (uint16_t)message_handle * sizeof(console_message));
    message->age = 0;
    if (color == (ColorARGB *)0) {
        color = (ColorARGB *)&k_default_color;
    }
    message->color = *color;
    _vsnprintf(message->text, 0xfe, format, args);

    message->is_command_echo = strstr(message->text, console_echo_prefix) != (char *)0;
    chimera__console_out_copy(message->text);
}

/**
 * Mirrors one printed console line: forwards it to an active rcon session (reentrancy-guarded), and, if a
 * win32 console is attached, copies the line into a scratch buffer, strips the two internal formatting tokens,
 * clears the console's bottom line and writes the result out followed by the developer-console line
 * terminator, then redraws the input line.
 * blam-cc: EAX -> text
 *
 * @address 0x496e90
 */
void ConsoleTerminal::out_copy(char *text)
{
    char line[0x104];
    uint32_t chars_written;
    uint32_t length;

    if (halo::main::globals().console_rcon_handle != -1 && console_rcon_out_reentrant_guard == 0) {
        console_rcon_out_reentrant_guard = 1;
        chimera__rcon_out(halo::main::globals().console_rcon_handle);
        console_rcon_out_reentrant_guard = 0;
    }
    if (halo::main::globals().console_win32_attached != 0) {
        line[0] = '\0';
        strncpy(line, text, 0x100);
        string_replace_all_in_place(line, console_echo_prefix, state::console_tab_text);
        string_replace_all_in_place(line, state::console_newline_escape, state::console_newline_text);
        length = strlen(line);
        console_clear_bottom_line(1);
        WriteConsoleA(console_output_handle, line, length, (LPDWORD)&chars_written, (void *)0);
        WriteConsoleA(console_output_handle, state::console_newline_text, 1, (LPDWORD)&chars_written, (void *)0);
        console_draw_input_line();
    }
}

/**
 * Moves the console cursor to column 0 of the last row; when clear_text is set, also blanks that entire row
 * (character then attribute) in the current attribute.
 *
 * @address 0x497010
 */
void ConsoleTerminal::clear_bottom_line(uint8_t clear_text)
{
    win32_console_screen_buffer_info info;
    win32_coord bottom_left;
    uint32_t written;

    if (halo::main::globals().console_win32_attached != 0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        bottom_left.X = 0;
        bottom_left.Y = (int16_t)(info.dwSize.Y - 1);
        SetConsoleCursorPosition(console_output_handle, bottom_left);
        if (clear_text != 0) {
            if (FillConsoleOutputCharacterA(console_output_handle, ' ', info.dwSize.X,
                                             bottom_left, (LPDWORD)&written) != 0) {
                FillConsoleOutputAttribute(console_output_handle, info.wAttributes, info.dwSize.X,
                                            bottom_left, (LPDWORD)&written);
            }
        }
    }
}

/**
 * Blanks every cell of the attached win32 console window: fills the whole buffer with spaces in the current
 * attribute, then reapplies that attribute over the same region.
 *
 * @address 0x496f90
 */
void ConsoleTerminal::clear_screen(void)
{
    win32_console_screen_buffer_info info;
    win32_coord origin = {0, 0};
    uint32_t written;

    if (halo::main::globals().console_win32_attached != 0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        if (FillConsoleOutputCharacterA(console_output_handle, ' ',
                                         (int32_t)info.dwSize.Y * (int32_t)info.dwSize.X,
                                         origin, (LPDWORD)&written) != 0) {
            FillConsoleOutputAttribute(console_output_handle, info.wAttributes,
                                        (int32_t)info.dwSize.Y * (int32_t)info.dwSize.X,
                                        origin, (LPDWORD)&written);
        }
    }
}

/**
 * Deactivates `console` if it is the currently active developer console: hides the win32 console cursor (when
 * one is attached) and clears console_active.
 *
 * @address 0x496580
 */
void ConsoleTerminal::close(terminal_console *console)
{
    win32_console_cursor_info info;
    int32_t ok;

    if (console == halo::main::globals().console_active) {
        if (halo::main::globals().console_win32_attached != 0) {
            ok = GetConsoleCursorInfo(console_output_handle, &info);
            if (ok != 0) {
                info.bVisible = 0;
                SetConsoleCursorInfo(console_output_handle, &info);
            }
        }
        halo::main::globals().console_active = (terminal_console *)0;
    }
}

/**
 * Draws "<window title> <input line>" over the last row of the attached win32 console window.
 *
 * @address 0x4970a0
 */
void ConsoleTerminal::draw_input_line(void)
{
    char line[0x11e];
    win32_console_screen_buffer_info info;
    win32_coord bottom_left;
    uint32_t written;
    uint32_t length;

    if (halo::main::globals().console_win32_attached != 0 && halo::main::globals().console_active != (terminal_console *)0) {
        _snprintf(line, 0x11e, "%s %s", console_window_title, halo::main::globals().console_active->input);
        strcpy(line + strlen(console_window_title), halo::main::globals().console_active->input);
        if (GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
            bottom_left.X = 0;
            bottom_left.Y = (int16_t)(info.dwSize.Y - 1);
            if (FillConsoleOutputCharacterA(console_output_handle, ' ', info.dwSize.X,
                                             bottom_left, (LPDWORD)&written) != 0) {
                length = strlen(line);
                WriteConsoleOutputCharacterA(console_output_handle, line, length, bottom_left,
                                              (LPDWORD)&written);
            }
            console_position_cursor();
        }
    }
}

/**
 * Draws the developer console overlay. When the terminal has been initialized: if a console is active, builds
 * "prompt + input" into a scratch line, splices in a caret glyph (0x7f) at the cursor position when the caret
 * is currently visible, and draws it through the globals font_terminal font; then, if message display is
 * enabled, draws each live console_message (newest first) climbing up the screen one line-height at a time,
 * fading each one's alpha by its age and boxing command-echo messages, until running out of vertical room.
 *
 * @address 0x496730
 */
void ConsoleTerminal::draw_overlay(void)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t font_terminal_id;
    Font *font;
    int16_t line_height;
    char line[288];
    int32_t cursor;
    datum_index message_handle;
    console_message *message;
    float fade;
    int16_t y;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    font_terminal_id = *(int32_t *)&interface_bitmaps->font_terminal.tag_id;

    if (halo::main::globals().terminal_initialized == 0) {
        return;
    }

    font = (Font *)halo::cache::globals().tag_instances[(uint16_t)font_terminal_id].data;
    line_height = font->ascending_height + font->descending_height + font->leading_height;

    if (halo::main::globals().console_active != (terminal_console *)0) {
        halo::main::globals().console_active->prompt[0x1f] = '\0';
        halo::main::globals().console_active->input[0xff] = '\0';
        strcpy(line, halo::main::globals().console_active->prompt);
        strcat(line, halo::main::globals().console_active->input);

        hud_text_draw_color_a = halo::main::globals().console_active->color.alpha;
        hud_text_draw_color_r = halo::main::globals().console_active->color.red;
        hud_text_draw_color_g = halo::main::globals().console_active->color.green;
        hud_text_draw_color_b = halo::main::globals().console_active->color.blue;
        hud_text_draw_color_or_flags = 0xffff;
        halo::text::globals().hud_text_draw_column = 0;
        halo::text::globals().hud_text_draw_unknown_4730 = 0;

        if (console_caret_visible != 0) {
            cursor = halo::main::globals().console_active->edit.cursor + (int16_t)strlen(halo::main::globals().console_active->prompt);
            if (line[cursor] == '\0') {
                line[cursor + 1] = '\0';
            }
            line[cursor] = '\x7f';
        }

        hud_text_draw_font_tag_id = font_terminal_id;
        {
            Rectangle2D rect;

            rect.top = (int16_t)(0x1e0 - line_height - render_viewport_top[0]);
            rect.left = (int16_t)(render_viewport_top[5] - render_viewport_top[1]);
            rect.bottom = (int16_t)(0x1e0 - render_viewport_top[0]);
            rect.right = (int16_t)(0x280 - render_viewport_top[1]);
            halo::rasterizer::chimera__draw_8_bit_text(0, (int32_t *)&rect, 0, 0, line);
        }
    }

    if (console_show_messages != 0) {
        y = 0x1e0 - line_height;
        message_handle = halo::main::globals().console_message_head;
        while (message_handle != (datum_index)0xffffffff && y != line_height && y - line_height >= 0) {
            message = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                           (uint16_t)message_handle * sizeof(console_message));
            hud_text_draw_color_r = message->color.red;
            hud_text_draw_color_g = message->color.green;
            hud_text_draw_color_b = message->color.blue;
            fade = 4.0f - (float)message->age * 0.033333335f;
            if (fade < 0.0f) {
                fade = 0.0f;
            } else if (fade > 1.0f) {
                fade = 1.0f;
            }
            hud_text_draw_color_a = fade * message->color.alpha;
            y = y - line_height;
            if (message->is_command_echo != 0) {
                halo::text::globals().hud_text_draw_background_mode = 3;
                text_tab_stops = 0x014000a0;
                hud_text_draw_box_field_474e = 0x000001d6;
            }
            hud_text_draw_color_or_flags = 0xffff;
            halo::text::globals().hud_text_draw_column = 0;
            halo::text::globals().hud_text_draw_unknown_4730 = 0;
            hud_text_draw_font_tag_id = font_terminal_id;
            {
                Rectangle2D rect;

                rect.top = (int16_t)(y - render_viewport_top[0]);
                rect.left = (int16_t)(render_viewport_top[5] - render_viewport_top[1]);
                rect.bottom = (int16_t)(y + line_height - render_viewport_top[0]);
                rect.right = (int16_t)(0x280 - render_viewport_top[1]);
                halo::rasterizer::chimera__draw_8_bit_text(0, (int32_t *)&rect, 0, 0, message->text);
            }
            halo::text::globals().hud_text_draw_background_mode = 0;
            message_handle = message->next;
        }
    }
}

/**
 * Unlinks `message` from the newest-to-oldest console_message list, patching the neighbours (or the head/tail
 * globals when it was at an end of the list), then frees its datum.
 *
 * @address 0x496490
 */
void ConsoleTerminal::message_delete(datum_index message)
{
    console_message *record;
    datum_index next;
    datum_index previous;

    record = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                  (uint16_t)message * sizeof(console_message));
    next = record->next;
    previous = record->previous;
    if (next == (datum_index)0xffffffff) {
        halo::main::globals().console_message_tail = previous;
    } else {
        ((console_message *)((char *)halo::main::globals().terminal_messages->data +
                              (uint16_t)next * sizeof(console_message)))->previous = previous;
    }
    if (previous != (datum_index)0xffffffff) {
        ((console_message *)((char *)halo::main::globals().terminal_messages->data +
                              (uint16_t)previous * sizeof(console_message)))->next = next;
        halo::memory::datum_delete(halo::main::globals().terminal_messages, message);
        return;
    }
    halo::main::globals().console_message_head = next;
    halo::memory::datum_delete(halo::main::globals().terminal_messages, message);
}

/**
 * Walks every live console_message from newest to oldest, incrementing its age each frame and deleting it once
 * that age passes 150.
 *
 * @address 0x4966e0
 */
void ConsoleTerminal::message_expire_old(void)
{
    datum_index current;
    datum_index next;
    console_message *record;

    current = halo::main::globals().console_message_head;
    while (current != (datum_index)0xffffffff) {
        record = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                      (uint16_t)current * sizeof(console_message));
        next = record->next;
        record->age = record->age + 1;
        if (record->age > 0x96) {
            console_message_delete(current);
        }
        current = next;
    }
}

/**
 * Allocates a new console_message slot, evicting the oldest message first if the terminal output array's high-
 * water mark has reached its capacity, and links the new slot in at the head of the newest-to-oldest list.
 *
 * @address 0x496420
 */
datum_index ConsoleTerminal::message_new(void)
{
    datum_index old_head;
    datum_index new_message;
    console_message *record;

    if (halo::main::globals().terminal_messages->last_index == 0x20) {
        console_message_delete(halo::main::globals().console_message_tail);
    }
    new_message = halo::memory::datum_new(halo::main::globals().terminal_messages);
    old_head = halo::main::globals().console_message_head;
    record = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                  (uint16_t)new_message * sizeof(console_message));
    record->next = halo::main::globals().console_message_head;
    record->previous = (datum_index)0xffffffff;
    halo::main::globals().console_message_head = new_message;
    if (old_head != (datum_index)0xffffffff) {
        ((console_message *)((char *)halo::main::globals().terminal_messages->data +
                              (uint16_t)old_head * sizeof(console_message)))->previous =
            new_message;
    } else {
        halo::main::globals().console_message_tail = new_message;
    }
    return new_message;
}

/**
 * Lazily activates the developer console for `console` the first time it is opened: wires up its embedded
 * text_edit_state to edit `input` in place, seeds the cursor at the end of whatever text is already there and
 * clears any selection, then restores the win32 console cursor. Returns 1 if this call actually opened the
 * console, 0 if one was already active. FIXED (objdump): every ret sets only AL; the upper bits of EAX are
 * left as they were
 *
 * @address 0x496510
 */
uint8_t ConsoleTerminal::open(terminal_console *console)
{
    int32_t opened;

    opened = 0;
    if (halo::main::globals().console_active == (terminal_console *)0) {
        console->edit.text = console->input;
        halo::main::globals().console_active = console;
        console->edit.maximum_length = 0xff;
        widget_text_edit_clamp_selection(&console->edit);
        console->edit.cursor = (int16_t)strlen(console->edit.text);
        console->edit.selection_anchor = -1;
        console->key_event_count = 0;
        console_restore_cursor();
        opened = 1;
    }
    return opened;
}

/**
 * Places the win32 console caret over the input line: column is the window-title length plus the console's
 * edit cursor, clamped to the buffer width; row is always the last row.
 *
 * @address 0x4971a0
 */
void ConsoleTerminal::position_cursor(void)
{
    win32_console_screen_buffer_info info;
    win32_coord position;

    if (halo::main::globals().console_win32_attached != 0 && halo::main::globals().console_active != (terminal_console *)0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        position.X = (int16_t)(halo::main::globals().console_active->edit.cursor + (int32_t)strlen(console_window_title));
        if (info.dwSize.X - 1 < (int32_t)position.X) {
            position.X = (int16_t)(info.dwSize.X - 1);
        }
        position.Y = (int16_t)(info.dwSize.Y - 1);
        SetConsoleCursorPosition(console_output_handle, position);
    }
}

/**
 * Debug/verbose console print: only above verbosity level 3 (and only once the terminal has been initialized),
 * formats `format` with vsnprintf into a fresh console_message (defaulting its color to (1.0, 0.7, 0.7, 0.7)
 * when `color` is NULL), marks it as a command echo if its text contains the console's echo prefix, and
 * mirrors it out via chimera__console_out_copy.
 * blam-cc: EAX -> color, stack -> format, ...
 *
 * @address 0x496a80
 */
void ConsoleTerminal::printf_verbose(ColorARGB *color, char *format, va_list args)
{
    static const ColorARGB k_default_color = { 1.0f, 0.7f, 0.7f, 0.7f };
    datum_index message_handle;
    console_message *message;

    if (halo::cseries::globals().debug_log_level <= 3 || halo::main::globals().terminal_initialized == 0) {
        return;
    }

    message_handle = console_message_new();
    if (message_handle == (datum_index)0xffffffff) {
        return;
    }

    message = (console_message *)((char *)halo::main::globals().terminal_messages->data +
                                   (uint16_t)message_handle * sizeof(console_message));
    message->age = 0;
    if (color == (ColorARGB *)0) {
        color = (ColorARGB *)&k_default_color;
    }
    message->color = *color;
    _vsnprintf(message->text, 0xfe, format, args);

    message->is_command_echo = strstr(message->text, console_echo_prefix) != (char *)0;
    chimera__console_out_copy(message->text);
}

/**
 * WM_SYSKEYDOWN, 0x106 WM_SYSCHAR; only the first two are ever produced here) Drains the attached win32
 * console's input queue and, for every key-down event, forwards it into the input system as a synthetic
 * WM_KEYDOWN followed by a synthetic WM_CHAR.
 * blam-cc: EAX -> key_or_char, ECX -> message (0x100 WM_KEYDOWN, 0x102 WM_CHAR, 0x104
 *
 * @address 0x496c80
 */
void ConsoleTerminal::process_input_events(void)
{
    uint32_t event_count;
    uint32_t events_read;
    uint32_t i;
    win32_input_record record;

    if (halo::main::globals().console_win32_attached == 0) {
        return;
    }
    if (GetNumberOfConsoleInputEvents(console_input_handle, (LPDWORD)&event_count) == 0) {
        return;
    }
    if (event_count == 0) {
        return;
    }
    for (i = 0; i < event_count; i++) {
        if (ReadConsoleInputA(console_input_handle, (PINPUT_RECORD)&record, 1, (LPDWORD)&events_read) != 0 &&
            record.EventType == 1) {
            if (record.KeyEvent.bKeyDown != 0) {
                halo::input::DirectInput::record_windows_key_message(record.KeyEvent.wVirtualKeyCode, 0x100);
                halo::input::DirectInput::record_windows_key_message(record.KeyEvent.uChar, 0x102);
            }
        }
    }
}

/**
 * Per-frame developer-console update: while a scripted/queued key stream is active (state byte 0x00712542 is
 * not exactly 1, has bit 0x08 clear and bit 0x04 set) and the ring still has buffered events, drains one event
 * per call into console_active's own key_events log (capped at 0x20) and feeds it to
 * widget_text_edit_process_key against the console's edit state, refreshing the caret-blink timer each time.
 * With nothing left to drain, toggles the caret's visibility once 500ms have passed since the last change.
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x4965e0
 */
uint8_t ConsoleTerminal::process_queued_input(void)
{
    large_integer counter;
    int32_t now_ms;
    ui_key_event event;

    if (halo::main::globals().console_active == (terminal_console *)0) {
        return 0;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    halo::main::globals().console_active->key_event_count = 0;

    while (controls_input_capture_flags != 1 && (controls_input_capture_flags & 8) == 0 &&
           (controls_input_capture_flags & 4) != 0 &&
           key_event_read_index < key_event_count) {
        event = key_events[key_event_read_index];
        key_event_read_index = key_event_read_index + 1;
        if (halo::main::globals().console_active->key_event_count < 0x20) {
            halo::main::globals().console_active->key_events[halo::main::globals().console_active->key_event_count] = event;
            halo::main::globals().console_active->key_event_count = halo::main::globals().console_active->key_event_count + 1;
        }
        widget_text_edit_process_key(&halo::main::globals().console_active->edit, &event);
        console_caret_visible = 1;
        halo::main::globals().console_caret_blink_time = now_ms;
    }

    if (halo::main::globals().console_caret_blink_time + 500 < now_ms) {
        console_caret_visible = (console_caret_visible == 0);
        halo::main::globals().console_caret_blink_time = now_ms;
    }
    return 1;
}

/**
 * While a win32 console is attached, forces its cursor back to visible, refreshes the window title from the
 * active console's prompt, and redraws the input line.
 *
 * @address 0x496c20
 */
void ConsoleTerminal::restore_cursor(void)
{
    win32_console_cursor_info info;
    int32_t ok;

    if (halo::main::globals().console_win32_attached != 0) {
        ok = GetConsoleCursorInfo(console_output_handle, &info);
        if (ok != 0) {
            info.bVisible = 1;
            SetConsoleCursorInfo(console_output_handle, &info);
        }
        strncpy(console_window_title, halo::main::globals().console_active->prompt, 0x1f);
        console_draw_input_line();
    }
}

/**
 * Per-frame refresh of the attached win32 console window: if the input line text changed since the last draw,
 * redraws the input line and remembers the new text; then, independently, if the cursor column moved,
 * remembers the new column and repositions the caret.
 *
 * @address 0x496d40
 */
void ConsoleTerminal::update_display(void)
{
    if (halo::main::globals().console_win32_attached != 0 && halo::main::globals().console_active != (terminal_console *)0) {
        if (strcmp(console_last_line, halo::main::globals().console_active->input) != 0) {
            console_draw_input_line();
            strncpy(console_last_line, halo::main::globals().console_active->input, 0xff);
        }
        if (console_last_cursor_column != (int32_t)halo::main::globals().console_active->edit.cursor) {
            console_last_cursor_column = (int32_t)halo::main::globals().console_active->edit.cursor;
            console_position_cursor();
        }
    }
}

}

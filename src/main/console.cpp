/**
 * Developer console: input, autocomplete, command dispatch and output.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include <string.h>
#include <ctype.h>
#include <stdint.h> 
#include "input.h"
#include "crt.h"
#include <stdio.h>
#include <stdarg.h>
#include "hs.h"

#include "halo/main/console.hpp"
#include "halo/memory/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/layout.hpp"

extern "C" { void console_autocomplete_command(void); }
extern "C" { uint32_t console_command_context_mask(uint32_t context_flags); }
extern "C" { void console_deactivate(void); }
extern "C" { uint8_t console_exec_file_run(const char *file_name); }
extern "C" { void console_out_printf(uint8_t clear_first, const char *format, ...); }
extern "C" { uint32_t console_paste_clipboard_text(void); }
extern "C" { char console_process_command(char *command_line, uint32_t context_flags); }
extern "C" { void console_toggle(void); }

extern "C" { extern int32_t rasterizer_window_requested; }
extern "C" { extern uint8_t command_line_check_flag(const char *flag_name, const char **out_value); }
namespace halo::main {

/**
 * Runs the startup exec script named by "-exec <file>" (or init.txt when -exec has no value or
 * was not given), truncating a caller-supplied name to 127 characters. If the script could not
 * be run (the file did not exist) and the window was requested, queues "map_name b30" as a
 * console command instead.
 *
 * @address 0x4c6390
 */
void Console::chimera__exec_init(void)
{
    char exec_file_name[k_main_path_length / 2];
    const char *exec_arg;
    uint8_t exec_flag_present;
    uint8_t ran_script;

    exec_flag_present = command_line_check_flag("-exec", &exec_arg);
    if (exec_flag_present && exec_arg != 0) {
        strncpy(exec_file_name, exec_arg, sizeof(exec_file_name) - 1);
    } else {
        strncpy(exec_file_name, "init.txt", sizeof(exec_file_name) - 1);
    }
    ran_script = console_exec_file_run(exec_file_name);
    if (!ran_script && rasterizer_window_requested != 0) {
        console_process_command((char *)"map_name b30", 0);
    }
}

}

extern "C" { extern console_globals console_globals_data; }
extern "C" { extern int standalone_devmode(void); }
extern "C" { extern int16_t hs_autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count, uint16_t gametype_mask); }
namespace halo::main {

/**
 * Tab completion for the console input line: gathers every hs name that starts with the word
 * under completion, prints them (one per line, or four per line joined with |t tab escapes when
 * there are more than 16), and replaces the word with the longest prefix (compared case
 * insensitively) the names share, leaving the edit cursor after it. profile_load is never
 * listed, and a lone profile_load match is not completed at all.
 *
 * @address 0x4c6bc0
 */
void Console::autocomplete_command(void)
{
    char *input;
    char *word;
    char *after_space;
    char *after_paren;
    char *after_quote;
    char **cursor;
    int32_t printed_count;
    int16_t common_index;
    uint32_t remaining;
    uint8_t many_matches;
    char line[0x400];
    char *names[0x100];
    char *name;
    int16_t match_count;
    int16_t limit;
    int16_t i;
    uint32_t length;
    int first;

    input = console_globals_data.terminal.input;
    word = input;
    after_space = strrchr(input, ' ') + 1;
    after_paren = strrchr(input, '(') + 1;
    after_quote = strrchr(input, '"') + 1;
    if ((uintptr_t)after_space >= (uintptr_t)word) {
        word = after_space;
    }
    if ((uintptr_t)word <= (uintptr_t)after_paren) {
        word = after_paren;
    }
    if ((uintptr_t)word <= (uintptr_t)after_quote) {
        word = after_quote;
    }

    match_count = hs_autocomplete_gather(0x28, names, word, 0x100,
        standalone_devmode() ? 0 : _console_context_default_bit);
    if (match_count == 0) {
        return;
    }
    many_matches = match_count > 0x10;
    common_index = 0x7fff;
    if (match_count == 1 && strcmp(names[0], "profile_load") == 0) {
        return;
    }

    line[0] = 0;
    console_out_printf(0, "");
    printed_count = 0;
    if (match_count > 0) {
        remaining = (uint16_t)match_count;
        cursor = names;
        do {
            name = *cursor;
            if (strcmp(name, "profile_load") != 0) {
                length = (uint32_t)strlen(name);
                if ((uint32_t)(int32_t)common_index > length) {
                    limit = (int16_t)length;
                } else {
                    limit = common_index;
                }
                i = 0;
                first = tolower((int)(int8_t)names[0][0]);
                if (tolower((int)(int8_t)name[0]) == first) {
                    do {
                        if (i > limit) {
                            break;
                        }
                        i++;
                        first = tolower((int)(int8_t)names[0][i]);
                    } while (tolower((int)(int8_t)(*cursor)[i]) == first);
                }
                common_index = (int16_t)(i - 1);

                if (many_matches != 0) {
                    strcat(line, *cursor);
                    strcat(line, "|t");
                    if (printed_count % 4 == 3) {
                        console_out_printf(0, line);
                        line[0] = 0;
                    }
                } else {
                    console_out_printf(0, *cursor);
                }
                printed_count++;
            }
            cursor++;
            remaining--;
        } while (remaining != 0);
    }

    if (many_matches != 0 && (printed_count - 1) % 4 != 3) {
        console_out_printf(0, line);
    }
    if (common_index != 0x7fff) {
        strncpy(word, names[0], (int32_t)common_index + 1);
        word[common_index + 1] = 0;
        console_globals_data.terminal.edit.cursor =
            (int16_t)((int16_t)(word - input) + common_index + 1);
    }
}

}

extern "C" { extern game_engine_definition *current_game_engine; }
extern "C" { extern main_globals main_globals_data; }
extern "C" { extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; }
namespace halo::main {

/**
 * Builds the command-availability mask console_process_command checks a command's flags against
 * (and chimera__autocomplete_gather filters candidates with): bit 0 and bit 6 are always set;
 * bit 4 (no multiplayer engine loaded) or bit 3 (one is) is set depending on current_game_engine;
 * bit 1 is set for a network host; a client additionally forbids bits 1 and 2; bit 5 may be
 * requested by context_flags but is forbidden (cleared) unless the end-credits profile flag
 * (saved_player_profile_flags bit 2, 0x0004) is set, or unconditionally forbidden while a
 * multiplayer engine is loaded. context_flags itself may also carry forbidding bits (high byte)
 * for any of the low seven bits, applied last.
 *
 * @address 0x4c69c0
 */
uint32_t Console::command_context_mask(uint32_t context_flags)
{
    uint32_t mask;

    if (current_game_engine == 0) {
        mask = _console_context_default_bit;
        if ((profile_globals_block[0].profile.flags & _saved_player_profile_end_credits_reached_bit) == 0) {
            mask = _console_context_default_bit | k_console_context_exec_file;
        }
        mask = mask | _console_context_no_multiplayer_bit;
    } else {
        mask = _console_context_default_bit | _console_context_multiplayer_bit | k_console_context_exec_file;
    }

    mask = mask | _console_context_always_bit;
    if (main_globals_data.game_connection == _game_connection_network_client) {
        mask = mask | k_console_context_client_forbidden | _console_context_always_bit;
    } else if (main_globals_data.game_connection == _game_connection_network_server) {
        mask = mask | _console_context_host_bit;
    }

    mask = mask | context_flags;
    if (mask & (_console_context_default_bit << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_default_bit & 0xffffu);
    }
    if (mask & (_console_context_host_bit << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_host_bit & 0xffffu);
    }
    if (mask & (_console_context_unknown_04 << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_unknown_04 & 0xffffu);
    }
    if (mask & (_console_context_multiplayer_bit << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_multiplayer_bit & 0xffffu);
    }
    if (mask & (_console_context_no_multiplayer_bit << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_no_multiplayer_bit & 0xffffu);
    }
    if (mask & (_console_context_unknown_20 << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_unknown_20 & 0xffffu);
    }
    if (mask & (_console_context_always_bit << k_console_context_forbidden_shift)) {
        mask = mask & (~_console_context_always_bit & 0xffffu);
    }
    return mask;
}

}

extern "C" { extern input_abstraction_globals input_globals; }
extern "C" { extern void *keyboard_device; }
extern "C" { extern uint8_t key_frames[0x6d]; }
extern "C" { extern uint8_t key_release_pending[0x6d]; }
extern "C" { extern void console_close(terminal_console *console); }
namespace halo::main {

/**
 * Closes the developer console (if it is both enabled and currently open), clears the console
 * input-capture bit of the input mode flags, and, when the keyboard device is acquired, flushes
 * its buffered key event queue and clears the key-frame/release-pending arrays.
 *
 * @address 0x4c64b0
 */
void Console::deactivate(void)
{
    uint32_t flush_all;

    if (console_globals_data.active != 0 && console_globals_data.enabled != 0) {
        console_close(&console_globals_data.terminal);
        input_globals.mode_flags = input_globals.mode_flags & (uint8_t)~_input_mode_keyboard_capture_bit;
        console_globals_data.active = 0;
        if (keyboard_device != 0) {
            flush_all = k_dword_none;
            ((idirectinputdevice8_getdevicedata_proc)(*(void ***)keyboard_device)[k_directinput_get_device_data_slot])
                (keyboard_device, sizeof(di_device_object_data), (di_device_object_data *)0, &flush_all, 0);
            memset(key_release_pending, 0, sizeof(key_release_pending));
            memset(key_frames, 0, sizeof(key_frames));
        }
    }
}

}

namespace halo::main {

/**
 * Runs every line of file_name as a console command (used for the startup "-exec" script). Each
 * line is cut at the first \r, \n or \t (trailing line-ending / comment-style whitespace) before
 * being handed to console_process_command with the "exec file" context bit set. Returns whether
 * the file could be opened at all; a file that opens but is empty still returns true.
 *
 * @address 0x4c6420
 */
uint8_t Console::exec_file_run(const char *file_name)
{
    FILE *file;
    char line[k_console_exec_line_length];

    file = (FILE *)fopen(file_name, "r");
    if (file == 0) {
        return 0;
    }
    while (fgets(line, k_console_exec_line_length - 1, file) != 0) {
        strtok(line, "\r\n\t");
        console_process_command(line, k_console_context_exec_file);
    }
    fclose(file);
    return 1;
}

}

extern "C" { extern ColorARGB console_default_color; }
extern "C" { extern char **shell_argv; }
extern "C" { extern int32_t shell_argc; }
namespace halo::main {

/**
 * Resets the console: default input color, the "halo( " prompt, an empty input line and
 * history, and enables it when the command line carries -console.
 *
 * @address 0x4c62d0
 */
void Console::initialize(void)
{
    static const char k_console_prompt[7] = "halo( ";
    int32_t i;

    console_globals_data.terminal.color = console_default_color;
    for (i = 0; i < 7; i++) {
        console_globals_data.terminal.prompt[i] = k_console_prompt[i];
    }
    console_globals_data.history_newest_index = -1;
    console_globals_data.history_browse_index = -1;
    console_globals_data.terminal.input[0] = 0;
    console_globals_data.history_count = 0;

    for (i = 0; i < shell_argc; i++) {
        char *argument = shell_argv[i];
        if (argument[0] == '-' && _stricmp("-console", argument) == 0) {
            console_globals_data.enabled = 1;
            return;
        }
    }
    console_globals_data.enabled = 0;
}

}

extern "C" { extern uint8_t terminal_initialized; }
extern "C" { extern data_array *terminal_messages; }
extern "C" { extern datum_index console_message_head; }
extern "C" { extern datum_index console_message_tail; }
extern "C" { extern uint8_t error_file_logging_enabled; }
extern "C" { extern void console_clear_screen(void); }
extern "C" { extern void chimera__console_out(ColorARGB *color, char *format, ...); }
/**
 * While the console is active: optionally clears the terminal's message history first (when
 * clear_first is set and the terminal has been initialized), then formats a printf-style message
 * and prints it via chimera__console_out, logging it (with a trailing CRLF) to debug.txt when
 * error-file logging is enabled. Does nothing at all while the console is not active.
 *
 * @address 0x4c6860
 */
extern "C" void console_out_printf(uint8_t clear_first, const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    if (console_globals_data.active == 0) {
        return;
    }

    if (clear_first != 0 && terminal_initialized != 0) {
        console_message_head = k_datum_index_none;
        console_message_tail = k_datum_index_none;
        halo::memory::data_delete_all(terminal_messages);
        console_clear_screen();
    }

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    chimera__console_out(0, (char *)"%s", formatted);
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400);
        halo::cseries::write_to_error_file(formatted, 1);
    }
}

extern "C" { extern terminal_console *console_active; }
extern "C" { extern uint32_t clipboard_get_text(char *buffer, uint32_t capacity); }
extern "C" { extern void widget_text_edit_insert_string(text_edit_state *state, char *insert_str); }
namespace halo::main {

/**
 * Reads the current clipboard text into a local buffer and, if the console's own text edit is
 * the currently active one, inserts it at the cursor. Returns whether clipboard text was read at
 * all (independent of whether it was actually pasted into the console).
 *
 * @address 0x4c6570
 */
uint32_t Console::paste_clipboard_text(void)
{
    char clipboard_text[0x100];
    uint32_t have_text;

    have_text = clipboard_get_text(clipboard_text, 0xff);
    if (have_text != 0 && console_active == &console_globals_data.terminal) {
        widget_text_edit_insert_string(&console_active->edit, clipboard_text);
    }
    return have_text;
}

}

extern "C" { extern void console_printf_verbose(ColorARGB *color, char *format, ...); }
/**
 * Optionally clears the terminal's message history first (when clear_first is set and the
 * terminal has been initialized), then formats a printf-style message and prints it via
 * console_printf_verbose (so it only actually shows once debug_log_level > 3), additionally
 * logging it (with a trailing CRLF) to debug.txt when error-file logging is enabled.
 *
 * Original register convention: AL -> clear_first.
 *
 * @address 0x4c67c0
 */
extern "C" void console_print_error_va(uint8_t clear_first, const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    if (clear_first != 0 && terminal_initialized != 0) {
        console_message_head = k_datum_index_none;
        console_message_tail = k_datum_index_none;
        halo::memory::data_delete_all(terminal_messages);
        console_clear_screen();
    }

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    console_printf_verbose(0, (char *)"%s", formatted);
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400);
        halo::cseries::write_to_error_file(formatted, 1);
    }
}

extern "C" { extern ColorARGB *console_message_default_color; }
/**
 * Formats a printf-style message and appends it to the console's message list via
 * console_printf_verbose (so it only actually shows once debug_log_level > 3), additionally
 * logging it (with a trailing CRLF) to debug.txt when error-file logging is enabled.
 *
 * @address 0x4c6920
 */
extern "C" void console_print_va(const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    console_printf_verbose(console_message_default_color, (char *)"%s", formatted);
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400);
        halo::cseries::write_to_error_file(formatted, 1);
    }
}

extern "C" { extern uint8_t hs_preserve_token_case; }
extern "C" { extern char hs_compile_and_evaluate(const char *command); }
namespace halo::main {

/**
 * Records command_line in the command history ring (unless it is a comment: leading ';', '#' or
 * "//"), then checks that its first space-delimited word names an hs function currently allowed
 * by console_command_context_mask(context_flags) before compiling and evaluating it. Reports and
 * refuses to evaluate a command that is not currently available. Returns the compiled script's
 * result, or 0 if the line was a comment or the command was unavailable.
 *
 * @address 0x4c6a80
 */
char Console::process_command(char *command_line, uint32_t context_flags)
{
    char command_name[0x100];
    char *space;
    char *out_names[0x28];
    uint32_t context_mask;
    int16_t match_count;
    int16_t i;
    int16_t history_index;
    char result;

    if (command_line[0] == ';' || command_line[0] == '#' ||
        (command_line[0] == '/' && command_line[1] == '/')) {
        return 0;
    }

    strncpy(command_name, command_line, 0xff);
    space = strchr(command_name, ' ');
    if (space != 0) {
        *space = 0;
    }

    history_index = (int16_t)((console_globals_data.history_newest_index + 1) & 7);
    console_globals_data.history_newest_index = history_index;
    strcpy(console_globals_data.history[history_index], command_line);

    if (console_globals_data.history_count < 8) {
        console_globals_data.history_count = console_globals_data.history_count + 1;
    } else {
        console_globals_data.history_count = 8;
    }
    console_globals_data.history_browse_index = -1;

    context_mask = console_command_context_mask(context_flags);
    if (standalone_devmode()) {
        context_mask = 0;
    }
    match_count = hs_autocomplete_gather(0x28, out_names, command_name, 0x100, (uint16_t)context_mask);
    for (i = match_count - 1; i >= 0; i--) {
        if (_stricmp(command_name, out_names[i]) == 0) {
            hs_preserve_token_case = 1;
            result = hs_compile_and_evaluate(command_line);
            hs_preserve_token_case = 0;
            return result;
        }
    }
    console_out_printf(0, "Requested function \"%s\" cannot be executed now.", command_name);
    return 0;
}

}

extern "C" { extern uint8_t chat_dialog_open; }
extern "C" { extern void widget_text_edit_reset_length(text_edit_state *state); }
namespace halo::main {

/**
 * Per-frame console input: toggles the console on the grave key, and while open, consumes every
 * buffered key event of the frame (paste, tab-completion, Enter/close, and command history
 * recall with Up/Down), submitting the input line and recording it once Enter is pressed.
 * Returns whether the console ends the frame open.
 *
 * @address 0x4c65c0
 */
uint8_t Console::process_key_events(void)
{
    int16_t i;
    int16_t key_code;
    int16_t browse_index;
    int16_t history_index;

    if (console_globals_data.enabled != 0 && chat_dialog_open == 0) {
        if (input_globals.system_key_states[0] == 1) {
            console_toggle();
            return console_globals_data.active;
        }
        if (console_globals_data.active != 0) {
            if (halo::input::input_get_mouse_button_state(2) == 1) {
                console_paste_clipboard_text();
            }
            for (i = 0; i < console_globals_data.terminal.key_event_count; i++) {
                key_code = console_globals_data.terminal.key_events[i].key_code;
                switch (key_code) {
                case 6:
                    console_paste_clipboard_text();
                    break;

                case _input_key_tab:
                    console_autocomplete_command();
                    break;

                case _input_key_enter:
                case _input_key_numpad_enter:
                    if (console_globals_data.terminal.input[0] == 0) {
                        console_deactivate();
                    } else {
                        console_process_command(console_globals_data.terminal.input, 0);
                        console_globals_data.terminal.input[0] = 0;
                        console_globals_data.terminal.edit.cursor = 0;
                        console_globals_data.terminal.edit.selection_anchor = -1;
                    }
                    break;

                case _input_key_up:
                    console_globals_data.history_browse_index = console_globals_data.history_browse_index + 2;

                case _input_key_down:
                    browse_index = console_globals_data.history_browse_index - 1;
                    if (browse_index < 1) {
                        browse_index = 0;
                    }
                    if (browse_index > console_globals_data.history_count - 1) {
                        browse_index = console_globals_data.history_count - 1;
                    }
                    console_globals_data.history_browse_index = browse_index;
                    if (browse_index != -1) {
                        history_index = (int16_t)((console_globals_data.history_newest_index - browse_index + 8) & 7);
                        strcpy(console_globals_data.terminal.input, console_globals_data.history[history_index]);
                        widget_text_edit_reset_length(&console_globals_data.terminal.edit);
                    }
                    break;
                }
            }
        }
    }
    return console_globals_data.active;
}

}

extern "C" { extern int32_t console_rcon_handle; }
namespace halo::main {

/**
 * Temporarily routes console output to rcon_handle, runs command_line as an ordinary console
 * command (with no extra context bits), then restores console_rcon_handle to -1 (local console).
 *
 * @address 0x4c69a0
 */
void Console::process_rcon_command(int32_t rcon_handle, char *command_line)
{
    console_rcon_handle = rcon_handle;
    console_process_command(command_line, 0);
    console_rcon_handle = -1;
}

}

extern "C" { extern uint8_t virtual_keyboard; }
extern "C" { extern uint8_t console_open(terminal_console *console); }
namespace halo::main {

/**
 * Closes the console if it is open; otherwise opens it (when enabled and not blocked by
 * virtual_keyboard), clearing the input line first, and always turns on keyboard
 * capture mode afterward.
 *
 * @address 0x4c6530
 */
void Console::toggle(void)
{
    if (console_globals_data.active != 0) {
        console_deactivate();
        return;
    }
    if (console_globals_data.enabled != 0 && virtual_keyboard == 0) {
        console_globals_data.terminal.input[0] = 0;
        console_globals_data.active = console_open(&console_globals_data.terminal);
        halo::input::input_keyboard_set_capture_mode(1);
    }
}

}

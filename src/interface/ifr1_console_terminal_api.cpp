#include "halo/interface/ifr1_console_terminal.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::out.
 * blam-cc: EAX -> color, stack -> format, ...
 *
 * @address 0x496b50
 */
void chimera__console_out(ColorARGB *color, char *format, ...)
{
    va_list args;
    va_start(args, format);
    halo::interface::ConsoleTerminal::out(color, format, args);
    va_end(args);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::out_copy.
 * blam-cc: EAX -> text
 *
 * @address 0x496e90
 */
void chimera__console_out_copy(char *text)
{
    halo::interface::ConsoleTerminal::out_copy(text);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::clear_bottom_line.
 *
 * @address 0x497010
 */
void console_clear_bottom_line(uint8_t clear_text)
{
    halo::interface::ConsoleTerminal::clear_bottom_line(clear_text);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::clear_screen.
 *
 * @address 0x496f90
 */
void console_clear_screen(void)
{
    halo::interface::ConsoleTerminal::clear_screen();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::close.
 *
 * @address 0x496580
 */
void console_close(terminal_console *console)
{
    halo::interface::ConsoleTerminal::close(console);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::draw_input_line.
 *
 * @address 0x4970a0
 */
void console_draw_input_line(void)
{
    halo::interface::ConsoleTerminal::draw_input_line();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::draw_overlay.
 *
 * @address 0x496730
 */
void console_draw_overlay(void)
{
    halo::interface::ConsoleTerminal::draw_overlay();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::message_delete.
 *
 * @address 0x496490
 */
void console_message_delete(datum_index message)
{
    halo::interface::ConsoleTerminal::message_delete(message);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::message_expire_old.
 *
 * @address 0x4966e0
 */
void console_message_expire_old(void)
{
    halo::interface::ConsoleTerminal::message_expire_old();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::message_new.
 *
 * @address 0x496420
 */
datum_index console_message_new(void)
{
    return halo::interface::ConsoleTerminal::message_new();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::open.
 *
 * @address 0x496510
 */
uint8_t console_open(terminal_console *console)
{
    return halo::interface::ConsoleTerminal::open(console);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::position_cursor.
 *
 * @address 0x4971a0
 */
void console_position_cursor(void)
{
    halo::interface::ConsoleTerminal::position_cursor();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::printf_verbose.
 * blam-cc: EAX -> color, stack -> format, ...
 *
 * @address 0x496a80
 */
void console_printf_verbose(ColorARGB *color, char *format, ...)
{
    va_list args;
    va_start(args, format);
    halo::interface::ConsoleTerminal::printf_verbose(color, format, args);
    va_end(args);
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::process_input_events.
 * blam-cc: EAX -> key_or_char, ECX -> message (0x100 WM_KEYDOWN, 0x102 WM_CHAR, 0x104
 *
 * @address 0x496c80
 */
void console_process_input_events(void)
{
    halo::interface::ConsoleTerminal::process_input_events();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::process_queued_input.
 *
 * @address 0x4965e0
 */
uint8_t console_process_queued_input(void)
{
    return halo::interface::ConsoleTerminal::process_queued_input();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::restore_cursor.
 *
 * @address 0x496c20
 */
void console_restore_cursor(void)
{
    halo::interface::ConsoleTerminal::restore_cursor();
}

/**
 * C ABI entry point; forwards to halo::interface::ConsoleTerminal::update_display.
 *
 * @address 0x496d40
 */
void console_update_display(void)
{
    halo::interface::ConsoleTerminal::update_display();
}

}

#pragma once

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * The developer console: message pool, input handling, display and the verbose printf paths.
 */
class ConsoleTerminal {
public:
    static void out(ColorARGB *color, const char *format, va_list args);
    static void out_copy(char *text);
    static void clear_bottom_line(uint8_t clear_text);
    static void clear_screen(void);
    static void close(terminal_console *console);
    static void draw_input_line(void);
    static void draw_overlay(void);
    static void message_delete(datum_index message);
    static void message_expire_old(void);
    static datum_index message_new(void);
    static uint8_t open(terminal_console *console);
    static void position_cursor(void);
    static void printf_verbose(ColorARGB *color, const char *format, va_list args);
    static void process_input_events(void);
    static uint8_t process_queued_input(void);
    static void restore_cursor(void);
    static void update_display(void);
};

}

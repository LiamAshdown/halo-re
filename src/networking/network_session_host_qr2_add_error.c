// network_session_host_qr2_add_error  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x578100, size 24 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578100..0x578117: the qr2 add-error callback (error, message, user data):
//   "qr2_adderror_callback - %s" to the console in its standard color.
// blam-cc: cdecl (a qr2 add-error callback)

#include "tags.h"
#include <string.h>
#include <wchar.h>

typedef struct ColorARGB ColorARGB;
extern void *console_color_00685218; // 0x00685218
extern void console_printf_verbose(ColorARGB *color, char *format, ...); // 0x496a80, blam-cc: EAX color

void network_session_host_qr2_add_error(int32_t error, char *message, void *user_data)
{
    (void)error;
    (void)user_data;
    console_printf_verbose((ColorARGB *)console_color_00685218, "qr2_adderror_callback - %s", message);
}

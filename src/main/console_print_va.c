// console_print_va  (Ghidra: console_print_va, already named)
// address 0x4c6920, size 114 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: disassembly (objdump -d -M intel, bin/halo.exe, 0x4c6920..0x4c6991) shows the real
// stack reservation is 0x400 (1024) bytes, not the 255 Ghidra's local_400 array declares (the
// name reflects the stack offset, not the size); FUN_00496a80 is console_printf_verbose
// (src/interface/console_printf_verbose.c), called here with EAX loaded from 0x00685218 (a
// ColorARGB* also loaded into EAX ahead of dozens of unrelated chimera__console_out call sites
// across the codebase, e.g. sv_map/sv_kick/sv_ban) rather than NULL, and format = "%s"
// (0x0065efec) with the already-formatted text as its sole vararg, so any % characters the
// caller's format produced are not re-interpreted. The "\r\n" (0x0065f010) and
// write_to_error_file tail are shared verbatim with console_out_printf and console_print_error_va.
// register convention: __cdecl, printf-style varargs.
// UNSURE: 0x00685218's pointee (0x00655168, floats {0.0, 1.0, 0.0, 0.0}) has no established name;
// declared here as a generic default-color pointer.
// reconciled: R01 comment: 0x0087ac06 console_verbosity -> debug_log_level

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern uint8_t error_file_logging_enabled; // 0x007196d3
extern ColorARGB *console_message_default_color; // 0x00685218, UNSURE: unnamed shared default

extern void console_printf_verbose(ColorARGB *color, char *format, ...); // 0x496a80, blam-cc: EAX -> color
extern void write_to_error_file(char *message, char with_timestamp); // 0x449450

// Formats a printf-style message and appends it to the console's message list via
// console_printf_verbose (so it only actually shows once debug_log_level > 3), additionally
// logging it (with a trailing CRLF) to debug.txt when error-file logging is enabled.
void console_print_va(const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    console_printf_verbose(console_message_default_color, (char *)"%s", formatted); // EAX -> console_message_default_color; 0x0065efec == "%s"
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400); // 0x0065f010 == "\r\n"
        write_to_error_file(formatted, 1);
    }
}

#if 0
Original Ghidra decompilation (0x4c6920):

void __cdecl console_print_va(char *format,...)

{
  char local_400 [255];
  undefined1 local_301;

  _vsprintf(local_400,format,&stack0x00000008);
  local_301 = 0;
  FUN_00496a80(&DAT_0065efec,local_400);
  if (DAT_007196d3 != '\0') {
    _strncat(local_400,"\r\n",0x400);
    write_to_error_file(local_400,'\x01');
  }
  return;
}

Disassembly (0x4c6920..0x4c6991) confirms the real buffer size and the EAX color argument:

004c6920:  sub    esp,0x400
...
004c6940:  lea    eax,[esp+0xc]
004c6944:  push   eax                   ; format = "formatted" (single vararg for "%s")
004c6945:  mov    eax,ds:0x685218       ; EAX = console_message_default_color
004c694a:  push   0x65efec              ; "%s"
004c694f:  mov    BYTE PTR [esp+0x113],0x0
004c6957:  call   0x496a80              ; console_printf_verbose(EAX, "%s", formatted)
004c695c:  mov    al,ds:0x7196d3
004c6961:  add    esp,0x14
004c6964:  test   al,al
004c6966:  je     0x4c698b
004c6968:  push   0x400
004c696d:  lea    ecx,[esp+0x4]
004c6971:  push   0x65f010              ; "\r\n"
004c6976:  push   ecx
004c6977:  call   0x625c50              ; strncat(formatted, "\r\n", 0x400)
004c697c:  lea    edx,[esp+0xc]
004c6980:  push   0x1
004c6982:  push   edx
004c6983:  call   0x449450              ; write_to_error_file(formatted, 1)
#endif

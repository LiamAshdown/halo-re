// console_out_printf  (Ghidra: console_out_printf, already named)
// address 0x4c6860, size 177 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: disassembly (objdump -d -M intel, bin/halo.exe, 0x4c6860..0x4c6910); types/main.h
// console_globals_data.active; types/interface.h terminal_initialized/console_message_head/tail
// (0x006b2efc/0x006b2f04/0x006b2f08, k_datum_index_none). chimera__console_out (0x496b50,
// already rewritten in src/interface/chimera__console_out.c) is called here with EAX = 0
// (color = NULL, its own internal default), format = "%s" (0x0065efec) and the already-formatted
// text as the sole vararg. The real local buffer is 0x400 (1024) bytes (Ghidra's local_400[255]
// name reflects its stack offset, not its size).
// register convention: __cdecl(clear_first, format, ...); no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern console_globals console_globals_data;    // 0x006b7020
extern uint8_t terminal_initialized;       // 0x006b2efc
extern data_array *terminal_messages;      // 0x006b2f00, "terminal output"
extern datum_index console_message_head;   // 0x006b2f04
extern datum_index console_message_tail;   // 0x006b2f08
extern uint8_t error_file_logging_enabled; // 0x007196d3

extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array (see src/ai/ai_reset_for_new_map.c)
extern void console_clear_screen(void); // 0x496f90
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, blam-cc: EAX -> color
extern void write_to_error_file(char *message, char with_timestamp); // 0x449450

// While the console is active: optionally clears the terminal's message history first (when
// clear_first is set and the terminal has been initialized), then formats a printf-style message
// and prints it via chimera__console_out, logging it (with a trailing CRLF) to debug.txt when
// error-file logging is enabled. Does nothing at all while the console is not active.
void console_out_printf(uint8_t clear_first, const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    if (console_globals_data.active == 0) {
        return;
    }

    if (clear_first != 0 && terminal_initialized != 0) {
        console_message_head = k_datum_index_none;
        console_message_tail = k_datum_index_none;
        data_delete_all(terminal_messages); // ESI -> terminal_messages
        console_clear_screen();
    }

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    chimera__console_out(0, "%s", formatted); // EAX = 0 (default color); 0x0065efec == "%s"
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400); // 0x0065f010 == "\r\n"
        write_to_error_file(formatted, 1);
    }
}

#if 0
Original Ghidra decompilation (0x4c6860):

void __cdecl console_out_printf(char clear_first,char *format,...)

{
  char local_400 [255];
  undefined1 local_301;

  if (DAT_006b7020 != '\0') {
    if ((clear_first != '\0') && (DAT_006b2efc != '\0')) {
      DAT_006b2f04 = 0xffffffff;
      DAT_006b2f08 = 0xffffffff;
      data_delete_all();
      console_clear_screen();
    }
    _vsprintf(local_400,format,&stack0x0000000c);
    local_301 = 0;
    chimera__console_out(&DAT_0065efec,local_400);
    if (DAT_007196d3 != '\0') {
      _strncat(local_400,"\r\n",0x400);
      write_to_error_file(local_400,'\x01');
    }
  }
  return;
}

Disassembly (0x4c6860..0x4c690a):

004c6860:  mov    al,ds:0x6b7020
004c6865:  sub    esp,0x400
004c686b:  test   al,al
004c686d:  je     0x4c690a
004c6873:  mov    al,BYTE PTR [esp+0x404]   ; clear_first
004c687a:  test   al,al
004c687c:  je     0x4c68a8
004c687e:  mov    al,ds:0x6b2efc            ; terminal_initialized
004c6883:  test   al,al
004c6885:  je     0x4c68a8
004c6887:  mov    eax,0xffffffff
004c688c:  push   esi
004c688d:  mov    esi,DWORD PTR ds:0x6b2f00
004c6893:  mov    ds:0x6b2f04,eax           ; console_message_head = -1
004c6898:  mov    ds:0x6b2f08,eax           ; console_message_tail = -1
004c689d:  call   0x4d0580                  ; data_delete_all()
004c68a2:  call   0x496f90                  ; console_clear_screen()
004c68a7:  pop    esi
004c68a8:  mov    ecx,DWORD PTR [esp+0x408] ; format
004c68af:  lea    eax,[esp+0x40c]           ; varargs
004c68b6:  push   eax
004c68b7:  push   ecx
004c68b8:  lea    edx,[esp+0x8]             ; &formatted
004c68bc:  push   edx
004c68bd:  call   0x625963                  ; vsprintf(&formatted, format, args)
004c68c2:  lea    eax,[esp+0xc]             ; &formatted (single vararg for "%s")
004c68c6:  push   eax
004c68c7:  push   0x65efec                  ; "%s"
004c68cc:  xor    eax,eax                   ; EAX = 0 (color = NULL)
004c68ce:  mov    BYTE PTR [esp+0x113],0x0
004c68d6:  call   0x496b50                  ; chimera__console_out(EAX, "%s", &formatted)
004c68db:  mov    al,ds:0x7196d3
004c68e0:  add    esp,0x14
004c68e3:  test   al,al
004c68e5:  je     0x4c690a
004c68e7:  push   0x400
004c68ec:  lea    ecx,[esp+0x4]
004c68f0:  push   0x65f010                  ; "\r\n"
004c68f5:  push   ecx
004c68f6:  call   0x625c50                  ; strncat(&formatted, "\r\n", 0x400)
004c68fb:  lea    edx,[esp+0xc]
004c68ff:  push   0x1
004c6901:  push   edx
004c6902:  call   0x449450                  ; write_to_error_file(&formatted, 1)
#endif

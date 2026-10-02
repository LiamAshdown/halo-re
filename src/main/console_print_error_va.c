// console_print_error_va  (Ghidra: console_print_error_va, already named)
// address 0x4c67c0, size 157 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: disassembly (objdump -d -M intel, bin/halo.exe, 0x4c67c0..0x4c685c). Ghidra's
// decompile shows `char *param_1` as the sole parameter and `in_AL` as an unresolved register;
// per out/phase4/main_types_notes.md ("console_print_error_va 0x4c67c0: AL = clear the terminal
// first, then format and varargs on the stack"), AL is clear_first and param_1 is format.
// console_printf_verbose (0x496a80, src/interface/console_printf_verbose.c) is called with
// EAX = 0 (color = NULL, its own internal default), format = "%s" (0x0065efec) and the
// already-formatted text as the sole vararg; data_delete_all (0x4d0580) takes ESI ->
// terminal_messages (0x006b2f00), per src/ai/ai_reset_for_new_map.c's register convention.
// register convention: AL -> clear_first, stack -> format, ... // blam-cc: AL -> clear_first
// reconciled: R01 comment: 0x0087ac06 console_verbosity -> debug_log_level

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern uint8_t terminal_initialized;       // 0x006b2efc
extern data_array *terminal_messages;      // 0x006b2f00, "terminal output"
extern datum_index console_message_head;   // 0x006b2f04
extern datum_index console_message_tail;   // 0x006b2f08
extern uint8_t error_file_logging_enabled; // 0x007196d3

extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array
extern void console_clear_screen(void); // 0x496f90
extern void console_printf_verbose(ColorARGB *color, char *format, ...); // 0x496a80, blam-cc: EAX -> color
extern void write_to_error_file(char *message, char with_timestamp); // 0x449450

// blam-cc: AL -> clear_first
// Optionally clears the terminal's message history first (when clear_first is set and the
// terminal has been initialized), then formats a printf-style message and prints it via
// console_printf_verbose (so it only actually shows once debug_log_level > 3), additionally
// logging it (with a trailing CRLF) to debug.txt when error-file logging is enabled.
void console_print_error_va(uint8_t clear_first, const char *format, ...)
{
    char formatted[0x400];
    va_list args;

    if (clear_first != 0 && terminal_initialized != 0) {
        console_message_head = k_datum_index_none;
        console_message_tail = k_datum_index_none;
        data_delete_all(terminal_messages); // ESI -> terminal_messages
        console_clear_screen();
    }

    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);

    console_printf_verbose(0, (char *)"%s", formatted); // EAX = 0 (default color); 0x0065efec == "%s"
    if (error_file_logging_enabled != 0) {
        strncat(formatted, "\r\n", 0x400); // 0x0065f010 == "\r\n"
        write_to_error_file(formatted, 1);
    }
}

#if 0
Original Ghidra decompilation (0x4c67c0):

void console_print_error_va(char *param_1)

{
  char in_AL;
  char local_400 [255];
  undefined1 local_301;

  if ((in_AL != '\0') && (DAT_006b2efc != '\0')) {
    DAT_006b2f04 = 0xffffffff;
    DAT_006b2f08 = 0xffffffff;
    data_delete_all();
    console_clear_screen();
  }
  _vsprintf(local_400,param_1,&stack0x00000008);
  local_301 = 0;
  FUN_00496a80(&DAT_0065efec,local_400);
  if (DAT_007196d3 != '\0') {
    _strncat(local_400,"\r\n",0x400);
    write_to_error_file(local_400,'\x01');
  }
  return;
}

Disassembly (0x4c67c0..0x4c685c):

004c67c0:  sub    esp,0x400
004c67c6:  test   al,al                   ; clear_first
004c67c8:  je     0x4c67f4
004c67ca:  mov    al,ds:0x6b2efc          ; terminal_initialized
004c67cf:  test   al,al
004c67d1:  je     0x4c67f4
004c67d3:  mov    eax,0xffffffff
004c67d8:  push   esi
004c67d9:  mov    esi,DWORD PTR ds:0x6b2f00   ; terminal_messages
004c67df:  mov    ds:0x6b2f04,eax         ; console_message_head = -1
004c67e4:  mov    ds:0x6b2f08,eax         ; console_message_tail = -1
004c67e9:  call   0x4d0580                ; data_delete_all(ESI = terminal_messages)
004c67ee:  call   0x496f90                ; console_clear_screen()
004c67f3:  pop    esi
004c67f4:  mov    ecx,DWORD PTR [esp+0x404]  ; format
004c67fb:  lea    eax,[esp+0x408]         ; varargs
004c6802:  push   eax
004c6803:  push   ecx
004c6804:  lea    edx,[esp+0x8]           ; &formatted
004c6808:  push   edx
004c6809:  call   0x625963                ; vsprintf(&formatted, format, args)
004c680e:  lea    eax,[esp+0xc]           ; &formatted (single vararg for "%s")
004c6812:  push   eax
004c6813:  push   0x65efec                ; "%s"
004c6818:  xor    eax,eax                 ; EAX = 0 (color = NULL)
004c681a:  mov    BYTE PTR [esp+0x113],0x0
004c6822:  call   0x496a80                ; console_printf_verbose(EAX, "%s", &formatted)
004c6827:  mov    al,ds:0x7196d3
004c682c:  add    esp,0x14
004c682f:  test   al,al
004c6831:  je     0x4c6856
004c6833:  push   0x400
004c6838:  lea    ecx,[esp+0x4]
004c683c:  push   0x65f010                ; "\r\n"
004c6841:  push   ecx
004c6842:  call   0x625c50                ; strncat(&formatted, "\r\n", 0x400)
004c6847:  lea    edx,[esp+0xc]
004c684b:  push   0x1
004c684d:  push   edx
004c684e:  call   0x449450                ; write_to_error_file(&formatted, 1)
#endif

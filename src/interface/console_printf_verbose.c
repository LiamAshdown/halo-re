// console_printf_verbose  (Ghidra: FUN_00496a80, unnamed)
// address 0x496a80, size 206 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/interface_functions.md "Formats a string with vsnprintf and appends it
// to the console message list, but only when a verbosity [threshold is met]"; types/interface.h
// "global 0x0087ac06: uint8_t debug_log_level" (0x496a86 cmp BYTE ...,0x4: prints only above 3) and
// console_message's documented default color (1.0, 0.7, 0.7, 0.7); chimera__console_out_copy.c's
// precedent for console_echo_prefix (0x00669140) and _strstr (FUN_00625430).
// register convention: format string as the recognized stack parameter, optional ColorARGB* in
// EAX (in_EAX, NULL means "use the default gray"), plus the varargs that follow format on the
// stack. // blam-cc: EAX -> color, stack -> format, ...
// reconciled: R01 0x0087ac06 int32 console_verbosity -> uint8 debug_log_level (the binary reads a byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdarg.h>

extern uint8_t debug_log_level;           // 0x0087ac06, byte-wide (R01)
extern uint8_t terminal_initialized;       // 0x006b2efc
extern data_array *terminal_messages;      // 0x006b2f00, "terminal output"
extern char console_echo_prefix[];         // 0x00669140, matched by chimera__console_out

extern datum_index console_message_new(void); // 0x496420
extern char *_strstr(char *haystack, const char *needle);
extern int32_t __vsnprintf(char *buffer, uint32_t limit, const char *format, va_list args);
extern void chimera__console_out_copy(char *text); // 0x496e90

// blam-cc: EAX -> color, stack -> format, ...
// Debug/verbose console print: only above verbosity level 3 (and only once the terminal has
// been initialized), formats `format` with vsnprintf into a fresh console_message (defaulting
// its color to (1.0, 0.7, 0.7, 0.7) when `color` is NULL), marks it as a command echo if its
// text contains the console's echo prefix, and mirrors it out via chimera__console_out_copy.
void console_printf_verbose(ColorARGB *color, char *format, ...)
{
    static const ColorARGB k_default_color = { 1.0f, 0.7f, 0.7f, 0.7f };
    va_list args;
    datum_index message_handle;
    console_message *message;

    if (debug_log_level <= 3 || terminal_initialized == 0) {
        return;
    }

    message_handle = console_message_new();
    if (message_handle == (datum_index)0xffffffff) {
        return;
    }

    message = (console_message *)((char *)terminal_messages->data +
                                   (uint16_t)message_handle * sizeof(console_message));
    message->age = 0;
    if (color == (ColorARGB *)0) {
        color = (ColorARGB *)&k_default_color;
    }
    message->color = *color;

    va_start(args, format);
    __vsnprintf(message->text, 0xfe, format, args);
    va_end(args);

    message->is_command_echo = _strstr(message->text, console_echo_prefix) != (char *)0;
    chimera__console_out_copy(message->text);
}

#if 0
Original Ghidra decompilation (0x496a80):

void FUN_00496a80(char *param_1)

{
  undefined4 *in_EAX;
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_10 = 0x3f800000;
  local_c = 0x3f333333;
  local_8 = 0x3f333333;
  local_4 = 0x3f333333;
  if ((3 < DAT_0087ac06) && (DAT_006b2efc != '\0')) {
    uVar1 = console_message_new();
    if (uVar1 != 0xffffffff) {
      iVar2 = (uVar1 & 0xffff) * 0x124 + *(int *)(DAT_006b2f00 + 0x34);
      *(undefined4 *)(iVar2 + 0x120) = 0;
      if (in_EAX == (undefined4 *)0x0) {
        in_EAX = &local_10;
      }
      *(undefined4 *)(iVar2 + 0x110) = *in_EAX;
      *(undefined4 *)(iVar2 + 0x114) = in_EAX[1];
      *(undefined4 *)(iVar2 + 0x118) = in_EAX[2];
      *(undefined4 *)(iVar2 + 0x11c) = in_EAX[3];
      __vsnprintf((char *)(iVar2 + 0xd),0xfe,param_1,&stack0x00000008);
      iVar3 = FUN_00625430((char *)(iVar2 + 0xd),&DAT_00669140);
      *(bool *)(iVar2 + 0xc) = iVar3 != 0;
      chimera__console_out_copy();
    }
  }
  return;
}
#endif

// console_close  (Ghidra: console_close, already named)
// address 0x496580, size 82 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: out/phase4/interface_functions.md "Deactivates the developer console for the given
// handle, hiding the win32 console cursor."; mirrors console_restore_cursor's cursor-info calls
// with bVisible set to 0 instead of 1, and only acts when `console` is the currently active one.
// register convention: terminal_console* in EAX (in_EAX). // blam-cc: EAX -> console

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern terminal_console *console_active; // 0x006b2f0c
extern uint8_t console_win32_attached;   // 0x006b2f18
extern void *console_output_handle;      // 0x006b2dd0, win32 console output handle


// Deactivates `console` if it is the currently active developer console: hides the win32
// console cursor (when one is attached) and clears console_active.
void console_close(terminal_console *console)
{
    win32_console_cursor_info info;
    int32_t ok;

    if (console == console_active) {
        if (console_win32_attached != 0) {
            ok = GetConsoleCursorInfo(console_output_handle, &info);
            if (ok != 0) {
                info.bVisible = 0;
                SetConsoleCursorInfo(console_output_handle, &info);
            }
        }
        console_active = (terminal_console *)0;
    }
}

#if 0
Original Ghidra decompilation (0x496580):

void console_close(void)

{
  int in_EAX;
  BOOL BVar1;
  _CONSOLE_CURSOR_INFO local_8;

  if (in_EAX == DAT_006b2f0c) {
    if (DAT_006b2f18 != '\0') {
      BVar1 = GetConsoleCursorInfo(DAT_006b2dd0,&local_8);
      if (BVar1 != 0) {
        local_8.bVisible = 0;
        SetConsoleCursorInfo(DAT_006b2dd0,&local_8);
      }
    }
    DAT_006b2f0c = 0;
  }
  return;
}
#endif

// console_position_cursor  (Ghidra: console_position_cursor, already named)
// address 0x4971a0, size 164 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: places the win32 console caret at column (title length + edit cursor) on the last
// row, clamped to the buffer width -- consistent with console_draw_input_line.c drawing
// "title" immediately followed by "input" with no separating space.
// register convention: no register-passed arguments.
// TYPES-GAP: COORD / CONSOLE_SCREEN_BUFFER_INFO are plain win32 console API structs, not engine
// types; declared locally rather than added to types/interface.h.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint8_t console_win32_attached;   // 0x006b2f18
extern terminal_console *console_active; // 0x006b2f0c
extern char console_window_title[0x20];  // 0x006b2dd8
extern void *console_output_handle;      // 0x006b2dd0

extern uint32_t strlen(const char *s);

// Places the win32 console caret over the input line: column is the window-title length plus
// the console's edit cursor, clamped to the buffer width; row is always the last row.
void console_position_cursor(void)
{
    win32_console_screen_buffer_info info;
    win32_coord position;

    if (console_win32_attached != 0 && console_active != (terminal_console *)0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        position.X = (int16_t)(console_active->edit.cursor + (int32_t)strlen(console_window_title));
        if (info.dwSize.X - 1 < (int32_t)position.X) {
            position.X = (int16_t)(info.dwSize.X - 1);
        }
        position.Y = (int16_t)(info.dwSize.Y - 1);
        SetConsoleCursorPosition(console_output_handle, position);
    }
}

#if 0
Original Ghidra decompilation (0x4971a0):

void console_position_cursor(void)

{
  char cVar1;
  BOOL BVar2;
  char *pcVar3;
  COORD local_1c;
  _CONSOLE_SCREEN_BUFFER_INFO local_18;

  if (((DAT_006b2f18 != '\0') && (DAT_006b2f0c != 0)) &&
     (BVar2 = GetConsoleScreenBufferInfo(DAT_006b2dd0,&local_18), BVar2 != 0)) {
    pcVar3 = &DAT_006b2dd8;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    local_1c.X = *(short *)(DAT_006b2f0c + 0x1ba) + (short)pcVar3 + -0x2dd9;
    if (local_18.dwSize.X + -1 < (int)local_1c.X) {
      local_1c.X = local_18.dwSize.X + -1;
    }
    local_1c.Y = local_18.dwSize.Y + -1;
    SetConsoleCursorPosition(DAT_006b2dd0,local_1c);
  }
  return;
}
#endif

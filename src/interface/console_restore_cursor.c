// console_restore_cursor  (Ghidra: FUN_00496c20, renamed per types/interface.h)
// address 0x496c20, size 94 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: types/interface.h groups this with console_open/console_close/console_draw_overlay
// as "console_restore_cursor @0x496c20"; out/phase4/interface_functions.md "Restores the
// console cursor visibility and refreshes the console window title/input line."; the copied
// bytes at console_active + 0x94 match terminal_console::prompt exactly.
// register convention: none (void).

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t console_win32_attached;    // 0x006b2f18
extern void *console_output_handle;       // 0x006b2dd0, win32 console output handle
extern terminal_console *console_active;  // 0x006b2f0c
extern char console_window_title[0x20];   // 0x006b2dd8

extern void console_draw_input_line(void); // 0x4970a0

// While a win32 console is attached, forces its cursor back to visible, refreshes the window
// title from the active console's prompt, and redraws the input line.
void console_restore_cursor(void)
{
    win32_console_cursor_info info;
    int32_t ok;

    if (console_win32_attached != 0) {
        ok = GetConsoleCursorInfo(console_output_handle, &info);
        if (ok != 0) {
            info.bVisible = 1;
            SetConsoleCursorInfo(console_output_handle, &info);
        }
        strncpy(console_window_title, console_active->prompt, 0x1f);
        console_draw_input_line();
    }
}

#if 0
Original Ghidra decompilation (0x496c20):

void FUN_00496c20(void)

{
  BOOL BVar1;
  _CONSOLE_CURSOR_INFO local_8;

  if (DAT_006b2f18 != '\0') {
    BVar1 = GetConsoleCursorInfo(DAT_006b2dd0,&local_8);
    if (BVar1 != 0) {
      local_8.bVisible = 1;
      SetConsoleCursorInfo(DAT_006b2dd0,&local_8);
    }
    _strncpy(&DAT_006b2dd8,(char *)(DAT_006b2f0c + 0x94),0x1f);
    console_draw_input_line();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

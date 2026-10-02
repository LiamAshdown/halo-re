// console_clear_bottom_line  (Ghidra: console_clear_bottom_line, already named)
// address 0x497010, size 140 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: moves the console cursor to the last row and, when asked, blanks that row; matches
// the given name exactly. Called by chimera__console_out_copy @0x496e90 with clear_text = 1.
// register convention: clear_text as the recognized stack parameter (Ghidra already resolved
// it as param_1).

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
extern uint8_t console_win32_attached;  // 0x006b2f18
extern void *console_output_handle;     // 0x006b2dd0


// Moves the console cursor to column 0 of the last row; when clear_text is set, also blanks that
// entire row (character then attribute) in the current attribute.
void console_clear_bottom_line(uint8_t clear_text)
{
    win32_console_screen_buffer_info info;
    win32_coord bottom_left;
    uint32_t written;

    if (console_win32_attached != 0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        bottom_left.X = 0;
        bottom_left.Y = (int16_t)(info.dwSize.Y - 1);
        SetConsoleCursorPosition(console_output_handle, bottom_left);
        if (clear_text != 0) {
            if (FillConsoleOutputCharacterA(console_output_handle, ' ', info.dwSize.X,
                                             bottom_left, (LPDWORD)&written) != 0) {
                FillConsoleOutputAttribute(console_output_handle, info.wAttributes, info.dwSize.X,
                                            bottom_left, (LPDWORD)&written);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x497010):

void console_clear_bottom_line(char param_1)

{
  COORD dwWriteCoord;
  BOOL BVar1;
  COORD local_1c;
  _CONSOLE_SCREEN_BUFFER_INFO local_18;

  if ((DAT_006b2f18 != '\0') &&
     (BVar1 = GetConsoleScreenBufferInfo(DAT_006b2dd0,&local_18), BVar1 != 0)) {
    local_1c = (COORD)((uint)(ushort)(local_18.dwSize.Y - 1) << 0x10);
    dwWriteCoord = local_1c;
    SetConsoleCursorPosition(DAT_006b2dd0,local_1c);
    if (param_1 != '\0') {
      BVar1 = FillConsoleOutputCharacterA
                        (DAT_006b2dd0,' ',(int)local_18.dwSize.X,dwWriteCoord,(LPDWORD)&local_1c);
      if (BVar1 != 0) {
        FillConsoleOutputAttribute
                  (DAT_006b2dd0,local_18.wAttributes,(int)local_18.dwSize.X,dwWriteCoord,
                   (LPDWORD)&local_1c);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// console_clear_screen  (Ghidra: console_clear_screen, already named)
// address 0x496f90, size 124 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: straight win32 console API usage (GetConsoleScreenBufferInfo then two
// FillConsoleOutput* calls over the whole buffer), matches the given name exactly.
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

extern uint8_t console_win32_attached;  // 0x006b2f18
extern void *console_output_handle;     // 0x006b2dd0


// Blanks every cell of the attached win32 console window: fills the whole buffer with spaces in
// the current attribute, then reapplies that attribute over the same region.
void console_clear_screen(void)
{
    win32_console_screen_buffer_info info;
    win32_coord origin = {0, 0};
    uint32_t written;

    if (console_win32_attached != 0 &&
        GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
        if (FillConsoleOutputCharacterA(console_output_handle, ' ',
                                         (int32_t)info.dwSize.Y * (int32_t)info.dwSize.X,
                                         origin, &written) != 0) {
            FillConsoleOutputAttribute(console_output_handle, info.wAttributes,
                                        (int32_t)info.dwSize.Y * (int32_t)info.dwSize.X,
                                        origin, &written);
        }
    }
}

#if 0
Original Ghidra decompilation (0x496f90):

void console_clear_screen(void)

{
  BOOL BVar1;
  DWORD local_1c;
  _CONSOLE_SCREEN_BUFFER_INFO local_18;

  if ((DAT_006b2f18 != '\0') &&
     (BVar1 = GetConsoleScreenBufferInfo(DAT_006b2dd0,&local_18), BVar1 != 0)) {
    BVar1 = FillConsoleOutputCharacterA
                      (DAT_006b2dd0,' ',(int)local_18.dwSize.Y * (int)local_18.dwSize.X,(COORD)0x0,
                       &local_1c);
    if (BVar1 != 0) {
      FillConsoleOutputAttribute
                (DAT_006b2dd0,local_18.wAttributes,(int)local_18.dwSize.Y * (int)local_18.dwSize.X,
                 (COORD)0x0,&local_1c);
    }
  }
  return;
}
#endif

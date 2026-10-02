// console_draw_input_line  (Ghidra: console_draw_input_line, already named)
// address 0x4970a0, size 254 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: draws "<window title> <input line>" over the last row of the attached win32
// console, matching the given name; called by console_update_display and
// chimera__console_out_copy after either changes. The "%s %s" string is confirmed by
// out/phase4 naming hints against this exact address.
// register convention: no register-passed arguments.
// UNSURE: after formatting "%s %s" with _snprintf, the function immediately does what is
// arithmetically an in-place strcpy(line + strlen(title), input) -- Ghidra shows this as a
// byte-copy loop indexed through a 16-bit-truncated pointer difference that only resolves
// correctly because the compiler folded a compile-time-constant global address into it. The
// net, reproducible effect is that it overwrites the separating space _snprintf just wrote and
// re-pastes input right after title with no space between them; preserved verbatim, not "fixed".
// TYPES-GAP: COORD / CONSOLE_SCREEN_BUFFER_INFO are plain win32 console API structs, not engine
// types; declared locally rather than added to types/interface.h.

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
extern uint8_t console_win32_attached;   // 0x006b2f18
extern terminal_console *console_active; // 0x006b2f0c
extern char console_window_title[0x20];  // 0x006b2dd8
extern void *console_output_handle;      // 0x006b2dd0

extern void console_position_cursor(void); // 0x4971a0

// Draws "<window title> <input line>" over the last row of the attached win32 console window.
void console_draw_input_line(void)
{
    char line[0x11e];
    win32_console_screen_buffer_info info;
    win32_coord bottom_left;
    uint32_t written;
    uint32_t length;

    if (console_win32_attached != 0 && console_active != (terminal_console *)0) {
        _snprintf(line, 0x11e, "%s %s", console_window_title, console_active->input);
        strcpy(line + strlen(console_window_title), console_active->input);
        if (GetConsoleScreenBufferInfo(console_output_handle, &info) != 0) {
            bottom_left.X = 0;
            bottom_left.Y = (int16_t)(info.dwSize.Y - 1);
            if (FillConsoleOutputCharacterA(console_output_handle, ' ', info.dwSize.X,
                                             bottom_left, (LPDWORD)&written) != 0) {
                length = strlen(line);
                WriteConsoleOutputCharacterA(console_output_handle, line, length, bottom_left,
                                              (LPDWORD)&written);
            }
            console_position_cursor();
        }
    }
}

#if 0
Original Ghidra decompilation (0x4970a0):

void console_draw_input_line(void)

{
  char cVar1;
  COORD dwWriteCoord;
  char *pcVar2;
  int iVar3;
  BOOL BVar4;
  char *pcVar5;
  COORD local_13c;
  _CONSOLE_SCREEN_BUFFER_INFO local_138;
  char local_120 [288];

  if ((DAT_006b2f18 != '\0') && (DAT_006b2f0c != 0)) {
    __snprintf(local_120,0x11e,"%s %s",&DAT_006b2dd8,DAT_006b2f0c + 0xb4);
    pcVar2 = &DAT_006b2dd8;
    pcVar5 = (char *)(DAT_006b2f0c + 0xb4);
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    iVar3 = (int)(short)((short)pcVar2 + -0x2dd9) - (int)pcVar5;
    do {
      cVar1 = *pcVar5;
      (local_120 + iVar3)[(int)pcVar5] = cVar1;
      pcVar5 = pcVar5 + 1;
    } while (cVar1 != '\0');
    BVar4 = GetConsoleScreenBufferInfo(DAT_006b2dd0,&local_138);
    if (BVar4 != 0) {
      local_13c = (COORD)((uint)(ushort)(local_138.dwSize.Y - 1) << 0x10);
      dwWriteCoord = local_13c;
      BVar4 = FillConsoleOutputCharacterA
                        (DAT_006b2dd0,' ',(int)local_138.dwSize.X,local_13c,(LPDWORD)&local_13c);
      if (BVar4 != 0) {
        pcVar2 = local_120;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        WriteConsoleOutputCharacterA
                  (DAT_006b2dd0,local_120,(int)pcVar2 - (int)(local_120 + 1),dwWriteCoord,
                   (LPDWORD)&local_13c);
      }
      console_position_cursor();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

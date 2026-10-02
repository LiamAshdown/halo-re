// console_update_display  (Ghidra: FUN_00496d40; named by types/interface.h's own
// terminal_console note)
// address 0x496d40, size 167 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: types/interface.h documents this address as console_update_display, reading the
// cursor at terminal_console + 0x1ba (edit.cursor) to decide whether to reposition the caret;
// console_last_line (0x006b2df8, 0x100 bytes) and console_last_cursor_column (0x006b2ef8) are
// named in the same header's globals list.
// register convention: no register-passed arguments.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t console_win32_attached;        // 0x006b2f18
extern terminal_console *console_active;      // 0x006b2f0c
extern char console_last_line[0x100];         // 0x006b2df8
extern int32_t console_last_cursor_column;    // 0x006b2ef8

extern void console_draw_input_line(void);    // 0x4970a0
extern void console_position_cursor(void);    // 0x4971a0

// Per-frame refresh of the attached win32 console window: if the input line text changed since
// the last draw, redraws the input line and remembers the new text; then, independently, if the
// cursor column moved, remembers the new column and repositions the caret.
void console_update_display(void)
{
    if (console_win32_attached != 0 && console_active != (terminal_console *)0) {
        if (strcmp(console_last_line, console_active->input) != 0) {
            console_draw_input_line();
            strncpy(console_last_line, console_active->input, 0xff);
        }
        if (console_last_cursor_column != (int32_t)console_active->edit.cursor) {
            console_last_cursor_column = (int32_t)console_active->edit.cursor;
            console_position_cursor();
        }
    }
}

#if 0
Original Ghidra decompilation (0x496d40):

void FUN_00496d40(void)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  byte *pbVar4;
  bool bVar5;

  if ((DAT_006b2f18 != '\0') && (DAT_006b2f0c != 0)) {
    pbVar4 = (byte *)(DAT_006b2f0c + 0xb4);
    pbVar2 = &DAT_006b2df8;
    do {
      bVar1 = *pbVar2;
      bVar5 = bVar1 < *pbVar4;
      if (bVar1 != *pbVar4) {
LAB_00496d98:
        iVar3 = (1 - (uint)bVar5) - (uint)(bVar5 != 0);
        goto LAB_00496d9d;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar5 = bVar1 < pbVar4[1];
      if (bVar1 != pbVar4[1]) goto LAB_00496d98;
      pbVar2 = pbVar2 + 2;
      pbVar4 = pbVar4 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00496d9d:
    if (iVar3 != 0) {
      console_draw_input_line();
      _strncpy(&DAT_006b2df8,(char *)(DAT_006b2f0c + 0xb4),0xff);
    }
    if (DAT_006b2ef8 != *(short *)(DAT_006b2f0c + 0x1ba)) {
      DAT_006b2ef8 = (int)*(short *)(DAT_006b2f0c + 0x1ba);
      console_position_cursor();
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

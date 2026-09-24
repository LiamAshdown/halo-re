// console_paste_clipboard_text  (Ghidra: console_paste_clipboard_text, already named)
// address 0x4c6570, size 70 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: disassembly (objdump -d -M intel, bin/halo.exe) resolves the two Ghidra locals:
// console_active (0x006b2f0c, types/interface.h) compared against &console_globals_data.terminal
// (0x006b7024) gates the insert, and `lea esi,[eax+0x1b4]` shows the insert target is
// &console_active->edit (text_edit_state at +0x1b4 of terminal_console, consistent with
// edit.cursor documented at +0x1ba); widget_text_edit_insert_string (0x44c640, already renamed
// in src/interface/widget_text_edit_insert_string.c) takes state in ESI and the string in EAX,
// exactly the registers set up here.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"

extern terminal_console *console_active; // 0x006b2f0c
extern console_globals console_globals_data;  // 0x006b7020 (0x006b7024 is &console_globals_data.terminal)

extern uint32_t clipboard_get_text(char *buffer, uint32_t capacity); // 0x541ac0
extern void widget_text_edit_insert_string(text_edit_state *state, char *insert_str); // 0x44c640, blam-cc: ESI -> state, EAX -> insert_str

// Reads the current clipboard text into a local buffer and, if the console's own text edit is
// the currently active one, inserts it at the cursor. Returns whether clipboard text was read at
// all (independent of whether it was actually pasted into the console).
uint32_t console_paste_clipboard_text(void)
{
    char clipboard_text[0x100];
    uint32_t have_text;

    have_text = clipboard_get_text(clipboard_text, 0xff);
    if (have_text != 0 && console_active == &console_globals_data.terminal) {
        widget_text_edit_insert_string(&console_active->edit, clipboard_text); // ESI -> &edit, EAX -> clipboard_text
    }
    return have_text;
}

#if 0
Original Ghidra decompilation (0x4c6570):

char __cdecl console_paste_clipboard_text(void)

{
  char cVar1;
  undefined1 local_100 [256];

  cVar1 = clipboard_get_text(local_100,0xff);
  if ((cVar1 != '\0') && (DAT_006b2f0c == &DAT_006b7024)) {
    FUN_0044c640();
  }
  return cVar1;
}

Disassembly (0x4c6570..0x4c65b5):

004c6570:  sub    esp,0x100
004c6576:  push   ebx
004c6577:  lea    eax,[esp+0x4]         ; &clipboard_text
004c657b:  push   0xff
004c6580:  push   eax
004c6581:  call   0x541ac0              ; clipboard_get_text(&clipboard_text, 0xff)
004c6586:  mov    bl,al
004c6588:  add    esp,0x8
004c658b:  test   bl,bl
004c658d:  je     0x4c65ac
004c658f:  mov    eax,ds:0x6b2f0c       ; console_active
004c6594:  cmp    eax,0x6b7024          ; &console_globals_data.terminal
004c6599:  jne    0x4c65ac
004c659b:  push   esi
004c659c:  lea    esi,[eax+0x1b4]       ; &console_active->edit
004c65a2:  lea    eax,[esp+0x8]         ; &clipboard_text
004c65a6:  call   0x44c640              ; widget_text_edit_insert_string(ESI, EAX)
004c65ab:  pop    esi
004c65ac:  mov    al,bl
004c65ae:  pop    ebx
004c65af:  add    esp,0x100
004c65b5:  ret
#endif

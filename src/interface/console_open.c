// console_open  (Ghidra: console_open, already named)
// address 0x496510, size 100 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Lazily activates the developer console/terminal
// for the first time, wiring up its edit-line structure"; types/interface.h terminal_console
// ("console_open builds the edit state in place: edit.text is set to the address of input[0]
// and edit.maximum_length to 0xff").
// register convention: terminal_console* in EDI (unaff_EDI). // blam-cc: EDI -> console
// UNSURE: the call into widget_text_edit_clamp_selection is immediately followed by console_open
// overwriting edit.cursor and edit.selection_anchor with fresh values of its own, making the
// clamp call's effect on those two fields moot; preserved verbatim rather than dropped as dead.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern terminal_console *console_active; // 0x006b2f0c


// Lazily activates the developer console for `console` the first time it is opened: wires up
// its embedded text_edit_state to edit `input` in place, seeds the cursor at the end of
// whatever text is already there and clears any selection, then restores the win32 console
// cursor. Returns 1 if this call actually opened the console, 0 if one was already active.
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t console_open(terminal_console *console)
{
    int32_t opened;

    opened = 0;
    if (console_active == (terminal_console *)0) {
        console->edit.text = console->input;
        console_active = console;
        console->edit.maximum_length = 0xff;
        widget_text_edit_clamp_selection(&console->edit);
        console->edit.cursor = (int16_t)strlen(console->edit.text);
        console->edit.selection_anchor = -1;
        console->key_event_count = 0;
        console_restore_cursor();
        opened = 1;
    }
    return opened;
}

#if 0
Original Ghidra decompilation (0x496510):

undefined4 console_open(void)

{
  undefined4 *puVar1;
  char cVar2;
  short sVar3;
  undefined4 uVar4;
  char *pcVar5;
  undefined2 *unaff_EDI;

  uVar4 = 0;
  if (DAT_006b2f0c == (undefined2 *)0x0) {
    puVar1 = (undefined4 *)(unaff_EDI + 0xda);
    *puVar1 = unaff_EDI + 0x5a;
    DAT_006b2f0c = unaff_EDI;
    unaff_EDI[0xdc] = 0xff;
    FUN_0044c780(puVar1);
    pcVar5 = (char *)*puVar1;
    sVar3 = (short)pcVar5;
    do {
      cVar2 = *pcVar5;
      pcVar5 = pcVar5 + 1;
    } while (cVar2 != '\0');
    unaff_EDI[0xdd] = (short)pcVar5 - (sVar3 + 1);
    unaff_EDI[0xde] = 0xffff;
    *unaff_EDI = 0;
    FUN_00496c20();
    uVar4 = 1;
  }
  return uVar4;
}
#endif

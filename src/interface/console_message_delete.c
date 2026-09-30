// console_message_delete  (Ghidra: console_message_delete, already named)
// address 0x496490, size 117 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Removes and frees one entry from the on-screen
// console message linked list."; types/interface.h console_message (previous toward the newest,
// next toward the oldest) and the console_message_head/console_message_tail globals.
// register convention: message handle in EDX (in_EDX). // blam-cc: EDX -> message

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern data_array *terminal_messages;     // 0x006b2f00, "terminal output"
extern datum_index console_message_head;  // 0x006b2f04, newest
extern datum_index console_message_tail;  // 0x006b2f08, oldest

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510

// Unlinks `message` from the newest-to-oldest console_message list, patching the neighbours
// (or the head/tail globals when it was at an end of the list), then frees its datum.
void console_message_delete(datum_index message)
{
    console_message *record;
    datum_index next;      // toward the oldest
    datum_index previous;  // toward the newest

    record = (console_message *)((char *)terminal_messages->data +
                                  (uint16_t)message * sizeof(console_message));
    next = record->next;
    previous = record->previous;
    if (next == (datum_index)0xffffffff) {
        console_message_tail = previous;
    } else {
        ((console_message *)((char *)terminal_messages->data +
                              (uint16_t)next * sizeof(console_message)))->previous = previous;
    }
    if (previous != (datum_index)0xffffffff) {
        ((console_message *)((char *)terminal_messages->data +
                              (uint16_t)previous * sizeof(console_message)))->next = next;
        datum_delete(terminal_messages, message);
        return;
    }
    console_message_head = next;
    datum_delete(terminal_messages, message);
}

#if 0
Original Ghidra decompilation (0x496490):

void console_message_delete(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint in_EDX;

  iVar3 = DAT_006b2f00;
  iVar4 = (in_EDX & 0xffff) * 0x124;
  iVar1 = *(int *)(DAT_006b2f00 + 0x34);
  uVar2 = *(uint *)(iVar4 + 8 + iVar1);
  iVar4 = iVar4 + iVar1;
  if (uVar2 == 0xffffffff) {
    DAT_006b2f08 = *(undefined4 *)(iVar4 + 4);
  }
  else {
    *(undefined4 *)((uVar2 & 0xffff) * 0x124 + 4 + iVar1) = *(undefined4 *)(iVar4 + 4);
  }
  if (*(uint *)(iVar4 + 4) != 0xffffffff) {
    *(undefined4 *)((*(uint *)(iVar4 + 4) & 0xffff) * 0x124 + 8 + *(int *)(iVar3 + 0x34)) =
         *(undefined4 *)(iVar4 + 8);
    datum_delete();
    return;
  }
  DAT_006b2f04 = *(undefined4 *)(iVar4 + 8);
  datum_delete();
  return;
}
#endif

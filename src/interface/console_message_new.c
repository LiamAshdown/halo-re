// console_message_new  (Ghidra: console_message_new, already named)
// address 0x496420, size 110 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Allocates a new fading on-screen console message
// slot from the console data array, evicting the oldest when full."; types/interface.h's
// console_message (previous/next link toward the newest/oldest message respectively) and the
// console_message_head/console_message_tail globals.
// register convention: none (void); returns the new message's datum_index in EAX.
// UNSURE: Ghidra shows `datum_new()` with no visible arguments and a stray high dword (in_EDX
// widened to undefined8) that is really the surviving `terminal_messages` register argument
// read back after the call, not part of the return value -- resolved against
// src/memory/datum_new.c's documented `data_array*` in EDX convention.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *terminal_messages;     // 0x006b2f00, "terminal output"
extern datum_index console_message_head;  // 0x006b2f04, newest
extern datum_index console_message_tail;  // 0x006b2f08, oldest

extern datum_index datum_new(data_array *array); // 0x4d0480
extern void console_message_delete(datum_index message); // 0x496490

// Allocates a new console_message slot, evicting the oldest message first if the terminal
// output array's high-water mark has reached its capacity, and links the new slot in at the
// head of the newest-to-oldest list.
datum_index console_message_new(void)
{
    datum_index old_head;
    datum_index new_message;
    console_message *record;

    if (terminal_messages->last_index == 0x20) {
        console_message_delete(console_message_tail);
    }
    new_message = datum_new(terminal_messages);
    old_head = console_message_head;
    record = (console_message *)((char *)terminal_messages->data +
                                  (uint16_t)new_message * sizeof(console_message));
    record->next = console_message_head;
    record->previous = (datum_index)0xffffffff;
    console_message_head = new_message;
    if (old_head != (datum_index)0xffffffff) {
        ((console_message *)((char *)terminal_messages->data +
                              (uint16_t)old_head * sizeof(console_message)))->previous =
            new_message;
    } else {
        console_message_tail = new_message;
    }
    return new_message;
}

#if 0
Original Ghidra decompilation (0x496420):

void console_message_new(void)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  undefined8 uVar6;

  if (*(short *)(DAT_006b2f00 + 0x2e) == 0x20) {
    console_message_delete();
  }
  uVar6 = datum_new();
  uVar1 = DAT_006b2f04;
  iVar4 = (int)((ulonglong)uVar6 >> 0x20);
  uVar2 = (uint)uVar6;
  iVar3 = (uVar2 & 0xffff) * 0x124 + *(int *)(iVar4 + 0x34);
  *(uint *)(iVar3 + 8) = DAT_006b2f04;
  *(undefined4 *)(iVar3 + 4) = 0xffffffff;
  bVar5 = DAT_006b2f04 != 0xffffffff;
  DAT_006b2f04 = uVar2;
  if (bVar5) {
    *(uint *)((uVar1 & 0xffff) * 0x124 + 4 + *(int *)(iVar4 + 0x34)) = uVar2;
    return;
  }
  DAT_006b2f08 = uVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

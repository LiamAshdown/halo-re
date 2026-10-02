// console_message_expire_old  (Ghidra: chimera__console_fade_fn, renamed per types/interface.h)
// address 0x4966e0, size 79 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: types/interface.h console_message struct comment lists this address as
// console_message_expire_old; out/phase4/interface_functions.md "Ages every on-screen console
// message each frame and deletes any that have been visible for more than [150 frames]."; the
// age field and its 0x96 (150) cutoff match console_message::age exactly.
// register convention: none (void).

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

extern void console_message_delete(datum_index message); // 0x496490

// Walks every live console_message from newest to oldest, incrementing its age each frame and
// deleting it once that age passes 150.
void console_message_expire_old(void)
{
    datum_index current;
    datum_index next;
    console_message *record;

    current = console_message_head;
    while (current != (datum_index)0xffffffff) {
        record = (console_message *)((char *)terminal_messages->data +
                                      (uint16_t)current * sizeof(console_message));
        next = record->next;
        record->age = record->age + 1;
        if (record->age > 0x96) {
            console_message_delete(current);
        }
        current = next;
    }
}

#if 0
Original Ghidra decompilation (0x4966e0):

void chimera__console_fade_fn(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;

  uVar1 = DAT_006b2f04;
  while (uVar1 != 0xffffffff) {
    iVar2 = (uVar1 & 0xffff) * 0x124;
    iVar3 = iVar2 + *(int *)(DAT_006b2f00 + 0x34);
    uVar1 = *(uint *)(iVar3 + 8);
    iVar2 = *(int *)(iVar2 + 0x120 + *(int *)(DAT_006b2f00 + 0x34)) + 1;
    *(int *)(iVar3 + 0x120) = iVar2;
    if (0x96 < iVar2) {
      console_message_delete();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

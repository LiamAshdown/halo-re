// actor_flee_look_away  (Ghidra: actor_flee_look_away, renamed)
// address 0x40d4c0, size 91 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: phase-4 summary "while fleeing, makes the actor look away from the threat by
// committing a randomized-look order"; gated on mode == death (4) here even though the
// summary says "fleeing" -- see UNSURE below.
// register convention: actor_index in EDI (Ghidra's unaff_EDI: this function has no
// parameters of its own visible in the decompilation, only a register the caller left set).
// blam-cc: EDI -> actor_index (unaff_EDI)
// UNSURE: types/ai.h names mode 4 _actor_mode_death, but the phase-4 summary describes
// this as a flee behaviour; the mode_data byte at +0xf (actor+0xab) that gates it is inside
// the per-mode union and is not otherwise named. actor_build_order_look and actor_set_mode are called
// with zero visible arguments in the decompiled C; almost certainly actor_index (still in
// EDI) plus, for actor_set_mode, a mode number and mode_data pointer that Ghidra failed to
// resolve here. Needs the disassembly review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360
extern int32_t actor_build_order_look(uint32_t actor_index, actor_order *order, actor_look_request *request); // 0x4046c0, EAX, ESI, EBX
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

// blam-cc: EDI -> actor_index
uint32_t actor_flee_look_away(datum_index actor_index)
{
    actor *self;
    uint32_t result;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    result = 0;
    if (self->mode == _actor_mode_death && self->mode_data.raw[0xab - 0x9c] != 0) {
        // 0x40d4f0: a look order from the flee mode's request (+0x9c), then guard (mode 6) with it
        uint8_t order[0x84];    // [esp+0x8]

        memset(order, 0, sizeof(order));
        actor_build_order_look(actor_index, (actor_order *)order, (actor_look_request *)((uint8_t *)self + 0x9c));
        actor_set_mode(actor_index, 6, order);
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x40d4c0):

undefined4 FUN_0040d4c0(void)

{
  undefined4 uVar1;
  int iVar2;
  uint unaff_EDI;

  iVar2 = (unaff_EDI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar1 = 0;
  if ((*(short *)(iVar2 + 0x6c) == 4) && (*(char *)(iVar2 + 0xab) != '\0')) {
    FUN_004046c0();
    actor_set_mode();
    uVar1 = 1;
  }
  return uVar1;
}
#endif

// actor_clear_recognition_history  (Ghidra: actor_clear_recognition_history, renamed)
// address 0x414140, size 95 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: writes exactly the fields types/ai.h attributes to this address --
//   recognition_cursor (0x3c6) zeroed, all four recognition[i].firing_position_index
//   (0x3ca + i*4) set to -1, and recognition_valid (0x3d8) conditionally cleared.
//   actor_find_best_firing_position @0x412ba0 inlines the same sequence twice with the
//   keep_when_typed argument zero, which is what pins the conditional.
// register convention: actor_index in EAX (Ghidra in_EAX); the gate byte is the one
//   Ghidra recognized as a stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> keep_when_typed
// Resets the four-entry recognition ring: the cursor goes back to slot 0 and every slot
// gets its firing position index invalidated. recognition_valid is only cleared when the
// caller passes keep_when_typed as zero, or when it is set but the currently latched
// recognition_type is itself nonzero.
void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed)
{
    actor *self;
    int i;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    self->recognition_cursor = 0;
    for (i = 0; i < 4; i++) {
        self->recognition[i].firing_position_index = -1;
    }

    if (self->recognition_valid != 0 && (keep_when_typed == 0 || self->recognition_type != 0)) {
        self->recognition_valid = 0;
    }
}

#if 0
Original Ghidra decompilation (0x414140):

void FUN_00414140(char param_1)

{
  uint in_EAX;
  int iVar1;
  undefined2 *puVar2;
  int iVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined2 *)(iVar1 + 0x3c6) = 0;
  puVar2 = (undefined2 *)(iVar1 + 0x3ca);
  iVar3 = 4;
  do {
    *puVar2 = 0xffff;
    puVar2 = puVar2 + 2;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  if ((*(char *)(iVar1 + 0x3d8) != '\0') &&
     ((param_1 == '\0' || (*(char *)(iVar1 + 0x3d9) != '\0')))) {
    *(undefined1 *)(iVar1 + 0x3d8) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

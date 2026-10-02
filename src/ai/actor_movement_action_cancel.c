// actor_movement_action_cancel  (Ghidra: actor_movement_action_cancel, renamed per types/ai.h's own citation)
// address 0x428650, size 101 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (REWRITTEN from objdump)
// evidence: types/ai.h actor_movement_action.cancelled (0x02, actor_movement_action_cancel
//   sets it) cites this exact address; actor.secondary_action(0x46c)/firing_position_index
//   (0x3b8)/active_movement.extra(0x480). The mode-table call at 0x00655278 is
//   actor_mode_definitions[mode]+0x24, a fifth unnamed per-mode procedure slot inside
//   types/ai.h's actor_mode_definition.unknown_1c[28] padding (see also
//   actor_replace_object_reference's +0x20 slot and actor_clear_target_state's +0x28 slot).
// register convention: EDI -> actor_index (unaff_EDI).
//   // blam-cc: EDI -> actor_index

// VERIFIED against disassembly 0x428650..0x4286b4 (2026-09-30): the mode table row is 0x38 bytes, the +0x24 handler is
//   called cdecl with the actor index (push edi; call eax).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

// blam-cc: EDI -> actor_index
void actor_movement_action_cancel(datum_index actor_index)
{
    // REWRITTEN from objdump 0x428650..0x4286b4: the claimed firing position (+0x3b8) is released; an ACTIVE
    //   movement (+0x46c type) of kind 3 or 4 is reset to 0 with its extra (+0x480) cleared; then the mode's
    //   +0x24 handler runs with the actor pushed. The draft tested secondary_action (+0x418) and called the
    //   handler without its actor argument.
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t movement_type = self->active_movement.type;

    self->firing_position_index = -1;
    if (movement_type == 3 || movement_type == 4) {
        self->active_movement.type = 0;
        self->active_movement.extra = 0xffffffff;
    }

    {
        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x24);
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x428650):

void FUN_00428650(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint unaff_EDI;

  iVar2 = DAT_00880360;
  iVar3 = (unaff_EDI & 0xffff) * 0x724;
  sVar1 = *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x46c + iVar3);
  iVar4 = *(int *)(DAT_00880360 + 0x34) + iVar3;
  *(undefined2 *)(iVar4 + 0x3b8) = 0xffff;
  if ((sVar1 == 3) || (sVar1 == 4)) {
    *(undefined2 *)(iVar4 + 0x46c) = 0;
    *(undefined4 *)(iVar4 + 0x480) = 0xffffffff;
  }
  if (*(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar2 + 0x34) + iVar3 + 0x6c) * 0x38) !=
      (code *)0x0) {
    (**(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar2 + 0x34) + iVar3 + 0x6c) * 0x38))();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

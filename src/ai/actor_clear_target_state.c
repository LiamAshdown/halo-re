// actor_clear_target_state  (Ghidra: actor_clear_target_state, renamed)
// address 0x4286c0, size 212 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4286c0..0x428793; swarm / movement offsets match; the mode proc tail call now gets the actor)
// evidence: types/ai.h actor.unknown_164/search_unknown_324/unknown_494; swarm.component_count/
//   component_index; swarm_component.marker_index (0x10). Fields 0x144/0x148 fall inside
//   actor.unknown_138[0x20] (unnamed) and are accessed as raw offsets. The mode-table call at
//   0x0065527c is actor_mode_definitions[mode]+0x28, a sixth unnamed per-mode procedure slot
//   (see actor_movement_action_cancel and actor_replace_object_reference for the others);
//   Ghidra could not recover its jump table, so it is modeled here as a plain no-argument
//   call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

// Clears a cached target field and various movement/search scratch fields, and, if the
// actor is in mode 3 or 4 (per its queued/active movement action types), the corresponding
// action's parameter; also clears every swarm component's death marker, then invokes the
// per-type callback table.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
void actor_clear_target_state(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    *(int16_t *)((uint8_t *)self + 0x148) = -1; // UNSURE offset (within unknown_138[0x20])
    *(uint32_t *)((uint8_t *)self + 0x144) = 0xffffffff; // UNSURE offset
    self->pathfinding_surface_index = 0xffffffff;
    self->search_unknown_324 = 0xffffffff;

    if (self->queued_movement.type == 2) {
        self->queued_movement.parameter = 0xffffffff;
    }
    if (self->active_movement.type == 2) {
        self->active_movement.parameter = 0xffffffff;
    }

    self->destination_surface_index = 0xffffffff;

    if (self->swarm != 0 && self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & 0xffff];
            component->marker_index = (datum_index)k_datum_index_none;
        }
    }

    {
        // 0x428774..0x428791: the mode's target-cleared procedure (definition +0x28: 0x401490 alert, 0x405180 guard,
        // 0x4112b0) is TAIL-JUMPED to with this function's own stack argument still in place, so it receives the
        // actor index at [esp+4]. FIXED 2026-09-27: the draft called it with no argument (garbage actor).
        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x28);
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4286c0):

void FUN_004286c0(uint param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;

  iVar4 = (param_1 & 0xffff) * 0x724;
  iVar2 = *(int *)(DAT_00880360 + 0x34) + iVar4;
  *(undefined2 *)(iVar2 + 0x148) = 0xffff;
  *(undefined4 *)(iVar2 + 0x144) = 0xffffffff;
  *(undefined4 *)(iVar2 + 0x164) = 0xffffffff;
  *(undefined4 *)(iVar2 + 0x324) = 0xffffffff;
  if (*(short *)(iVar2 + 0x400) == 2) {
    *(undefined4 *)(iVar2 + 0x410) = 0xffffffff;
  }
  if (*(short *)(iVar2 + 0x46c) == 2) {
    *(undefined4 *)(iVar2 + 0x47c) = 0xffffffff;
  }
  *(undefined4 *)(iVar2 + 0x494) = 0xffffffff;
  iVar1 = DAT_00880358;
  if ((*(char *)(iVar2 + 6) != '\0') && (*(uint *)(iVar2 + 0x28) != 0xffffffff)) {
    iVar2 = (*(uint *)(iVar2 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar3 = 0;
    if (0 < *(short *)(iVar2 + 2)) {
      do {
        iVar5 = (int)sVar3;
        sVar3 = sVar3 + 1;
        *(undefined4 *)
         ((*(uint *)(iVar2 + 0x58 + iVar5 * 4) & 0xffff) * 0x40 + 0x10 + *(int *)(iVar1 + 0x34)) =
             0xffffffff;
      } while (sVar3 < *(short *)(iVar2 + 2));
    }
  }
  if (*(code **)(&DAT_0065527c + *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iVar4) * 0x38) ==
      (code *)0x0) {
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x00428791. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(&DAT_0065527c + *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iVar4) * 0x38))();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

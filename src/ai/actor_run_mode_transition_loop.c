// actor_run_mode_transition_loop  (Ghidra: actor_run_mode_transition_loop, already named)
// address 0x429ee0, size 216 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h actor.mode_changed(0x70)/mode(0x6c)/type(0x04); actor_type_procs[16]
//   (0x006853b8, unknown_14 slot per actor_type_table_entry); actor_mode_definitions[16]
//   (0x00655254). Calls actor_set_mode (0x40d8d0, already established: actor_index, mode,
//   mode_data).
//   UNSURE: the table call at 0x00655260 is actor_mode_definitions[mode]+0xc, inside that
//   struct's unknown_0c[8] gap -- a per-mode "should keep transitioning" predicate with no
//   established name, returning the bool this loop tests. The zeroed run at actor+0x2ec for
//   0x19 dwords spans from inside unknown_2e8[5] through the whole look-at/search/perception
//   scratch region up to actor.unknown_350; addressed here as a raw pointer since it crosses
//   several individually-named fields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data;      // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0

// Drives an actor's mode-transition state machine: each pass clears mode_changed, invokes
// the per-type "unknown_14" callback, clears the whole look-at/search/perception scratch
// region, and, unless both the per-mode "keep transitioning" predicate and mode_changed are
// clear, loops again (up to 10 times); forces mode 0 if it never settles within that budget.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
void actor_run_mode_transition_loop(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t keep_going = 0;
    int iterations = 0;

    for (;;) {
        iterations = iterations + 1;
        self->mode_changed = 0;

        {
            actor_type_table_entry *type_entry = (actor_type_table_entry *)actor_type_procs[self->type];
            if (type_entry->unknown_14 != 0) {
                ((void (*)(datum_index))type_entry->unknown_14)(actor_index);
            }
        }

        memset((uint8_t *)self + 0x2ec, 0, 0x19 * sizeof(uint32_t)); // 0x2ec..0x34f

        if ((keep_going != 0 && self->mode_changed == 0) || iterations > 9) {
            break;
        }

        keep_going = 0;
        {
            uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0xc); // UNSURE offset
            if (proc != 0) {
                keep_going = ((uint8_t (*)(datum_index))proc)(actor_index);
            }
        }

        if (keep_going == 0 && self->mode_changed == 0) {
            return;
        }
    }

    actor_set_mode(actor_index, 0, 0);
}

#if 0
Original Ghidra decompilation (0x429ee0):

void actor_run_mode_transition_loop(uint param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int local_4;

  iVar5 = (param_1 & 0xffff) * 0x724;
  iVar4 = *(int *)(DAT_00880360 + 0x34) + iVar5;
  cVar1 = '\0';
  local_4 = 0;
  iVar3 = DAT_00880360;
  while( true ) {
    local_4 = local_4 + 1;
    *(undefined1 *)(iVar4 + 0x70) = 0;
    (**(code **)((&PTR_PTR_006853b8)[*(short *)(*(int *)(iVar3 + 0x34) + 4 + iVar5)] + 0x14))
              (param_1);
    iVar3 = DAT_00880360;
    puVar6 = (undefined4 *)(*(int *)(DAT_00880360 + 0x34) + 0x2ec + iVar5);
    for (iVar2 = 0x19; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    if (((cVar1 != '\0') && (*(char *)(iVar4 + 0x70) == '\0')) || (9 < local_4)) break;
    cVar1 = '\0';
    if (*(code **)(&DAT_00655260 + *(short *)(*(int *)(iVar3 + 0x34) + iVar5 + 0x6c) * 0x38) !=
        (code *)0x0) {
      cVar1 = (**(code **)(&DAT_00655260 + *(short *)(*(int *)(iVar3 + 0x34) + iVar5 + 0x6c) * 0x38)
              )(param_1);
      iVar3 = DAT_00880360;
    }
    if ((cVar1 == '\0') && (*(char *)(iVar4 + 0x70) == '\0')) {
      return;
    }
  }
  actor_set_mode(param_1,0,0);
  return;
}
#endif

// actor_clear_perceived_props  (Ghidra: actor_clear_perceived_props, renamed)
// address 0x427e00, size 89 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: types/ai.h actor.first_prop (0x50, head of the prop list chained through
//   prop.next_in_actor at +0x08). Calls actor_replace_object_reference (0x428470, in this
//   rewrite range, not yet written when this file was authored) with a clear single visible
//   argument (actor_index). actor_unlink_prop and datum_delete are called with none in Ghidra's
//   decompile; src/ai/ai_clear_object_references.c already declares actor_unlink_prop as a plain
//   void(void) with the same UNSURE caveat, so it is kept that way here too rather than
//   guessed differently per call site. datum_delete's handle is assumed to be the prop this
//   iteration is removing (the loop's only live prop index), matching its established
//   EAX/EDX convention with prop_data supplied explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference); // 0x428470, already rewritten in this module
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove); // 0x43ea20, EAX, EDI
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

// Empties and frees every entry in the actor's perceived-unit (prop) list: for as long as
// the actor still has a first_prop, asks actor_replace_object_reference to scrub references
// to it, unlinks it, and deletes its datum.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
void actor_clear_perceived_props(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    while (self->first_prop != (datum_index)k_datum_index_none) {
        datum_index prop_index = self->first_prop;
        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
        // 0x427e30: ESI -1, EDI the prop itself (not its object), stack the actor; then EAX actor, EDI prop
        (void)p;
        actor_replace_object_reference(actor_index, 0xffffffff, prop_index);
        actor_unlink_prop(actor_index, prop_index);
        datum_delete(prop_data, prop_index);
    }
}

#if 0
Original Ghidra decompilation (0x427e00):

void FUN_00427e00(uint param_1)

{
  int iVar1;
  int iVar2;

  iVar2 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = *(int *)(iVar2 + 0x50);
  while (iVar1 != -1) {
    actor_replace_object_reference(param_1);
    FUN_0043ea20();
    datum_delete();
    iVar1 = *(int *)(iVar2 + 0x50);
  }
  return;
}
#endif

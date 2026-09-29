// actor_dispatch_squad_order  (Ghidra: actor_dispatch_squad_order, renamed)
// address 0x42a540, size 104 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: types/ai.h prop.owner_actor_index (0x1c). Calls actor_queue_search_and_relay_perception
//   (0x4221f0, already rewritten in this module), actor_scan_ally_death_panic_reaction
//   (0x4233d0, already rewritten in this module), datum_get (0x4d0680) and actor_target_data_acquire
//   (outside this rewrite's range, UNSURE signature). Phase-4 summary: "Dispatches one of
//   several squad-order handlers based on an order record's type field."
//   UNSURE: EBX (actor_index) is never set explicitly in Ghidra's decompile of this
//   function; assumed to be a passthrough from this function's own caller, matching the
//   register roles already established for actor_queue_search_and_relay_perception and actor_scan_ally_death_panic_reaction.
// register convention: EAX -> prop_index, ECX -> order (a record with a type field at +0x14),
//   EBX -> actor_index (implicit passthrough).
//   // blam-cc: EAX -> prop_index, ECX -> order, EBX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *prop_data;  // 0x008802c0
extern data_array *actor_data; // 0x00880360


extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680


// A caller-owned order record; only the type field at +0x14 is read here.

// blam-cc: EAX -> prop_index, ECX -> order, EBX -> actor_index
// Dispatches one of several squad-order handlers based on an order record's type field: 2
// relays search/perception state, 3 forwards the target prop's owning actor (if still valid)
// to actor_target_data_acquire, and 4 runs the ally-death panic reaction.
void actor_dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index)
{
    if (order == 0) {
        return;
    }

    if (order->type == 2) {
        actor_queue_search_and_relay_perception(prop_index, actor_index);
    } else if (order->type == 3) {
        // 0x42a55d: the ordered prop (order +0x18) is looked up in prop_data; its object is acquired with this
        // prop's owner (+0x1c) and the ordered prop as the pair reference
        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
        if (p->owner_actor_index != (datum_index)k_datum_index_none) {
            datum_index ordered = *(datum_index *)((uint8_t *)order + 0x18);
            prop *other = (prop *)datum_get(ordered, prop_data);

            if (other != 0) {
                actor_target_data_acquire(actor_index, other->object_index, p->owner_actor_index, ordered);
            }
        }
    } else if (order->type == 4) {
        actor_scan_ally_death_panic_reaction(prop_index, actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x42a540):

void FUN_0042a540(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int in_ECX;

  if (in_ECX != 0) {
    sVar1 = *(short *)(in_ECX + 0x14);
    if (sVar1 == 2) {
      FUN_004221f0();
      return;
    }
    if (sVar1 == 3) {
      if ((*(int *)((in_EAX & 0xffff) * 0x138 + 0x1c + *(int *)(DAT_008802c0 + 0x34)) != -1) &&
         (iVar2 = datum_get(), iVar2 != 0)) {
        FUN_0041f7d0();
      }
    }
    else if (sVar1 == 4) {
      FUN_004233d0();
      return;
    }
  }
  return;
}
#endif

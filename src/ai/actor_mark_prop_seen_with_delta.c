// actor_mark_prop_seen_with_delta  (Ghidra: actor_mark_prop_seen_with_delta, renamed)
// address 0x428840, size 153 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: types/ai.h prop.seen_state(0x6c)/seen(0x74)/unknown_70/pair_index(0xc). Phase-4
//   summary: "Applies a time delta to a squad's timer/flags (and a linked squad's, if any)
//   before invoking further per-actor cleanup via actor_queue_directional_reaction_event." Calls actor_find_or_create_shared_prop (outside
//   this rewrite's range, UNSURE signature -- resolves `key` to a prop index) and
//   actor_queue_directional_reaction_event (0x422270, already rewritten in this module,
//   though its real 3-parameter signature does not obviously match this call site's single
//   visible argument; kept as a best-effort passthrough).
// register convention: EAX -> squad_index (-1 disables the whole call), stack -> key, delta.
//   // blam-cc: EAX -> squad_index, stack -> key, delta

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // 0x43eb30, UNSURE signature
extern void actor_queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index, datum_index actor_index); // 0x422270, UNSURE how this call site's single argument maps to it

// blam-cc: EAX -> squad_index, stack -> key, delta
void actor_mark_prop_seen_with_delta(datum_index squad_index, uint32_t key, float delta)
{
    datum_index prop_index;

    if (squad_index == (datum_index)k_datum_index_none) {
        return;
    }

    prop_index = actor_find_or_create_shared_prop(key, 1, 1);
    if (prop_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
        p->seen_state = 0;
        p->seen = 1;
        p->unknown_70 = delta + p->unknown_70;

        if (p->pair_index != (datum_index)k_datum_index_none) {
            prop *paired = &((prop *)prop_data->data)[p->pair_index & 0xffff];
            paired->seen_state = 0;
            paired->seen = 1;
            paired->unknown_70 = delta + paired->unknown_70;
        }
    }

    // UNSURE: the original passes its own `key` argument (param_1) straight through here
    // with no other visible setup; see file header.
    actor_queue_directional_reaction_event(0, (datum_index)key, (datum_index)k_datum_index_none);
}

#if 0
Original Ghidra decompilation (0x428840):

void FUN_00428840(undefined4 param_1,float param_2)

{
  int in_EAX;
  uint uVar1;
  int iVar2;
  int iVar3;

  if (in_EAX != -1) {
    uVar1 = FUN_0043eb30(param_1,1,1);
    iVar3 = DAT_008802c0;
    if (uVar1 != 0xffffffff) {
      iVar2 = (uVar1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
      *(undefined2 *)(iVar2 + 0x6c) = 0;
      *(undefined1 *)(iVar2 + 0x74) = 1;
      *(float *)(iVar2 + 0x70) = param_2 + *(float *)(iVar2 + 0x70);
      if (*(uint *)(iVar2 + 0xc) != 0xffffffff) {
        iVar3 = (*(uint *)(iVar2 + 0xc) & 0xffff) * 0x138 + *(int *)(iVar3 + 0x34);
        *(undefined2 *)(iVar3 + 0x6c) = 0;
        *(undefined1 *)(iVar3 + 0x74) = 1;
        *(float *)(iVar3 + 0x70) = param_2 + *(float *)(iVar3 + 0x70);
      }
    }
    FUN_00422270(param_1);
  }
  return;
}
#endif

// actor_mark_prop_seen_with_delta  (Ghidra: actor_mark_prop_seen_with_delta, renamed)
// address 0x428840, size 153 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// REWRITTEN from objdump 0x428840..0x4288d8. EAX: the object that hurt or threatened the actor (none: nothing
//   happens); stack: actor, delta, direction. The actor's prop for that object (found or created, 0x43eb30 with
//   create 1 / flag 1) is marked seen now (+0x6c = 0, +0x74 = 1) and its +0x70 raised by delta, and so is its
//   pair (+0xc). The directional reaction (0x422270) then gets the direction, the prop when it is kind 2..3
//   (else none) and the actor. The draft looked the prop up by the actor alone and queued the reaction with a
//   NULL direction and the actor in the prop slot.
// blam-cc: EAX -> object_index, stack -> actor_index, delta, direction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *prop_data; // 0x008802c0

extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack


#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

void actor_mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta,
    const real_vector3d *direction)
{
    datum_index prop_index;

    if (object_index == k_datum_index_none) {
        return;
    }
    prop_index = actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index != k_datum_index_none) {
        uint8_t *p = PROP(prop_index);
        datum_index pair = *(datum_index *)(p + 0xc);
        int16_t kind;

        *(int16_t *)(p + 0x6c) = 0;
        p[0x74] = 1;
        *(float *)(p + 0x70) = delta + *(float *)(p + 0x70);
        if (pair != k_datum_index_none) {
            uint8_t *q = PROP(pair);

            *(int16_t *)(q + 0x6c) = 0;
            q[0x74] = 1;
            *(float *)(q + 0x70) = delta + *(float *)(q + 0x70);
        }
        kind = *(int16_t *)(p + 0x24);
        if (kind < 2 || kind > 3) {
            prop_index = k_datum_index_none;
        }
    }
    actor_queue_directional_reaction_event(direction, prop_index, actor_index);
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

// actor_find_prop_for_object  (Ghidra: actor_find_prop_for_object, renamed)
// address 0x43ea80, size 166 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/ai.h actor.first_prop(+0x50), prop.next_in_actor(+0x08)/has_parent(+0x14)/
// owner_actor_index(+0x1c)/object_index(+0x18)/kind(+0x24); object+0x1f4/+0x1f8 ("the
// controlling actor handle" / "actor cluster links" per the module header). phase-4 summary
// "searches an object's firing-position node list for one owned by a given actor or matching
// a target's cluster, skipping reserved node types."
// register convention: ECX -> actor_index; stack -> object_index.
//   // blam-cc: ECX -> actor_index, stack -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0
extern data_array *actor_data;  // 0x00880360
extern data_array *prop_data;   // 0x008802c0

// blam-cc: ECX -> actor_index, stack -> object_index
datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
    int32_t cluster_ref = *(int32_t *)(object + 0x1f8);
    actor *self;
    datum_index cur;

    if (cluster_ref == -1) {
        cluster_ref = *(int32_t *)(object + 500);
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    cur = self->first_prop;

    for (;;) {
        prop *p;
        if (cur == (datum_index)0xffffffff) {
            return (datum_index)0xffffffff;
        }
        p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));

        if (!(((-1 < p->kind) && (p->kind < 2)) ||
              ((p->object_index != object_index) &&
               ((p->has_parent == 0) || (p->owner_actor_index == (datum_index)0xffffffff) ||
                ((int32_t)p->owner_actor_index != cluster_ref))))) {
            return cur;
        }
        cur = p->next_in_actor;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ea80 @ 0x43ea80) ----
uint FUN_0043ea80(uint param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint in_ECX;
  int iVar4;
  uint uVar5;

  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)(iVar4 + 0x1f8);
  if (iVar3 == -1) {
    iVar3 = *(int *)(iVar4 + 500);
  }
  uVar2 = *(uint *)((in_ECX & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  do {
    uVar5 = uVar2;
    if (uVar5 == 0xffffffff) {
      return 0xffffffff;
    }
    iVar4 = (uVar5 & 0xffff) * 0x138;
    sVar1 = *(short *)(iVar4 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
    iVar4 = iVar4 + *(int *)(DAT_008802c0 + 0x34);
    uVar2 = *(uint *)(iVar4 + 8);
  } while (((-1 < sVar1) && (sVar1 < 2)) ||
          ((*(uint *)(iVar4 + 0x18) != param_1 &&
           (((*(char *)(iVar4 + 0x14) == '\0' || (*(int *)(iVar4 + 0x1c) == -1)) ||
            (*(int *)(iVar4 + 0x1c) != iVar3))))));
  return uVar5;
}
#endif

// actor_allocate_paired_prop_with_kind  (Ghidra: actor_allocate_paired_prop_with_kind, renamed)
// address 0x43e980, size 146 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/ai.h prop.pair_index(+0x0c)/kind(+0x24). phase-4 summary "allocates and
// links a new firing-position node as the counterpart of an existing one, propagating a
// bounded type value from a third reference node." Same structure as
// actor_allocate_paired_prop.c (0x43e910), plus the kind propagation.
// register convention: stack -> object_index, existing_prop, reference_prop.
//   // blam-cc: stack -> object_index, existing_prop, reference_prop

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0
extern datum_index datum_new(void); // 0x4d0480
extern void actor_init_prop_from_object(datum_index new_prop); // 0x43e640, see actor_allocate_paired_prop.c
extern void actor_copy_prop_and_reset(void); // 0x43e840, see actor_allocate_paired_prop.c

// blam-cc: stack -> object_index, existing_prop, reference_prop
datum_index actor_allocate_paired_prop_with_kind(datum_index object_index, datum_index existing_prop,
                                                 datum_index reference_prop)
{
    datum_index new_prop = datum_new();
    (void)object_index;

    actor_init_prop_from_object(new_prop);

    if (new_prop == (datum_index)0xffffffff) {
        return (datum_index)0xffffffff;
    }

    {
        prop *existing = (prop *)((uint8_t *)prop_data->data + (existing_prop & 0xffff) * sizeof(prop));
        prop *created = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));
        prop *reference = (prop *)((uint8_t *)prop_data->data + (reference_prop & 0xffff) * sizeof(prop));

        actor_copy_prop_and_reset();

        existing->pair_index = new_prop;
        created->pair_index = existing_prop;

        if ((3 < reference->kind) && (reference->kind < 6)) {
            created->kind = reference->kind;
        }
    }
    return new_prop;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e980 @ 0x43e980) ----
uint FUN_0043e980(undefined4 param_1,uint param_2,uint param_3)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  int iVar4;

  iVar2 = DAT_008802c0;
  uVar3 = datum_new();
  FUN_0043e640(uVar3);
  if (uVar3 != 0xffffffff) {
    iVar2 = *(int *)(iVar2 + 0x34);
    iVar4 = (uVar3 & 0xffff) * 0x138 + iVar2;
    FUN_0043e840();
    *(uint *)((param_2 & 0xffff) * 0x138 + iVar2 + 0xc) = uVar3;
    *(uint *)(iVar4 + 0xc) = param_2;
    sVar1 = *(short *)((param_3 & 0xffff) * 0x138 + iVar2 + 0x24);
    if ((3 < sVar1) && (sVar1 < 6)) {
      *(short *)(iVar4 + 0x24) = sVar1;
    }
    return uVar3;
  }
  return 0xffffffff;
}
#endif

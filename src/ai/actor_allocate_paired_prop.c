// actor_allocate_paired_prop  (Ghidra: actor_allocate_paired_prop, renamed)
// address 0x43e910, size 101 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/ai.h prop.pair_index(+0x0c, "the paired prop allocated by 0x43e910 /
// 0x43e980"). phase-4 summary "allocates a new firing-position node and links it as the
// paired counterpart of an existing node." Calls datum_new (established), and
// actor_init_prop_from_object / actor_copy_prop_and_reset (0x43e640/0x43e840, this rewrite),
// both called here with fewer visible arguments than their own files show -- see those
// files' own headers for the same situation.
// register convention: stack -> object_index, existing_prop.
//   // blam-cc: stack -> object_index, existing_prop

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0
extern datum_index datum_new(void); // 0x4d0480
extern void actor_init_prop_from_object(datum_index new_prop); // 0x43e640, see header UNSURE on the arity mismatch
extern void actor_copy_prop_and_reset(void); // 0x43e840, see header UNSURE (called here with no visible arguments)

// blam-cc: stack -> object_index, existing_prop
datum_index actor_allocate_paired_prop(datum_index object_index, datum_index existing_prop)
{
    datum_index new_prop = datum_new();
    (void)object_index;

    actor_init_prop_from_object(new_prop);

    if (new_prop != (datum_index)0xffffffff) {
        prop *existing = (prop *)((uint8_t *)prop_data->data + (existing_prop & 0xffff) * sizeof(prop));
        prop *created;

        actor_copy_prop_and_reset();

        created = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));
        existing->pair_index = new_prop;
        created->pair_index = existing_prop;
    }
    return new_prop;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e910 @ 0x43e910) ----
uint FUN_0043e910(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint uVar2;

  iVar1 = DAT_008802c0;
  uVar2 = datum_new();
  FUN_0043e640(uVar2);
  if (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(iVar1 + 0x34);
    FUN_0043e840();
    *(uint *)((param_2 & 0xffff) * 0x138 + iVar1 + 0xc) = uVar2;
    *(uint *)((uVar2 & 0xffff) * 0x138 + iVar1 + 0xc) = param_2;
  }
  return uVar2;
}
#endif

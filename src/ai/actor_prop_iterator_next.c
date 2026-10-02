// actor_prop_iterator_next  (Ghidra: actor_prop_iterator_next, renamed)
// address 0x43ecf0, size 41 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/ai.h prop.next_in_actor(+0x08). phase-4 summary "advances a {current,next}
// iterator over the firing-position node linked list, returning the current node's record
// offset." Paired with actor_prop_iterator_init.c (0x43ecd0), which establishes the
// iterator's shape.
// register convention: EDX -> iterator.
//   // blam-cc: EDX -> iterator

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *prop_data; // 0x008802c0

// TYPES-GAP: duplicated from actor_prop_iterator_init.c (each rewritten file is compiled
// independently).
// blam-cc: EDX -> iterator
prop *actor_prop_iterator_next(actor_prop_iterator *iterator)
{
    datum_index cur = iterator->next;
    prop *result = 0;

    iterator->current = cur;
    if (cur != (datum_index)0xffffffff) {
        result = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
        iterator->next = result->next_in_actor;
    }
    return result;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ecf0 @ 0x43ecf0) ----
int FUN_0043ecf0(void)

{
  uint uVar1;
  int iVar2;
  uint *in_EDX;

  uVar1 = in_EDX[1];
  iVar2 = 0;
  *in_EDX = uVar1;
  if (uVar1 != 0xffffffff) {
    iVar2 = (uVar1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    in_EDX[1] = *(uint *)(iVar2 + 8);
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

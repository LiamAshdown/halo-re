// ai_unit_clear_actor_vocalization  (Ghidra: ai_unit_clear_actor_vocalization; named for this rewrite)
// address 0x435a50, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: for a unit's controlling actor (unit_data.actor_index, object+0x1f4), clears the
// three vocalization fields at actor+0x544/0x546/0x548 (types/ai.h: "actor_clear_
// vocalization @0x414560" -- the same fields that dedicated function, outside this rewrite's
// range, already clears).
//   // blam-cc: EAX -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern data_array *actor_data;  // 0x00880360

void ai_unit_clear_actor_vocalization(datum_index unit_index)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[unit->actor_index & 0xffff];
        ((actor *)a)->vocalization_variant = 0;
        ((actor *)a)->vocalization_line = 0;
        ((actor *)a)->vocalization_state = 0;
    }
}

#if 0
Original Ghidra decompilation (0x435a50):

void FUN_00435a50(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;

  if ((in_EAX != 0xffffffff) &&
     (uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 500)
     , uVar1 != 0xffffffff)) {
    iVar2 = (uVar1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    *(undefined2 *)(iVar2 + 0x546) = 0;
    *(undefined2 *)(iVar2 + 0x544) = 0;
    *(undefined2 *)(iVar2 + 0x548) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

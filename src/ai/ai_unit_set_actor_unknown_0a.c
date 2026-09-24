// ai_unit_set_actor_unknown_0a  (Ghidra: ai_unit_set_actor_unknown_0a; named for this rewrite)
// address 0x435540, size 75 bytes
// name confidence: 0.25   rewrite confidence: 0.4
// evidence: for a unit's controlling actor (unit_data.actor_index, object+0x1f4), if
// actor.unknown_09 (types/ai.h: "actor_new sets 0") is set, writes a caller-supplied byte to
// actor.unknown_0a[0]. Matches the phase-4 summary ("only while it currently has an active
// target" is plausible for unknown_09 but not confirmed).
// register convention: Ghidra fully resolved the byte parameter and left the unit index in
// EAX.
//   // blam-cc: EAX -> unit_index, stack -> value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0
extern data_array *actor_data;  // 0x00880360

void ai_unit_set_actor_unknown_0a(datum_index unit_index, uint8_t value)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        if (a->unknown_09 != 0) {
            a->unknown_0a = value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x435540):

void FUN_00435540(undefined1 param_1)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;

  if (((in_EAX != 0xffffffff) &&
      (uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 500
                        ), uVar1 != 0xffffffff)) &&
     (iVar2 = (uVar1 & 0xffff) * 0x724, *(char *)(iVar2 + 9 + *(int *)(DAT_00880360 + 0x34)) != '\0'
     )) {
    *(undefined1 *)(iVar2 + *(int *)(DAT_00880360 + 0x34) + 10) = param_1;
  }
  return;
}
#endif

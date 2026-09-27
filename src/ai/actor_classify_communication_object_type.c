// actor_classify_communication_object_type  (Ghidra: actor_classify_communication_object_type; named for this rewrite)
// address 0x42f9a0, size 58 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: phase-4 summary ("classifies an object's type for communication purposes into
// one of three categories based on its type-definition flag bits"); types/ai.h's
// actor_type_procs[16] table at 0x006853b8, indexed by actor.type.
// register convention: EAX -> actor_index (in_EAX, the only register Ghidra's own decompile
// shows).
// blam-cc: EAX -> actor_index
//
// UNSURE: actor_type_procs[type]+4 is read here as a uint16 flags field; types/ai.h's own
// actor_type_table_entry only names offsets 0x06/0x08/0x0a/0x0c/0x10/0x14/0x18 for this
// table, not +4, so this may index a different (untyped) structure the table's pointers
// point at.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;         // 0x00880360
extern void *actor_type_procs[16];  // 0x006853b8

// blam-cc: EAX -> actor_index
// Classifies actor_index's ActorType-derived flags into a three-way communication category:
// -1 for the default/other case, 0 when flag bit 1 is set, 1 when bit 2 is set (bit 1 takes
// priority over bit 2).
int32_t actor_classify_communication_object_type(datum_index actor_index)
{
    actor *a;
    uint16_t flags;
    int32_t result;

    a = &((actor *)actor_data->data)[actor_index & 0xffff];
    flags = *(uint16_t *)((uint8_t *)actor_type_procs[a->type] + 4);

    result = -1;
    if ((flags & 2) != 0) {
        return 0;
    }
    if ((flags & 4) != 0) {
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x42f9a0):

undefined4 FUN_0042f9a0(void)

{
  uint in_EAX;
  undefined4 uVar1;

  uVar1 = 0xffffffff;
  if ((*(ushort *)
        ((&PTR_PTR_006853b8)
         [*(short *)((in_EAX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4) & 2) != 0)
  {
    return 0;
  }
  if ((*(ushort *)
        ((&PTR_PTR_006853b8)
         [*(short *)((in_EAX & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4) & 4) != 0)
  {
    uVar1 = 1;
  }
  return uVar1;
}
#endif

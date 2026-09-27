// ai_unit_dispatch_actor_event_d  (Ghidra: ai_unit_dispatch_actor_event_d; named for this rewrite)
// address 0x435a00, size 76 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: for a unit's controlling actor (unit_data.actor_index, object+0x1f4), dispatches
// a fixed event (type 0xd, parameter 6) via actor_begin_vocalization (outside this rewrite's range).
// Matches the phase-4 summary.
// register convention: Ghidra resolved neither parameter.
//   // blam-cc: EAX -> unit_index, ECX -> unused (checked against -1 but never otherwise read)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0

extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant, void *context); // 0x4142d0, EAX, stack

// blam-cc: EAX -> unit_index, ECX -> unused
void ai_unit_dispatch_actor_event_d(datum_index unit_index, int32_t unused)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

    if (unused != -1 && unit->actor_index != (datum_index)k_datum_index_none) {
        // FIXED (objdump 0x435a2d..0x435a40): EAX = the unit's actor, stack = (0xd, 1, &payload), where the payload is
        //   {word 6, dword ECX}. The draft passed no actor and dropped ECX.
        int16_t payload[8] = {0};
        payload[0] = 6;
        *(int32_t *)&payload[2] = unused;
        actor_begin_vocalization(unit->actor_index, 0xd, 1, payload);
    }
}

#if 0
Original Ghidra decompilation (0x435a00):

void FUN_00435a00(void)

{
  uint in_EAX;
  int in_ECX;
  undefined2 local_10 [8];

  if (((in_EAX != 0xffffffff) && (in_ECX != -1)) &&
     (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 500) != -1))
  {
    local_10[0] = 6;
    FUN_004142d0(0xd,1,local_10);
  }
  return;
}
#endif

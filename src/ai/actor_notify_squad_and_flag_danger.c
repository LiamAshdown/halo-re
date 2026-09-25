// actor_notify_squad_and_flag_danger  (Ghidra: actor_notify_squad_and_flag_danger, renamed)
// address 0x423600, size 99 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/ai.h actor.unit_index (0x18), actor.unknown_308/unknown_30c (a danger-slot
//   pair also written by actor_scan_backup_and_panic_reaction/0x4233d0/0x4234f0/0x423220 in this same file group, all
//   using the same {code, payload} shape at 0x308/0x30c). ai_communication_broadcast already
//   established elsewhere in this module.
// register convention: EAX -> actor_index, CL -> alternate_event, stack -> raise_danger_flag.
//   // blam-cc: EAX -> actor_index, ECX -> alternate_event, stack -> raise_danger_flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

// blam-cc: EAX -> actor_index, ECX -> alternate_event, stack -> raise_danger_flag
// If the actor controls a unit, broadcasts a "retreat/regroup" squad event (0x17, or 0x16
// when alternate_event is set). If raise_danger_flag is set and no higher-priority danger
// slot (unknown_308) is already claimed, claims danger code 6 with no payload object.
void actor_notify_squad_and_flag_danger(datum_index actor_index, uint8_t alternate_event, uint8_t raise_danger_flag)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->unit_index != (datum_index)k_datum_index_none) {
        ai_communication_broadcast(0x17 - (alternate_event != 0), self->unit_index,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    }

    if (raise_danger_flag != 0 && self->unknown_308 < 6) {
        self->unknown_308 = 6;
        self->unknown_30c = 0xffffffff;
    }
}

#if 0
Original Ghidra decompilation (0x423600):

void FUN_00423600(char param_1)

{
  uint in_EAX;
  int iVar1;
  char in_CL;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(int *)(iVar1 + 0x18) != -1) {
    ai_communication_broadcast
              (0x17 - (uint)(in_CL != '\0'),*(int *)(iVar1 + 0x18),0xffffffff,0xffffffff,0xffffffff,
               0xffffffff,0);
  }
  if ((param_1 != '\0') && (*(short *)(iVar1 + 0x308) < 6)) {
    *(undefined2 *)(iVar1 + 0x308) = 6;
    *(undefined4 *)(iVar1 + 0x30c) = 0xffffffff;
  }
  return;
}
#endif

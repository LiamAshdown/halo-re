// actor_notify_target_engaged  (Ghidra: actor_notify_target_engaged, renamed)
// address 0x4220c0, size 107 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: types/ai.h prop.is_vault (0x127), prop.is_unit (0x60), prop.object_index (0x18);
//   actor.unit_index (0x18); phase-4 summary "notifies the actor's squad-event system that a
//   target unit has become engaged, choosing between two event codes based on a flag".
//   ai_communication_broadcast's signature is already established in this module
//   (src/ai/actor_report_command_status.c).
// register convention: EAX -> target_prop_index, ECX -> actor_index, DL -> alternate_event.
//   // blam-cc: EAX -> target_prop_index, ECX -> actor_index, EDX -> alternate_event

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data;  // 0x008802c0
extern data_array *actor_data; // 0x00880360

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module).

// blam-cc: EAX -> target_prop_index, ECX -> actor_index, EDX -> alternate_event
// If the target prop is not a vault, the notifying actor still controls a unit, and the
// target prop is itself a unit, broadcasts an "engaged" chatter event (5, or 4 when
// alternate_event is set) naming the actor's unit and the target's object.
void actor_notify_target_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t alternate_event)
{
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    actor *notifier = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index unit_index;

    if (target->is_vault == 0) {
        unit_index = notifier->unit_index;
        if (unit_index != (datum_index)k_datum_index_none && target->is_unit != 0) {
            ai_communication_broadcast(5 - (alternate_event != 0), unit_index, target->object_index, 3,
                                       (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4220c0):

void FUN_004220c0(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  uint in_ECX;
  char in_DL;

  iVar1 = (in_EAX & 0xffff) * 0x138;
  iVar2 = iVar1 + *(int *)(DAT_008802c0 + 0x34);
  if (((*(char *)(iVar1 + 0x127 + *(int *)(DAT_008802c0 + 0x34)) == '\0') &&
      (iVar1 = *(int *)((in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x18),
      iVar1 != -1)) && (*(char *)(iVar2 + 0x60) != '\0')) {
    ai_communication_broadcast
              (5 - (uint)(in_DL != '\0'),iVar1,*(undefined4 *)(iVar2 + 0x18),3,0xffffffff,0xffffffff
               ,0);
  }
  return;
}
#endif

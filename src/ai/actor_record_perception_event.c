// actor_record_perception_event  (Ghidra: actor_record_perception_event, renamed)
// address 0x422070, size 72 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: types/ai.h actor.perception_event (0x34a) / perception_event_data (0x34c), read
//   back by actor_update_awareness_level @0x420290 (already rewritten), which consumes and
//   zeroes perception_event once its priority is positive. This function is the writer side:
//   it keeps the highest-priority event seen this tick, and on a tie keeps the larger of the
//   two data values (mirrors the tie-break actor_update_awareness_level itself does for its
//   own running maximum).
// register convention: EAX -> actor_index, EDX -> event (priority), ESI -> data.
//   // blam-cc: EAX -> actor_index, EDX -> event, ESI -> data

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, EDX -> event, ESI -> data
// Records a pending perception event for the actor to be picked up by
// actor_update_awareness_level: a higher-priority event replaces the current one outright,
// an equal-priority event keeps the larger of the two data values, and a lower-priority
// event is dropped.
void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->perception_event < event) {
        self->perception_event = event;
        self->perception_event_data = data;
    } else if (self->perception_event == event) {
        if (self->perception_event_data <= data) {
            self->perception_event_data = data;
        }
    }
}

#if 0
Original Ghidra decompilation (0x422070):

void FUN_00422070(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  short in_DX;
  int unaff_ESI;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  sVar1 = *(short *)(iVar2 + 0x34a + *(int *)(DAT_00880360 + 0x34));
  iVar2 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if (sVar1 < in_DX) {
    *(short *)(iVar2 + 0x34a) = in_DX;
    *(int *)(iVar2 + 0x34c) = unaff_ESI;
    return;
  }
  if (sVar1 == in_DX) {
    iVar3 = *(int *)(iVar2 + 0x34c);
    if (*(int *)(iVar2 + 0x34c) <= unaff_ESI) {
      iVar3 = unaff_ESI;
    }
    *(int *)(iVar2 + 0x34c) = iVar3;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

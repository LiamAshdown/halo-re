// ai_reference_reset_or_wake_awareness  (Ghidra: ai_reference_reset_or_wake_awareness; named for this rewrite)
// address 0x434500, size 139 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: for every actor a packed ai reference names: when waking (flag == 0), raises
// awareness_level to 2 only if it is currently 0; when resetting (flag != 0), clears
// awareness_level and mode to 0 and calls actor_clear_perceived_props / actor_dispatch_perception_reset (outside this
// rewrite's range) plus actor_set_units_active (already established, called here with the
// single-argument convention already used elsewhere in this module).
//   // blam-cc: EAX -> packed_reference, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern void actor_clear_perceived_props(datum_index actor_index); // 0x427e00, outside this rewrite's range, UNSURE signature
extern void actor_dispatch_perception_reset(void); // 0x429000, outside this rewrite's range, UNSURE signature
extern void actor_set_units_active(datum_index actor_index, uint8_t activate); // 0x427860, blam-cc: EAX, BL

void ai_reference_reset_or_wake_awareness(uint32_t packed_reference, char flag)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (flag == 0) {
            if (a->awareness_level == 0) {
                a->awareness_level = 2;
            }
        } else {
            a->awareness_level = 0;
            a->mode = 0;
            actor_clear_perceived_props(iterator.actor_index);
            actor_dispatch_perception_reset();
            actor_set_units_active(iterator.actor_index, 0);
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434500):

void FUN_00434500(char param_1)

{
  int in_EAX;
  int iVar1;
  uint local_8;

  if (in_EAX != -1) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      iVar1 = (local_8 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      if (param_1 == '\0') {
        if (*(short *)(iVar1 + 0x6a) == 0) {
          *(undefined2 *)(iVar1 + 0x6a) = 2;
        }
      }
      else {
        *(undefined2 *)(iVar1 + 0x6c) = 0;
        *(undefined2 *)(iVar1 + 0x6a) = 0;
        FUN_00427e00(local_8);
        FUN_00429000();
        actor_set_units_active();
      }
      iVar1 = FUN_004326d0();
    }
  }
  return;
}
#endif

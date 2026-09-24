// ai_count_actors_in_mode9_group  (Ghidra: ai_count_actors_in_mode9_group; named for this rewrite)
// address 0x433e20, size 124 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: walks every actor (actor_iterator_next, 0x436a70, this batch) and counts those
// in mode 9 (types/ai.h _actor_mode_vocalize) whose mode_data's first dword equals the
// caller-supplied value. Ghidra's own decompile calls actor_iterator_next with no visible
// actor_iterator_new first, meaning this function (like several others in this cluster)
// actually receives a live actor_iterator_state pointer from its caller rather than
// building its own; this rewrite builds a fresh one instead (not independently confirmed).
// register convention: Ghidra fully resolved the parameter.
//   // blam-cc: stack -> group_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only); // 0x436a30, this batch
extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70, this batch

int16_t ai_count_actors_in_mode9_group(int32_t group_id)
{
    int16_t count = 0;
    actor_iterator_state iterator;
    actor *a;

    actor_iterator_new(&iterator, 0);
    a = actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->mode == _actor_mode_vocalize && *(int32_t *)a->mode_data == group_id) {
            count = count + 1;
        }
        a = actor_iterator_next(&iterator);
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x433e20):

short FUN_00433e20(int param_1)

{
  int iVar1;
  short sVar2;

  sVar2 = 0;
  iVar1 = actor_iterator_next();
  while (iVar1 != 0) {
    if ((*(short *)(iVar1 + 0x6c) == 9) && (*(int *)(iVar1 + 0x9c) == param_1)) {
      sVar2 = sVar2 + 1;
    }
    iVar1 = actor_iterator_next();
  }
  return sVar2;
}
#endif

// actor_update_idle_stagger  (Ghidra: actor_update_idle_stagger, renamed)
// address 0x429430, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump (branch-for-branch; offsets 0x6a/0x6c/0x72/0x74/0x78/0x268/0x34a, ai_globals +3/+4/+6).)
// evidence: types/ai.h actor.idle_counter(0x4a)/needs_new_path(0x4c)/mode(0x6c); ai_globals.
//   stagger_claimed(0x03)/stagger_threshold(0x04)/stagger_highest(0x06). Offset 0xa0 falls
//   inside actor.mode_data.raw (mode_data[4], meaningful only in vehicle mode).
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;     // 0x00880360
extern ai_globals *ai_globals_ptr; // 0x00880354

// blam-cc: EAX -> actor_index
// Advances the actor's idle/boredom counter (faster -- +3 instead of +1 -- while in a
// boarding-ish vehicle sub-state), and, once it exceeds a global per-tick cap (and is itself
// over 15), claims the shared per-tick "stagger" slot and resets, raising needs_new_path;
// otherwise just tracks the highest idle counter observed this cycle.
void actor_update_idle_stagger(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t vehicle_substate = *(int16_t *)&self->mode_data.raw[4]; // UNSURE offset (mode_data union)
    int fast = self->mode == _actor_mode_vehicle &&
               (vehicle_substate == 2 || vehicle_substate == 3 || vehicle_substate == 4 || vehicle_substate == 5);

    self->idle_counter = self->idle_counter + (fast ? 3 : 1);

    if (ai_globals_ptr->stagger_claimed == 0 && ai_globals_ptr->stagger_threshold < self->idle_counter &&
        self->idle_counter > 15) {
        self->idle_counter = 0;
        ai_globals_ptr->stagger_claimed = 1;
        self->needs_new_path = 1;
        return;
    }

    if (ai_globals_ptr->stagger_highest < self->idle_counter) {
        ai_globals_ptr->stagger_highest = self->idle_counter;
    }
    self->needs_new_path = 0;
}

#if 0
Original Ghidra decompilation (0x429430):

void FUN_00429430(void)

{
  short sVar1;
  byte bVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;

  iVar3 = DAT_00880354;
  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  bVar2 = 0;
  if ((*(short *)(iVar4 + 0x6c) == 10) &&
     ((((sVar1 = *(short *)(iVar4 + 0xa0), sVar1 == 2 || (sVar1 == 3)) || (sVar1 == 4)) ||
      (sVar1 == 5)))) {
    bVar2 = 1;
  }
  *(short *)(iVar4 + 0x4a) = *(short *)(iVar4 + 0x4a) + (ushort)bVar2 * 2 + 1;
  sVar1 = *(short *)(iVar4 + 0x4a);
  if (((*(char *)(iVar3 + 3) == '\0') && (*(short *)(iVar3 + 4) < sVar1)) && (0xf < sVar1)) {
    *(undefined2 *)(iVar4 + 0x4a) = 0;
    *(undefined1 *)(iVar3 + 3) = 1;
    *(undefined1 *)(iVar4 + 0x4c) = 1;
    return;
  }
  if (*(short *)(iVar3 + 6) < sVar1) {
    *(short *)(iVar3 + 6) = sVar1;
  }
  *(undefined1 *)(iVar4 + 0x4c) = 0;
  return;
}
#endif

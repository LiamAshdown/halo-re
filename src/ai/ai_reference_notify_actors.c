// ai_reference_notify_actors  (Ghidra: ai_reference_notify_actors; named for this rewrite)
// address 0x432bd0, size 66 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: walks every actor named by a packed ai reference (via
// ai_reference_actor_iterator_new/_next, 0x432650/0x4326d0, this batch) and calls
// actor_mark_units_and_release(0, actor.actor_index) for each, with AL carrying a byte this function itself
// never sets -- confirmed by objdump (bin/halo.exe 0x432bd0..0x432c11: `mov al,bl` with no
// prior write to bl in this function) to be a genuine register parameter (BL) this function
// receives and forwards, not the "uninitialized" `local_8` Ghidra's own decompile shows
// (Ghidra lost the stack-argument setup for the unrecognized callee entirely).
// register convention: confirmed by objdump: EAX -> packed_reference, BL -> flag (forwarded
// to actor_mark_units_and_release's AL).
//   // blam-cc: EAX -> packed_reference, BL -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern void actor_mark_units_and_release(int32_t unused, datum_index actor_index, uint8_t flag); // 0x4289c0, outside this
                                                                                  // rewrite's range; blam-cc:
                                                                                  // AL -> flag, stack -> unused, actor_index

// blam-cc: EAX -> packed_reference, BL -> flag
// Calls actor_mark_units_and_release(0, actor_index, flag) for every actor named by a packed ai reference.
void ai_reference_notify_actors(uint32_t packed_reference, uint8_t flag)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            actor_mark_units_and_release(0, iterator.actor_index, flag);
            a = ai_reference_actor_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432bd0):

void FUN_00432bd0(void)

{
  int in_EAX;
  int iVar1;
  undefined4 local_8;

  if (in_EAX != -1) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      FUN_004289c0(local_8,0);
      iVar1 = FUN_004326d0();
    }
  }
  return;
}

Real disassembly (0x432bd0-0x432c11), used to recover the BL register parameter and the
true call arguments Ghidra's decompile lost:

00432bf1: mov    eax,[esp+0x10]     ; iterator.actor_index
00432bf5: push   0x0
00432bf7: push   eax
00432bf8: mov    al,bl              ; flag, never written by this function itself
00432bfa: call   0x4289c0
#endif

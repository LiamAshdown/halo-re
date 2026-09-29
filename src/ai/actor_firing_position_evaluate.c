// actor_firing_position_evaluate  (Ghidra: actor_firing_position_evaluate, renamed)
// address 0x412820, size 82 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: resets the four mutable fields of one actor_firing_position_candidate, runs the
//   score table, optionally reports the movement request, snapshots the score into
//   score_before_rejects and then runs the rejection table, storing its verdict back into
//   valid. actor_select_firing_position @0x413e50 calls it for the single fallback
//   candidate it builds by hand.
// register convention: the candidate is in EAX, the query in ECX, and actor_index in ESI; both
//   callees take their arguments in registers as well.
// FIXED (register inputs, objdump): ESI carries actor_index (pushed at 0x41282a, alongside the
// count=1 stack argument to actor_firing_position_run_score_rules at 0x412838; reused unchanged
// at 0x412853 `mov eax,esi` for actor_report_firing_position_request and at 0x41285d
// `push esi` for actor_firing_position_run_reject_rules); it was previously modeled as an
// ordinary trailing (stack) parameter instead of a register.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"


// blam-cc: EAX -> candidate, ECX -> query, ESI -> actor_index
// Scores and vets a single candidate end to end. Returns whether it survived.
uint8_t actor_firing_position_evaluate(actor_firing_position_candidate *candidate,
                                       actor_firing_position_query *query,
                                       datum_index actor_index)
{
    candidate->score = 0.0f;
    candidate->score_before_rejects = 0.0f;
    candidate->valid = 1;
    candidate->rejected = 0;

    actor_firing_position_run_score_rules(actor_index, 1, query, candidate);

    if (candidate->valid != 0) {
        if (query->have_target != 0) {
            actor_report_firing_position_request(actor_index, query, candidate);
        }
        candidate->score_before_rejects = candidate->score;
        candidate->valid = actor_firing_position_run_reject_rules(actor_index, query, candidate);
    }

    return candidate->valid;
}

#if 0
Original Ghidra decompilation (0x412820):

undefined4 FUN_00412820(void)

{
  undefined1 extraout_AL;
  int in_EAX;
  undefined4 uVar1;
  undefined3 uVar2;
  undefined3 extraout_var;
  int in_ECX;

  *(undefined4 *)(in_EAX + 0x38) = 0;
  *(undefined4 *)(in_EAX + 0x34) = 0;
  *(undefined1 *)(in_EAX + 0x30) = 1;
  *(undefined1 *)(in_EAX + 0x31) = 0;
  uVar1 = FUN_004126f0();
  uVar2 = (undefined3)((uint)uVar1 >> 8);
  if (*(char *)(in_EAX + 0x30) != '\0') {
    if (*(char *)(in_ECX + 0x5fc) != '\0') {
      FUN_004120f0();
    }
    *(undefined4 *)(in_EAX + 0x34) = *(undefined4 *)(in_EAX + 0x38);
    FUN_00412730();
    *(undefined1 *)(in_EAX + 0x30) = extraout_AL;
    uVar2 = extraout_var;
  }
  return CONCAT31(uVar2,*(undefined1 *)(in_EAX + 0x30));
}
#endif

// actor_firing_position_probe_reject_rules  (Ghidra: actor_firing_position_probe_reject_rules, renamed)
// address 0x412770, size 62 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: byte for byte the same table walk as
//   actor_firing_position_run_reject_rules @0x412730, except that it first zeroes
//   query.baseline_penalty (0x660) and then calls every applicable rule with no candidate
//   at all. Every rule in the table has a "candidate is null" branch that adds its own
//   penalty to baseline_penalty and reports acceptance, so this run measures what an ideal
//   candidate would still be charged. actor_find_best_firing_position @0x412ba0 stores the
//   result in query.baseline_accept (0x65c) and uses baseline_penalty as the early-out
//   margin while scanning the sorted candidates.
// register convention: the query is in EDI; the candidate register is left holding zero,
//   which is what each rule sees.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern actor_firing_position_rule actor_firing_position_reject_rules[6]; // 0x006555f8

// blam-cc: EDI -> query
// Runs the rejection table with no candidate to establish the baseline penalty and whether
// any candidate could be accepted at all.
uint8_t actor_firing_position_probe_reject_rules(actor_firing_position_query *query)
{
    actor_firing_position_rule *row;
    uint8_t (*proc)(datum_index actor_index, actor_firing_position_query *query,
                    actor_firing_position_candidate *candidate);
    uint8_t accepted;

    accepted = 1;
    query->baseline_penalty = 0.0f;

    row = actor_firing_position_reject_rules;
    do {
        if (row->proc == 0) {
            return accepted;
        }
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            // The original calls through with no arguments at all: actor_index is whatever
            // the caller left in place and the candidate register is zero.
            proc = (uint8_t (*)(datum_index, actor_firing_position_query *,
                                actor_firing_position_candidate *))row->proc;
            accepted = proc((datum_index)0, query, (actor_firing_position_candidate *)0);
        }
        row++;
    } while (accepted != 0);

    return accepted;
}

#if 0
Original Ghidra decompilation (0x412770):

void FUN_00412770(void)

{
  char cVar1;
  undefined **ppuVar2;
  int unaff_EDI;

  cVar1 = '\x01';
  *(undefined4 *)(unaff_EDI + 0x660) = 0;
  ppuVar2 = &PTR_FUN_006555fc;
  do {
    if ((code *)*ppuVar2 == (code *)0x0) {
      return;
    }
    if (((int)*(short *)(ppuVar2 + -1) & 1 << (*(byte *)(unaff_EDI + 4) & 0x1f)) != 0) {
      cVar1 = (*(code *)*ppuVar2)();
    }
    ppuVar2 = ppuVar2 + 2;
  } while (cVar1 != '\0');
  return;
}
#endif

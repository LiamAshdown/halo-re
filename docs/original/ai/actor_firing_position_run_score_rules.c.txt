// actor_firing_position_run_score_rules  (Ghidra: actor_firing_position_run_score_rules, renamed)
// address 0x4126f0, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: walks the { int16 kinds, void *proc } table at 0x006555c0 and calls every row
//   whose kinds bitmask contains (1 << query.goal_kind). The table was read straight out of
//   bin/halo.exe and is documented in types/ai.h as actor_firing_position_rule; its six
//   rows are 0x4112b0, 0x411bf0, 0x411ee0, 0x411b60, 0x411980 and 0x411840.
// register convention: the query is in EDI and the candidate array in a second register
//   Ghidra dropped; actor_index and the candidate count are the visible stack arguments.
// FIXED (register inputs, objdump): EBX carries candidates (read at 0x41271a, `push ebx`, the
//   last argument pushed for the per-rule proc call, i.e. the 4th/rightmost C parameter); the
//   body already used the `candidates` parameter correctly, only the "blam-cc" note was
//   missing it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern actor_firing_position_rule actor_firing_position_score_rules[7]; // 0x006555c0

// blam-cc: EDI -> query, EBX -> candidates, stack -> actor_index, count
// Runs the scoring half of the firing position pipeline: every rule that applies to this
// goal kind gets to add its penalty to every candidate score.
void actor_firing_position_run_score_rules(datum_index actor_index, uint16_t count,
                                           actor_firing_position_query *query,
                                           actor_firing_position_candidate *candidates)
{
    actor_firing_position_rule *row;
    void (*proc)(datum_index actor_index, actor_firing_position_query *query, uint16_t count,
                 actor_firing_position_candidate *candidates);

    // The original is a do-while that executes row 0 without testing its procedure and then
    // stops as soon as the next row has a null one; row 0 is never null in practice.
    row = actor_firing_position_score_rules;
    do {
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            proc = (void (*)(datum_index, actor_firing_position_query *, uint16_t,
                             actor_firing_position_candidate *))row->proc;
            proc(actor_index, query, count, candidates);
        }
        row++;
    } while (row->proc != 0);
}

#if 0
Original Ghidra decompilation (0x4126f0):

void FUN_004126f0(undefined4 param_1)

{
  undefined **ppuVar1;
  undefined **ppuVar2;
  int unaff_EDI;

  ppuVar2 = &PTR_FUN_006555c4;
  do {
    if ((1 << (*(byte *)(unaff_EDI + 4) & 0x1f) & (int)*(short *)(ppuVar2 + -1)) != 0) {
      (*(code *)*ppuVar2)(param_1);
    }
    ppuVar1 = ppuVar2 + 2;
    ppuVar2 = ppuVar2 + 2;
  } while (*ppuVar1 != (undefined *)0x0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

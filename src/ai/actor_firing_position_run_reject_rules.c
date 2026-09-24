// actor_firing_position_run_reject_rules  (Ghidra: actor_firing_position_run_reject_rules, renamed)
// address 0x412730, size 62 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: walks the { int16 kinds, void *proc } table at 0x006555f8, the rejection twin
//   of the scoring table at 0x006555c0. Read straight out of bin/halo.exe; its five rows
//   are 0x412290, 0x412620, 0x412570, 0x4124c0 and 0x412350.
// register convention: the query is in EDI and the candidate in a second register Ghidra
//   dropped; actor_index is the visible stack argument. Ghidra types this as void but the
//   two callers both consume the byte left in AL, which is the last rule result.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern actor_firing_position_rule actor_firing_position_reject_rules[6]; // 0x006555f8

// blam-cc: EDI -> query, a second register -> candidate; stack -> actor_index
// Runs the rejection half of the pipeline over one candidate and stops at the first rule
// that returns zero. Returns whether the candidate survived; a table with no applicable
// rows leaves the initial 1.
uint8_t actor_firing_position_run_reject_rules(datum_index actor_index,
                                               actor_firing_position_query *query,
                                               actor_firing_position_candidate *candidate)
{
    actor_firing_position_rule *row;
    uint8_t (*proc)(datum_index actor_index, actor_firing_position_query *query,
                    actor_firing_position_candidate *candidate);
    uint8_t accepted;

    accepted = 1;
    row = actor_firing_position_reject_rules;
    do {
        if (row->proc == 0) {
            return accepted;
        }
        if ((((int32_t)row->kinds) & (1 << (((uint8_t)query->goal_kind) & 0x1f))) != 0) {
            proc = (uint8_t (*)(datum_index, actor_firing_position_query *,
                                actor_firing_position_candidate *))row->proc;
            accepted = proc(actor_index, query, candidate);
        }
        row++;
    } while (accepted != 0);

    return accepted;
}

#if 0
Original Ghidra decompilation (0x412730):

void FUN_00412730(undefined4 param_1)

{
  char cVar1;
  undefined **ppuVar2;
  int unaff_EDI;

  cVar1 = '\x01';
  ppuVar2 = &PTR_FUN_006555fc;
  do {
    if ((code *)*ppuVar2 == (code *)0x0) {
      return;
    }
    if (((int)*(short *)(ppuVar2 + -1) & 1 << (*(byte *)(unaff_EDI + 4) & 0x1f)) != 0) {
      cVar1 = (*(code *)*ppuVar2)(param_1);
    }
    ppuVar2 = ppuVar2 + 2;
  } while (cVar1 != '\0');
  return;
}
#endif

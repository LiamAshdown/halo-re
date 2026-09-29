// ai_reference_mark_squads_unknown_11  (Ghidra: ai_reference_mark_squads_unknown_11; named for this rewrite)
// address 0x432f10, size 52 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: walks every encounter_squad_state a packed ai reference names (via
// ai_reference_squad_iterator_new/_next, 0x4324f0/0x4325b0, this batch) and sets
// encounter_squad_state.unknown_11 (types/ai.h: "encounter_new zeroes it") to 1.
// register convention: matches every ai_reference_* iterator consumer in this batch.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

// TYPES-GAP: mirrors ai_reference_squad_iterator_new.c's local struct of the same name.


// blam-cc: EAX -> packed_reference
void ai_reference_mark_squads_unknown_11(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            state->timer_started = 1;
            state = ai_reference_squad_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432f10):

void FUN_00432f10(void)

{
  int in_EAX;
  int iVar1;

  if (in_EAX != -1) {
    FUN_004324f0();
    iVar1 = FUN_004325b0();
    while (iVar1 != 0) {
      *(undefined1 *)(iVar1 + 0x11) = 1;
      iVar1 = FUN_004325b0();
    }
  }
  return;
}
#endif

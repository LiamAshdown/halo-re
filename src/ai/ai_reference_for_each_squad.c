// ai_reference_for_each_squad  (Ghidra: ai_reference_for_each_squad; named for this rewrite)
// address 0x432f50, size 60 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: walks every encounter_squad_state a packed ai reference names (via
// ai_reference_squad_iterator_new/_next, 0x4324f0/0x4325b0, this batch) and calls
// encounter_squad_clear_spawn_delay on each -- outside this rewrite's range, so its true argument (presumably
// the encounter_squad_state pointer, passed through the register ai_reference_squad_iterator_next returns it
// in) is not independently confirmed.
// register convention: matches every ai_reference_* iterator consumer in this batch.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// TYPES-GAP: mirrors ai_reference_squad_iterator_new.c's local struct of the same name.
extern void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator); // 0x4324f0, this batch
extern encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator); // 0x4325b0, this batch
extern void encounter_squad_clear_spawn_delay(encounter_squad_state *squad_state); // 0x439270, outside this rewrite's range, UNSURE signature

// blam-cc: EAX -> packed_reference
void ai_reference_for_each_squad(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            encounter_squad_clear_spawn_delay(state);
            state = ai_reference_squad_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x432f50):

void FUN_00432f50(void)

{
  int in_EAX;
  int iVar1;

  if (in_EAX != -1) {
    FUN_004324f0();
    iVar1 = FUN_004325b0();
    while (iVar1 != 0) {
      FUN_00439270();
      iVar1 = FUN_004325b0();
    }
  }
  return;
}
#endif

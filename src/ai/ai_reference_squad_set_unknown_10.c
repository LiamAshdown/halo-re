// ai_reference_squad_set_unknown_10  (Ghidra: ai_reference_squad_set_unknown_10; named for this rewrite)
// address 0x435ab0, size 51 bytes
// name confidence: 0.25   rewrite confidence: 0.4
// evidence: sets encounter_squad_state.unknown_10 (types/ai.h: "ScenarioSquad.flags bit 5")
// to a caller-supplied byte for every squad a packed ai reference names.
// register convention: Ghidra could not resolve the byte parameter at all.
//   // blam-cc: EAX -> packed_reference, BL -> value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator); // 0x4324f0, this batch
extern encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator); // 0x4325b0, this batch

// blam-cc: EAX -> packed_reference, BL -> value
void ai_reference_squad_set_unknown_10(uint32_t packed_reference, uint8_t value)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            state->automatic_migration = value;
            state = ai_reference_squad_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435ab0):

void FUN_00435ab0(void)

{
  int in_EAX;
  int iVar1;
  undefined1 unaff_BL;

  if (in_EAX != -1) {
    FUN_004324f0();
    iVar1 = FUN_004325b0();
    while (iVar1 != 0) {
      *(undefined1 *)(iVar1 + 0x10) = unaff_BL;
      iVar1 = FUN_004325b0();
    }
  }
  return;
}
#endif

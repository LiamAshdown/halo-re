// ai_reference_set_squads_unknown_14  (Ghidra: ai_reference_set_squads_unknown_14; named for this rewrite)
// address 0x435bc0, size 56 bytes
// name confidence: 0.25   rewrite confidence: 0.4
// evidence: sets encounter_squad_state.unknown_14 (types/ai.h: "uint8_t unknown_14[2]") to
// (flag == 0) for every squad a packed ai reference names, matching the phase-4 summary.
//   // blam-cc: EAX -> packed_reference, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_squad_iterator_new(uint32_t packed_reference, ai_reference_squad_iterator *out_iterator); // 0x4324f0, this batch
extern encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator); // 0x4325b0, this batch

void ai_reference_set_squads_unknown_14(uint32_t packed_reference, char flag)
{
    ai_reference_squad_iterator iterator;
    encounter_squad_state *state;
    uint8_t value = (flag == 0);

    ai_reference_squad_iterator_new(packed_reference, &iterator);
    state = ai_reference_squad_iterator_next(&iterator);
    while (state != 0) {
        state->unknown_14 = value;
        state = ai_reference_squad_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x435bc0):

void FUN_00435bc0(char param_1)

{
  int iVar1;

  FUN_004324f0();
  iVar1 = FUN_004325b0();
  if (iVar1 != 0) {
    do {
      *(bool *)(iVar1 + 0x14) = param_1 == '\0';
      iVar1 = FUN_004325b0();
    } while (iVar1 != 0);
  }
  return;
}
#endif

// unit_ai_update_stagger_reset  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_562020" from the object_type_definition vtable evidence)
// VERIFIED against disassembly 0x562020..0x56202c (2026-09-30)
// address 0x562020, size 12 bytes
// name confidence 0.4, rewrite confidence 0.8
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are ... 0x562020
//   (+0x1c reset) ...". objdump confirms a single `mov DWORD PTR [eax],0` (4 bytes), i.e. it
//   zeroes exactly ai_update_stagger's first two int16 fields (threshold, highest) together and
//   leaves `claimed` untouched -- the same "unit_update writes two of its three fields" split
//   types/units.h already documents for this record.
// register convention: no parameters (pure global-state resetter, called once per new game state
//   like every other module's `_reset`).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"

typedef struct ai_update_stagger_state { int16_t threshold; int16_t highest; uint8_t claimed; } ai_update_stagger_state;
extern ai_update_stagger_state *ai_update_stagger; // 0x006ef910

// object_type_definition "unit" row, +0x1c column. Zeroes ai_update_stagger's threshold and
// highest fields (a single dword store in the original); claimed is left as-is.
void unit_ai_update_stagger_reset(void)
{
    ai_update_stagger->threshold = 0;
    ai_update_stagger->highest = 0;
}

#if 0
Original Ghidra decompilation (0x562020):

void missed_562020(void)

{
  *DAT_006ef910 = 0;
  return;
}
#endif

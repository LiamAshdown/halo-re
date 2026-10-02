// unit_animation_set_state  (Ghidra: unit_animation_set_state, already named)
// address 0x569450, size 1 byte, name confidence 0.4, rewrite confidence 0.9
// functions.md: "Effectively a no-op stub (single return instruction) under this name; no
// observable behavior to confirm or refute it."
// UNSURE: 1 byte of code (a bare `ret`), 0 callers found by the analysis pass. Likely either
// dead code left by the linker/optimizer or a tail shared with the following function that
// Ghidra split off; reproduced as a true no-op.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void unit_animation_set_state(void)
{
    return;
}

#if 0
Original Ghidra decompilation (0x569450):

void unit_animation_set_state(void)

{
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

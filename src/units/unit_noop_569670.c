// unit_noop_569670  (Ghidra: FUN_00569670)
// address 0x569670, size 118 bytes, name confidence 0.25, rewrite confidence 0.9
// functions.md: "Currently a no-op; its referenced globals suggest it once performed unit/tag-
// table work that has since been inlined away."
// UNSURE: Ghidra's own decompilation is a bare `return;` despite the function occupying 118
// bytes and having 3 callers; the real body was almost certainly folded into its callers by the
// original compiler (dead-store/whole-function elimination after inlining) and only a stub or
// padding remains at this address. Reproduced as a true no-op; not renamed to a real Blam name
// since there is no observable behavior to name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

void unit_noop_569670(void)
{
    return;
}

#if 0
Original Ghidra decompilation (0x569670):

void FUN_00569670(void)

{
  return;
}
#endif

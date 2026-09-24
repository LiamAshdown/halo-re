// object_damage_effect_dispatch
// address 0x4f0250, size 56 bytes
// name confidence: 0.2 (still named mdpi_encode by the inherited PDB, but
// out/phase4/objects_types_notes.md: "Thin wrapper forwarding to the damage-effect placement
// routine effect_new_on_object_with_node_table; despite its inherited name it performs no encoding")
// rewrite confidence: 0.3
// evidence: none beyond the single forwarded call.
// register convention: none visible; whatever registers effect_new_on_object_with_node_table needs pass through
// unmodeled, exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void effect_new_on_object_with_node_table(); // effects module, 0x450870
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments.

void object_damage_effect_dispatch(void)
{
    effect_new_on_object_with_node_table();
}

#if 0
Original Ghidra decompilation (0x4f0250):

void mdpi_encode(void)

{
  FUN_00450870();
  return;
}
#endif

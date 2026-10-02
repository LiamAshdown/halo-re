// object_damage_effect_dispatch
// address 0x4f0250, size 56 bytes
// rewrite confidence: 1.0 (FRAGMENT: 0x4f0250 is inside damage_effect_new_at_location 0x4f0010..0x4f028f, whose C covers this code; reached only by falling through from 0x4f024e)
// name confidence: 0.2 (still named mdpi_encode by the inherited PDB, but
// out/phase4/objects_types_notes.md: "Thin wrapper forwarding to the damage-effect placement
// routine effect_new_on_object_with_node_table; despite its inherited name it performs no encoding")
// rewrite confidence: 0.3
// evidence: none beyond the single forwarded call.
// register convention: none visible; whatever registers effect_new_on_object_with_node_table needs pass through
// unmodeled, exactly as decompiled.
//   // blam-cc: EAX -> push_value, EDI -> node_object
// FIXED (register inputs, objdump): EAX carries push_value (read at 0x4f0274, `push eax`, one of
// effect_new_on_object_with_node_table's stack arguments) and EDI carries node_object (read at
// 0x4f0275, `mov edx,edi` immediately followed by `mov eax,edi` -- the same incoming value is
// forwarded into both EAX and EDX for the call). Both are genuine live-in registers of this
// function, but they are left unused below: this file's own extern already documents that
// effect_new_on_object_with_node_table's true convention (which stack/register args it takes,
// and in what order) is unresolved across its ~20 call sites in this module, and is deliberately
// declared with an empty parameter list so every site agrees on one declaration rather than each
// guessing a different signature; adding real arguments to just this one call would contradict
// that documented, deliberate simplification.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void effect_new_on_object_with_node_table(); // effects module, 0x450870
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments.

// blam-cc: EAX -> push_value, EDI -> node_object
void object_damage_effect_dispatch(int32_t push_value, int32_t node_object)
{
    (void)push_value; (void)node_object; // see FIXED note above: genuine inputs, not forwardable here
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

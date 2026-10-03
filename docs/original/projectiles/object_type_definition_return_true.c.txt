// object_type_definition_return_true  (Ghidra: missed_572a80, created by hand this pass --
// Ghidra never recovered it as a function; only reachable through object_type_definition rows)
// address 0x00572a80, size 2 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: the projectile row (0x0069b9a0) carries this address at +0x78
//   (out/phase4/projectiles_types_notes.md, "+0x78 0x00572a80 shared `mov al,1; ret`"); reading
//   the equipment row (0x0069b810) and weapon row (0x0069b748) directly out of .data shows the
//   identical address at their own +0x78 too, so this one two-instruction stub is shared as the
//   "always true" filler for a query-style vtable column across at least three object types.
//   types/objects.h object_type_definition's +0x74 comment ("defaults to true when no override
//   exists") describes the same idiom one column over.
// register convention: none. objdump -d -M intel --start-address=0x572a80
//   --stop-address=0x572a83 bin/halo.exe is exactly `mov al,1` / `ret` -- no operand is read, no
//   stack is touched, so it is callable under any convention a caller expects for a
//   zero-argument, AL-returning predicate.
// blam-cc: (none) -> returns AL = 1

#include "tags.h"

// Shared "always true" filler used in several object_type_definition rows for a query-style
// vtable column that this build never actually wires to type-specific behaviour. Touches
// nothing but AL.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t object_type_definition_return_true(void)
{
    return 1;
}

#if 0
Original Ghidra decompilation (0x572a80):

undefined1 missed_572a80(void)

{
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

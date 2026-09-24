// object_type_definition_return_false  (Ghidra: missed_571dd0, created by hand this pass --
// Ghidra never recovered it as a function; only reachable through object_type_definition rows)
// address 0x00571dd0, size 2 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: the projectile row (0x0069b9a0) carries this address at +0x60
//   (out/phase4/projectiles_types_notes.md, "+0x60 0x00571dd0 shared `xor al,al; ret`"); reading
//   the equipment row (0x0069b810) and weapon row (0x0069b748) directly out of .data shows the
//   identical address at their own +0x60 too, so this one two-instruction stub is shared as the
//   "always false" filler for a query-style vtable column across at least three object types.
//   types/objects.h object_type_definition's own +0x60 is documented as "never reached from this
//   module" for objects.c's own broadcast helpers -- true there, but the type rows still
//   populate the slot with this stub for whatever other module does call it.
// register convention: none. objdump -d -M intel --start-address=0x571dd0
//   --stop-address=0x571dd3 bin/halo.exe is exactly `xor al,al` / `ret` -- no operand is read,
//   no stack is touched, so it is callable under any convention a caller expects for a
//   zero-argument, AL-returning predicate.
// blam-cc: (none) -> returns AL = 0

#include "tags.h"

// Shared "always false" filler used in several object_type_definition rows for a query-style
// vtable column that this build never actually wires to type-specific behaviour. Touches
// nothing but AL.
uint8_t object_type_definition_return_false(void)
{
    return 0;
}

#if 0
Original Ghidra decompilation (0x571dd0):

undefined1 missed_571dd0(void)

{
  return 0;
}
#endif

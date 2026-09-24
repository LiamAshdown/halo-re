// object_dispatch_effect_notify
// address 0x4efff0, size 20 bytes
// name confidence: 0.2 (still FUN_004efff0 in Ghidra; out/phase4/objects_types_notes.md calls it
// a "thin wrapper that forwards to the object-changed notifier effect_new_on_object", but effect_new_on_object's
// own body (out of this module's range) is a particle_system_new-based effect spawner, not a
// change notifier, so that inherited description is not trusted here either)
// rewrite confidence: 0.3
// evidence: none beyond the single forwarded call.
// register convention: none visible; whatever registers effect_new_on_object itself needs pass through
// unmodeled, exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void effect_new_on_object(); // effects module, 0x4507a0
    // The convention of this foreign callee is not established: different call sites in this
    // module pass different numbers of visible arguments, and it also takes values in EAX
    // and ECX that the decompiler never models. Declared with an empty parameter list so
    // every site in the module agrees on ONE declaration without fabricating arguments.

void object_dispatch_effect_notify(void)
{
    effect_new_on_object();
}

#if 0
Original Ghidra decompilation (0x4efff0):

void FUN_004efff0(void)

{
  FUN_004507a0();
  return;
}
#endif

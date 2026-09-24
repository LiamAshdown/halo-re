// object_new
// address 0x4f5460, size 67 bytes
// name confidence: 0.35 (still FUN_004f5460 in Ghidra; functions.md: "Thin wrapper entry point
//   that forwards directly to object_new_with_datum_role_control" -- named as the natural public
//   entry point for the internal constructor)
// rewrite confidence: 0.3
// evidence: callee object_new_with_datum_role_control 0x4f54b0.
// register convention: unknown -- Ghidra shows a zero-argument call in both this function's own
//   signature and its call to object_new_with_datum_role_control, which is a 1308-byte
//   constructor that clearly needs a placement_data*/role pair; the real argument registers are
//   not visible anywhere in this function's decompilation.
// UNSURE: this is a pure, unmodified tail-forward as far as Ghidra could see; no arguments are
//   modeled here because none are visible, but real callers must be passing them through
//   registers this decompilation does not show.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0, this module; UNSURE: real
                                                        //   parameters not visible at this call site

void object_new(void)
{
    object_new_with_datum_role_control(0, 0);
}

#if 0
Original Ghidra decompilation (0x4f5460):

void FUN_004f5460(void)

{
  object_new_with_datum_role_control();
  return;
}
#endif

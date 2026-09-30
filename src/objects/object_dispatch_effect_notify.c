// object_dispatch_effect_notify
// address 0x4efff0, size 20 bytes
// name confidence: 0.2 (still FUN_004efff0 in Ghidra; out/phase4/objects_types_notes.md calls it
// a "thin wrapper that forwards to the object-changed notifier effect_new_on_object", but effect_new_on_object's
// own body (out of this module's range) is a particle_system_new-based effect spawner, not a
// change notifier, so that inherited description is not trusted here either)
// rewrite confidence: 0.9 (VERIFIED against objdump 0x4efff0..0x4f0003)
// evidence: none beyond the single forwarded call.
// register convention: EAX -> forwarded_eax, ECX -> forwarded_ecx (both pass straight through to
// effect_new_on_object; see below).
//   // blam-cc: EAX -> forwarded_eax, ECX -> forwarded_ecx
// FIXED (register inputs, objdump): objdump 0x4efff0..0x4f0003 shows this function is not a
// bare forward -- it pushes five literal stack args (0, 0, 0, 0, -1) then EAX (`push eax` at
// 0x4efffa) before the call, and ECX is read live-in at the call itself (0x4efffb), i.e. this
// function's own EAX/ECX are genuine inputs forwarded to effect_new_on_object, not scratch.
// They were previously undeclared entirely. effect_new_on_object's own real prototype is still
// not recovered (see the extern's own note), so forwarded_eax/forwarded_ecx are passed through
// positionally without claiming to know which formal parameters they land on.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_objects.h"

extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six

void object_dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx)
{
    // 0x4efff0..0x4efffb: forwards EAX (the object, also pushed) and ECX (the effect), then -1, 0, 0, 0, 0
    effect_new_on_object(forwarded_eax, forwarded_ecx, forwarded_eax, -1, 0.0f, 0.0f, 0, 0);
}

#if 0
Original Ghidra decompilation (0x4efff0):

void FUN_004efff0(void)

{
  FUN_004507a0();
  return;
}
#endif

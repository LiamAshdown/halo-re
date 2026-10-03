// sound_effect_object_listener_supported  (Ghidra: missed_54ef10; 0 callers in this module --
// reached only through the sound_effect_object_vtable's listener_supported slot 0x0c, shared by
// all three EAX vtables: 0x00671d04 EAX2, 0x00671d28 EAX3, 0x00671d4c EAX1)
// address 0x54ef10, size 3 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: out/phase4/sound_types_notes.md "Slots 3/4 are one-instruction getters of `this+0x10`
//   / `this+0x14` (0x54ef10 / 0x54ef20)"; types/sound.h sound_effect_object_vtable.
//   listener_supported comment ("0x0c 0x54ef10 returns +0x10") and sound_effect_object.
//   listener_supported field (0x10); confirmed at 0x00671d04+0x0c, 0x00671d28+0x0c and
//   0x00671d4c+0x0c (all three vtables' listener_supported slots point here).
// register convention: __thiscall (ECX -> this), no stack parameters.
// blam-cc: ECX -> this
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

// blam-cc: ECX -> this
// Returns whether QuerySupport found the listener properties supported (set by
// sound_eax1_effect_initialize / sound_eax20_effect_initialize / sound_eax30_effect_initialize).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t __thiscall sound_effect_object_listener_supported(sound_effect_object *this_object)
{
    return this_object->listener_supported;
}

#if 0
Original Ghidra decompilation (0x54ef10):

undefined4 missed_54ef10(void)

{
  int in_ECX;

  return *(undefined4 *)(in_ECX + 0x10);
}

Disassembly (0x54ef10..0x54ef13; phase-4 review):

0x54ef10: mov eax, dword ptr [ecx + 0x10]
0x54ef13: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

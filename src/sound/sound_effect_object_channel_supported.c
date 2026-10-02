// sound_effect_object_channel_supported  (Ghidra: missed_54ef20; 0 callers in this module --
// reached only through the sound_effect_object_vtable's channel_supported slot 0x10, shared by
// the EAX2 and EAX3 vtables: 0x00671d04 EAX2, 0x00671d28 EAX3. EAX1 (0x00671d4c) uses its own
// implementation instead (sound_eax1_effect_channel_supported, 0x551250, this batch), since EAX1
// has no per-channel property sets.
// address 0x54ef20, size 4 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: out/phase4/sound_types_notes.md "Slots 3/4 are one-instruction getters of `this+0x10`
//   / `this+0x14` (0x54ef10 / 0x54ef20)"; types/sound.h sound_effect_object_vtable.
//   channel_supported comment ("0x10 0x54ef20 returns +0x14") and sound_effect_object.
//   channel_supported field (0x14); confirmed at 0x00671d04+0x10 and 0x00671d28+0x10 (both
//   EAX2/EAX3 vtables' channel_supported slots point here; 0x00671d4c+0x10, EAX1's, points at
//   0x551250 instead).
// register convention: __thiscall (ECX -> this), no stack parameters.
// blam-cc: ECX -> this
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> this
// Returns whether QuerySupport found the per-channel buffer properties supported (set by
// sound_eax20_effect_initialize / sound_eax30_effect_initialize).
int32_t __thiscall sound_effect_object_channel_supported(sound_effect_object *this_object)
{
    return this_object->channel_supported;
}

#if 0
Original Ghidra decompilation (0x54ef20):

undefined4 missed_54ef20(void)

{
  int in_ECX;

  return *(undefined4 *)(in_ECX + 0x14);
}

Disassembly (0x54ef20..0x54ef23; phase-4 review):

0x54ef20: mov eax, dword ptr [ecx + 0x14]
0x54ef23: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

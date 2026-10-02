// sound_eax1_effect_initialize_channel  (Ghidra: missed_551240; 0 callers in this module --
// reached only through the EAX1 vtable's initialize_channel slot, 0x00671d4c+0x08)
// address 0x551240, size 8 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: confirmed by reading .rdata at 0x00671d4c (EAX1 vtable): +0x08 == 0x00551240.
//   types/sound.h sound_effect_object_vtable.initialize_channel comment lists "0x08 0x54f6e0 /
//   0x551240" -- 0x54f6e0 is EAX2/EAX3's shared per-channel IKsPropertySet QueryInterface
//   (sound_eax_effect_initialize_channel, this batch); EAX1 has no per-channel property array
//   (sound_effect_object is 0x1c bytes with no channel_property_sets, unlike
//   sound_eax_effect_object's 0xe8), so this slot is a no-op that always reports success.
// register convention: __thiscall (ECX -> this, unused), stack -> channel_index (unused).
// blam-cc: ECX -> this, stack -> channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ECX -> this, stack -> channel_index
// EAX1 has no per-channel EAX state to set up; always reports success.
int32_t __thiscall sound_eax1_effect_initialize_channel(sound_effect_object *this_object, int32_t channel_index)
{
    return 1;
}

#if 0
Original Ghidra decompilation (0x551240):

undefined4 missed_551240(void)

{
  return 1;
}

Disassembly (0x551240..0x551245; phase-4 review):

0x551240: mov eax, 1
0x551245: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

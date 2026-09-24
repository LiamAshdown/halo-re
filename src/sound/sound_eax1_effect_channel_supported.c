// sound_eax1_effect_channel_supported  (Ghidra: missed_551250; 0 callers in this module --
// reached only through the EAX1 vtable's channel_supported slot, 0x00671d4c+0x10)
// address 0x551250, size 3 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: confirmed by reading .rdata at 0x00671d4c (EAX1 vtable): +0x10 == 0x00551250 (not
//   the shared getter at 0x54ef20 used by EAX2/EAX3's vtables at the same slot). types/sound.h
//   sound_effect_object_vtable.channel_supported comment; EAX1 has no per-channel properties
//   (sound_effect_object, 0x1c bytes, has no channel_property_sets array), so this always
//   reports "not supported".
// register convention: __thiscall (ECX -> this, unused), no stack parameters.
// blam-cc: ECX -> this
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

// blam-cc: ECX -> this
// EAX1 has no per-channel EAX properties; always reports unsupported.
int32_t __thiscall sound_eax1_effect_channel_supported(sound_effect_object *this_object)
{
    return 0;
}

#if 0
Original Ghidra decompilation (0x551250):

undefined4 missed_551250(void)

{
  return 0;
}

Disassembly (0x551250..0x551252; phase-4 review):

0x551250: xor eax, eax
0x551252: ret
#endif

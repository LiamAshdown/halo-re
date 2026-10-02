// sound_eax1_effect_apply_channel  (Ghidra: missed_551260; 0 callers in this module -- reached
// only through the EAX1 vtable's apply_channel slot, 0x00671d4c+0x14)
// address 0x551260, size 3 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: confirmed by reading .rdata at 0x00671d4c (EAX1 vtable): +0x14 == 0x00551260.
//   types/sound.h sound_effect_object_vtable.apply_channel comment; EAX1 has no per-channel
//   properties to apply (sound_eax1_effect_channel_supported, this batch, always reports
//   unsupported, so callers never reach here with anything to do), matching the empty body.
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
// No-op: EAX1 has no per-channel EAX state to reapply.
void __thiscall sound_eax1_effect_apply_channel(sound_effect_object *this_object, int32_t channel_index)
{
}

#if 0
Original Ghidra decompilation (0x551260):

void missed_551260(void)

{
  return;
}

Disassembly (0x551260..0x551262; phase-4 review):

0x551260: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

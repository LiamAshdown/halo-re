// sound_driver_channel_continue  (Ghidra: missed_5483d0; 0 callers in this module -- reached
// only through the DirectSound driver's channel_continue vtable slot,
// sound_driver.channel_continue 0x1c)
// address 0x5483d0, size 59 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md driver slot table "0x1c 0x5483d0 -> 0x5478c0" and
//   types/sound.h sound_driver.channel_continue comment ("only the channel is used; the byte is
//   pushed to 0x5478c0, which ignores it"); reuses sound_channel_bind_hardware (0x5482e0, this
//   module) and sound_channel_stream_update (0x5478c0, this module, AX -> channel_index,
//   stack -> unused, which it never reads).
// register convention: Ghidra found 2 stack words (channel_index, unused); `sound_class`, the
//   driver's declared 3rd parameter, is never read here (matches types/sound.h's own note on
//   this slot).
// blam-cc: stack -> (channel_index, unused, sound_class)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4

extern void sound_channel_bind_hardware(int16_t logical_channel_index); // 0x5482e0, blam-cc: stack
extern void sound_channel_stream_update(int16_t channel_index, uint8_t unused); // 0x5478c0, blam-cc: AX, stack

// blam-cc: stack -> (channel_index, unused, sound_class)
// Binds a hardware channel to `channel_index` if it does not already have one, then refreshes
// that hardware channel's streaming fill. `sound_class` is never read.
void sound_driver_channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        sound_channel_bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        sound_channel_stream_update(directsound_bindings[channel_index].hardware_channel_index, unused);
    }
}

#if 0
Original Ghidra decompilation (0x5483d0):

void missed_5483d0(undefined4 param_1,undefined4 param_2)

{
  if ((&DAT_007252e4)[(short)param_1 * 2] == -1) {
    sound_channel_bind_hardware(param_1);
  }
  if ((&DAT_007252e4)[(short)param_1 * 2] != -1) {
    sound_channel_stream_update(param_2);
  }
  return;
}

Disassembly (0x5483d0..0x54840a; phase-4 review):

0x5483d0: mov eax, dword ptr [esp + 4]
0x5483d4: push esi
0x5483d5: movsx esi, ax
0x5483d8: cmp word ptr [esi*4 + 0x7252e4], 0xffff
0x5483e1: lea esi, [esi*4 + 0x7252e4]
0x5483e8: jne 0x5483f3
0x5483ea: push eax
0x5483eb: call 0x5482e0
0x5483f0: add esp, 4
0x5483f3: xor eax, eax
0x5483f5: mov ax, word ptr [esi]
0x5483f8: cmp ax, 0xffff
0x5483fc: pop esi
0x5483fd: je 0x54840a
0x5483ff: mov ecx, dword ptr [esp + 8]
0x548403: push ecx
0x548404: call 0x5478c0
0x548409: pop ecx
0x54840a: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

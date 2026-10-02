// sound_driver_channel_stop  (Ghidra: missed_548410; 0 callers in this module -- reached only
// through the DirectSound driver's channel_stop vtable slot, sound_driver.channel_stop 0x20)
// address 0x548410, size 54 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/sound_types_notes.md driver slot table "0x20 0x548410 -> 0x547f60" and
//   types/sound.h sound_driver.channel_stop comment; sound_channel_binding.hardware_channel_index
//   (0x00, types/sound.h) matches by offset; reuses sound_channel_reset (0x547f60, this module,
//   AX -> channel_index, which itself sets directsound_channel.state back to idle -- this
//   function additionally clears the binding).
// register convention: single recognized stack parameter, channel_index.
// blam-cc: stack -> channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern void sound_channel_reset(int16_t channel_index); // 0x547f60, blam-cc: AX

// blam-cc: stack -> channel_index
// If `channel_index` has a bound hardware channel, resets that hardware channel and clears the
// logical channel's state (idle) and its binding (unbound).
void sound_driver_channel_stop(int16_t channel_index)
{
    sound_channel_binding *binding = &directsound_bindings[channel_index];

    if (binding->hardware_channel_index != -1) {
        sound_channel_reset(binding->hardware_channel_index);
        directsound_channels[binding->hardware_channel_index].sound_channel_index = -1;
        binding->hardware_channel_index = -1;
    }
}

#if 0
Original Ghidra decompilation (0x548410):

void missed_548410(short param_1)

{
  short *psVar1;

  psVar1 = &DAT_007252e4 + param_1 * 2;
  if (*psVar1 != -1) {
    sound_channel_reset();
    *(undefined2 *)(&DAT_00725432 + *psVar1 * 0x678) = 0xffff;
    *psVar1 = -1;
  }
  return;
}

Disassembly (0x548410..0x548445; phase-4 review):

0x548410: push esi
0x548411: movsx esi, word ptr [esp + 8]
0x548416: lea esi, [esi*4 + 0x7252e4]
0x54841d: xor eax, eax
0x54841f: mov ax, word ptr [esi]
0x548422: cmp ax, 0xffff
0x548426: je 0x548444
0x548428: call 0x547f60
0x54842d: movsx eax, word ptr [esi]
0x548430: imul eax, eax, 0x678
0x548436: mov word ptr [eax + 0x725432], 0xffff
0x54843f: mov word ptr [esi], 0xffff
0x548444: pop esi
0x548445: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

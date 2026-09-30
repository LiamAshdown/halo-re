// sound_driver_channel_get_state  (Ghidra: missed_548450; 0 callers in this module -- reached
// only through the DirectSound driver's channel_get_state vtable slot,
// sound_driver.channel_get_state 0x24)
// address 0x548450, size 32 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/sound_types_notes.md driver slot table "0x24 0x548450 -> 0x548050" and
//   types/sound.h sound_driver.channel_get_state comment; sound_channel_binding.
//   hardware_channel_index (0x00, types/sound.h) matches by offset; tail-calls
//   sound_channel_check_loop_boundary (0x548050, this module, AX -> channel_index) when a
//   hardware channel is bound.
// register convention: single recognized stack parameter, channel_index; the tail call to
//   0x548050 passes the resolved hardware index in AX and returns its result unchanged.
// blam-cc: stack -> channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. When unbound, only AX is cleared (the upper half of
// EAX is left over from the -1 sign-extension of the unbound lookup) -- harmless, since the
// driver's channel_get_state returns int16_t and only AX is read.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4


// blam-cc: stack -> channel_index
// If `channel_index` has a bound hardware channel, returns its loop-boundary-checked state;
// otherwise returns idle (0).
directsound_channel_state sound_driver_channel_get_state(int16_t channel_index)
{
    int16_t hardware_channel_index = directsound_bindings[channel_index].hardware_channel_index;

    if (hardware_channel_index != -1) {
        return sound_channel_check_loop_boundary(hardware_channel_index);
    }
    return _directsound_channel_idle;
}

#if 0
Original Ghidra decompilation (0x548450):

undefined4 missed_548450(short param_1)

{
  undefined4 uVar1;

  if ((&DAT_007252e4)[param_1 * 2] != -1) {
    uVar1 = sound_channel_check_loop_boundary();
    return uVar1;
  }
  return 0xffff0000;
}

Disassembly (0x548450..0x54846f; phase-4 review):

0x548450: movsx eax, word ptr [esp + 4]
0x548455: lea eax, [eax*4 + 0x7252e4]
0x54845c: movsx eax, word ptr [eax]
0x54845f: xor ecx, ecx
0x548461: cmp ax, 0xffff
0x548465: je 0x54846c
0x548467: jmp 0x548050
0x54846c: mov ax, cx
0x54846f: ret
#endif

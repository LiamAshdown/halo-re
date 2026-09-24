// sound_driver_channel_set_parameters  (Ghidra: missed_5484d0; 0 callers in this module --
// reached only through the DirectSound driver's channel_set_parameters vtable slot,
// sound_driver.channel_set_parameters 0x34)
// address 0x5484d0, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md driver slot table "0x34 0x5484d0 -> 0x5475b0" and
//   types/sound.h sound_driver.channel_set_parameters comment; reuses sound_channel_bind_hardware
//   (0x5482e0, this module) and sound_channel_set_parameters (0x5475b0, this module, whose own
//   documented convention is "stack -> (channel_index, update), EDI -> parameters" -- this
//   wrapper loads `parameters` into EDI before the call).
// register convention: all 3 parameters are this function's own recognized stack parameters
//   (Ghidra only surfaced 2 in its own decompile; the 3rd, `parameters`, is loaded into EDI for
//   the tail call, matching the driver's channel_set_parameters(channel_index, parameters,
//   unknown) prototype in types/sound.h).
// blam-cc: stack -> (channel_index, parameters, unknown)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. Fixes against the Ghidra draft: recovers the
// `parameters` argument that Ghidra's own signature dropped.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4

extern void sound_channel_bind_hardware(int16_t logical_channel_index); // 0x5482e0, blam-cc: stack
extern void sound_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update);
    // 0x5475b0, blam-cc: stack -> (channel_index, update), EDI -> parameters (prototype order as in
    // src/sound/sound_channel_set_parameters.c)

// blam-cc: stack -> (channel_index, parameters, unknown)
// Binds a hardware channel to `channel_index` if it does not already have one, then forwards
// `parameters` to that hardware channel.
void sound_driver_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        sound_channel_bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        sound_channel_set_parameters(directsound_bindings[channel_index].hardware_channel_index, parameters, unknown);
    }
}

#if 0
Original Ghidra decompilation (0x5484d0):

void missed_5484d0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  if ((&DAT_007252e4)[(short)param_1 * 2] == -1) {
    sound_channel_bind_hardware(param_1);
  }
  if ((&DAT_007252e4)[(short)param_1 * 2] != -1) {
    sound_channel_set_parameters((int)(short)(&DAT_007252e4)[(short)param_1 * 2],param_3);
  }
  return;
}

Disassembly (0x5484d0..0x548511; phase-4 review):

0x5484d0: mov eax, dword ptr [esp + 4]
0x5484d4: push esi
0x5484d5: movsx esi, ax
0x5484d8: cmp word ptr [esi*4 + 0x7252e4], 0xffff
0x5484e1: lea esi, [esi*4 + 0x7252e4]
0x5484e8: jne 0x5484f3
0x5484ea: push eax
0x5484eb: call 0x5482e0
0x5484f0: add esp, 4
0x5484f3: movsx esi, word ptr [esi]
0x5484f6: cmp si, 0xffff
0x5484fa: je 0x548510
0x5484fc: mov eax, dword ptr [esp + 0x10]
0x548500: push edi
0x548501: mov edi, dword ptr [esp + 0x10]
0x548505: push eax
0x548506: push esi
0x548507: call 0x5475b0
0x54850c: add esp, 8
0x54850f: pop edi
0x548510: pop esi
0x548511: ret
#endif

// sound_eax_effect_initialize_channel  (Ghidra: missed_54f6e0; 0 callers in this module --
// reached only through the EAX2 and EAX3 vtables' initialize_channel slot, 0x00671d04+0x08 and
// 0x00671d28+0x08 (both point here; EAX1's own slot, 0x00671d4c+0x08, is
// sound_eax1_effect_initialize_channel, 0x551240, this batch))
// address 0x54f6e0, size 63 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md "0x54f6e0 / 0x551240" listed as the two
//   initialize_channel implementations; confirmed shared by reading .rdata at both vtables'
//   +0x08 slot. sound_eax_effect_object.channel_property_sets (0x1c, types/sound.h) and
//   directsound_channel.buffer_3d (0x674) match by offset; IUnknown::QueryInterface is vtable
//   slot 0 (per the module header's own vtable-slot note); sound_eax_property_set_guid
//   (0x0064e20c, IID_IKsPropertySet) is reused from sound_eax1_effect_initialize.c (this module).
// register convention: __thiscall (ECX -> this), stack -> channel_index.
// blam-cc: ECX -> this_object, stack -> channel_index
// FIXED (register inputs, objdump): ECX carries this_object (read at 0x54f6ed,
// lea esi,[ecx+edx*4+0x1c]); the note said "ECX -> this" but the parameter is named
// this_object, so the checker's alias match failed even though the code already used it
// correctly.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax_property_set_guid[16]; // 0x0064e20c, IID_IKsPropertySet
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

// blam-cc: ECX -> this_object, stack -> channel_index
// Queries the channel's 3D buffer for IKsPropertySet and caches it in this_object's per-channel
// property-set slot. Reports success only if the query succeeded and returned a non-null
// interface pointer.
int32_t __thiscall sound_eax_effect_initialize_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **property_set_out = &this_object->channel_property_sets[channel_index];
    void **vtable = *(void ***)channel->buffer_3d;
    int32_t (__stdcall *query_interface)(void *, const uint8_t *, void **) =
        (int32_t (__stdcall *)(void *, const uint8_t *, void **))vtable[0];
    int32_t result = query_interface(channel->buffer_3d, sound_eax_property_set_guid, property_set_out);

    return (result >= 0 && *property_set_out != 0) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x54f6e0):

undefined4 missed_54f6e0(int param_1)

{
  int *piVar1;
  int iVar2;
  int in_ECX;

  piVar1 = (int *)(in_ECX + 0x1c + param_1 * 4);
  iVar2 = (*(code *)**(undefined4 **)(&DAT_00725aa4)[param_1 * 0x19e])
                    ((undefined4 *)(&DAT_00725aa4)[param_1 * 0x19e],&DAT_0064e20c,piVar1);
  if ((-1 < iVar2) && (*piVar1 != 0)) {
    return 1;
  }
  return 0;
}

Disassembly (0x54f6e0..0x54f71c; phase-4 review):

0x54f6e0: mov edx, dword ptr [esp + 4]
0x54f6e4: mov eax, edx
0x54f6e6: imul eax, eax, 0x678
0x54f6ec: push esi
0x54f6ed: lea esi, [ecx + edx*4 + 0x1c]
0x54f6f1: push esi
0x54f6f2: add eax, 0x725430
0x54f6f7: mov eax, dword ptr [eax + 0x674]
0x54f6fd: mov ecx, dword ptr [eax]
0x54f6ff: push 0x64e20c
0x54f704: push eax
0x54f705: call dword ptr [ecx]
0x54f707: test eax, eax
0x54f709: jl 0x54f719
0x54f70b: cmp dword ptr [esi], 0
0x54f70e: je 0x54f719
0x54f710: mov eax, 1
0x54f715: pop esi
0x54f716: ret 4
0x54f719: xor eax, eax
0x54f71b: pop esi
0x54f71c: ret 4
#endif

// sound_eax30_effect_apply_channel  (Ghidra: sound_eax30_effect_apply_channel, already named,
// __thiscall; 0 callers in this module -- reached only through the EAX vtable's apply_channel
// slot)
// address 0x550890, size 884 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Computes and applies per-channel obstruction/
// occlusion EAX 3.0 buffer reverb parameters for one sound channel."; directsound_channel field
// offsets (spatialized 0x06, underwater 0x07, eax_value 0x60, occlusion 0x48,
// obstruction 0x44, types/sound.h) confirmed exactly against the global addresses used
// (0x00725430 + those offsets).
// register convention: __thiscall (ECX -> this), stack -> channel_index.
// The 9 x 2 near-identical Set() blocks (deferred vs non-deferred id) are folded into one loop
// over an {id, bit, value pointer} table; the redundant nested duplicate check on id 0x14 (bit
// 0x100000, inside the id-8 block) is preserved as its own separate table entry rather than
// deduplicated.
// Phase-4 review (disassembly appended below): property 5 is the underwater direct gain
// (0x0069ff28) at max 1000 (the draft used the class eax_value), property 9 is the occlusion
// (+0x48) in millibels (the draft used the cone outside gain, +0x54); the remaining slots
// (6, 7, 8, 0x14, 0xa, 0xb, 0xc = 0.2) matched.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax30_buffer_property_guid[16]; // 0x0064e320
extern float sound_underwater_direct_gain; // 0x0069ff28
extern uint8_t directsound_deferred_dirty; // 0x00746132

extern int32_t sound_gain_to_directsound_volume(float gain, int32_t maximum); // this module, 0x54ee70
extern int __cdecl sound_gain_to_millibels(float gain); // this module, 0x54eec0
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

void __thiscall sound_eax30_effect_apply_channel(sound_eax_effect_object *this_object, int32_t channel_index)
{
    directsound_channel *channel_state;
    void *property_set;
    sound_property_set_fn set;
    uint32_t supported;
    uint8_t deferred;
    int32_t obstruction_at_1000, obstruction_at_0, occlusion_millibels, self_obstruction_millibels;
    int32_t underwater_direct, underwater_room;
    int32_t unused_id20 = 0, id10_value = 0;
    int32_t id12_value = 0x3e4ccccd; // ~0.2 float bit pattern

    obstruction_at_1000 = k_sound_minimum_volume;
    obstruction_at_0 = k_sound_minimum_volume;
    occlusion_millibels = 0;
    self_obstruction_millibels = 0;
    underwater_direct = 0;
    underwater_room = 0;

    property_set = this_object->channel_property_sets[channel_index];
    if (property_set == 0) {
        return;
    }

    channel_state = &directsound_channels[channel_index];

    if (channel_state->spatialized) {
        float eax_value = channel_state->eax_value;
        if (channel_state->underwater) {
            underwater_room = sound_gain_to_directsound_volume(sound_underwater_direct_gain, 0);
            underwater_direct = sound_gain_to_directsound_volume(sound_underwater_direct_gain, 1000);
        }
        obstruction_at_1000 = sound_gain_to_directsound_volume(1.0f - eax_value, 1000);
        obstruction_at_0 = sound_gain_to_directsound_volume(1.0f - eax_value, 0);
        occlusion_millibels = sound_gain_to_millibels(channel_state->occlusion); // id 9 takes the occlusion
        self_obstruction_millibels = sound_gain_to_millibels(channel_state->obstruction);
    }

    supported = this_object->base.supported_properties;
    deferred = (supported & 1) != 0;
    set = (sound_property_set_fn)(*(void ***)property_set)[4];

    {
        struct { uint32_t bit; uint32_t id; int32_t *value; } fields[9] = {
            {0x00000020, 5,    &underwater_direct},
            {0x00000040, 6,    &underwater_room},
            {0x00000080, 7,    &obstruction_at_1000},
            {0x00000100, 8,    &obstruction_at_0},
            {0x00100000, 0x14, &unused_id20},
            {0x00000200, 9,    &occlusion_millibels},
            {0x00000400, 0x0a, &id10_value},
            {0x00000800, 0x0b, &self_obstruction_millibels},
            {0x00001000, 0x0c, &id12_value},
        };
        int32_t i;
        for (i = 0; i < 9; i++) {
            if (supported & fields[i].bit) {
                uint32_t id = deferred ? (fields[i].id | 0x80000000u) : fields[i].id;
                set(property_set, sound_eax30_buffer_property_guid, id, 0, 0, fields[i].value, 4);
            }
        }
    }

    directsound_deferred_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x550890): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x550890.md for the full 884-byte listing, folded into the table above.

Disassembly (0x550890..0x550c04, capstone; phase-4 review):

0x550890: sub esp, 0x24
0x550893: push ebx
0x550894: push esi
0x550895: xor ebx, ebx
0x550897: mov eax, 0xffffd8f0
0x55089c: push edi
0x55089d: mov edi, dword ptr [esp + 0x34]
0x5508a1: mov esi, ecx
0x5508a3: mov dword ptr [esp + 0x14], eax
0x5508a7: mov dword ptr [esp + 0x18], eax
0x5508ab: cmp dword ptr [esi + edi*4 + 0x1c], ebx
0x5508af: mov dword ptr [esp + 0x1c], ebx
0x5508b3: mov dword ptr [esp + 0x20], ebx
0x5508b7: mov dword ptr [esp + 0xc], ebx
0x5508bb: mov dword ptr [esp + 0x10], ebx
0x5508bf: mov dword ptr [esp + 0x24], ebx
0x5508c3: mov dword ptr [esp + 0x28], ebx
0x5508c7: mov dword ptr [esp + 0x2c], 0x3e4ccccd
0x5508cf: je 0x550bfb
0x5508d5: push ebp
0x5508d6: mov ebp, edi
0x5508d8: imul ebp, ebp, 0x678
0x5508de: add ebp, 0x725430
0x5508e4: mov al, byte ptr [ebp + 6]
0x5508e7: test al, al
0x5508e9: je 0x550960
0x5508eb: fld dword ptr [0x672ac4]
0x5508f1: mov al, byte ptr [ebp + 7]
0x5508f4: test al, al
0x5508f6: fsub dword ptr [ebp + 0x60]
0x5508f9: fstp dword ptr [esp + 0x38]
0x5508fd: je 0x550922
0x5508ff: push ebx
0x550900: mov ebx, dword ptr [0x69ff28]
0x550906: push ebx
0x550907: call 0x54ee70
0x55090c: push 0x3e8
0x550911: push ebx
0x550912: mov dword ptr [esp + 0x24], eax
0x550916: call 0x54ee70
0x55091b: add esp, 0x10
0x55091e: mov dword ptr [esp + 0x10], eax
0x550922: mov ebx, dword ptr [esp + 0x38]
0x550926: push 0x3e8
0x55092b: push ebx
0x55092c: call 0x54ee70
0x550931: push 0
0x550933: push ebx
0x550934: mov dword ptr [esp + 0x28], eax
0x550938: call 0x54ee70
0x55093d: mov dword ptr [esp + 0x2c], eax
0x550941: mov eax, dword ptr [ebp + 0x48]
0x550944: push eax
0x550945: call 0x54eec0
0x55094a: mov ecx, dword ptr [ebp + 0x44]
0x55094d: push ecx
0x55094e: mov dword ptr [esp + 0x38], eax
0x550952: call 0x54eec0
0x550957: add esp, 0x18
0x55095a: mov dword ptr [esp + 0x24], eax
0x55095e: xor ebx, ebx
0x550960: mov eax, dword ptr [esi + 8]
0x550963: test al, 1
0x550965: je 0x550ac7
0x55096b: test al, 0x20
0x55096d: je 0x55098c
0x55096f: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550973: mov edx, dword ptr [eax]
0x550975: push 4
0x550977: lea ecx, [esp + 0x14]
0x55097b: push ecx
0x55097c: push ebx
0x55097d: push ebx
0x55097e: push 0x80000005
0x550983: push 0x64e320
0x550988: push eax
0x550989: call dword ptr [edx + 0x10]
0x55098c: test byte ptr [esi + 8], 0x40
0x550990: je 0x5509af
0x550992: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550996: mov edx, dword ptr [eax]
0x550998: push 4
0x55099a: lea ecx, [esp + 0x18]
0x55099e: push ecx
0x55099f: push ebx
0x5509a0: push ebx
0x5509a1: push 0x80000006
0x5509a6: push 0x64e320
0x5509ab: push eax
0x5509ac: call dword ptr [edx + 0x10]
0x5509af: mov al, byte ptr [esi + 8]
0x5509b2: test al, al
0x5509b4: jns 0x5509d3
0x5509b6: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x5509ba: mov edx, dword ptr [eax]
0x5509bc: push 4
0x5509be: lea ecx, [esp + 0x1c]
0x5509c2: push ecx
0x5509c3: push ebx
0x5509c4: push ebx
0x5509c5: push 0x80000007
0x5509ca: push 0x64e320
0x5509cf: push eax
0x5509d0: call dword ptr [edx + 0x10]
0x5509d3: mov eax, dword ptr [esi + 8]
0x5509d6: test ah, 1
0x5509d9: je 0x5509f8
0x5509db: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x5509df: mov edx, dword ptr [eax]
0x5509e1: push 4
0x5509e3: lea ecx, [esp + 0x20]
0x5509e7: push ecx
0x5509e8: push ebx
0x5509e9: push ebx
0x5509ea: push 0x80000008
0x5509ef: push 0x64e320
0x5509f4: push eax
0x5509f5: call dword ptr [edx + 0x10]
0x5509f8: test dword ptr [esi + 8], 0x100000
0x5509ff: je 0x550a1e
0x550a01: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550a05: mov edx, dword ptr [eax]
0x550a07: push 4
0x550a09: lea ecx, [esp + 0x2c]
0x550a0d: push ecx
0x550a0e: push ebx
0x550a0f: push ebx
0x550a10: push 0x80000014
0x550a15: push 0x64e320
0x550a1a: push eax
0x550a1b: call dword ptr [edx + 0x10]
0x550a1e: mov eax, dword ptr [esi + 8]
0x550a21: test ah, 2
0x550a24: je 0x550a43
0x550a26: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550a2a: mov edx, dword ptr [eax]
0x550a2c: push 4
0x550a2e: lea ecx, [esp + 0x24]
0x550a32: push ecx
0x550a33: push ebx
0x550a34: push ebx
0x550a35: push 0x80000009
0x550a3a: push 0x64e320
0x550a3f: push eax
0x550a40: call dword ptr [edx + 0x10]
0x550a43: mov eax, dword ptr [esi + 8]
0x550a46: test ah, 4
0x550a49: je 0x550a68
0x550a4b: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550a4f: mov edx, dword ptr [eax]
0x550a51: push 4
0x550a53: lea ecx, [esp + 0x30]
0x550a57: push ecx
0x550a58: push ebx
0x550a59: push ebx
0x550a5a: push 0x8000000a
0x550a5f: push 0x64e320
0x550a64: push eax
0x550a65: call dword ptr [edx + 0x10]
0x550a68: mov eax, dword ptr [esi + 8]
0x550a6b: test ah, 8
0x550a6e: je 0x550a8d
0x550a70: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550a74: mov edx, dword ptr [eax]
0x550a76: push 4
0x550a78: lea ecx, [esp + 0x28]
0x550a7c: push ecx
0x550a7d: push ebx
0x550a7e: push ebx
0x550a7f: push 0x8000000b
0x550a84: push 0x64e320
0x550a89: push eax
0x550a8a: call dword ptr [edx + 0x10]
0x550a8d: mov eax, dword ptr [esi + 8]
0x550a90: test ah, 0x10
0x550a93: je 0x550bf3
0x550a99: mov esi, dword ptr [esi + edi*4 + 0x1c]
0x550a9d: mov edx, dword ptr [esi]
0x550a9f: push 4
0x550aa1: lea eax, [esp + 0x34]
0x550aa5: push eax
0x550aa6: push ebx
0x550aa7: push ebx
0x550aa8: push 0x8000000c
0x550aad: push 0x64e320
0x550ab2: push esi
0x550ab3: call dword ptr [edx + 0x10]
0x550ab6: pop ebp
0x550ab7: pop edi
0x550ab8: pop esi
0x550ab9: mov byte ptr [0x746132], 1
0x550ac0: pop ebx
0x550ac1: add esp, 0x24
0x550ac4: ret 4
0x550ac7: test al, 0x20
0x550ac9: je 0x550ae5
0x550acb: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550acf: mov ecx, dword ptr [eax]
0x550ad1: push 4
0x550ad3: lea edx, [esp + 0x14]
0x550ad7: push edx
0x550ad8: push ebx
0x550ad9: push ebx
0x550ada: push 5
0x550adc: push 0x64e320
0x550ae1: push eax
0x550ae2: call dword ptr [ecx + 0x10]
0x550ae5: test byte ptr [esi + 8], 0x40
0x550ae9: je 0x550b05
0x550aeb: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550aef: mov ecx, dword ptr [eax]
0x550af1: push 4
0x550af3: lea edx, [esp + 0x18]
0x550af7: push edx
0x550af8: push ebx
0x550af9: push ebx
0x550afa: push 6
0x550afc: push 0x64e320
0x550b01: push eax
0x550b02: call dword ptr [ecx + 0x10]
0x550b05: mov al, byte ptr [esi + 8]
0x550b08: test al, al
0x550b0a: jns 0x550b26
0x550b0c: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550b10: mov ecx, dword ptr [eax]
0x550b12: push 4
0x550b14: lea edx, [esp + 0x1c]
0x550b18: push edx
0x550b19: push ebx
0x550b1a: push ebx
0x550b1b: push 7
0x550b1d: push 0x64e320
0x550b22: push eax
0x550b23: call dword ptr [ecx + 0x10]
0x550b26: mov eax, dword ptr [esi + 8]
0x550b29: mov ebp, 0x100
0x550b2e: test ebp, eax
0x550b30: je 0x550b6b
0x550b32: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550b36: mov ecx, dword ptr [eax]
0x550b38: push 4
0x550b3a: lea edx, [esp + 0x20]
0x550b3e: push edx
0x550b3f: push ebx
0x550b40: push ebx
0x550b41: push 8
0x550b43: push 0x64e320
0x550b48: push eax
0x550b49: call dword ptr [ecx + 0x10]
0x550b4c: test dword ptr [esi + 8], ebp
0x550b4f: je 0x550b6b
0x550b51: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550b55: mov ecx, dword ptr [eax]
0x550b57: push 4
0x550b59: lea edx, [esp + 0x2c]
0x550b5d: push edx
0x550b5e: push ebx
0x550b5f: push ebx
0x550b60: push 0x14
0x550b62: push 0x64e320
0x550b67: push eax
0x550b68: call dword ptr [ecx + 0x10]
0x550b6b: mov eax, dword ptr [esi + 8]
0x550b6e: test ah, 2
0x550b71: je 0x550b8d
0x550b73: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550b77: mov ecx, dword ptr [eax]
0x550b79: push 4
0x550b7b: lea edx, [esp + 0x24]
0x550b7f: push edx
0x550b80: push ebx
0x550b81: push ebx
0x550b82: push 9
0x550b84: push 0x64e320
0x550b89: push eax
0x550b8a: call dword ptr [ecx + 0x10]
0x550b8d: mov eax, dword ptr [esi + 8]
0x550b90: test ah, 4
0x550b93: je 0x550baf
0x550b95: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550b99: mov ecx, dword ptr [eax]
0x550b9b: push 4
0x550b9d: lea edx, [esp + 0x30]
0x550ba1: push edx
0x550ba2: push ebx
0x550ba3: push ebx
0x550ba4: push 0xa
0x550ba6: push 0x64e320
0x550bab: push eax
0x550bac: call dword ptr [ecx + 0x10]
0x550baf: mov eax, dword ptr [esi + 8]
0x550bb2: test ah, 8
0x550bb5: je 0x550bd1
0x550bb7: mov eax, dword ptr [esi + edi*4 + 0x1c]
0x550bbb: mov ecx, dword ptr [eax]
0x550bbd: push 4
0x550bbf: lea edx, [esp + 0x28]
0x550bc3: push edx
0x550bc4: push ebx
0x550bc5: push ebx
0x550bc6: push 0xb
0x550bc8: push 0x64e320
0x550bcd: push eax
0x550bce: call dword ptr [ecx + 0x10]
0x550bd1: mov eax, dword ptr [esi + 8]
0x550bd4: test ah, 0x10
0x550bd7: je 0x550bf3
0x550bd9: mov esi, dword ptr [esi + edi*4 + 0x1c]
0x550bdd: mov eax, dword ptr [esi]
0x550bdf: push 4
0x550be1: lea ecx, [esp + 0x34]
0x550be5: push ecx
0x550be6: push ebx
0x550be7: push ebx
0x550be8: push 0xc
0x550bea: push 0x64e320
0x550bef: push esi
0x550bf0: call dword ptr [eax + 0x10]
0x550bf3: mov byte ptr [0x746132], 1
0x550bfa: pop ebp
0x550bfb: pop edi
0x550bfc: pop esi
0x550bfd: pop ebx
0x550bfe: add esp, 0x24
0x550c01: ret 4
#endif

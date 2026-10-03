// sound_eax20_effect_shutdown  (Ghidra: sound_eax20_effect_shutdown, already named, __thiscall)
// address 0x54ef30, size 825 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Restores default EAX 2.0 listener/buffer reverb
// parameters and releases all cached IKsPropertySet interfaces held by the sound effects
// object."; this (sound_eax_effect_object, types/sound.h) fields property_set (base.0x18,
// listener) and channel_property_sets[51] (0x1c) match exactly; k_maximum_eax_channels (51)
// matches the "0x33" channel loop count.
// register convention: ECX -> this_object (thiscall).
// The 11/9 near-identical "if bit set, Set(id, default)" blocks are each folded into one small
// loop over a {bit, property_id, default_bits} table, which is semantically identical to
// Ghidra's repeated blocks.
// Phase-4 review (disassembly appended below): the listener defaults are Set on
// channel_property_sets[0] (+0x1c) while the gate and the Release use base.property_set (+0x18);
// REVERB (id 9) resets to -10000 mB, not 0 (every other default matched).

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax20_listener_property_guid[16]; // 0x0064e2f0
extern const uint8_t sound_eax20_buffer_property_guid[16];   // 0x0064e300

typedef struct sound_eax20_default_property {
    uint32_t bit;
    uint32_t id;
    uint32_t default_bits; // reinterpreted as float or int32 by the property, bit pattern only
} sound_eax20_default_property;

static const sound_eax20_default_property k_listener_defaults[11] = {
    {0x00000004, 2,  0xffffd8f0u}, // Environment
    {0x00000008, 3,  0xffffd8f0u}, // EnvironmentSize/RoomHF
    {0x00000010, 4,  0x00000000u},
    {0x00000020, 5,  0x3dcccccdu}, // 0.1
    {0x00000040, 6,  0x3dcccccdu}, // 0.1
    {0x00000080, 7,  0xffffd8f0u},
    {0x00000100, 8,  0x00000000u},
    {0x00000200, 9,  0xffffd8f0u}, // REVERB -10000 mB
    {0x00000400, 10, 0x00000000u},
    {0x00002000, 13, 0x00000000u},
    {0x00008000, 15, 0x00000000u},
};

static const sound_eax20_default_property k_channel_defaults[9] = {
    {0x00000004, 2,  0xffffd8f0u},
    {0x00000008, 3,  0xffffd8f0u},
    {0x00000010, 4,  0xffffd8f0u},
    {0x00000020, 5,  0xffffd8f0u},
    {0x00000040, 6,  0x00000000u},
    {0x00000080, 7,  0x00000000u},
    {0x00000100, 8,  0x00000000u},
    {0x00000200, 9,  0x00000000u},
    {0x00000400, 10, 0x00000000u},
};

void __thiscall sound_eax20_effect_shutdown(sound_eax_effect_object *this_object)
{
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    uint32_t default_bits;

    property_set = this_object->base.property_set;
    if (property_set != 0) {
        // the listener defaults are written through channel_property_sets[0] (+0x1c, the set that
        // carries the listener properties); only the +0x18 interface is released here
        void *listener_set = this_object->channel_property_sets[0];

        set = (sound_property_set_fn)(*(void ***)listener_set)[4];
        for (i = 0; i < 11; i++) {
            if (this_object->base.supported_properties & k_listener_defaults[i].bit) {
                default_bits = k_listener_defaults[i].default_bits;
                set(listener_set, sound_eax20_listener_property_guid, k_listener_defaults[i].id, 0, 0,
                    &default_bits, 4);
            }
        }
        ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[2])(property_set); // Release
        this_object->base.property_set = 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        property_set = this_object->channel_property_sets[i];
        if (property_set != 0) {
            int32_t j;
            set = (sound_property_set_fn)(*(void ***)property_set)[4];
            for (j = 0; j < 9; j++) {
                if (this_object->base.supported_properties & k_channel_defaults[j].bit) {
                    default_bits = k_channel_defaults[j].default_bits;
                    set(property_set, sound_eax20_buffer_property_guid, k_channel_defaults[j].id, 0, 0,
                        &default_bits, 4);
                }
            }
            ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[2])(property_set); // Release
        }
        this_object->channel_property_sets[i] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x54ef30): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x54ef30.md for the full 825-byte listing (11 near-identical listener
Set() blocks followed by a 51-iteration loop of 9 near-identical channel Set() blocks, folded
into the two small tables above).

Disassembly (0x54ef30..0x54f269, capstone; phase-4 review):

0x54ef30: sub esp, 0x2c
0x54ef33: push ebp
0x54ef34: push esi
0x54ef35: mov esi, ecx
0x54ef37: mov eax, dword ptr [esi + 0x18]
0x54ef3a: xor ebp, ebp
0x54ef3c: cmp eax, ebp
0x54ef3e: push edi
0x54ef3f: je 0x54f0e9
0x54ef45: mov eax, 0xffffd8f0
0x54ef4a: mov dword ptr [esp + 0xc], eax
0x54ef4e: mov dword ptr [esp + 0x10], eax
0x54ef52: mov dword ptr [esp + 0x20], eax
0x54ef56: mov dword ptr [esp + 0x28], eax
0x54ef5a: test byte ptr [esi + 8], 4
0x54ef5e: mov dword ptr [esp + 0x34], ebp
0x54ef62: mov dword ptr [esp + 0x14], ebp
0x54ef66: mov dword ptr [esp + 0x18], 0x3dcccccd
0x54ef6e: mov dword ptr [esp + 0x1c], 0x3dcccccd
0x54ef76: mov dword ptr [esp + 0x24], ebp
0x54ef7a: mov dword ptr [esp + 0x2c], ebp
0x54ef7e: mov dword ptr [esp + 0x30], ebp
0x54ef82: je 0x54ef9d
0x54ef84: mov eax, dword ptr [esi + 0x1c]
0x54ef87: mov ecx, dword ptr [eax]
0x54ef89: push 4
0x54ef8b: lea edx, [esp + 0x10]
0x54ef8f: push edx
0x54ef90: push ebp
0x54ef91: push ebp
0x54ef92: push 2
0x54ef94: push 0x64e2f0
0x54ef99: push eax
0x54ef9a: call dword ptr [ecx + 0x10]
0x54ef9d: test byte ptr [esi + 8], 8
0x54efa1: je 0x54efbc
0x54efa3: mov eax, dword ptr [esi + 0x1c]
0x54efa6: mov ecx, dword ptr [eax]
0x54efa8: push 4
0x54efaa: lea edx, [esp + 0x14]
0x54efae: push edx
0x54efaf: push ebp
0x54efb0: push ebp
0x54efb1: push 3
0x54efb3: push 0x64e2f0
0x54efb8: push eax
0x54efb9: call dword ptr [ecx + 0x10]
0x54efbc: test byte ptr [esi + 8], 0x10
0x54efc0: je 0x54efdb
0x54efc2: mov eax, dword ptr [esi + 0x1c]
0x54efc5: mov ecx, dword ptr [eax]
0x54efc7: push 4
0x54efc9: lea edx, [esp + 0x18]
0x54efcd: push edx
0x54efce: push ebp
0x54efcf: push ebp
0x54efd0: push 4
0x54efd2: push 0x64e2f0
0x54efd7: push eax
0x54efd8: call dword ptr [ecx + 0x10]
0x54efdb: test byte ptr [esi + 8], 0x20
0x54efdf: je 0x54effa
0x54efe1: mov eax, dword ptr [esi + 0x1c]
0x54efe4: mov ecx, dword ptr [eax]
0x54efe6: push 4
0x54efe8: lea edx, [esp + 0x1c]
0x54efec: push edx
0x54efed: push ebp
0x54efee: push ebp
0x54efef: push 5
0x54eff1: push 0x64e2f0
0x54eff6: push eax
0x54eff7: call dword ptr [ecx + 0x10]
0x54effa: test byte ptr [esi + 8], 0x40
0x54effe: je 0x54f019
0x54f000: mov eax, dword ptr [esi + 0x1c]
0x54f003: mov ecx, dword ptr [eax]
0x54f005: push 4
0x54f007: lea edx, [esp + 0x20]
0x54f00b: push edx
0x54f00c: push ebp
0x54f00d: push ebp
0x54f00e: push 6
0x54f010: push 0x64e2f0
0x54f015: push eax
0x54f016: call dword ptr [ecx + 0x10]
0x54f019: mov al, byte ptr [esi + 8]
0x54f01c: test al, al
0x54f01e: jns 0x54f039
0x54f020: mov eax, dword ptr [esi + 0x1c]
0x54f023: mov ecx, dword ptr [eax]
0x54f025: push 4
0x54f027: lea edx, [esp + 0x24]
0x54f02b: push edx
0x54f02c: push ebp
0x54f02d: push ebp
0x54f02e: push 7
0x54f030: push 0x64e2f0
0x54f035: push eax
0x54f036: call dword ptr [ecx + 0x10]
0x54f039: mov eax, dword ptr [esi + 8]
0x54f03c: test ah, 1
0x54f03f: je 0x54f05a
0x54f041: mov eax, dword ptr [esi + 0x1c]
0x54f044: mov ecx, dword ptr [eax]
0x54f046: push 4
0x54f048: lea edx, [esp + 0x28]
0x54f04c: push edx
0x54f04d: push ebp
0x54f04e: push ebp
0x54f04f: push 8
0x54f051: push 0x64e2f0
0x54f056: push eax
0x54f057: call dword ptr [ecx + 0x10]
0x54f05a: mov eax, dword ptr [esi + 8]
0x54f05d: test ah, 2
0x54f060: je 0x54f07b
0x54f062: mov eax, dword ptr [esi + 0x1c]
0x54f065: mov ecx, dword ptr [eax]
0x54f067: push 4
0x54f069: lea edx, [esp + 0x2c]
0x54f06d: push edx
0x54f06e: push ebp
0x54f06f: push ebp
0x54f070: push 9
0x54f072: push 0x64e2f0
0x54f077: push eax
0x54f078: call dword ptr [ecx + 0x10]
0x54f07b: mov eax, dword ptr [esi + 8]
0x54f07e: test ah, 4
0x54f081: je 0x54f09c
0x54f083: mov eax, dword ptr [esi + 0x1c]
0x54f086: mov ecx, dword ptr [eax]
0x54f088: push 4
0x54f08a: lea edx, [esp + 0x30]
0x54f08e: push edx
0x54f08f: push ebp
0x54f090: push ebp
0x54f091: push 0xa
0x54f093: push 0x64e2f0
0x54f098: push eax
0x54f099: call dword ptr [ecx + 0x10]
0x54f09c: mov eax, dword ptr [esi + 8]
0x54f09f: test ah, 0x20
0x54f0a2: je 0x54f0bd
0x54f0a4: mov eax, dword ptr [esi + 0x1c]
0x54f0a7: mov ecx, dword ptr [eax]
0x54f0a9: push 4
0x54f0ab: lea edx, [esp + 0x34]
0x54f0af: push edx
0x54f0b0: push ebp
0x54f0b1: push ebp
0x54f0b2: push 0xd
0x54f0b4: push 0x64e2f0
0x54f0b9: push eax
0x54f0ba: call dword ptr [ecx + 0x10]
0x54f0bd: mov eax, dword ptr [esi + 8]
0x54f0c0: test ah, ah
0x54f0c2: jns 0x54f0dd
0x54f0c4: mov eax, dword ptr [esi + 0x1c]
0x54f0c7: mov ecx, dword ptr [eax]
0x54f0c9: push 4
0x54f0cb: lea edx, [esp + 0x38]
0x54f0cf: push edx
0x54f0d0: push ebp
0x54f0d1: push ebp
0x54f0d2: push 0xf
0x54f0d4: push 0x64e2f0
0x54f0d9: push eax
0x54f0da: call dword ptr [ecx + 0x10]
0x54f0dd: mov eax, dword ptr [esi + 0x18]
0x54f0e0: mov ecx, dword ptr [eax]
0x54f0e2: push eax
0x54f0e3: call dword ptr [ecx + 8]
0x54f0e6: mov dword ptr [esi + 0x18], ebp
0x54f0e9: lea edi, [esi + 0x1c]
0x54f0ec: mov dword ptr [esp + 0xc], 0x33
0x54f0f4: mov eax, dword ptr [edi]
0x54f0f6: cmp eax, ebp
0x54f0f8: je 0x54f24e
0x54f0fe: mov ecx, 0xffffd8f0
0x54f103: mov dword ptr [esp + 0x2c], ecx
0x54f107: mov dword ptr [esp + 0x28], ecx
0x54f10b: mov dword ptr [esp + 0x34], ecx
0x54f10f: mov dword ptr [esp + 0x30], ecx
0x54f113: test byte ptr [esi + 8], 4
0x54f117: mov dword ptr [esp + 0x20], ebp
0x54f11b: mov dword ptr [esp + 0x18], ebp
0x54f11f: mov dword ptr [esp + 0x24], 0
0x54f127: mov dword ptr [esp + 0x1c], 0
0x54f12f: mov dword ptr [esp + 0x14], 0
0x54f137: je 0x54f14f
0x54f139: mov edx, dword ptr [eax]
0x54f13b: push 4
0x54f13d: lea ecx, [esp + 0x38]
0x54f141: push ecx
0x54f142: push ebp
0x54f143: push ebp
0x54f144: push 2
0x54f146: push 0x64e300
0x54f14b: push eax
0x54f14c: call dword ptr [edx + 0x10]
0x54f14f: test byte ptr [esi + 8], 8
0x54f153: je 0x54f16d
0x54f155: mov eax, dword ptr [edi]
0x54f157: mov edx, dword ptr [eax]
0x54f159: push 4
0x54f15b: lea ecx, [esp + 0x34]
0x54f15f: push ecx
0x54f160: push ebp
0x54f161: push ebp
0x54f162: push 3
0x54f164: push 0x64e300
0x54f169: push eax
0x54f16a: call dword ptr [edx + 0x10]
0x54f16d: test byte ptr [esi + 8], 0x10
0x54f171: je 0x54f18b
0x54f173: mov eax, dword ptr [edi]
0x54f175: mov edx, dword ptr [eax]
0x54f177: push 4
0x54f179: lea ecx, [esp + 0x30]
0x54f17d: push ecx
0x54f17e: push ebp
0x54f17f: push ebp
0x54f180: push 4
0x54f182: push 0x64e300
0x54f187: push eax
0x54f188: call dword ptr [edx + 0x10]
0x54f18b: test byte ptr [esi + 8], 0x20
0x54f18f: je 0x54f1a9
0x54f191: mov eax, dword ptr [edi]
0x54f193: mov edx, dword ptr [eax]
0x54f195: push 4
0x54f197: lea ecx, [esp + 0x2c]
0x54f19b: push ecx
0x54f19c: push ebp
0x54f19d: push ebp
0x54f19e: push 5
0x54f1a0: push 0x64e300
0x54f1a5: push eax
0x54f1a6: call dword ptr [edx + 0x10]
0x54f1a9: test byte ptr [esi + 8], 0x40
0x54f1ad: je 0x54f1c7
0x54f1af: mov eax, dword ptr [edi]
0x54f1b1: mov edx, dword ptr [eax]
0x54f1b3: push 4
0x54f1b5: lea ecx, [esp + 0x28]
0x54f1b9: push ecx
0x54f1ba: push ebp
0x54f1bb: push ebp
0x54f1bc: push 6
0x54f1be: push 0x64e300
0x54f1c3: push eax
0x54f1c4: call dword ptr [edx + 0x10]
0x54f1c7: mov al, byte ptr [esi + 8]
0x54f1ca: test al, al
0x54f1cc: jns 0x54f1e6
0x54f1ce: mov eax, dword ptr [edi]
0x54f1d0: mov edx, dword ptr [eax]
0x54f1d2: push 4
0x54f1d4: lea ecx, [esp + 0x24]
0x54f1d8: push ecx
0x54f1d9: push ebp
0x54f1da: push ebp
0x54f1db: push 7
0x54f1dd: push 0x64e300
0x54f1e2: push eax
0x54f1e3: call dword ptr [edx + 0x10]
0x54f1e6: mov eax, dword ptr [esi + 8]
0x54f1e9: test ah, 1
0x54f1ec: je 0x54f206
0x54f1ee: mov eax, dword ptr [edi]
0x54f1f0: mov edx, dword ptr [eax]
0x54f1f2: push 4
0x54f1f4: lea ecx, [esp + 0x20]
0x54f1f8: push ecx
0x54f1f9: push ebp
0x54f1fa: push ebp
0x54f1fb: push 8
0x54f1fd: push 0x64e300
0x54f202: push eax
0x54f203: call dword ptr [edx + 0x10]
0x54f206: mov eax, dword ptr [esi + 8]
0x54f209: test ah, 2
0x54f20c: je 0x54f226
0x54f20e: mov eax, dword ptr [edi]
0x54f210: mov edx, dword ptr [eax]
0x54f212: push 4
0x54f214: lea ecx, [esp + 0x1c]
0x54f218: push ecx
0x54f219: push ebp
0x54f21a: push ebp
0x54f21b: push 9
0x54f21d: push 0x64e300
0x54f222: push eax
0x54f223: call dword ptr [edx + 0x10]
0x54f226: mov eax, dword ptr [esi + 8]
0x54f229: test ah, 4
0x54f22c: je 0x54f246
0x54f22e: mov eax, dword ptr [edi]
0x54f230: mov edx, dword ptr [eax]
0x54f232: push 4
0x54f234: lea ecx, [esp + 0x18]
0x54f238: push ecx
0x54f239: push ebp
0x54f23a: push ebp
0x54f23b: push 0xa
0x54f23d: push 0x64e300
0x54f242: push eax
0x54f243: call dword ptr [edx + 0x10]
0x54f246: mov eax, dword ptr [edi]
0x54f248: mov edx, dword ptr [eax]
0x54f24a: push eax
0x54f24b: call dword ptr [edx + 8]
0x54f24e: mov eax, dword ptr [esp + 0xc]
0x54f252: mov dword ptr [edi], ebp
0x54f254: add edi, 4
0x54f257: dec eax
0x54f258: mov dword ptr [esp + 0xc], eax
0x54f25c: jne 0x54f0f4
0x54f262: pop edi
0x54f263: pop esi
0x54f264: pop ebp
0x54f265: add esp, 0x2c
0x54f268: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

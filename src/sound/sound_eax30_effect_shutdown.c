// sound_eax30_effect_shutdown  (Ghidra: sound_eax30_effect_shutdown, already named, __thiscall)
// address 0x54fff0, size 882 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Restores default EAX 3.0-style listener/buffer
// reverb parameters and releases all cached property-set interfaces held by the sound effects
// object."; same shape as sound_eax20_effect_shutdown.c (0x54ef30), different property ids/bits.
// register convention: ECX -> this_object (thiscall).
// The listener table includes two genuine oddities transcribed literally rather than "fixed":
// property id 4 and id 0x16 (22) are both gated by the *same* bit (0x10), and property id 8's
// buffer-side Set is wrapped in a redundant nested check of the same bit it is already inside.

// Phase-4 review (disassembly appended below): the listener defaults are Set on
// channel_property_sets[0] (+0x1c), the gate and Release use base.property_set (+0x18); the
// default table (ids 5, 6, 0x18, 8, 9, 0xb, 0xc, 0xe, 0xf, 4, 0x19, 0x16) matched.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax30_listener_property_guid[16]; // 0x0064e310
extern const uint8_t sound_eax30_buffer_property_guid[16];   // 0x0064e320

typedef struct sound_eax30_default_property { uint32_t bit; uint32_t id; uint32_t default_bits; } sound_eax30_default_property;

static const sound_eax30_default_property k_listener_defaults[12] = {
    {0x00000020, 5,    0xffffd8f0u},
    {0x00000040, 6,    0xffffd8f0u},
    {0x01000000, 0x18, 0x00000000u},
    {0x00000100, 8,    0x3dcccccdu}, // 0.1
    {0x00000200, 9,    0x3dcccccdu}, // 0.1
    {0x00000800, 0x0b, 0xffffd8f0u},
    {0x00001000, 0x0c, 0x00000000u},
    {0x00004000, 0x0e, 0xffffd8f0u},
    {0x00008000, 0x0f, 0x00000000u},
    {0x00000010, 4,    0x00000000u},
    {0x02000000, 0x19, 0x00000000u},
    {0x00000010, 0x16, 0x447a0000u}, // 1000.0
};

static const sound_eax30_default_property k_channel_defaults[9] = {
    {0x00000020, 5,    0x00000000u},
    {0x00000040, 6,    0x00000000u},
    {0x00000080, 7,    0xffffd8f0u},
    {0x00000100, 8,    0xffffd8f0u}, // also gates the redundant duplicate id 0x14 Set below
    {0x00000100, 0x14, 0x00000000u}, // duplicate-guarded by the same bit as id 8, see file header
    {0x00000200, 9,    0x00000000u},
    {0x00000400, 0x0a, 0x00000000u},
    {0x00000800, 0x0b, 0x00000000u},
    {0x00001000, 0x0c, 0x00000000u},
};

void __thiscall sound_eax30_effect_shutdown(sound_eax_effect_object *this_object)
{
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    uint32_t default_bits;

    property_set = this_object->base.property_set;
    if (property_set != 0) {
        // the listener defaults go through channel_property_sets[0] (+0x1c); only the +0x18
        // interface is released here
        void *listener_set = this_object->channel_property_sets[0];

        set = (sound_property_set_fn)(*(void ***)listener_set)[4];
        for (i = 0; i < 12; i++) {
            if (this_object->base.supported_properties & k_listener_defaults[i].bit) {
                default_bits = k_listener_defaults[i].default_bits;
                set(listener_set, sound_eax30_listener_property_guid, k_listener_defaults[i].id, 0, 0,
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
                    set(property_set, sound_eax30_buffer_property_guid, k_channel_defaults[j].id, 0, 0,
                        &default_bits, 4);
                }
            }
            ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[2])(property_set); // Release
        }
        this_object->channel_property_sets[i] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x54fff0): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x54fff0.md for the full 882-byte listing, folded into the two tables
above.

Disassembly (0x54fff0..0x550362, capstone; phase-4 review):

0x54fff0: sub esp, 0x30
0x54fff3: push ebx
0x54fff4: push ebp
0x54fff5: push esi
0x54fff6: mov esi, ecx
0x54fff8: mov eax, dword ptr [esi + 0x18]
0x54fffb: xor ebp, ebp
0x54fffd: cmp eax, ebp
0x54ffff: push edi
0x550000: je 0x5501da
0x550006: mov eax, 0xffffd8f0
0x55000b: mov dword ptr [esp + 0x10], eax
0x55000f: mov dword ptr [esp + 0x14], eax
0x550013: mov dword ptr [esp + 0x24], eax
0x550017: mov dword ptr [esp + 0x2c], eax
0x55001b: test byte ptr [esi + 8], 0x20
0x55001f: mov dword ptr [esp + 0x38], ebp
0x550023: mov dword ptr [esp + 0x18], ebp
0x550027: mov dword ptr [esp + 0x1c], 0x3dcccccd
0x55002f: mov dword ptr [esp + 0x20], 0x3dcccccd
0x550037: mov dword ptr [esp + 0x28], ebp
0x55003b: mov dword ptr [esp + 0x30], ebp
0x55003f: mov dword ptr [esp + 0x34], ebp
0x550043: mov dword ptr [esp + 0x3c], 0x447a0000
0x55004b: je 0x550066
0x55004d: mov eax, dword ptr [esi + 0x1c]
0x550050: mov ecx, dword ptr [eax]
0x550052: push 4
0x550054: lea edx, [esp + 0x14]
0x550058: push edx
0x550059: push ebp
0x55005a: push ebp
0x55005b: push 5
0x55005d: push 0x64e310
0x550062: push eax
0x550063: call dword ptr [ecx + 0x10]
0x550066: test byte ptr [esi + 8], 0x40
0x55006a: je 0x550085
0x55006c: mov eax, dword ptr [esi + 0x1c]
0x55006f: mov ecx, dword ptr [eax]
0x550071: push 4
0x550073: lea edx, [esp + 0x18]
0x550077: push edx
0x550078: push ebp
0x550079: push ebp
0x55007a: push 6
0x55007c: push 0x64e310
0x550081: push eax
0x550082: call dword ptr [ecx + 0x10]
0x550085: test dword ptr [esi + 8], 0x1000000
0x55008c: je 0x5500a7
0x55008e: mov eax, dword ptr [esi + 0x1c]
0x550091: mov ecx, dword ptr [eax]
0x550093: push 4
0x550095: lea edx, [esp + 0x1c]
0x550099: push edx
0x55009a: push ebp
0x55009b: push ebp
0x55009c: push 0x18
0x55009e: push 0x64e310
0x5500a3: push eax
0x5500a4: call dword ptr [ecx + 0x10]
0x5500a7: mov eax, dword ptr [esi + 8]
0x5500aa: test ah, 1
0x5500ad: je 0x5500c8
0x5500af: mov eax, dword ptr [esi + 0x1c]
0x5500b2: mov ecx, dword ptr [eax]
0x5500b4: push 4
0x5500b6: lea edx, [esp + 0x20]
0x5500ba: push edx
0x5500bb: push ebp
0x5500bc: push ebp
0x5500bd: push 8
0x5500bf: push 0x64e310
0x5500c4: push eax
0x5500c5: call dword ptr [ecx + 0x10]
0x5500c8: mov eax, dword ptr [esi + 8]
0x5500cb: test ah, 2
0x5500ce: je 0x5500e9
0x5500d0: mov eax, dword ptr [esi + 0x1c]
0x5500d3: mov ecx, dword ptr [eax]
0x5500d5: push 4
0x5500d7: lea edx, [esp + 0x24]
0x5500db: push edx
0x5500dc: push ebp
0x5500dd: push ebp
0x5500de: push 9
0x5500e0: push 0x64e310
0x5500e5: push eax
0x5500e6: call dword ptr [ecx + 0x10]
0x5500e9: mov eax, dword ptr [esi + 8]
0x5500ec: test ah, 8
0x5500ef: je 0x55010a
0x5500f1: mov eax, dword ptr [esi + 0x1c]
0x5500f4: mov ecx, dword ptr [eax]
0x5500f6: push 4
0x5500f8: lea edx, [esp + 0x28]
0x5500fc: push edx
0x5500fd: push ebp
0x5500fe: push ebp
0x5500ff: push 0xb
0x550101: push 0x64e310
0x550106: push eax
0x550107: call dword ptr [ecx + 0x10]
0x55010a: mov eax, dword ptr [esi + 8]
0x55010d: test ah, 0x10
0x550110: je 0x55012b
0x550112: mov eax, dword ptr [esi + 0x1c]
0x550115: mov ecx, dword ptr [eax]
0x550117: push 4
0x550119: lea edx, [esp + 0x2c]
0x55011d: push edx
0x55011e: push ebp
0x55011f: push ebp
0x550120: push 0xc
0x550122: push 0x64e310
0x550127: push eax
0x550128: call dword ptr [ecx + 0x10]
0x55012b: mov eax, dword ptr [esi + 8]
0x55012e: test ah, 0x40
0x550131: je 0x55014c
0x550133: mov eax, dword ptr [esi + 0x1c]
0x550136: mov ecx, dword ptr [eax]
0x550138: push 4
0x55013a: lea edx, [esp + 0x30]
0x55013e: push edx
0x55013f: push ebp
0x550140: push ebp
0x550141: push 0xe
0x550143: push 0x64e310
0x550148: push eax
0x550149: call dword ptr [ecx + 0x10]
0x55014c: mov eax, dword ptr [esi + 8]
0x55014f: test ah, ah
0x550151: jns 0x55016c
0x550153: mov eax, dword ptr [esi + 0x1c]
0x550156: mov ecx, dword ptr [eax]
0x550158: push 4
0x55015a: lea edx, [esp + 0x34]
0x55015e: push edx
0x55015f: push ebp
0x550160: push ebp
0x550161: push 0xf
0x550163: push 0x64e310
0x550168: push eax
0x550169: call dword ptr [ecx + 0x10]
0x55016c: mov al, byte ptr [esi + 8]
0x55016f: mov bl, 0x10
0x550171: test bl, al
0x550173: je 0x55018e
0x550175: mov eax, dword ptr [esi + 0x1c]
0x550178: mov ecx, dword ptr [eax]
0x55017a: push 4
0x55017c: lea edx, [esp + 0x38]
0x550180: push edx
0x550181: push ebp
0x550182: push ebp
0x550183: push 4
0x550185: push 0x64e310
0x55018a: push eax
0x55018b: call dword ptr [ecx + 0x10]
0x55018e: test dword ptr [esi + 8], 0x2000000
0x550195: je 0x5501b0
0x550197: mov eax, dword ptr [esi + 0x1c]
0x55019a: mov ecx, dword ptr [eax]
0x55019c: push 4
0x55019e: lea edx, [esp + 0x3c]
0x5501a2: push edx
0x5501a3: push ebp
0x5501a4: push ebp
0x5501a5: push 0x19
0x5501a7: push 0x64e310
0x5501ac: push eax
0x5501ad: call dword ptr [ecx + 0x10]
0x5501b0: test byte ptr [esi + 8], bl
0x5501b3: je 0x5501ce
0x5501b5: mov eax, dword ptr [esi + 0x1c]
0x5501b8: mov ecx, dword ptr [eax]
0x5501ba: push 4
0x5501bc: lea edx, [esp + 0x40]
0x5501c0: push edx
0x5501c1: push ebp
0x5501c2: push ebp
0x5501c3: push 0x16
0x5501c5: push 0x64e310
0x5501ca: push eax
0x5501cb: call dword ptr [ecx + 0x10]
0x5501ce: mov eax, dword ptr [esi + 0x18]
0x5501d1: mov ecx, dword ptr [eax]
0x5501d3: push eax
0x5501d4: call dword ptr [ecx + 8]
0x5501d7: mov dword ptr [esi + 0x18], ebp
0x5501da: lea edi, [esi + 0x1c]
0x5501dd: mov dword ptr [esp + 0x10], 0x33
0x5501e5: mov eax, dword ptr [edi]
0x5501e7: cmp eax, ebp
0x5501e9: je 0x550346
0x5501ef: mov ecx, 0xffffd8f0
0x5501f4: mov dword ptr [esp + 0x34], ecx
0x5501f8: mov dword ptr [esp + 0x30], ecx
0x5501fc: test byte ptr [esi + 8], 0x20
0x550200: mov dword ptr [esp + 0x28], ebp
0x550204: mov dword ptr [esp + 0x20], ebp
0x550208: mov dword ptr [esp + 0x3c], ebp
0x55020c: mov dword ptr [esp + 0x38], ebp
0x550210: mov dword ptr [esp + 0x2c], 0
0x550218: mov dword ptr [esp + 0x24], 0
0x550220: mov dword ptr [esp + 0x1c], 0
0x550228: je 0x550240
0x55022a: mov edx, dword ptr [eax]
0x55022c: push 4
0x55022e: lea ecx, [esp + 0x40]
0x550232: push ecx
0x550233: push ebp
0x550234: push ebp
0x550235: push 5
0x550237: push 0x64e320
0x55023c: push eax
0x55023d: call dword ptr [edx + 0x10]
0x550240: test byte ptr [esi + 8], 0x40
0x550244: je 0x55025e
0x550246: mov eax, dword ptr [edi]
0x550248: mov edx, dword ptr [eax]
0x55024a: push 4
0x55024c: lea ecx, [esp + 0x3c]
0x550250: push ecx
0x550251: push ebp
0x550252: push ebp
0x550253: push 6
0x550255: push 0x64e320
0x55025a: push eax
0x55025b: call dword ptr [edx + 0x10]
0x55025e: mov al, byte ptr [esi + 8]
0x550261: test al, al
0x550263: jns 0x55027d
0x550265: mov eax, dword ptr [edi]
0x550267: mov edx, dword ptr [eax]
0x550269: push 4
0x55026b: lea ecx, [esp + 0x38]
0x55026f: push ecx
0x550270: push ebp
0x550271: push ebp
0x550272: push 7
0x550274: push 0x64e320
0x550279: push eax
0x55027a: call dword ptr [edx + 0x10]
0x55027d: mov eax, dword ptr [esi + 8]
0x550280: mov ebx, 0x100
0x550285: test ebx, eax
0x550287: je 0x5502be
0x550289: mov eax, dword ptr [edi]
0x55028b: mov edx, dword ptr [eax]
0x55028d: push 4
0x55028f: lea ecx, [esp + 0x34]
0x550293: push ecx
0x550294: push ebp
0x550295: push ebp
0x550296: push 8
0x550298: push 0x64e320
0x55029d: push eax
0x55029e: call dword ptr [edx + 0x10]
0x5502a1: test dword ptr [esi + 8], ebx
0x5502a4: je 0x5502be
0x5502a6: mov eax, dword ptr [edi]
0x5502a8: mov edx, dword ptr [eax]
0x5502aa: push 4
0x5502ac: lea ecx, [esp + 0x30]
0x5502b0: push ecx
0x5502b1: push ebp
0x5502b2: push ebp
0x5502b3: push 0x14
0x5502b5: push 0x64e320
0x5502ba: push eax
0x5502bb: call dword ptr [edx + 0x10]
0x5502be: mov eax, dword ptr [esi + 8]
0x5502c1: test ah, 2
0x5502c4: je 0x5502de
0x5502c6: mov eax, dword ptr [edi]
0x5502c8: mov edx, dword ptr [eax]
0x5502ca: push 4
0x5502cc: lea ecx, [esp + 0x2c]
0x5502d0: push ecx
0x5502d1: push ebp
0x5502d2: push ebp
0x5502d3: push 9
0x5502d5: push 0x64e320
0x5502da: push eax
0x5502db: call dword ptr [edx + 0x10]
0x5502de: mov eax, dword ptr [esi + 8]
0x5502e1: test ah, 4
0x5502e4: je 0x5502fe
0x5502e6: mov eax, dword ptr [edi]
0x5502e8: mov edx, dword ptr [eax]
0x5502ea: push 4
0x5502ec: lea ecx, [esp + 0x28]
0x5502f0: push ecx
0x5502f1: push ebp
0x5502f2: push ebp
0x5502f3: push 0xa
0x5502f5: push 0x64e320
0x5502fa: push eax
0x5502fb: call dword ptr [edx + 0x10]
0x5502fe: mov eax, dword ptr [esi + 8]
0x550301: test ah, 8
0x550304: je 0x55031e
0x550306: mov eax, dword ptr [edi]
0x550308: mov edx, dword ptr [eax]
0x55030a: push 4
0x55030c: lea ecx, [esp + 0x24]
0x550310: push ecx
0x550311: push ebp
0x550312: push ebp
0x550313: push 0xb
0x550315: push 0x64e320
0x55031a: push eax
0x55031b: call dword ptr [edx + 0x10]
0x55031e: mov eax, dword ptr [esi + 8]
0x550321: test ah, 0x10
0x550324: je 0x55033e
0x550326: mov eax, dword ptr [edi]
0x550328: mov edx, dword ptr [eax]
0x55032a: push 4
0x55032c: lea ecx, [esp + 0x20]
0x550330: push ecx
0x550331: push ebp
0x550332: push ebp
0x550333: push 0xc
0x550335: push 0x64e320
0x55033a: push eax
0x55033b: call dword ptr [edx + 0x10]
0x55033e: mov eax, dword ptr [edi]
0x550340: mov edx, dword ptr [eax]
0x550342: push eax
0x550343: call dword ptr [edx + 8]
0x550346: mov eax, dword ptr [esp + 0x10]
0x55034a: mov dword ptr [edi], ebp
0x55034c: add edi, 4
0x55034f: dec eax
0x550350: mov dword ptr [esp + 0x10], eax
0x550354: jne 0x5501e5
0x55035a: pop edi
0x55035b: pop esi
0x55035c: pop ebp
0x55035d: pop ebx
0x55035e: add esp, 0x30
0x550361: ret 
#endif

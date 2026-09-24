// sound_eax20_effect_initialize  (Ghidra: sound_eax20_effect_initialize, already named, __thiscall)
// address 0x54f270, size 1126 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Initializes the EAX 2.0 sound effects object by
// querying IKsPropertySet support for listener and buffer reverb properties and reports whether
// both are fully supported."; this (sound_eax_effect_object) fields match exactly, as in
// sound_eax20_effect_shutdown.c (0x54ef30). The ~22 near-identical QuerySupport calls (14
// listener + 8 buffer) are folded into two small loops over an {id, bit} table, transcribed in
// the exact order/values Ghidra shows -- including the apparently-redundant repeat queries for
// listener ids 8 and 15, which are preserved rather than deduplicated. Bit 0x80000000 (property
// id 0x80000000, "AllProperties" by EAX SDK convention) sets result bit 1.
// register convention: __thiscall (ECX -> this), stack -> listener (directsound_channel *).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax_property_set_guid[16];      // 0x0064e20c, IID for QueryInterface
extern const uint8_t sound_eax20_listener_property_guid[16]; // 0x0064e2f0
extern const uint8_t sound_eax20_buffer_property_guid[16];   // 0x0064e300


typedef struct sound_eax20_query { uint32_t id; uint32_t bit; } sound_eax20_query;

static const sound_eax20_query k_listener_queries[14] = {
    {2, 0x00000004}, {3, 0x00000008}, {4, 0x00000010}, {5, 0x00000020}, {6, 0x00000040},
    {7, 0x00000080}, {8, 0x00000100}, {9, 0x00000200}, {10, 0x00000400}, {13, 0x00002000},
    {8, 0x00000100}, {15, 0x00008000}, {15, 0x00008000}, {0x80000000u, 0x00000001},
};
static const sound_eax20_query k_buffer_queries[8] = {
    {2, 0x00000004}, {3, 0x00000008}, {4, 0x00000010}, {6, 0x00000040},
    {7, 0x00000080}, {8, 0x00000100}, {9, 0x00000200}, {0x80000000u, 0x00000001},
};

#define SOUND_EAX20_LISTENER_REQUIRED 0x000027fcu /* bits for ids 2..10,13 */
#define SOUND_EAX20_CHANNEL_REQUIRED  0x000007dcu /* bits for ids 2,3,4,6,7,8,9,10 */

int __thiscall sound_eax20_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener,
    int32_t unused) // `ret 8`: the second stack word (the channel index from the caller) is not read
{
    (void)unused;
    int32_t i;
    void *property_set;
    sound_query_support_fn query;
    uint32_t out_value;
    uint32_t supported;

    this_object->base.property_set = 0;
    this_object->base.supported_properties = 0;
    this_object->base.listener_supported = 0;
    this_object->base.channel_supported = 0;

    if (listener == 0 || listener->buffer_3d == 0) {
        return 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        this_object->channel_property_sets[i] = 0;
    }

    if (((int32_t (*)(void *, const uint8_t *, void **))(*(void ***)listener->buffer_3d)[0])(listener->buffer_3d,
            sound_eax_property_set_guid, &this_object->base.property_set) >= 0) {
        property_set = this_object->base.property_set;
        query = (sound_query_support_fn)(*(void ***)property_set)[5];

        for (i = 0; i < 14; i++) {
            if (query(property_set, sound_eax20_listener_property_guid, k_listener_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_listener_queries[i].bit;
            }
        }
        for (i = 0; i < 8; i++) {
            if (query(property_set, sound_eax20_buffer_property_guid, k_buffer_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_buffer_queries[i].bit;
            }
        }
    } else {
        this_object->base.property_set = 0;
    }

    supported = this_object->base.supported_properties;
    this_object->base.listener_supported = (supported & SOUND_EAX20_LISTENER_REQUIRED) == SOUND_EAX20_LISTENER_REQUIRED;
    this_object->base.channel_supported = (supported & SOUND_EAX20_CHANNEL_REQUIRED) == SOUND_EAX20_CHANNEL_REQUIRED;

    return this_object->base.listener_supported && this_object->base.channel_supported;
}

#if 0
Original Ghidra decompilation (0x54f270): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x54f270.md for the full 1126-byte listing (14 listener + 8 buffer
QuerySupport calls, folded into the two tables above).

Disassembly (0x54f270..0x54f6d6, capstone; phase-4 review):

0x54f270: mov edx, dword ptr [esp + 4]
0x54f274: xor eax, eax
0x54f276: cmp edx, eax
0x54f278: push ebp
0x54f279: mov ebp, ecx
0x54f27b: push esi
0x54f27c: lea esi, [ebp + 0x18]
0x54f27f: mov dword ptr [esi], eax
0x54f281: mov dword ptr [ebp + 8], eax
0x54f284: mov dword ptr [ebp + 0x10], eax
0x54f287: mov dword ptr [ebp + 0x14], eax
0x54f28a: je 0x54f6cf
0x54f290: cmp dword ptr [edx + 0x674], eax
0x54f296: je 0x54f6cf
0x54f29c: push edi
0x54f29d: push esi
0x54f29e: mov ecx, 0x33
0x54f2a3: lea edi, [ebp + 0x1c]
0x54f2a6: rep stosd dword ptr es:[edi], eax
0x54f2a8: mov eax, dword ptr [edx + 0x674]
0x54f2ae: mov ecx, dword ptr [eax]
0x54f2b0: push 0x64e20c
0x54f2b5: push eax
0x54f2b6: call dword ptr [ecx]
0x54f2b8: test eax, eax
0x54f2ba: jl 0x54f649
0x54f2c0: mov eax, dword ptr [esi]
0x54f2c2: lea ecx, [esp + 0x10]
0x54f2c6: push ecx
0x54f2c7: push 2
0x54f2c9: push 0x64e2f0
0x54f2ce: mov dword ptr [esp + 0x1c], 0
0x54f2d6: mov edx, dword ptr [eax]
0x54f2d8: push eax
0x54f2d9: call dword ptr [edx + 0x14]
0x54f2dc: test eax, eax
0x54f2de: jl 0x54f2f0
0x54f2e0: mov edx, dword ptr [esp + 0x10]
0x54f2e4: and edx, 3
0x54f2e7: cmp dl, 3
0x54f2ea: jne 0x54f2f0
0x54f2ec: or dword ptr [ebp + 8], 4
0x54f2f0: mov eax, dword ptr [esi]
0x54f2f2: mov ecx, dword ptr [eax]
0x54f2f4: lea edx, [esp + 0x10]
0x54f2f8: push edx
0x54f2f9: push 3
0x54f2fb: push 0x64e2f0
0x54f300: push eax
0x54f301: call dword ptr [ecx + 0x14]
0x54f304: test eax, eax
0x54f306: jl 0x54f317
0x54f308: mov eax, dword ptr [esp + 0x10]
0x54f30c: and eax, 3
0x54f30f: cmp al, 3
0x54f311: jne 0x54f317
0x54f313: or dword ptr [ebp + 8], 8
0x54f317: mov eax, dword ptr [esi]
0x54f319: mov ecx, dword ptr [eax]
0x54f31b: lea edx, [esp + 0x10]
0x54f31f: push edx
0x54f320: push 4
0x54f322: push 0x64e2f0
0x54f327: push eax
0x54f328: call dword ptr [ecx + 0x14]
0x54f32b: test eax, eax
0x54f32d: jl 0x54f33e
0x54f32f: mov eax, dword ptr [esp + 0x10]
0x54f333: and eax, 3
0x54f336: cmp al, 3
0x54f338: jne 0x54f33e
0x54f33a: or dword ptr [ebp + 8], 0x10
0x54f33e: mov eax, dword ptr [esi]
0x54f340: mov ecx, dword ptr [eax]
0x54f342: lea edx, [esp + 0x10]
0x54f346: push edx
0x54f347: push 5
0x54f349: push 0x64e2f0
0x54f34e: push eax
0x54f34f: call dword ptr [ecx + 0x14]
0x54f352: test eax, eax
0x54f354: jl 0x54f365
0x54f356: mov eax, dword ptr [esp + 0x10]
0x54f35a: and eax, 3
0x54f35d: cmp al, 3
0x54f35f: jne 0x54f365
0x54f361: or dword ptr [ebp + 8], 0x20
0x54f365: mov eax, dword ptr [esi]
0x54f367: mov ecx, dword ptr [eax]
0x54f369: lea edx, [esp + 0x10]
0x54f36d: push edx
0x54f36e: push 6
0x54f370: push 0x64e2f0
0x54f375: push eax
0x54f376: call dword ptr [ecx + 0x14]
0x54f379: test eax, eax
0x54f37b: jl 0x54f38c
0x54f37d: mov eax, dword ptr [esp + 0x10]
0x54f381: and eax, 3
0x54f384: cmp al, 3
0x54f386: jne 0x54f38c
0x54f388: or dword ptr [ebp + 8], 0x40
0x54f38c: mov eax, dword ptr [esi]
0x54f38e: mov ecx, dword ptr [eax]
0x54f390: lea edx, [esp + 0x10]
0x54f394: push edx
0x54f395: push 7
0x54f397: push 0x64e2f0
0x54f39c: push eax
0x54f39d: call dword ptr [ecx + 0x14]
0x54f3a0: test eax, eax
0x54f3a2: jl 0x54f3b6
0x54f3a4: mov eax, dword ptr [esp + 0x10]
0x54f3a8: and eax, 3
0x54f3ab: cmp al, 3
0x54f3ad: jne 0x54f3b6
0x54f3af: or dword ptr [ebp + 8], 0x80
0x54f3b6: mov eax, dword ptr [esi]
0x54f3b8: mov ecx, dword ptr [eax]
0x54f3ba: lea edx, [esp + 0x10]
0x54f3be: push edx
0x54f3bf: push 8
0x54f3c1: push 0x64e2f0
0x54f3c6: push eax
0x54f3c7: call dword ptr [ecx + 0x14]
0x54f3ca: test eax, eax
0x54f3cc: jl 0x54f3e0
0x54f3ce: mov eax, dword ptr [esp + 0x10]
0x54f3d2: and eax, 3
0x54f3d5: cmp al, 3
0x54f3d7: jne 0x54f3e0
0x54f3d9: or dword ptr [ebp + 8], 0x100
0x54f3e0: mov eax, dword ptr [esi]
0x54f3e2: mov ecx, dword ptr [eax]
0x54f3e4: lea edx, [esp + 0x10]
0x54f3e8: push edx
0x54f3e9: push 9
0x54f3eb: push 0x64e2f0
0x54f3f0: push eax
0x54f3f1: call dword ptr [ecx + 0x14]
0x54f3f4: test eax, eax
0x54f3f6: jl 0x54f40a
0x54f3f8: mov eax, dword ptr [esp + 0x10]
0x54f3fc: and eax, 3
0x54f3ff: cmp al, 3
0x54f401: jne 0x54f40a
0x54f403: or dword ptr [ebp + 8], 0x200
0x54f40a: mov eax, dword ptr [esi]
0x54f40c: mov ecx, dword ptr [eax]
0x54f40e: lea edx, [esp + 0x10]
0x54f412: push edx
0x54f413: push 0xa
0x54f415: push 0x64e2f0
0x54f41a: push eax
0x54f41b: call dword ptr [ecx + 0x14]
0x54f41e: test eax, eax
0x54f420: jl 0x54f434
0x54f422: mov eax, dword ptr [esp + 0x10]
0x54f426: and eax, 3
0x54f429: cmp al, 3
0x54f42b: jne 0x54f434
0x54f42d: or dword ptr [ebp + 8], 0x400
0x54f434: mov eax, dword ptr [esi]
0x54f436: mov ecx, dword ptr [eax]
0x54f438: lea edx, [esp + 0x10]
0x54f43c: push edx
0x54f43d: push 0xd
0x54f43f: push 0x64e2f0
0x54f444: push eax
0x54f445: call dword ptr [ecx + 0x14]
0x54f448: test eax, eax
0x54f44a: jl 0x54f45e
0x54f44c: mov eax, dword ptr [esp + 0x10]
0x54f450: and eax, 3
0x54f453: cmp al, 3
0x54f455: jne 0x54f45e
0x54f457: or dword ptr [ebp + 8], 0x2000
0x54f45e: mov eax, dword ptr [esi]
0x54f460: mov ecx, dword ptr [eax]
0x54f462: lea edx, [esp + 0x10]
0x54f466: push edx
0x54f467: push 8
0x54f469: push 0x64e2f0
0x54f46e: push eax
0x54f46f: call dword ptr [ecx + 0x14]
0x54f472: test eax, eax
0x54f474: jl 0x54f488
0x54f476: mov eax, dword ptr [esp + 0x10]
0x54f47a: and eax, 3
0x54f47d: cmp al, 3
0x54f47f: jne 0x54f488
0x54f481: or dword ptr [ebp + 8], 0x100
0x54f488: mov eax, dword ptr [esi]
0x54f48a: mov ecx, dword ptr [eax]
0x54f48c: lea edx, [esp + 0x10]
0x54f490: push edx
0x54f491: push 0xf
0x54f493: push 0x64e2f0
0x54f498: push eax
0x54f499: call dword ptr [ecx + 0x14]
0x54f49c: test eax, eax
0x54f49e: mov edi, 0x8000
0x54f4a3: jl 0x54f4b3
0x54f4a5: mov eax, dword ptr [esp + 0x10]
0x54f4a9: and eax, 3
0x54f4ac: cmp al, 3
0x54f4ae: jne 0x54f4b3
0x54f4b0: or dword ptr [ebp + 8], edi
0x54f4b3: mov eax, dword ptr [esi]
0x54f4b5: mov ecx, dword ptr [eax]
0x54f4b7: lea edx, [esp + 0x10]
0x54f4bb: push edx
0x54f4bc: push 0xf
0x54f4be: push 0x64e2f0
0x54f4c3: push eax
0x54f4c4: call dword ptr [ecx + 0x14]
0x54f4c7: test eax, eax
0x54f4c9: jl 0x54f4d9
0x54f4cb: mov eax, dword ptr [esp + 0x10]
0x54f4cf: and eax, 3
0x54f4d2: cmp al, 3
0x54f4d4: jne 0x54f4d9
0x54f4d6: or dword ptr [ebp + 8], edi
0x54f4d9: mov eax, dword ptr [esi]
0x54f4db: mov ecx, dword ptr [eax]
0x54f4dd: lea edx, [esp + 0x10]
0x54f4e1: push edx
0x54f4e2: push 0x80000000
0x54f4e7: push 0x64e2f0
0x54f4ec: push eax
0x54f4ed: call dword ptr [ecx + 0x14]
0x54f4f0: test eax, eax
0x54f4f2: jl 0x54f503
0x54f4f4: mov eax, dword ptr [esp + 0x10]
0x54f4f8: and eax, 3
0x54f4fb: cmp al, 3
0x54f4fd: jne 0x54f503
0x54f4ff: or dword ptr [ebp + 8], 1
0x54f503: mov eax, dword ptr [esi]
0x54f505: mov ecx, dword ptr [eax]
0x54f507: lea edx, [esp + 0x10]
0x54f50b: push edx
0x54f50c: push 2
0x54f50e: push 0x64e300
0x54f513: push eax
0x54f514: call dword ptr [ecx + 0x14]
0x54f517: test eax, eax
0x54f519: jl 0x54f52a
0x54f51b: mov eax, dword ptr [esp + 0x10]
0x54f51f: and eax, 3
0x54f522: cmp al, 3
0x54f524: jne 0x54f52a
0x54f526: or dword ptr [ebp + 8], 4
0x54f52a: mov eax, dword ptr [esi]
0x54f52c: mov ecx, dword ptr [eax]
0x54f52e: lea edx, [esp + 0x10]
0x54f532: push edx
0x54f533: push 3
0x54f535: push 0x64e300
0x54f53a: push eax
0x54f53b: call dword ptr [ecx + 0x14]
0x54f53e: test eax, eax
0x54f540: jl 0x54f551
0x54f542: mov eax, dword ptr [esp + 0x10]
0x54f546: and eax, 3
0x54f549: cmp al, 3
0x54f54b: jne 0x54f551
0x54f54d: or dword ptr [ebp + 8], 8
0x54f551: mov eax, dword ptr [esi]
0x54f553: mov ecx, dword ptr [eax]
0x54f555: lea edx, [esp + 0x10]
0x54f559: push edx
0x54f55a: push 4
0x54f55c: push 0x64e300
0x54f561: push eax
0x54f562: call dword ptr [ecx + 0x14]
0x54f565: test eax, eax
0x54f567: jl 0x54f578
0x54f569: mov eax, dword ptr [esp + 0x10]
0x54f56d: and eax, 3
0x54f570: cmp al, 3
0x54f572: jne 0x54f578
0x54f574: or dword ptr [ebp + 8], 0x10
0x54f578: mov eax, dword ptr [esi]
0x54f57a: mov ecx, dword ptr [eax]
0x54f57c: lea edx, [esp + 0x10]
0x54f580: push edx
0x54f581: push 6
0x54f583: push 0x64e300
0x54f588: push eax
0x54f589: call dword ptr [ecx + 0x14]
0x54f58c: test eax, eax
0x54f58e: jl 0x54f59f
0x54f590: mov eax, dword ptr [esp + 0x10]
0x54f594: and eax, 3
0x54f597: cmp al, 3
0x54f599: jne 0x54f59f
0x54f59b: or dword ptr [ebp + 8], 0x40
0x54f59f: mov eax, dword ptr [esi]
0x54f5a1: mov ecx, dword ptr [eax]
0x54f5a3: lea edx, [esp + 0x10]
0x54f5a7: push edx
0x54f5a8: push 7
0x54f5aa: push 0x64e300
0x54f5af: push eax
0x54f5b0: call dword ptr [ecx + 0x14]
0x54f5b3: test eax, eax
0x54f5b5: jl 0x54f5c9
0x54f5b7: mov eax, dword ptr [esp + 0x10]
0x54f5bb: and eax, 3
0x54f5be: cmp al, 3
0x54f5c0: jne 0x54f5c9
0x54f5c2: or dword ptr [ebp + 8], 0x80
0x54f5c9: mov eax, dword ptr [esi]
0x54f5cb: mov ecx, dword ptr [eax]
0x54f5cd: lea edx, [esp + 0x10]
0x54f5d1: push edx
0x54f5d2: push 8
0x54f5d4: push 0x64e300
0x54f5d9: push eax
0x54f5da: call dword ptr [ecx + 0x14]
0x54f5dd: test eax, eax
0x54f5df: jl 0x54f5f3
0x54f5e1: mov eax, dword ptr [esp + 0x10]
0x54f5e5: and eax, 3
0x54f5e8: cmp al, 3
0x54f5ea: jne 0x54f5f3
0x54f5ec: or dword ptr [ebp + 8], 0x100
0x54f5f3: mov eax, dword ptr [esi]
0x54f5f5: mov ecx, dword ptr [eax]
0x54f5f7: lea edx, [esp + 0x10]
0x54f5fb: push edx
0x54f5fc: push 9
0x54f5fe: push 0x64e300
0x54f603: push eax
0x54f604: call dword ptr [ecx + 0x14]
0x54f607: test eax, eax
0x54f609: jl 0x54f61d
0x54f60b: mov eax, dword ptr [esp + 0x10]
0x54f60f: and eax, 3
0x54f612: cmp al, 3
0x54f614: jne 0x54f61d
0x54f616: or dword ptr [ebp + 8], 0x200
0x54f61d: mov esi, dword ptr [esi]
0x54f61f: mov ecx, dword ptr [esi]
0x54f621: lea edx, [esp + 0x10]
0x54f625: push edx
0x54f626: push 0x80000000
0x54f62b: push 0x64e300
0x54f630: push esi
0x54f631: call dword ptr [ecx + 0x14]
0x54f634: test eax, eax
0x54f636: jl 0x54f64f
0x54f638: mov eax, dword ptr [esp + 0x10]
0x54f63c: and eax, 3
0x54f63f: cmp al, 3
0x54f641: jne 0x54f64f
0x54f643: or dword ptr [ebp + 8], 1
0x54f647: jmp 0x54f64f
0x54f649: mov dword ptr [esi], 0
0x54f64f: mov eax, dword ptr [ebp + 8]
0x54f652: mov edx, eax
0x54f654: and edx, 4
0x54f657: pop edi
0x54f658: je 0x54f689
0x54f65a: test al, 8
0x54f65c: je 0x54f689
0x54f65e: test al, 0x10
0x54f660: je 0x54f689
0x54f662: test al, 0x20
0x54f664: je 0x54f689
0x54f666: test al, 0x40
0x54f668: je 0x54f689
0x54f66a: test al, al
0x54f66c: jns 0x54f689
0x54f66e: test ah, 1
0x54f671: je 0x54f689
0x54f673: test ah, 2
0x54f676: je 0x54f689
0x54f678: test ah, 4
0x54f67b: je 0x54f689
0x54f67d: test ah, 0x20
0x54f680: je 0x54f689
0x54f682: mov ecx, 1
0x54f687: jmp 0x54f68b
0x54f689: xor ecx, ecx
0x54f68b: test edx, edx
0x54f68d: mov dword ptr [ebp + 0x10], ecx
0x54f690: je 0x54f6b8
0x54f692: test al, 8
0x54f694: je 0x54f6b8
0x54f696: test al, 0x10
0x54f698: je 0x54f6b8
0x54f69a: test al, 0x40
0x54f69c: je 0x54f6b8
0x54f69e: test al, al
0x54f6a0: jns 0x54f6b8
0x54f6a2: test ah, 1
0x54f6a5: je 0x54f6b8
0x54f6a7: test ah, 2
0x54f6aa: je 0x54f6b8
0x54f6ac: test ah, 4
0x54f6af: je 0x54f6b8
0x54f6b1: mov eax, 1
0x54f6b6: jmp 0x54f6ba
0x54f6b8: xor eax, eax
0x54f6ba: test ecx, ecx
0x54f6bc: mov dword ptr [ebp + 0x14], eax
0x54f6bf: je 0x54f6cf
0x54f6c1: test eax, eax
0x54f6c3: je 0x54f6cf
0x54f6c5: pop esi
0x54f6c6: mov eax, 1
0x54f6cb: pop ebp
0x54f6cc: ret 8
0x54f6cf: pop esi
0x54f6d0: xor eax, eax
0x54f6d2: pop ebp
0x54f6d3: ret 8
#endif

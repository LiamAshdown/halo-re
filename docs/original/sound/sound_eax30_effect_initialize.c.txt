// sound_eax30_effect_initialize  (Ghidra: sound_eax30_effect_initialize, already named, __thiscall)
// address 0x550370, size 1300 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Initializes the EAX 3.0-style sound effects object,
// querying property-set support for listener and buffer reverb properties."; same shape as
// sound_eax20_effect_initialize.c (0x54f270), different ids/bits/required-set, and unlike EAX 2.0
// the final result is listener_supported OR channel_supported (not AND) -- confirmed directly
// from the decompiled `if (iVar5 != 0 || iVar3 != 0) return 1;`.
// register convention: __thiscall (ECX -> this_object), stack -> listener (directsound_channel *).
// The ~25 near-identical QuerySupport calls are folded into two loops over an {id, bit} table, in
// the exact order/values Ghidra shows.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax_property_set_guid[16];        // 0x0064e20c, IID for QueryInterface
extern const uint8_t sound_eax30_listener_property_guid[16]; // 0x0064e310
extern const uint8_t sound_eax30_buffer_property_guid[16];   // 0x0064e320

typedef struct sound_eax30_query { uint32_t id; uint32_t bit; } sound_eax30_query;

static const sound_eax30_query k_listener_queries[13] = {
    {5, 0x00000020}, {6, 0x00000040}, {0x18, 0x01000000}, {8, 0x00000100}, {9, 0x00000200},
    {0x0b, 0x00000800}, {0x0c, 0x00001000}, {0x0e, 0x00004000}, {0x0f, 0x00008000},
    {4, 0x00000010}, {0x19, 0x02000000}, {0x16, 0x00400000}, {0x80000000u, 0x00000001},
};
static const sound_eax30_query k_buffer_queries[12] = {
    {5, 0x00000020}, {6, 0x00000040}, {7, 0x00000080}, {8, 0x00000100}, {0x14, 0x00100000},
    {9, 0x00000200}, {0x0a, 0x00000400}, {0x0b, 0x00000800}, {0x0c, 0x00001000}, {0x0f, 0x00008000},
    {0x10, 0x00010000}, {0x80000000u, 0x00000001},
};

#define SOUND_EAX30_LISTENER_REQUIRED 0x0140db70u
#define SOUND_EAX30_CHANNEL_REQUIRED  0x00119fe0u

int __thiscall sound_eax30_effect_initialize(sound_eax_effect_object *this_object, directsound_channel *listener,
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

    if (((int32_t (__stdcall *)(void *, const uint8_t *, void **))(*(void ***)listener->buffer_3d)[0])(listener->buffer_3d,
            sound_eax_property_set_guid, &this_object->base.property_set) >= 0) {
        property_set = this_object->base.property_set;
        query = (sound_query_support_fn)(*(void ***)property_set)[5];

        for (i = 0; i < 13; i++) {
            if (query(property_set, sound_eax30_listener_property_guid, k_listener_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_listener_queries[i].bit;
            }
        }
        for (i = 0; i < 12; i++) {
            if (query(property_set, sound_eax30_buffer_property_guid, k_buffer_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_buffer_queries[i].bit;
            }
        }
    } else {
        this_object->base.property_set = 0;
    }

    supported = this_object->base.supported_properties;
    this_object->base.listener_supported = (supported & SOUND_EAX30_LISTENER_REQUIRED) == SOUND_EAX30_LISTENER_REQUIRED;
    this_object->base.channel_supported = (supported & SOUND_EAX30_CHANNEL_REQUIRED) == SOUND_EAX30_CHANNEL_REQUIRED;

    return this_object->base.listener_supported || this_object->base.channel_supported;
}

#if 0
Original Ghidra decompilation (0x550370): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x550370.md for the full 1300-byte listing, folded into the two tables
above.

Disassembly (0x550370..0x550884, capstone; phase-4 review):

0x550370: mov edx, dword ptr [esp + 4]
0x550374: xor eax, eax
0x550376: cmp edx, eax
0x550378: push ebp
0x550379: mov ebp, ecx
0x55037b: push esi
0x55037c: lea esi, [ebp + 0x18]
0x55037f: mov dword ptr [esi], eax
0x550381: mov dword ptr [ebp + 8], eax
0x550384: mov dword ptr [ebp + 0x10], eax
0x550387: mov dword ptr [ebp + 0x14], eax
0x55038a: je 0x55087d
0x550390: cmp dword ptr [edx + 0x674], eax
0x550396: je 0x55087d
0x55039c: push edi
0x55039d: push esi
0x55039e: mov ecx, 0x33
0x5503a3: lea edi, [ebp + 0x1c]
0x5503a6: rep stosd dword ptr es:[edi], eax
0x5503a8: mov eax, dword ptr [edx + 0x674]
0x5503ae: mov ecx, dword ptr [eax]
0x5503b0: push 0x64e20c
0x5503b5: push eax
0x5503b6: call dword ptr [ecx]
0x5503b8: test eax, eax
0x5503ba: mov edi, 0x20
0x5503bf: jl 0x5507d9
0x5503c5: mov eax, dword ptr [esi]
0x5503c7: lea ecx, [esp + 0x10]
0x5503cb: push ecx
0x5503cc: push 5
0x5503ce: push 0x64e310
0x5503d3: mov dword ptr [esp + 0x1c], 0
0x5503db: mov edx, dword ptr [eax]
0x5503dd: push eax
0x5503de: call dword ptr [edx + 0x14]
0x5503e1: test eax, eax
0x5503e3: jl 0x5503f4
0x5503e5: mov edx, dword ptr [esp + 0x10]
0x5503e9: and edx, 3
0x5503ec: cmp dl, 3
0x5503ef: jne 0x5503f4
0x5503f1: or dword ptr [ebp + 8], edi
0x5503f4: mov eax, dword ptr [esi]
0x5503f6: mov ecx, dword ptr [eax]
0x5503f8: lea edx, [esp + 0x10]
0x5503fc: push edx
0x5503fd: push 6
0x5503ff: push 0x64e310
0x550404: push eax
0x550405: call dword ptr [ecx + 0x14]
0x550408: test eax, eax
0x55040a: jl 0x55041b
0x55040c: mov eax, dword ptr [esp + 0x10]
0x550410: and eax, 3
0x550413: cmp al, 3
0x550415: jne 0x55041b
0x550417: or dword ptr [ebp + 8], 0x40
0x55041b: mov eax, dword ptr [esi]
0x55041d: mov ecx, dword ptr [eax]
0x55041f: lea edx, [esp + 0x10]
0x550423: push edx
0x550424: push 0x18
0x550426: push 0x64e310
0x55042b: push eax
0x55042c: call dword ptr [ecx + 0x14]
0x55042f: test eax, eax
0x550431: jl 0x550445
0x550433: mov eax, dword ptr [esp + 0x10]
0x550437: and eax, 3
0x55043a: cmp al, 3
0x55043c: jne 0x550445
0x55043e: or dword ptr [ebp + 8], 0x1000000
0x550445: mov eax, dword ptr [esi]
0x550447: mov ecx, dword ptr [eax]
0x550449: lea edx, [esp + 0x10]
0x55044d: push edx
0x55044e: push 8
0x550450: push 0x64e310
0x550455: push eax
0x550456: call dword ptr [ecx + 0x14]
0x550459: test eax, eax
0x55045b: jl 0x55046f
0x55045d: mov eax, dword ptr [esp + 0x10]
0x550461: and eax, 3
0x550464: cmp al, 3
0x550466: jne 0x55046f
0x550468: or dword ptr [ebp + 8], 0x100
0x55046f: mov eax, dword ptr [esi]
0x550471: mov ecx, dword ptr [eax]
0x550473: lea edx, [esp + 0x10]
0x550477: push edx
0x550478: push 9
0x55047a: push 0x64e310
0x55047f: push eax
0x550480: call dword ptr [ecx + 0x14]
0x550483: test eax, eax
0x550485: jl 0x550499
0x550487: mov eax, dword ptr [esp + 0x10]
0x55048b: and eax, 3
0x55048e: cmp al, 3
0x550490: jne 0x550499
0x550492: or dword ptr [ebp + 8], 0x200
0x550499: mov eax, dword ptr [esi]
0x55049b: mov ecx, dword ptr [eax]
0x55049d: lea edx, [esp + 0x10]
0x5504a1: push edx
0x5504a2: push 0xb
0x5504a4: push 0x64e310
0x5504a9: push eax
0x5504aa: call dword ptr [ecx + 0x14]
0x5504ad: test eax, eax
0x5504af: jl 0x5504c3
0x5504b1: mov eax, dword ptr [esp + 0x10]
0x5504b5: and eax, 3
0x5504b8: cmp al, 3
0x5504ba: jne 0x5504c3
0x5504bc: or dword ptr [ebp + 8], 0x800
0x5504c3: mov eax, dword ptr [esi]
0x5504c5: mov ecx, dword ptr [eax]
0x5504c7: lea edx, [esp + 0x10]
0x5504cb: push edx
0x5504cc: push 0xc
0x5504ce: push 0x64e310
0x5504d3: push eax
0x5504d4: call dword ptr [ecx + 0x14]
0x5504d7: test eax, eax
0x5504d9: jl 0x5504ed
0x5504db: mov eax, dword ptr [esp + 0x10]
0x5504df: and eax, 3
0x5504e2: cmp al, 3
0x5504e4: jne 0x5504ed
0x5504e6: or dword ptr [ebp + 8], 0x1000
0x5504ed: mov eax, dword ptr [esi]
0x5504ef: mov ecx, dword ptr [eax]
0x5504f1: lea edx, [esp + 0x10]
0x5504f5: push edx
0x5504f6: push 0xe
0x5504f8: push 0x64e310
0x5504fd: push eax
0x5504fe: call dword ptr [ecx + 0x14]
0x550501: test eax, eax
0x550503: jl 0x550517
0x550505: mov eax, dword ptr [esp + 0x10]
0x550509: and eax, 3
0x55050c: cmp al, 3
0x55050e: jne 0x550517
0x550510: or dword ptr [ebp + 8], 0x4000
0x550517: mov eax, dword ptr [esi]
0x550519: mov ecx, dword ptr [eax]
0x55051b: lea edx, [esp + 0x10]
0x55051f: push edx
0x550520: push 0xf
0x550522: push 0x64e310
0x550527: push eax
0x550528: call dword ptr [ecx + 0x14]
0x55052b: test eax, eax
0x55052d: jl 0x550541
0x55052f: mov eax, dword ptr [esp + 0x10]
0x550533: and eax, 3
0x550536: cmp al, 3
0x550538: jne 0x550541
0x55053a: or dword ptr [ebp + 8], 0x8000
0x550541: mov eax, dword ptr [esi]
0x550543: mov ecx, dword ptr [eax]
0x550545: lea edx, [esp + 0x10]
0x550549: push edx
0x55054a: push 4
0x55054c: push 0x64e310
0x550551: push eax
0x550552: call dword ptr [ecx + 0x14]
0x550555: test eax, eax
0x550557: jl 0x550568
0x550559: mov eax, dword ptr [esp + 0x10]
0x55055d: and eax, 3
0x550560: cmp al, 3
0x550562: jne 0x550568
0x550564: or dword ptr [ebp + 8], 0x10
0x550568: mov eax, dword ptr [esi]
0x55056a: mov ecx, dword ptr [eax]
0x55056c: lea edx, [esp + 0x10]
0x550570: push edx
0x550571: push 0x19
0x550573: push 0x64e310
0x550578: push eax
0x550579: call dword ptr [ecx + 0x14]
0x55057c: test eax, eax
0x55057e: jl 0x550592
0x550580: mov eax, dword ptr [esp + 0x10]
0x550584: and eax, 3
0x550587: cmp al, 3
0x550589: jne 0x550592
0x55058b: or dword ptr [ebp + 8], 0x2000000
0x550592: mov eax, dword ptr [esi]
0x550594: mov ecx, dword ptr [eax]
0x550596: lea edx, [esp + 0x10]
0x55059a: push edx
0x55059b: push 0x16
0x55059d: push 0x64e310
0x5505a2: push eax
0x5505a3: call dword ptr [ecx + 0x14]
0x5505a6: test eax, eax
0x5505a8: jl 0x5505bc
0x5505aa: mov eax, dword ptr [esp + 0x10]
0x5505ae: and eax, 3
0x5505b1: cmp al, 3
0x5505b3: jne 0x5505bc
0x5505b5: or dword ptr [ebp + 8], 0x400000
0x5505bc: mov eax, dword ptr [esi]
0x5505be: mov ecx, dword ptr [eax]
0x5505c0: lea edx, [esp + 0x10]
0x5505c4: push edx
0x5505c5: push 0x80000000
0x5505ca: push 0x64e310
0x5505cf: push eax
0x5505d0: call dword ptr [ecx + 0x14]
0x5505d3: test eax, eax
0x5505d5: jl 0x5505e6
0x5505d7: mov eax, dword ptr [esp + 0x10]
0x5505db: and eax, 3
0x5505de: cmp al, 3
0x5505e0: jne 0x5505e6
0x5505e2: or dword ptr [ebp + 8], 1
0x5505e6: mov eax, dword ptr [esi]
0x5505e8: mov ecx, dword ptr [eax]
0x5505ea: lea edx, [esp + 0x10]
0x5505ee: push edx
0x5505ef: push 5
0x5505f1: push 0x64e320
0x5505f6: push eax
0x5505f7: call dword ptr [ecx + 0x14]
0x5505fa: test eax, eax
0x5505fc: jl 0x55060c
0x5505fe: mov eax, dword ptr [esp + 0x10]
0x550602: and eax, 3
0x550605: cmp al, 3
0x550607: jne 0x55060c
0x550609: or dword ptr [ebp + 8], edi
0x55060c: mov eax, dword ptr [esi]
0x55060e: mov ecx, dword ptr [eax]
0x550610: lea edx, [esp + 0x10]
0x550614: push edx
0x550615: push 6
0x550617: push 0x64e320
0x55061c: push eax
0x55061d: call dword ptr [ecx + 0x14]
0x550620: test eax, eax
0x550622: jl 0x550633
0x550624: mov eax, dword ptr [esp + 0x10]
0x550628: and eax, 3
0x55062b: cmp al, 3
0x55062d: jne 0x550633
0x55062f: or dword ptr [ebp + 8], 0x40
0x550633: mov eax, dword ptr [esi]
0x550635: mov ecx, dword ptr [eax]
0x550637: lea edx, [esp + 0x10]
0x55063b: push edx
0x55063c: push 7
0x55063e: push 0x64e320
0x550643: push eax
0x550644: call dword ptr [ecx + 0x14]
0x550647: test eax, eax
0x550649: jl 0x55065d
0x55064b: mov eax, dword ptr [esp + 0x10]
0x55064f: and eax, 3
0x550652: cmp al, 3
0x550654: jne 0x55065d
0x550656: or dword ptr [ebp + 8], 0x80
0x55065d: mov eax, dword ptr [esi]
0x55065f: mov ecx, dword ptr [eax]
0x550661: lea edx, [esp + 0x10]
0x550665: push edx
0x550666: push 8
0x550668: push 0x64e320
0x55066d: push eax
0x55066e: call dword ptr [ecx + 0x14]
0x550671: test eax, eax
0x550673: jl 0x550687
0x550675: mov eax, dword ptr [esp + 0x10]
0x550679: and eax, 3
0x55067c: cmp al, 3
0x55067e: jne 0x550687
0x550680: or dword ptr [ebp + 8], 0x100
0x550687: mov eax, dword ptr [esi]
0x550689: mov ecx, dword ptr [eax]
0x55068b: lea edx, [esp + 0x10]
0x55068f: push edx
0x550690: push 0x14
0x550692: push 0x64e320
0x550697: push eax
0x550698: call dword ptr [ecx + 0x14]
0x55069b: test eax, eax
0x55069d: jl 0x5506b1
0x55069f: mov eax, dword ptr [esp + 0x10]
0x5506a3: and eax, 3
0x5506a6: cmp al, 3
0x5506a8: jne 0x5506b1
0x5506aa: or dword ptr [ebp + 8], 0x100000
0x5506b1: mov eax, dword ptr [esi]
0x5506b3: mov ecx, dword ptr [eax]
0x5506b5: lea edx, [esp + 0x10]
0x5506b9: push edx
0x5506ba: push 9
0x5506bc: push 0x64e320
0x5506c1: push eax
0x5506c2: call dword ptr [ecx + 0x14]
0x5506c5: test eax, eax
0x5506c7: jl 0x5506db
0x5506c9: mov eax, dword ptr [esp + 0x10]
0x5506cd: and eax, 3
0x5506d0: cmp al, 3
0x5506d2: jne 0x5506db
0x5506d4: or dword ptr [ebp + 8], 0x200
0x5506db: mov eax, dword ptr [esi]
0x5506dd: mov ecx, dword ptr [eax]
0x5506df: lea edx, [esp + 0x10]
0x5506e3: push edx
0x5506e4: push 0xa
0x5506e6: push 0x64e320
0x5506eb: push eax
0x5506ec: call dword ptr [ecx + 0x14]
0x5506ef: test eax, eax
0x5506f1: jl 0x550705
0x5506f3: mov eax, dword ptr [esp + 0x10]
0x5506f7: and eax, 3
0x5506fa: cmp al, 3
0x5506fc: jne 0x550705
0x5506fe: or dword ptr [ebp + 8], 0x400
0x550705: mov eax, dword ptr [esi]
0x550707: mov ecx, dword ptr [eax]
0x550709: lea edx, [esp + 0x10]
0x55070d: push edx
0x55070e: push 0xb
0x550710: push 0x64e320
0x550715: push eax
0x550716: call dword ptr [ecx + 0x14]
0x550719: test eax, eax
0x55071b: jl 0x55072f
0x55071d: mov eax, dword ptr [esp + 0x10]
0x550721: and eax, 3
0x550724: cmp al, 3
0x550726: jne 0x55072f
0x550728: or dword ptr [ebp + 8], 0x800
0x55072f: mov eax, dword ptr [esi]
0x550731: mov ecx, dword ptr [eax]
0x550733: lea edx, [esp + 0x10]
0x550737: push edx
0x550738: push 0xc
0x55073a: push 0x64e320
0x55073f: push eax
0x550740: call dword ptr [ecx + 0x14]
0x550743: test eax, eax
0x550745: jl 0x550759
0x550747: mov eax, dword ptr [esp + 0x10]
0x55074b: and eax, 3
0x55074e: cmp al, 3
0x550750: jne 0x550759
0x550752: or dword ptr [ebp + 8], 0x1000
0x550759: mov eax, dword ptr [esi]
0x55075b: mov ecx, dword ptr [eax]
0x55075d: lea edx, [esp + 0x10]
0x550761: push edx
0x550762: push 0xf
0x550764: push 0x64e320
0x550769: push eax
0x55076a: call dword ptr [ecx + 0x14]
0x55076d: test eax, eax
0x55076f: jl 0x550783
0x550771: mov eax, dword ptr [esp + 0x10]
0x550775: and eax, 3
0x550778: cmp al, 3
0x55077a: jne 0x550783
0x55077c: or dword ptr [ebp + 8], 0x8000
0x550783: mov eax, dword ptr [esi]
0x550785: mov ecx, dword ptr [eax]
0x550787: lea edx, [esp + 0x10]
0x55078b: push edx
0x55078c: push 0x10
0x55078e: push 0x64e320
0x550793: push eax
0x550794: call dword ptr [ecx + 0x14]
0x550797: test eax, eax
0x550799: jl 0x5507ad
0x55079b: mov eax, dword ptr [esp + 0x10]
0x55079f: and eax, 3
0x5507a2: cmp al, 3
0x5507a4: jne 0x5507ad
0x5507a6: or dword ptr [ebp + 8], 0x10000
0x5507ad: mov esi, dword ptr [esi]
0x5507af: mov ecx, dword ptr [esi]
0x5507b1: lea edx, [esp + 0x10]
0x5507b5: push edx
0x5507b6: push 0x80000000
0x5507bb: push 0x64e320
0x5507c0: push esi
0x5507c1: call dword ptr [ecx + 0x14]
0x5507c4: test eax, eax
0x5507c6: jl 0x5507df
0x5507c8: mov eax, dword ptr [esp + 0x10]
0x5507cc: and eax, 3
0x5507cf: cmp al, 3
0x5507d1: jne 0x5507df
0x5507d3: or dword ptr [ebp + 8], 1
0x5507d7: jmp 0x5507df
0x5507d9: mov dword ptr [esi], 0
0x5507df: mov eax, dword ptr [ebp + 8]
0x5507e2: mov edx, eax
0x5507e4: and edx, edi
0x5507e6: pop edi
0x5507e7: je 0x550823
0x5507e9: test al, 0x40
0x5507eb: je 0x550823
0x5507ed: test eax, 0x1000000
0x5507f2: je 0x550823
0x5507f4: test ah, 1
0x5507f7: je 0x550823
0x5507f9: test ah, 2
0x5507fc: je 0x550823
0x5507fe: test ah, 8
0x550801: je 0x550823
0x550803: test ah, 0x10
0x550806: je 0x550823
0x550808: test ah, 0x40
0x55080b: je 0x550823
0x55080d: test ah, ah
0x55080f: jns 0x550823
0x550811: test al, 0x10
0x550813: je 0x550823
0x550815: test eax, 0x400000
0x55081a: je 0x550823
0x55081c: mov ecx, 1
0x550821: jmp 0x550825
0x550823: xor ecx, ecx
0x550825: test edx, edx
0x550827: mov dword ptr [ebp + 0x10], ecx
0x55082a: je 0x550866
0x55082c: test al, 0x40
0x55082e: je 0x550866
0x550830: test al, al
0x550832: jns 0x550866
0x550834: test ah, 1
0x550837: je 0x550866
0x550839: test eax, 0x100000
0x55083e: je 0x550866
0x550840: test ah, 2
0x550843: je 0x550866
0x550845: test ah, 4
0x550848: je 0x550866
0x55084a: test ah, 8
0x55084d: je 0x550866
0x55084f: test ah, 0x10
0x550852: je 0x550866
0x550854: test ah, ah
0x550856: jns 0x550866
0x550858: test eax, 0x10000
0x55085d: je 0x550866
0x55085f: mov eax, 1
0x550864: jmp 0x550868
0x550866: xor eax, eax
0x550868: test ecx, ecx
0x55086a: mov dword ptr [ebp + 0x14], eax
0x55086d: jne 0x550873
0x55086f: test eax, eax
0x550871: je 0x55087d
0x550873: pop esi
0x550874: mov eax, 1
0x550879: pop ebp
0x55087a: ret 8
0x55087d: pop esi
0x55087e: xor eax, eax
0x550880: pop ebp
0x550881: ret 8
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

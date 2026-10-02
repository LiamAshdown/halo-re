// sound_eax20_effect_apply_listener  (Ghidra: sound_eax20_effect_apply_listener, already named,
// __thiscall; slot 7 (apply_listener) of the EAX2 vtable at 0x00671d04, called through the
// effects object by sound_listener_update 0x547070)
// address 0x54fa80, size 1188 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Applies a reverb environment's parameters to the
// active EAX 2.0 listener property set, converting several fields to millibels first."; field
// offsets match types/tags.h SoundEnvironment exactly (room_intensity 0x08, room_intensity_hf
// 0x0c, room_rolloff 0x10, decay_time 0x14, decay_hf_ratio 0x18, reflections_intensity 0x1c,
// reflections_delay 0x20, reverb_intensity 0x24, reverb_delay 0x28, diffusion 0x2c). The
// millibel conversion is the same log10*2000-then-clamp shape as sound_gain_to_millibels.c
// (0x54eec0), just with three different clamp ranges ([-10000,0], [-10000,1000],
// [-10000,2000]). supported_properties bit 0 selects "deferred" property ids (id | 0x80000000,
// per types/sound.h's own note on that bit) instead of a second code path.
// Phase-4 review (disassembly appended below): the properties go to channel_property_sets[0]
// (this + 0x1c), not base.property_set (+0x18); REFLECTIONS and REVERB add 1000 / 2000 mB to
// 2000 * log10(v) before clamping (the draft only clamped). Set order and ids (2..10, 13, 15
// with the constant 0) confirmed.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax20_listener_property_guid[16]; // 0x0064e2f0
extern uint8_t directsound_deferred_dirty; // 0x00746132

extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call

// mode: 0 = pass through as-is, 1/2/3 = 2000*log10(value) clamped to [-10000, 0/1000/2000]
typedef struct sound_eax20_listener_field { uint32_t bit; uint32_t id; int32_t offset; int32_t mode; } sound_eax20_listener_field;

static const sound_eax20_listener_field k_fields[11] = {
    {0x00000004, 2,  0x08, 1}, {0x00000008, 3,  0x0c, 1}, {0x00000010, 4,  0x10, 0},
    {0x00000020, 5,  0x14, 0}, {0x00000040, 6,  0x18, 0}, {0x00000080, 7,  0x1c, 2},
    {0x00000100, 8,  0x20, 0}, {0x00000200, 9,  0x24, 3}, {0x00000400, 10, 0x28, 0},
    {0x00002000, 13, 0x2c, 0}, {0x00008000, 15, -1,   0}, // -1: constant 0, not read from environment
};

static int32_t sound_eax20_convert_field(const uint8_t *environment, const sound_eax20_listener_field *field)
{
    float value;
    int32_t millibels;
    int32_t clamp_max;

    if (field->offset < 0) {
        return 0;
    }
    value = *(const float *)(environment + field->offset);
    if (field->mode == 0) {
        return *(const int32_t *)(environment + field->offset);
    }
    if (value == 0.0f) {
        return k_sound_minimum_volume;
    }
    // mode 1: 2000 * log10(v) in [-10000, 0]; mode 2 (reflections): + 1000, in [-10000, 1000];
    // mode 3 (reverb): + 2000, in [-10000, 2000]
    clamp_max = field->mode == 1 ? 0 : (field->mode == 2 ? 1000 : 2000);
    millibels = (int32_t)(log10((double)value) * 2000.0 + (double)clamp_max);
    if (millibels < k_sound_minimum_volume) {
        return k_sound_minimum_volume;
    }
    return millibels > clamp_max ? clamp_max : millibels;
}

void __thiscall sound_eax20_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
{
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    int32_t value;
    uint32_t id;
    uint8_t deferred;

    property_set = this_object->channel_property_sets[0]; // +0x1c: slot 0 carries the listener properties
    set = (sound_property_set_fn)(*(void ***)property_set)[4];
    deferred = (this_object->base.supported_properties & 1) != 0;

    for (i = 0; i < 11; i++) {
        if (this_object->base.supported_properties & k_fields[i].bit) {
            value = sound_eax20_convert_field((const uint8_t *)environment, &k_fields[i]);
            id = deferred ? (k_fields[i].id | 0x80000000u) : k_fields[i].id;
            set(property_set, sound_eax20_listener_property_guid, id, 0, 0, &value, 4);
        }
    }

    directsound_deferred_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x54fa80): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x54fa80.md for the full 1188-byte listing (11 fields x 2 near-identical
code paths for the deferred/non-deferred id, folded into the table and helper above).

Disassembly (0x54fa80..0x54ff24, capstone; phase-4 review):

0x54fa80: sub esp, 0x28
0x54fa83: push ebx
0x54fa84: push esi
0x54fa85: push edi
0x54fa86: mov edi, dword ptr [esp + 0x38]
0x54fa8a: fld dword ptr [edi + 8]
0x54fa8d: mov esi, ecx
0x54fa8f: fld dword ptr [0x672ac0]
0x54fa95: mov dword ptr [esp + 0x30], 0
0x54fa9d: fld st(1)
0x54fa9f: fucompp 
0x54faa1: fnstsw ax
0x54faa3: test ah, 0x44
0x54faa6: jp 0x54fab1
0x54faa8: fstp st(0)
0x54faaa: mov eax, 0xffffd8f0
0x54faaf: jmp 0x54fad6
0x54fab1: fldlg2 
0x54fab3: fxch st(1)
0x54fab5: fyl2x 
0x54fab7: fmul dword ptr [0x672e14]
0x54fabd: call 0x6391b4
0x54fac2: cmp eax, 0xffffd8f0
0x54fac7: jge 0x54fad0
0x54fac9: mov eax, 0xffffd8f0
0x54face: jmp 0x54fad6
0x54fad0: test eax, eax
0x54fad2: jle 0x54fad6
0x54fad4: xor eax, eax
0x54fad6: fld dword ptr [edi + 0xc]
0x54fad9: mov dword ptr [esp + 0x38], eax
0x54fadd: fld dword ptr [0x672ac0]
0x54fae3: fld st(1)
0x54fae5: fucompp 
0x54fae7: fnstsw ax
0x54fae9: test ah, 0x44
0x54faec: jp 0x54faf7
0x54faee: fstp st(0)
0x54faf0: mov eax, 0xffffd8f0
0x54faf5: jmp 0x54fb1c
0x54faf7: fldlg2 
0x54faf9: fxch st(1)
0x54fafb: fyl2x 
0x54fafd: fmul dword ptr [0x672e14]
0x54fb03: call 0x6391b4
0x54fb08: cmp eax, 0xffffd8f0
0x54fb0d: jge 0x54fb16
0x54fb0f: mov eax, 0xffffd8f0
0x54fb14: jmp 0x54fb1c
0x54fb16: test eax, eax
0x54fb18: jle 0x54fb1c
0x54fb1a: xor eax, eax
0x54fb1c: fld dword ptr [edi + 0x1c]
0x54fb1f: mov ecx, dword ptr [edi + 0x14]
0x54fb22: fld dword ptr [0x672ac0]
0x54fb28: mov edx, dword ptr [edi + 0x18]
0x54fb2b: fld st(1)
0x54fb2d: mov dword ptr [esp + 0xc], eax
0x54fb31: mov eax, dword ptr [edi + 0x10]
0x54fb34: fucompp 
0x54fb36: mov dword ptr [esp + 0x10], eax
0x54fb3a: mov dword ptr [esp + 0x14], ecx
0x54fb3e: mov dword ptr [esp + 0x18], edx
0x54fb42: fnstsw ax
0x54fb44: test ah, 0x44
0x54fb47: jp 0x54fb52
0x54fb49: fstp st(0)
0x54fb4b: mov eax, 0xffffd8f0
0x54fb50: jmp 0x54fb83
0x54fb52: fldlg2 
0x54fb54: fxch st(1)
0x54fb56: fyl2x 
0x54fb58: fmul dword ptr [0x672e14]
0x54fb5e: fadd dword ptr [0x672ae8]
0x54fb64: call 0x6391b4
0x54fb69: cmp eax, 0xffffd8f0
0x54fb6e: jge 0x54fb77
0x54fb70: mov eax, 0xffffd8f0
0x54fb75: jmp 0x54fb83
0x54fb77: cmp eax, 0x3e8
0x54fb7c: jle 0x54fb83
0x54fb7e: mov eax, 0x3e8
0x54fb83: fld dword ptr [edi + 0x24]
0x54fb86: mov dword ptr [esp + 0x1c], eax
0x54fb8a: fld dword ptr [0x672ac0]
0x54fb90: mov eax, dword ptr [edi + 0x20]
0x54fb93: fld st(1)
0x54fb95: mov dword ptr [esp + 0x20], eax
0x54fb99: fucompp 
0x54fb9b: fnstsw ax
0x54fb9d: test ah, 0x44
0x54fba0: jp 0x54fbab
0x54fba2: fstp st(0)
0x54fba4: mov eax, 0xffffd8f0
0x54fba9: jmp 0x54fbdc
0x54fbab: fldlg2 
0x54fbad: fxch st(1)
0x54fbaf: fyl2x 
0x54fbb1: fmul dword ptr [0x672e14]
0x54fbb7: fadd dword ptr [0x672e14]
0x54fbbd: call 0x6391b4
0x54fbc2: cmp eax, 0xffffd8f0
0x54fbc7: jge 0x54fbd0
0x54fbc9: mov eax, 0xffffd8f0
0x54fbce: jmp 0x54fbdc
0x54fbd0: cmp eax, 0x7d0
0x54fbd5: jle 0x54fbdc
0x54fbd7: mov eax, 0x7d0
0x54fbdc: mov ecx, dword ptr [edi + 0x28]
0x54fbdf: mov edx, dword ptr [edi + 0x2c]
0x54fbe2: mov dword ptr [esp + 0x24], eax
0x54fbe6: mov eax, dword ptr [esi + 8]
0x54fbe9: mov bl, 1
0x54fbeb: test bl, al
0x54fbed: mov dword ptr [esp + 0x28], ecx
0x54fbf1: mov dword ptr [esp + 0x2c], edx
0x54fbf5: je 0x54fda2
0x54fbfb: test al, 4
0x54fbfd: je 0x54fc1d
0x54fbff: mov eax, dword ptr [esi + 0x1c]
0x54fc02: mov ecx, dword ptr [eax]
0x54fc04: push 4
0x54fc06: lea edx, [esp + 0x3c]
0x54fc0a: push edx
0x54fc0b: push 0
0x54fc0d: push 0
0x54fc0f: push 0x80000002
0x54fc14: push 0x64e2f0
0x54fc19: push eax
0x54fc1a: call dword ptr [ecx + 0x10]
0x54fc1d: test byte ptr [esi + 8], 8
0x54fc21: je 0x54fc41
0x54fc23: mov eax, dword ptr [esi + 0x1c]
0x54fc26: mov ecx, dword ptr [eax]
0x54fc28: push 4
0x54fc2a: lea edx, [esp + 0x10]
0x54fc2e: push edx
0x54fc2f: push 0
0x54fc31: push 0
0x54fc33: push 0x80000003
0x54fc38: push 0x64e2f0
0x54fc3d: push eax
0x54fc3e: call dword ptr [ecx + 0x10]
0x54fc41: test byte ptr [esi + 8], 0x10
0x54fc45: je 0x54fc65
0x54fc47: mov eax, dword ptr [esi + 0x1c]
0x54fc4a: mov ecx, dword ptr [eax]
0x54fc4c: push 4
0x54fc4e: lea edx, [esp + 0x14]
0x54fc52: push edx
0x54fc53: push 0
0x54fc55: push 0
0x54fc57: push 0x80000004
0x54fc5c: push 0x64e2f0
0x54fc61: push eax
0x54fc62: call dword ptr [ecx + 0x10]
0x54fc65: test byte ptr [esi + 8], 0x20
0x54fc69: je 0x54fc89
0x54fc6b: mov eax, dword ptr [esi + 0x1c]
0x54fc6e: mov ecx, dword ptr [eax]
0x54fc70: push 4
0x54fc72: lea edx, [esp + 0x18]
0x54fc76: push edx
0x54fc77: push 0
0x54fc79: push 0
0x54fc7b: push 0x80000005
0x54fc80: push 0x64e2f0
0x54fc85: push eax
0x54fc86: call dword ptr [ecx + 0x10]
0x54fc89: test byte ptr [esi + 8], 0x40
0x54fc8d: je 0x54fcad
0x54fc8f: mov eax, dword ptr [esi + 0x1c]
0x54fc92: mov ecx, dword ptr [eax]
0x54fc94: push 4
0x54fc96: lea edx, [esp + 0x1c]
0x54fc9a: push edx
0x54fc9b: push 0
0x54fc9d: push 0
0x54fc9f: push 0x80000006
0x54fca4: push 0x64e2f0
0x54fca9: push eax
0x54fcaa: call dword ptr [ecx + 0x10]
0x54fcad: mov al, byte ptr [esi + 8]
0x54fcb0: test al, al
0x54fcb2: jns 0x54fcd2
0x54fcb4: mov eax, dword ptr [esi + 0x1c]
0x54fcb7: mov ecx, dword ptr [eax]
0x54fcb9: push 4
0x54fcbb: lea edx, [esp + 0x20]
0x54fcbf: push edx
0x54fcc0: push 0
0x54fcc2: push 0
0x54fcc4: push 0x80000007
0x54fcc9: push 0x64e2f0
0x54fcce: push eax
0x54fccf: call dword ptr [ecx + 0x10]
0x54fcd2: mov eax, dword ptr [esi + 8]
0x54fcd5: test ah, 1
0x54fcd8: je 0x54fcf8
0x54fcda: mov eax, dword ptr [esi + 0x1c]
0x54fcdd: mov ecx, dword ptr [eax]
0x54fcdf: push 4
0x54fce1: lea edx, [esp + 0x24]
0x54fce5: push edx
0x54fce6: push 0
0x54fce8: push 0
0x54fcea: push 0x80000008
0x54fcef: push 0x64e2f0
0x54fcf4: push eax
0x54fcf5: call dword ptr [ecx + 0x10]
0x54fcf8: mov eax, dword ptr [esi + 8]
0x54fcfb: test ah, 2
0x54fcfe: je 0x54fd1e
0x54fd00: mov eax, dword ptr [esi + 0x1c]
0x54fd03: mov ecx, dword ptr [eax]
0x54fd05: push 4
0x54fd07: lea edx, [esp + 0x28]
0x54fd0b: push edx
0x54fd0c: push 0
0x54fd0e: push 0
0x54fd10: push 0x80000009
0x54fd15: push 0x64e2f0
0x54fd1a: push eax
0x54fd1b: call dword ptr [ecx + 0x10]
0x54fd1e: mov eax, dword ptr [esi + 8]
0x54fd21: test ah, 4
0x54fd24: je 0x54fd44
0x54fd26: mov eax, dword ptr [esi + 0x1c]
0x54fd29: mov ecx, dword ptr [eax]
0x54fd2b: push 4
0x54fd2d: lea edx, [esp + 0x2c]
0x54fd31: push edx
0x54fd32: push 0
0x54fd34: push 0
0x54fd36: push 0x8000000a
0x54fd3b: push 0x64e2f0
0x54fd40: push eax
0x54fd41: call dword ptr [ecx + 0x10]
0x54fd44: mov eax, dword ptr [esi + 8]
0x54fd47: test ah, 0x20
0x54fd4a: je 0x54fd6a
0x54fd4c: mov eax, dword ptr [esi + 0x1c]
0x54fd4f: mov ecx, dword ptr [eax]
0x54fd51: push 4
0x54fd53: lea edx, [esp + 0x30]
0x54fd57: push edx
0x54fd58: push 0
0x54fd5a: push 0
0x54fd5c: push 0x8000000d
0x54fd61: push 0x64e2f0
0x54fd66: push eax
0x54fd67: call dword ptr [ecx + 0x10]
0x54fd6a: mov eax, dword ptr [esi + 8]
0x54fd6d: test ah, ah
0x54fd6f: jns 0x54ff15
0x54fd75: mov esi, dword ptr [esi + 0x1c]
0x54fd78: mov eax, dword ptr [esi]
0x54fd7a: push 4
0x54fd7c: lea ecx, [esp + 0x34]
0x54fd80: push ecx
0x54fd81: push 0
0x54fd83: push 0
0x54fd85: push 0x8000000f
0x54fd8a: push 0x64e2f0
0x54fd8f: push esi
0x54fd90: call dword ptr [eax + 0x10]
0x54fd93: pop edi
0x54fd94: pop esi
0x54fd95: mov byte ptr [0x746132], bl
0x54fd9b: pop ebx
0x54fd9c: add esp, 0x28
0x54fd9f: ret 4
0x54fda2: test al, 4
0x54fda4: je 0x54fdc1
0x54fda6: mov eax, dword ptr [esi + 0x1c]
0x54fda9: mov edx, dword ptr [eax]
0x54fdab: push 4
0x54fdad: lea ecx, [esp + 0x3c]
0x54fdb1: push ecx
0x54fdb2: push 0
0x54fdb4: push 0
0x54fdb6: push 2
0x54fdb8: push 0x64e2f0
0x54fdbd: push eax
0x54fdbe: call dword ptr [edx + 0x10]
0x54fdc1: test byte ptr [esi + 8], 8
0x54fdc5: je 0x54fde2
0x54fdc7: mov eax, dword ptr [esi + 0x1c]
0x54fdca: mov edx, dword ptr [eax]
0x54fdcc: push 4
0x54fdce: lea ecx, [esp + 0x10]
0x54fdd2: push ecx
0x54fdd3: push 0
0x54fdd5: push 0
0x54fdd7: push 3
0x54fdd9: push 0x64e2f0
0x54fdde: push eax
0x54fddf: call dword ptr [edx + 0x10]
0x54fde2: test byte ptr [esi + 8], 0x10
0x54fde6: je 0x54fe03
0x54fde8: mov eax, dword ptr [esi + 0x1c]
0x54fdeb: mov edx, dword ptr [eax]
0x54fded: push 4
0x54fdef: lea ecx, [esp + 0x14]
0x54fdf3: push ecx
0x54fdf4: push 0
0x54fdf6: push 0
0x54fdf8: push 4
0x54fdfa: push 0x64e2f0
0x54fdff: push eax
0x54fe00: call dword ptr [edx + 0x10]
0x54fe03: test byte ptr [esi + 8], 0x20
0x54fe07: je 0x54fe24
0x54fe09: mov eax, dword ptr [esi + 0x1c]
0x54fe0c: mov edx, dword ptr [eax]
0x54fe0e: push 4
0x54fe10: lea ecx, [esp + 0x18]
0x54fe14: push ecx
0x54fe15: push 0
0x54fe17: push 0
0x54fe19: push 5
0x54fe1b: push 0x64e2f0
0x54fe20: push eax
0x54fe21: call dword ptr [edx + 0x10]
0x54fe24: test byte ptr [esi + 8], 0x40
0x54fe28: je 0x54fe45
0x54fe2a: mov eax, dword ptr [esi + 0x1c]
0x54fe2d: mov edx, dword ptr [eax]
0x54fe2f: push 4
0x54fe31: lea ecx, [esp + 0x1c]
0x54fe35: push ecx
0x54fe36: push 0
0x54fe38: push 0
0x54fe3a: push 6
0x54fe3c: push 0x64e2f0
0x54fe41: push eax
0x54fe42: call dword ptr [edx + 0x10]
0x54fe45: mov al, byte ptr [esi + 8]
0x54fe48: test al, al
0x54fe4a: jns 0x54fe67
0x54fe4c: mov eax, dword ptr [esi + 0x1c]
0x54fe4f: mov edx, dword ptr [eax]
0x54fe51: push 4
0x54fe53: lea ecx, [esp + 0x20]
0x54fe57: push ecx
0x54fe58: push 0
0x54fe5a: push 0
0x54fe5c: push 7
0x54fe5e: push 0x64e2f0
0x54fe63: push eax
0x54fe64: call dword ptr [edx + 0x10]
0x54fe67: mov eax, dword ptr [esi + 8]
0x54fe6a: test ah, 1
0x54fe6d: je 0x54fe8a
0x54fe6f: mov eax, dword ptr [esi + 0x1c]
0x54fe72: mov edx, dword ptr [eax]
0x54fe74: push 4
0x54fe76: lea ecx, [esp + 0x24]
0x54fe7a: push ecx
0x54fe7b: push 0
0x54fe7d: push 0
0x54fe7f: push 8
0x54fe81: push 0x64e2f0
0x54fe86: push eax
0x54fe87: call dword ptr [edx + 0x10]
0x54fe8a: mov eax, dword ptr [esi + 8]
0x54fe8d: test ah, 2
0x54fe90: je 0x54fead
0x54fe92: mov eax, dword ptr [esi + 0x1c]
0x54fe95: mov edx, dword ptr [eax]
0x54fe97: push 4
0x54fe99: lea ecx, [esp + 0x28]
0x54fe9d: push ecx
0x54fe9e: push 0
0x54fea0: push 0
0x54fea2: push 9
0x54fea4: push 0x64e2f0
0x54fea9: push eax
0x54feaa: call dword ptr [edx + 0x10]
0x54fead: mov eax, dword ptr [esi + 8]
0x54feb0: test ah, 4
0x54feb3: je 0x54fed0
0x54feb5: mov eax, dword ptr [esi + 0x1c]
0x54feb8: mov edx, dword ptr [eax]
0x54feba: push 4
0x54febc: lea ecx, [esp + 0x2c]
0x54fec0: push ecx
0x54fec1: push 0
0x54fec3: push 0
0x54fec5: push 0xa
0x54fec7: push 0x64e2f0
0x54fecc: push eax
0x54fecd: call dword ptr [edx + 0x10]
0x54fed0: mov eax, dword ptr [esi + 8]
0x54fed3: test ah, 0x20
0x54fed6: je 0x54fef3
0x54fed8: mov eax, dword ptr [esi + 0x1c]
0x54fedb: mov edx, dword ptr [eax]
0x54fedd: push 4
0x54fedf: lea ecx, [esp + 0x30]
0x54fee3: push ecx
0x54fee4: push 0
0x54fee6: push 0
0x54fee8: push 0xd
0x54feea: push 0x64e2f0
0x54feef: push eax
0x54fef0: call dword ptr [edx + 0x10]
0x54fef3: mov eax, dword ptr [esi + 8]
0x54fef6: test ah, ah
0x54fef8: jns 0x54ff15
0x54fefa: mov esi, dword ptr [esi + 0x1c]
0x54fefd: mov edx, dword ptr [esi]
0x54feff: push 4
0x54ff01: lea eax, [esp + 0x34]
0x54ff05: push eax
0x54ff06: push 0
0x54ff08: push 0
0x54ff0a: push 0xf
0x54ff0c: push 0x64e2f0
0x54ff11: push esi
0x54ff12: call dword ptr [edx + 0x10]
0x54ff15: pop edi
0x54ff16: pop esi
0x54ff17: mov byte ptr [0x746132], bl
0x54ff1d: pop ebx
0x54ff1e: add esp, 0x28
0x54ff21: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

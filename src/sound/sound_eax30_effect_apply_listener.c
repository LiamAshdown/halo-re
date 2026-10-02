// sound_eax30_effect_apply_listener  (Ghidra: sound_eax30_effect_apply_listener, already named,
// __thiscall; slot 7 (apply_listener) of the EAX3 vtable at 0x00671d28, called through the
// effects object by sound_listener_update 0x547070)
// address 0x550c70, size 1284 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Applies a reverb environment's parameters to the
// active EAX 3.0-style listener property set, including a size/frequency scale computed via
// sound_reverb_size_scale."; same shape as sound_eax20_effect_apply_listener.c (0x54fa80), with
// the same ids/bits/defaults as sound_eax30_effect_shutdown.c (0x54fff0)'s listener table, plus
// two extra fields: diffusion (id 4, raw, SoundEnvironment.diffusion 0x2c) and a size/frequency
// scale (id 0x16, reusing bit 0x10 with id 4 -- the same "one bit, two ids" oddity already noted
// in sound_eax30_effect_shutdown.c) computed by sound_reverb_size_scale(hf_reference @ 0x34).
// Phase-4 review (disassembly appended below): the properties go to channel_property_sets[0]
// (this + 0x1c), not base.property_set; REFLECTIONS and REVERB add 1000 / 2000 mB before the
// clamp. The id / bit / field table was checked against every Set call and matches.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const uint8_t sound_eax30_listener_property_guid[16]; // 0x0064e310
extern uint8_t directsound_deferred_dirty; // 0x00746132

extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call
extern float sound_reverb_size_scale(float value); // this module, 0x550c10

// mode: 0 = raw int32 passthrough, 1/2/3 = 2000*log10(value) clamped to [-10000, 0/1000/2000],
// 4 = constant 0, 5 = sound_reverb_size_scale(value) as a float bit pattern
typedef struct sound_eax30_listener_field { uint32_t bit; uint32_t id; int32_t offset; int32_t mode; } sound_eax30_listener_field;

static const sound_eax30_listener_field k_fields[12] = {
    {0x00000020, 5,    0x08, 1},
    {0x00000040, 6,    0x0c, 1},
    {0x01000000, 0x18, 0x10, 0},
    {0x00000100, 8,    0x14, 0},
    {0x00000200, 9,    0x18, 0},
    {0x00000800, 0x0b, 0x1c, 2},
    {0x00001000, 0x0c, 0x20, 0},
    {0x00004000, 0x0e, 0x24, 3},
    {0x00008000, 0x0f, 0x28, 0},
    {0x00000010, 4,    0x2c, 0},
    {0x02000000, 0x19, -1,   4},
    {0x00000010, 0x16, 0x34, 5},
};

static int32_t sound_eax30_convert_field(const uint8_t *environment, const sound_eax30_listener_field *field)
{
    float value;
    int32_t millibels;
    int32_t clamp_max;

    switch (field->mode) {
        case 0:
            return *(const int32_t *)(environment + field->offset);
        case 4:
            return 0;
        case 5: {
            float scale = sound_reverb_size_scale(*(const float *)(environment + field->offset));
            return *(int32_t *)&scale;
        }
        default:
            break;
    }

    value = *(const float *)(environment + field->offset);
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

void __thiscall sound_eax30_effect_apply_listener(sound_eax_effect_object *this_object, const SoundEnvironment *environment)
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

    for (i = 0; i < 12; i++) {
        if (this_object->base.supported_properties & k_fields[i].bit) {
            value = sound_eax30_convert_field((const uint8_t *)environment, &k_fields[i]);
            id = deferred ? (k_fields[i].id | 0x80000000u) : k_fields[i].id;
            set(property_set, sound_eax30_listener_property_guid, id, 0, 0, &value, 4);
        }
    }

    directsound_deferred_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x550c70): see out/phase2/sound/02.md and
scratchpad/sound_packs/0x550c70.md for the full 1284-byte listing, folded into the table and
helper above.

Disassembly (0x550c70..0x551174, capstone; phase-4 review):

0x550c70: sub esp, 0x2c
0x550c73: push esi
0x550c74: push edi
0x550c75: mov edi, dword ptr [esp + 0x38]
0x550c79: fld dword ptr [edi + 8]
0x550c7c: mov esi, ecx
0x550c7e: fld dword ptr [0x672ac0]
0x550c84: mov dword ptr [esp + 0x2c], 0
0x550c8c: fld st(1)
0x550c8e: fucompp 
0x550c90: fnstsw ax
0x550c92: test ah, 0x44
0x550c95: jp 0x550ca0
0x550c97: fstp st(0)
0x550c99: mov eax, 0xffffd8f0
0x550c9e: jmp 0x550cc5
0x550ca0: fldlg2 
0x550ca2: fxch st(1)
0x550ca4: fyl2x 
0x550ca6: fmul dword ptr [0x672e14]
0x550cac: call 0x6391b4
0x550cb1: cmp eax, 0xffffd8f0
0x550cb6: jge 0x550cbf
0x550cb8: mov eax, 0xffffd8f0
0x550cbd: jmp 0x550cc5
0x550cbf: test eax, eax
0x550cc1: jle 0x550cc5
0x550cc3: xor eax, eax
0x550cc5: fld dword ptr [edi + 0xc]
0x550cc8: mov dword ptr [esp + 0x38], eax
0x550ccc: fld dword ptr [0x672ac0]
0x550cd2: fld st(1)
0x550cd4: fucompp 
0x550cd6: fnstsw ax
0x550cd8: test ah, 0x44
0x550cdb: jp 0x550ce6
0x550cdd: fstp st(0)
0x550cdf: mov eax, 0xffffd8f0
0x550ce4: jmp 0x550d0b
0x550ce6: fldlg2 
0x550ce8: fxch st(1)
0x550cea: fyl2x 
0x550cec: fmul dword ptr [0x672e14]
0x550cf2: call 0x6391b4
0x550cf7: cmp eax, 0xffffd8f0
0x550cfc: jge 0x550d05
0x550cfe: mov eax, 0xffffd8f0
0x550d03: jmp 0x550d0b
0x550d05: test eax, eax
0x550d07: jle 0x550d0b
0x550d09: xor eax, eax
0x550d0b: fld dword ptr [edi + 0x1c]
0x550d0e: mov ecx, dword ptr [edi + 0x14]
0x550d11: fld dword ptr [0x672ac0]
0x550d17: mov edx, dword ptr [edi + 0x18]
0x550d1a: fld st(1)
0x550d1c: mov dword ptr [esp + 8], eax
0x550d20: mov eax, dword ptr [edi + 0x10]
0x550d23: fucompp 
0x550d25: mov dword ptr [esp + 0xc], eax
0x550d29: mov dword ptr [esp + 0x10], ecx
0x550d2d: mov dword ptr [esp + 0x14], edx
0x550d31: fnstsw ax
0x550d33: test ah, 0x44
0x550d36: jp 0x550d41
0x550d38: fstp st(0)
0x550d3a: mov eax, 0xffffd8f0
0x550d3f: jmp 0x550d72
0x550d41: fldlg2 
0x550d43: fxch st(1)
0x550d45: fyl2x 
0x550d47: fmul dword ptr [0x672e14]
0x550d4d: fadd dword ptr [0x672ae8]
0x550d53: call 0x6391b4
0x550d58: cmp eax, 0xffffd8f0
0x550d5d: jge 0x550d66
0x550d5f: mov eax, 0xffffd8f0
0x550d64: jmp 0x550d72
0x550d66: cmp eax, 0x3e8
0x550d6b: jle 0x550d72
0x550d6d: mov eax, 0x3e8
0x550d72: fld dword ptr [edi + 0x24]
0x550d75: mov dword ptr [esp + 0x18], eax
0x550d79: fld dword ptr [0x672ac0]
0x550d7f: mov eax, dword ptr [edi + 0x20]
0x550d82: fld st(1)
0x550d84: mov dword ptr [esp + 0x1c], eax
0x550d88: fucompp 
0x550d8a: fnstsw ax
0x550d8c: test ah, 0x44
0x550d8f: jp 0x550d9a
0x550d91: fstp st(0)
0x550d93: mov eax, 0xffffd8f0
0x550d98: jmp 0x550dcb
0x550d9a: fldlg2 
0x550d9c: fxch st(1)
0x550d9e: fyl2x 
0x550da0: fmul dword ptr [0x672e14]
0x550da6: fadd dword ptr [0x672e14]
0x550dac: call 0x6391b4
0x550db1: cmp eax, 0xffffd8f0
0x550db6: jge 0x550dbf
0x550db8: mov eax, 0xffffd8f0
0x550dbd: jmp 0x550dcb
0x550dbf: cmp eax, 0x7d0
0x550dc4: jle 0x550dcb
0x550dc6: mov eax, 0x7d0
0x550dcb: mov ecx, dword ptr [edi + 0x28]
0x550dce: mov edx, dword ptr [edi + 0x2c]
0x550dd1: mov dword ptr [esp + 0x20], eax
0x550dd5: mov eax, dword ptr [edi + 0x34]
0x550dd8: push eax
0x550dd9: mov dword ptr [esp + 0x28], ecx
0x550ddd: mov dword ptr [esp + 0x2c], edx
0x550de1: call 0x550c10
0x550de6: fstp dword ptr [esp + 0x34]
0x550dea: mov eax, dword ptr [esi + 8]
0x550ded: add esp, 4
0x550df0: test al, 1
0x550df2: je 0x550fca
0x550df8: test al, 0x20
0x550dfa: je 0x550e1a
0x550dfc: mov eax, dword ptr [esi + 0x1c]
0x550dff: mov ecx, dword ptr [eax]
0x550e01: push 4
0x550e03: lea edx, [esp + 0x3c]
0x550e07: push edx
0x550e08: push 0
0x550e0a: push 0
0x550e0c: push 0x80000005
0x550e11: push 0x64e310
0x550e16: push eax
0x550e17: call dword ptr [ecx + 0x10]
0x550e1a: test byte ptr [esi + 8], 0x40
0x550e1e: je 0x550e3e
0x550e20: mov eax, dword ptr [esi + 0x1c]
0x550e23: mov ecx, dword ptr [eax]
0x550e25: push 4
0x550e27: lea edx, [esp + 0xc]
0x550e2b: push edx
0x550e2c: push 0
0x550e2e: push 0
0x550e30: push 0x80000006
0x550e35: push 0x64e310
0x550e3a: push eax
0x550e3b: call dword ptr [ecx + 0x10]
0x550e3e: test dword ptr [esi + 8], 0x1000000
0x550e45: je 0x550e65
0x550e47: mov eax, dword ptr [esi + 0x1c]
0x550e4a: mov ecx, dword ptr [eax]
0x550e4c: push 4
0x550e4e: lea edx, [esp + 0x10]
0x550e52: push edx
0x550e53: push 0
0x550e55: push 0
0x550e57: push 0x80000018
0x550e5c: push 0x64e310
0x550e61: push eax
0x550e62: call dword ptr [ecx + 0x10]
0x550e65: mov eax, dword ptr [esi + 8]
0x550e68: test ah, 1
0x550e6b: je 0x550e8b
0x550e6d: mov eax, dword ptr [esi + 0x1c]
0x550e70: mov ecx, dword ptr [eax]
0x550e72: push 4
0x550e74: lea edx, [esp + 0x14]
0x550e78: push edx
0x550e79: push 0
0x550e7b: push 0
0x550e7d: push 0x80000008
0x550e82: push 0x64e310
0x550e87: push eax
0x550e88: call dword ptr [ecx + 0x10]
0x550e8b: mov eax, dword ptr [esi + 8]
0x550e8e: test ah, 2
0x550e91: je 0x550eb1
0x550e93: mov eax, dword ptr [esi + 0x1c]
0x550e96: mov ecx, dword ptr [eax]
0x550e98: push 4
0x550e9a: lea edx, [esp + 0x18]
0x550e9e: push edx
0x550e9f: push 0
0x550ea1: push 0
0x550ea3: push 0x80000009
0x550ea8: push 0x64e310
0x550ead: push eax
0x550eae: call dword ptr [ecx + 0x10]
0x550eb1: mov eax, dword ptr [esi + 8]
0x550eb4: test ah, 8
0x550eb7: je 0x550ed7
0x550eb9: mov eax, dword ptr [esi + 0x1c]
0x550ebc: mov ecx, dword ptr [eax]
0x550ebe: push 4
0x550ec0: lea edx, [esp + 0x1c]
0x550ec4: push edx
0x550ec5: push 0
0x550ec7: push 0
0x550ec9: push 0x8000000b
0x550ece: push 0x64e310
0x550ed3: push eax
0x550ed4: call dword ptr [ecx + 0x10]
0x550ed7: mov eax, dword ptr [esi + 8]
0x550eda: test ah, 0x10
0x550edd: je 0x550efd
0x550edf: mov eax, dword ptr [esi + 0x1c]
0x550ee2: mov ecx, dword ptr [eax]
0x550ee4: push 4
0x550ee6: lea edx, [esp + 0x20]
0x550eea: push edx
0x550eeb: push 0
0x550eed: push 0
0x550eef: push 0x8000000c
0x550ef4: push 0x64e310
0x550ef9: push eax
0x550efa: call dword ptr [ecx + 0x10]
0x550efd: mov eax, dword ptr [esi + 8]
0x550f00: test ah, 0x40
0x550f03: je 0x550f23
0x550f05: mov eax, dword ptr [esi + 0x1c]
0x550f08: mov ecx, dword ptr [eax]
0x550f0a: push 4
0x550f0c: lea edx, [esp + 0x24]
0x550f10: push edx
0x550f11: push 0
0x550f13: push 0
0x550f15: push 0x8000000e
0x550f1a: push 0x64e310
0x550f1f: push eax
0x550f20: call dword ptr [ecx + 0x10]
0x550f23: mov eax, dword ptr [esi + 8]
0x550f26: test ah, ah
0x550f28: jns 0x550f48
0x550f2a: mov eax, dword ptr [esi + 0x1c]
0x550f2d: mov ecx, dword ptr [eax]
0x550f2f: push 4
0x550f31: lea edx, [esp + 0x28]
0x550f35: push edx
0x550f36: push 0
0x550f38: push 0
0x550f3a: push 0x8000000f
0x550f3f: push 0x64e310
0x550f44: push eax
0x550f45: call dword ptr [ecx + 0x10]
0x550f48: test byte ptr [esi + 8], 0x10
0x550f4c: je 0x550f6c
0x550f4e: mov eax, dword ptr [esi + 0x1c]
0x550f51: mov ecx, dword ptr [eax]
0x550f53: push 4
0x550f55: lea edx, [esp + 0x2c]
0x550f59: push edx
0x550f5a: push 0
0x550f5c: push 0
0x550f5e: push 0x80000004
0x550f63: push 0x64e310
0x550f68: push eax
0x550f69: call dword ptr [ecx + 0x10]
0x550f6c: test dword ptr [esi + 8], 0x2000000
0x550f73: je 0x550f93
0x550f75: mov eax, dword ptr [esi + 0x1c]
0x550f78: mov ecx, dword ptr [eax]
0x550f7a: push 4
0x550f7c: lea edx, [esp + 0x30]
0x550f80: push edx
0x550f81: push 0
0x550f83: push 0
0x550f85: push 0x80000019
0x550f8a: push 0x64e310
0x550f8f: push eax
0x550f90: call dword ptr [ecx + 0x10]
0x550f93: test byte ptr [esi + 8], 0x10
0x550f97: je 0x551165
0x550f9d: mov esi, dword ptr [esi + 0x1c]
0x550fa0: mov eax, dword ptr [esi]
0x550fa2: push 4
0x550fa4: lea ecx, [esp + 0x34]
0x550fa8: push ecx
0x550fa9: push 0
0x550fab: push 0
0x550fad: push 0x80000016
0x550fb2: push 0x64e310
0x550fb7: push esi
0x550fb8: call dword ptr [eax + 0x10]
0x550fbb: pop edi
0x550fbc: mov byte ptr [0x746132], 1
0x550fc3: pop esi
0x550fc4: add esp, 0x2c
0x550fc7: ret 4
0x550fca: test al, 0x20
0x550fcc: je 0x550fe9
0x550fce: mov eax, dword ptr [esi + 0x1c]
0x550fd1: mov edx, dword ptr [eax]
0x550fd3: push 4
0x550fd5: lea ecx, [esp + 0x3c]
0x550fd9: push ecx
0x550fda: push 0
0x550fdc: push 0
0x550fde: push 5
0x550fe0: push 0x64e310
0x550fe5: push eax
0x550fe6: call dword ptr [edx + 0x10]
0x550fe9: test byte ptr [esi + 8], 0x40
0x550fed: je 0x55100a
0x550fef: mov eax, dword ptr [esi + 0x1c]
0x550ff2: mov edx, dword ptr [eax]
0x550ff4: push 4
0x550ff6: lea ecx, [esp + 0xc]
0x550ffa: push ecx
0x550ffb: push 0
0x550ffd: push 0
0x550fff: push 6
0x551001: push 0x64e310
0x551006: push eax
0x551007: call dword ptr [edx + 0x10]
0x55100a: test dword ptr [esi + 8], 0x1000000
0x551011: je 0x55102e
0x551013: mov eax, dword ptr [esi + 0x1c]
0x551016: mov edx, dword ptr [eax]
0x551018: push 4
0x55101a: lea ecx, [esp + 0x10]
0x55101e: push ecx
0x55101f: push 0
0x551021: push 0
0x551023: push 0x18
0x551025: push 0x64e310
0x55102a: push eax
0x55102b: call dword ptr [edx + 0x10]
0x55102e: mov eax, dword ptr [esi + 8]
0x551031: test ah, 1
0x551034: je 0x551051
0x551036: mov eax, dword ptr [esi + 0x1c]
0x551039: mov edx, dword ptr [eax]
0x55103b: push 4
0x55103d: lea ecx, [esp + 0x14]
0x551041: push ecx
0x551042: push 0
0x551044: push 0
0x551046: push 8
0x551048: push 0x64e310
0x55104d: push eax
0x55104e: call dword ptr [edx + 0x10]
0x551051: mov eax, dword ptr [esi + 8]
0x551054: test ah, 2
0x551057: je 0x551074
0x551059: mov eax, dword ptr [esi + 0x1c]
0x55105c: mov edx, dword ptr [eax]
0x55105e: push 4
0x551060: lea ecx, [esp + 0x18]
0x551064: push ecx
0x551065: push 0
0x551067: push 0
0x551069: push 9
0x55106b: push 0x64e310
0x551070: push eax
0x551071: call dword ptr [edx + 0x10]
0x551074: mov eax, dword ptr [esi + 8]
0x551077: test ah, 8
0x55107a: je 0x551097
0x55107c: mov eax, dword ptr [esi + 0x1c]
0x55107f: mov edx, dword ptr [eax]
0x551081: push 4
0x551083: lea ecx, [esp + 0x1c]
0x551087: push ecx
0x551088: push 0
0x55108a: push 0
0x55108c: push 0xb
0x55108e: push 0x64e310
0x551093: push eax
0x551094: call dword ptr [edx + 0x10]
0x551097: mov eax, dword ptr [esi + 8]
0x55109a: test ah, 0x10
0x55109d: je 0x5510ba
0x55109f: mov eax, dword ptr [esi + 0x1c]
0x5510a2: mov edx, dword ptr [eax]
0x5510a4: push 4
0x5510a6: lea ecx, [esp + 0x20]
0x5510aa: push ecx
0x5510ab: push 0
0x5510ad: push 0
0x5510af: push 0xc
0x5510b1: push 0x64e310
0x5510b6: push eax
0x5510b7: call dword ptr [edx + 0x10]
0x5510ba: mov eax, dword ptr [esi + 8]
0x5510bd: test ah, 0x40
0x5510c0: je 0x5510dd
0x5510c2: mov eax, dword ptr [esi + 0x1c]
0x5510c5: mov edx, dword ptr [eax]
0x5510c7: push 4
0x5510c9: lea ecx, [esp + 0x24]
0x5510cd: push ecx
0x5510ce: push 0
0x5510d0: push 0
0x5510d2: push 0xe
0x5510d4: push 0x64e310
0x5510d9: push eax
0x5510da: call dword ptr [edx + 0x10]
0x5510dd: mov eax, dword ptr [esi + 8]
0x5510e0: test ah, ah
0x5510e2: jns 0x5510ff
0x5510e4: mov eax, dword ptr [esi + 0x1c]
0x5510e7: mov edx, dword ptr [eax]
0x5510e9: push 4
0x5510eb: lea ecx, [esp + 0x28]
0x5510ef: push ecx
0x5510f0: push 0
0x5510f2: push 0
0x5510f4: push 0xf
0x5510f6: push 0x64e310
0x5510fb: push eax
0x5510fc: call dword ptr [edx + 0x10]
0x5510ff: test byte ptr [esi + 8], 0x10
0x551103: je 0x551120
0x551105: mov eax, dword ptr [esi + 0x1c]
0x551108: mov edx, dword ptr [eax]
0x55110a: push 4
0x55110c: lea ecx, [esp + 0x2c]
0x551110: push ecx
0x551111: push 0
0x551113: push 0
0x551115: push 4
0x551117: push 0x64e310
0x55111c: push eax
0x55111d: call dword ptr [edx + 0x10]
0x551120: test dword ptr [esi + 8], 0x2000000
0x551127: je 0x551144
0x551129: mov eax, dword ptr [esi + 0x1c]
0x55112c: mov edx, dword ptr [eax]
0x55112e: push 4
0x551130: lea ecx, [esp + 0x30]
0x551134: push ecx
0x551135: push 0
0x551137: push 0
0x551139: push 0x19
0x55113b: push 0x64e310
0x551140: push eax
0x551141: call dword ptr [edx + 0x10]
0x551144: test byte ptr [esi + 8], 0x10
0x551148: je 0x551165
0x55114a: mov esi, dword ptr [esi + 0x1c]
0x55114d: mov edx, dword ptr [esi]
0x55114f: push 4
0x551151: lea eax, [esp + 0x34]
0x551155: push eax
0x551156: push 0
0x551158: push 0
0x55115a: push 0x16
0x55115c: push 0x64e310
0x551161: push esi
0x551162: call dword ptr [edx + 0x10]
0x551165: pop edi
0x551166: mov byte ptr [0x746132], 1
0x55116d: pop esi
0x55116e: add esp, 0x2c
0x551171: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

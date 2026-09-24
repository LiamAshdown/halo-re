// sound_looping_track_location_proc  (not a Ghidra function; code at 0x54dc10, between
//   sound_looping_create_detail_sound and sound_looping_detail_location_proc)
// address 0x54dc10, size 91 bytes (0x54dc10..0x54dc6a)
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: its address is stored as sound.location_proc by sound_looping_create_detail_sound
//   (0x54d9f0, `mov dword ptr [ebx+0x10], 0x54dc10`), the only reference to it. It validates the
//   owner looping_sound handle exactly like datum_try_and_get (index in range, identifier
//   nonzero, salt match when the handle carries one) and copies the owner's whole 0x40-byte
//   sound_location (looping_sound+0x0c). Added in the phase-4 review: Ghidra never made a
//   function here, so it was missing from out/phase4/sound_functions.md; types/sound.h had
//   called it a thunk to 0x54dc70, which it is not.
// register convention: plain stack (owner, callback_data unused, location); returns AL.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4

// blam-cc: stack -> (owner, callback_data, location)
// sound_location_proc of looping track sounds (start / loop / end parts): the sound simply sits
// wherever its looping sound is.
uint8_t sound_looping_track_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    int16_t index = (int16_t)owner;
    int16_t salt = (int16_t)(owner >> 16);
    looping_sound *state;

    (void)callback_data;
    if (owner == k_datum_index_none || index < 0 || index >= looping_sound_data->maximum_count) {
        return 0;
    }
    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (int32_t)looping_sound_data->size * index);
    if (state->identifier == 0 || (salt != 0 && state->identifier != salt)) {
        return 0;
    }
    *location = state->location;
    return 1;
}

#if 0
Disassembly (0x54dc10..0x54dc6b, capstone; no Ghidra function exists here):

0x54dc10: mov ecx, dword ptr [esp + 4]
0x54dc14: cmp ecx, -1
0x54dc17: push esi
0x54dc18: push edi
0x54dc19: je 0x54dc66
0x54dc1b: mov esi, ecx
0x54dc1d: sar esi, 0x10
0x54dc20: test cx, cx
0x54dc23: jl 0x54dc66
0x54dc25: mov edx, dword ptr [0x724a50]
0x54dc2b: cmp cx, word ptr [edx + 0x20]
0x54dc2f: jge 0x54dc66
0x54dc31: movsx eax, word ptr [edx + 0x22]
0x54dc35: mov edi, dword ptr [edx + 0x34]
0x54dc38: movsx ecx, cx
0x54dc3b: imul eax, ecx
0x54dc3e: mov cx, word ptr [eax + edi]
0x54dc42: add eax, edi
0x54dc44: test cx, cx
0x54dc47: je 0x54dc66
0x54dc49: test si, si
0x54dc4c: je 0x54dc53
0x54dc4e: cmp cx, si
0x54dc51: jne 0x54dc66
0x54dc53: mov edi, dword ptr [esp + 0x14]
0x54dc57: lea esi, [eax + 0xc]
0x54dc5a: mov ecx, 0x10
0x54dc5f: rep movsd dword ptr es:[edi], dword ptr [esi]
0x54dc61: pop edi
0x54dc62: mov al, 1
0x54dc64: pop esi
0x54dc65: ret 
0x54dc66: pop edi
0x54dc67: xor al, al
0x54dc69: pop esi
0x54dc6a: ret 
#endif

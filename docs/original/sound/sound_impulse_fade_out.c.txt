// sound_impulse_fade_out  (Ghidra: FUN_00549ee0; earlier draft name sound_schedule_initial_fade_in)
// address 0x549ee0, size 110 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: callers hold a sound handle returned by an earlier start (first_person_weapon_set_state,
//   hud_waypoint_draw, player_health_pack_screen_effect, sound_impulse_start 0x543e10).
//   The datum check is the inlined datum_try_and_get pattern (index in range, identifier
//   nonzero, salt match when the handle carries one); play_state 0 is _sound_play_impulse.
// Phase-4 review (disassembly appended below): the call is
//   sound_schedule_gain_fade(EBX = none, 0, 0.3f, sound_index), i.e. the sound is the fade-OUT
//   side (fade_end_gain 0: it dies when the fade completes). The earlier draft read it as a
//   fade-in of a not-yet-started sound.
// register convention: ECX -> sound_index.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *sound_data; // 0x007252c0

extern void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle); // 0x54af60, blam-cc: EBX, stack

// blam-cc: ECX -> sound_index
// Stops a one-shot sound with a 0.3 s fade-out, if the handle is still live.
void sound_impulse_fade_out(datum_index sound_index)
{
    int16_t slot = (int16_t)sound_index;
    int16_t salt = (int16_t)(sound_index >> 16);
    int16_t identifier;

    if (sound_data == (data_array *)0 || sound_index == k_datum_index_none) {
        return;
    }
    if (slot < 0 || slot >= sound_data->maximum_count) {
        return;
    }
    identifier = *(int16_t *)((uint8_t *)sound_data->data + (int32_t)sound_data->size * slot);
    if (identifier == 0 || (salt != 0 && identifier != salt)) {
        return;
    }
    if (((sound *)sound_data->data)[sound_index & 0xffff].play_state == _sound_play_impulse) {
        sound_schedule_gain_fade(k_datum_index_none, _sound_fade_linear, 0.3f, sound_index);
    }
}

#if 0
Original Ghidra decompilation (0x549ee0):

void FUN_00549ee0(void)

{
  short sVar1;
  uint in_ECX;
  short sVar2;

  if ((((DAT_007252c0 != 0) && (in_ECX != 0xffffffff)) && (sVar1 = (short)in_ECX, -1 < sVar1)) &&
     (sVar1 < *(short *)(DAT_007252c0 + 0x20))) {
    sVar1 = *(short *)((int)*(short *)(DAT_007252c0 + 0x22) * (int)sVar1 +
                      *(int *)(DAT_007252c0 + 0x34));
    if (((sVar1 != 0) && ((sVar2 = (short)(in_ECX >> 0x10), sVar2 == 0 || (sVar1 == sVar2)))) &&
       (*(short *)((in_ECX & 0xffff) * 0xb0 + 2 + *(int *)(DAT_007252c0 + 0x34)) == 0)) {
      FUN_0054af60(0,0x3e99999a);
    }
  }
  return;
}

Disassembly (0x549ee0..0x549f4e, capstone; phase-4 review):

0x549ee0: mov eax, dword ptr [0x7252c0]
0x549ee5: test eax, eax
0x549ee7: je 0x549f4d
0x549ee9: cmp ecx, -1
0x549eec: je 0x549f4d
0x549eee: push esi
0x549eef: mov esi, ecx
0x549ef1: sar esi, 0x10
0x549ef4: test cx, cx
0x549ef7: jl 0x549f4c
0x549ef9: cmp cx, word ptr [eax + 0x20]
0x549efd: jge 0x549f4c
0x549eff: mov edx, dword ptr [eax + 0x34]
0x549f02: movsx eax, word ptr [eax + 0x22]
0x549f06: push edi
0x549f07: movsx edi, cx
0x549f0a: imul eax, edi
0x549f0d: add eax, edx
0x549f0f: mov ax, word ptr [eax]
0x549f12: test ax, ax
0x549f15: pop edi
0x549f16: je 0x549f4c
0x549f18: test si, si
0x549f1b: je 0x549f22
0x549f1d: cmp ax, si
0x549f20: jne 0x549f4c
0x549f22: mov eax, ecx
0x549f24: and eax, 0xffff
0x549f29: imul eax, eax, 0xb0
0x549f2f: cmp word ptr [eax + edx + 2], 0
0x549f35: jne 0x549f4c
0x549f37: push ebx
0x549f38: push ecx
0x549f39: push 0x3e99999a
0x549f3e: push 0
0x549f40: or ebx, 0xffffffff
0x549f43: call 0x54af60
0x549f48: add esp, 0xc
0x549f4b: pop ebx
0x549f4c: pop esi
0x549f4d: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

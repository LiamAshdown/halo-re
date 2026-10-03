// sound_evaluate_fade_gain  (Ghidra: FUN_0054e3c0, still unnamed there)
// address 0x54e3c0, size 269 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Evaluates a playback channel's current gain-fade
// progress between its scheduled start and end gain values."; fields match types/sound.h sound
// (fade_start_time/fade_end_time/fade_curve/fade_start_gain/fade_end_gain, 0xa4/0xa8/0x92/0x9c/
// 0xa0) exactly; sound_fade_curve_exponent (0x0069f510, "2.5") matches the pow() call.
// register convention: EAX -> sound_handle.
// Phase-4 review (disassembly appended below): the power curve raises to 1 / exponent
// (fld 1.0; fdiv [0x0069f510]) and the falling case mirrors it as 1 - (1 - t)^(1/e); the draft
// used t^e and 1 - t^e.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *sound_data;          // 0x007252c0, "sounds" 0x200 x 0xb0
extern int32_t sound_time;              // 0x0072520c
extern float sound_fade_curve_exponent; // 0x0069f510

extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow

// blam-cc: EAX -> sound_handle
// Evaluates a sound's current fade progress: linear time fraction between fade_start_time and
// fade_end_time, clamped to [0,1], optionally reshaped by the power fade curve (inverted when
// fading out, i.e. fade_end_gain <= fade_start_gain), then lerped between fade_start_gain and
// fade_end_gain. Returns 1.0 (full gain, unmodified) if there is no fade in progress. Clears the
// fade's start/end time once progress reaches 1.0.
float sound_evaluate_fade_gain(datum_index sound_handle)
{
    sound *instance;
    float t;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

    if (instance->fade_start_time == instance->fade_end_time) {
        return 1.0f;
    }

    t = (float)(sound_time - instance->fade_start_time) / (float)(instance->fade_end_time - instance->fade_start_time);
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }

    if (instance->fade_curve == _sound_fade_power) {
        // exponent 1 / 2.5: fast start when fading in, fast start of the drop when fading out
        if (instance->fade_end_gain <= instance->fade_start_gain) {
            t = (float)(1.0 - pow((double)(1.0f - t), (double)(1.0f / sound_fade_curve_exponent)));
        } else {
            t = (float)pow((double)t, (double)(1.0f / sound_fade_curve_exponent));
        }
    }

    if (t == 1.0f) {
        instance->fade_end_time = 0;
        instance->fade_start_time = 0;
    }

    return (instance->fade_end_gain - instance->fade_start_gain) * t + instance->fade_start_gain;
}

#if 0
Original Ghidra decompilation (0x54e3c0):

float10 FUN_0054e3c0(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  float10 fVar3;

  fVar3 = (float10)1.0;
  iVar2 = (in_EAX & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
  iVar1 = *(int *)(iVar2 + 0xa4);
  if (iVar1 != *(int *)(iVar2 + 0xa8)) {
    fVar3 = ((float10)DAT_0072520c - (float10)iVar1) / (float10)(*(int *)(iVar2 + 0xa8) - iVar1);
    if ((float10)0.0 <= fVar3) {
      if ((float10)1.0 < fVar3) {
        fVar3 = (float10)1.0;
      }
    }
    else {
      fVar3 = (float10)0.0;
    }
    if (*(short *)(iVar2 + 0x92) == 1) {
      if (*(float *)(iVar2 + 0xa0) <= *(float *)(iVar2 + 0x9c)) {
        fVar3 = (float10)FUN_006283c0();
        fVar3 = (float10)1.0 - fVar3;
      }
      else {
        fVar3 = (float10)FUN_006283c0();
      }
    }
    if (fVar3 == (float10)1.0) {
      *(undefined4 *)(iVar2 + 0xa8) = 0;
      *(undefined4 *)(iVar2 + 0xa4) = 0;
    }
    fVar3 = ((float10)*(float *)(iVar2 + 0xa0) - (float10)*(float *)(iVar2 + 0x9c)) * fVar3 +
            (float10)*(float *)(iVar2 + 0x9c);
  }
  return fVar3;
}

Disassembly (0x54e3c0..0x54e4cd, capstone; phase-4 review):

0x54e3c0: push ecx
0x54e3c1: mov ecx, dword ptr [0x7252c0]
0x54e3c7: fld dword ptr [0x672ac4]
0x54e3cd: and eax, 0xffff
0x54e3d2: imul eax, eax, 0xb0
0x54e3d8: push esi
0x54e3d9: add eax, dword ptr [ecx + 0x34]
0x54e3dc: mov esi, eax
0x54e3de: mov ecx, dword ptr [esi + 0xa4]
0x54e3e4: mov eax, dword ptr [esi + 0xa8]
0x54e3ea: cmp ecx, eax
0x54e3ec: mov dword ptr [esp + 4], ecx
0x54e3f0: je 0x54e4ca
0x54e3f6: fstp st(0)
0x54e3f8: sub eax, ecx
0x54e3fa: fild dword ptr [0x72520c]
0x54e400: fisub dword ptr [esp + 4]
0x54e404: mov dword ptr [esp + 4], eax
0x54e408: fidiv dword ptr [esp + 4]
0x54e40c: fcom dword ptr [0x672ac0]
0x54e412: fst dword ptr [esp + 4]
0x54e416: fnstsw ax
0x54e418: test ah, 5
0x54e41b: jp 0x54e427
0x54e41d: fstp st(0)
0x54e41f: fld dword ptr [0x672ac0]
0x54e425: jmp 0x54e43c
0x54e427: fcom dword ptr [0x672ac4]
0x54e42d: fnstsw ax
0x54e42f: test ah, 0x41
0x54e432: jne 0x54e440
0x54e434: fstp st(0)
0x54e436: fld dword ptr [0x672ac4]
0x54e43c: fst dword ptr [esp + 4]
0x54e440: movsx eax, word ptr [esi + 0x92]
0x54e447: dec eax
0x54e448: jne 0x54e497
0x54e44a: fstp st(0)
0x54e44c: fld dword ptr [esi + 0xa0]
0x54e452: fcomp dword ptr [esi + 0x9c]
0x54e458: fnstsw ax
0x54e45a: test ah, 0x41
0x54e45d: jne 0x54e476
0x54e45f: fld dword ptr [esp + 4]
0x54e463: fld dword ptr [0x672ac4]
0x54e469: fdiv dword ptr [0x69f510]
0x54e46f: call 0x6283c0
0x54e474: jmp 0x54e497
0x54e476: fld dword ptr [0x672ac4]
0x54e47c: fsub dword ptr [esp + 4]
0x54e480: fld dword ptr [0x672ac4]
0x54e486: fdiv dword ptr [0x69f510]
0x54e48c: call 0x6283c0
0x54e491: fsubr qword ptr [0x672af8]
0x54e497: fld dword ptr [0x672ac4]
0x54e49d: fld st(1)
0x54e49f: fucompp 
0x54e4a1: fnstsw ax
0x54e4a3: test ah, 0x44
0x54e4a6: jp 0x54e4b6
0x54e4a8: xor eax, eax
0x54e4aa: mov dword ptr [esi + 0xa8], eax
0x54e4b0: mov dword ptr [esi + 0xa4], eax
0x54e4b6: fld dword ptr [esi + 0xa0]
0x54e4bc: fsub dword ptr [esi + 0x9c]
0x54e4c2: fmulp st(1)
0x54e4c4: fadd dword ptr [esi + 0x9c]
0x54e4ca: pop esi
0x54e4cb: pop ecx
0x54e4cc: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

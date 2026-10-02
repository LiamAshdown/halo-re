// sound_gain_to_directsound_volume  (Ghidra: sound_gain_to_directsound_volume, already named)
// address 0x54ee70, size 71 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: out/phase4/sound_functions.md "Converts a linear gain value into a DirectSound-style
// logarithmic volume unit clamped to [-10000, param_2]."; same log10*2000 pattern established in
// sound_linear_gain_to_millibels_clamped.c (0x54cda0), with the 2000.0 scale confirmed by
// reading the float at 0x672e14. -0x2711 == -10001 matches k_sound_minimum_volume - 1.
// register convention: stack -> (gain, maximum), both Ghidra-recognized.
// Phase-4 review (disassembly 0x54ee70..0x54eeb6): `fiadd [esp+0xc]` adds maximum to the scaled
// log before truncation; the draft only used it as the clamp.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call

// Converts a linear gain to a millibel level: 2000*log10(gain) + maximum, clamped to
// [k_sound_minimum_volume, maximum] (so `maximum` shifts the 0 dB point, as for EAX reflections /
// reverb levels). A gain of 0 (silence) returns k_sound_minimum_volume
// directly.
/* The original converts with the game's __ftol (0x6391b4): FISTP of a chopped value into a 64-bit integer, of which
   only EAX is used. A NaN or out-of-range value becomes the integer indefinite 0x8000000000000000, whose low dword
   is 0 -- so log10 of a negative gain gives 0 (full volume), not the minimum volume a C cast would give. */
static int32_t ftol_low_dword(double value)
{
    if (value != value || value >= 9.2233720368547758e18 || value < -9.2233720368547758e18) return 0;
    return (int32_t)(long long)value;
}

int32_t sound_gain_to_directsound_volume(float gain, int32_t maximum)
{
    int32_t volume;

    if (gain != 0.0f) {
        volume = ftol_low_dword(log10((double)gain) * 2000.0 + (double)maximum); // fiadd: the ceiling is also an offset
        if (volume > k_sound_minimum_volume - 1) {
            if (maximum < volume) {
                volume = maximum;
            }
            return volume;
        }
    }
    return k_sound_minimum_volume;
}

#if 0
Original Ghidra decompilation (0x54ee70):

int sound_gain_to_directsound_volume(float param_1,int param_2)

{
  int iVar1;

  if (param_1 != 0.0) {
    log2((float10)param_1);
    iVar1 = __ftol();
    if (-0x2711 < iVar1) {
      if (param_2 < iVar1) {
        iVar1 = param_2;
      }
      return iVar1;
    }
  }
  return -10000;
}

Disassembly (0x54ee70..0x54eeb7, capstone; phase-4 review):

0x54ee70: fld dword ptr [0x672ac0]
0x54ee76: push esi
0x54ee77: fld dword ptr [esp + 8]
0x54ee7b: mov esi, dword ptr [esp + 0xc]
0x54ee7f: fucompp 
0x54ee81: fnstsw ax
0x54ee83: test ah, 0x44
0x54ee86: jp 0x54ee8f
0x54ee88: mov eax, 0xffffd8f0
0x54ee8d: pop esi
0x54ee8e: ret 
0x54ee8f: fld dword ptr [esp + 8]
0x54ee93: fldlg2 
0x54ee95: fxch st(1)
0x54ee97: fyl2x 
0x54ee99: fmul dword ptr [0x672e14]
0x54ee9f: fiadd dword ptr [esp + 0xc]
0x54eea3: call 0x6391b4
0x54eea8: cmp eax, 0xffffd8f0
0x54eead: jl 0x54ee88
0x54eeaf: cmp eax, esi
0x54eeb1: jle 0x54eeb5
0x54eeb3: mov eax, esi
0x54eeb5: pop esi
0x54eeb6: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

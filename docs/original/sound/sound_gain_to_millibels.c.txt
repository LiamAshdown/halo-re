// sound_gain_to_millibels  (Ghidra: sound_gain_to_millibels, already named, __cdecl)
// address 0x54eec0, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Converts a linear gain/occlusion factor into a
// clamped millibel attenuation value for DirectSound/EAX properties."; same log10*2000 pattern
// as sound_gain_to_directsound_volume.c (0x54ee70), confirmed by reading the float at 0x672e14.
// register convention: __cdecl, gain as the recognized parameter.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call

// Converts a linear gain/occlusion factor (1.0 = fully open, 0.0 = fully closed) to a millibel
// attenuation: 2000*log10(1-gain), clamped to [k_sound_minimum_volume, 0]. gain == 1.0 (no
// attenuation) returns k_sound_minimum_volume directly (the log would be undefined/-infinity).
/* The original converts with the game's __ftol (0x6391b4): FISTP of a chopped value into a 64-bit integer, of which
   only EAX is used. A NaN or out-of-range value becomes the integer indefinite 0x8000000000000000, whose low dword
   is 0 -- so log10 of a negative gain gives 0 (full volume), not the minimum volume a C cast would give. */
static int32_t ftol_low_dword(double value)
{
    if (value != value || value >= 9.2233720368547758e18 || value < -9.2233720368547758e18) return 0;
    return (int32_t)(long long)value;
}

int __cdecl sound_gain_to_millibels(float gain)
{
    int32_t millibels;

    if (1.0f - gain != 0.0f) {
        millibels = ftol_low_dword(log10((double)(1.0f - gain)) * 2000.0);
        if (millibels > k_sound_minimum_volume - 1) {
            if (millibels > 0) {
                millibels = 0;
            }
            return millibels;
        }
    }
    return k_sound_minimum_volume;
}

#if 0
Original Ghidra decompilation (0x54eec0):

int __cdecl sound_gain_to_millibels(float gain)

{
  int iVar1;

  if ((float10)1.0 - (float10)gain != (float10)0.0) {
    log2((float10)1.0 - (float10)gain);
    iVar1 = __ftol();
    if (-0x2711 < iVar1) {
      if (0 < iVar1) {
        iVar1 = 0;
      }
      return iVar1;
    }
  }
  return -10000;
}

Disassembly (0x54eec0..0x54ef02, capstone; phase-4 review):

0x54eec0: fld dword ptr [0x672ac4]
0x54eec6: fsub dword ptr [esp + 4]
0x54eeca: fld dword ptr [0x672ac0]
0x54eed0: fld st(1)
0x54eed2: fucompp 
0x54eed4: fnstsw ax
0x54eed6: test ah, 0x44
0x54eed9: jp 0x54eee3
0x54eedb: fstp st(0)
0x54eedd: mov eax, 0xffffd8f0
0x54eee2: ret 
0x54eee3: fldlg2 
0x54eee5: fxch st(1)
0x54eee7: fyl2x 
0x54eee9: fmul dword ptr [0x672e14]
0x54eeef: call 0x6391b4
0x54eef4: cmp eax, 0xffffd8f0
0x54eef9: jl 0x54eedd
0x54eefb: test eax, eax
0x54eefd: jle 0x54ef01
0x54eeff: xor eax, eax
0x54ef01: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

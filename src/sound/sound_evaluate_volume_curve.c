// sound_evaluate_volume_curve  (Ghidra: FUN_0054cdf0, still unnamed there)
// address 0x54cdf0, size 83 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Evaluates a volume curve for a given attenuation
// value, passing silence through unchanged and clamping the result to a min/max range."; -10000
// matches k_sound_minimum_volume (types/sound.h), the DSBVOLUME_MIN sentinel. Constants at
// 0x672b08 (10.0) and 0x672b00 (0.0005) confirmed by reading the binary.
// register convention: stack -> (attenuation, minimum, maximum) (all Ghidra-recognized).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow

// Converts a DirectSound-style attenuation (hundredths of a decibel) into a linear gain via
// 10^(attenuation*0.0005), clamped to [minimum, maximum]; the sentinel k_sound_minimum_volume
// passes `minimum` straight through as the result instead of evaluating the curve.
float sound_evaluate_volume_curve(int32_t attenuation, int32_t minimum, int32_t maximum)
{
    float value;

    if (attenuation == k_sound_minimum_volume) {
        return (float)minimum;
    }

    value = (float)pow(10.0, (double)attenuation * 0.0005);
    if ((float)minimum <= value && value <= (float)maximum) {
        return value;
    }
    if (value < (float)minimum) {
        return (float)minimum;
    }
    return (float)maximum;
}

#if 0
Original Ghidra decompilation (0x54cdf0):

float10 FUN_0054cdf0(int param_1,int param_2,int param_3)

{
  float fVar1;
  float10 fVar2;

  if (param_1 == -10000) {
    return (float10)param_2;
  }
  fVar2 = (float10)FUN_006283c0();
  fVar1 = (float)fVar2;
  fVar2 = (float10)param_2;
  if ((fVar2 <= (float10)fVar1) && (fVar2 = (float10)param_3, (float10)fVar1 <= fVar2)) {
    fVar2 = (float10)fVar1;
  }
  return fVar2;
}

Disassembly (0x54cdf0..0x54ce43, capstone; phase-4 review):

0x54cdf0: cmp dword ptr [esp + 4], 0xffffd8f0
0x54cdf8: jne 0x54cdff
0x54cdfa: fild dword ptr [esp + 8]
0x54cdfe: ret 
0x54cdff: fld qword ptr [0x672b08]
0x54ce05: fild dword ptr [esp + 4]
0x54ce09: fmul dword ptr [0x672b00]
0x54ce0f: call 0x6283c0
0x54ce14: fstp dword ptr [esp + 4]
0x54ce18: fild dword ptr [esp + 8]
0x54ce1c: fld dword ptr [esp + 4]
0x54ce20: fcomp st(1)
0x54ce22: fnstsw ax
0x54ce24: test ah, 5
0x54ce27: jnp 0x54ce42
0x54ce29: fstp st(0)
0x54ce2b: fild dword ptr [esp + 0xc]
0x54ce2f: fld dword ptr [esp + 4]
0x54ce33: fcomp st(1)
0x54ce35: fnstsw ax
0x54ce37: test ah, 0x41
0x54ce3a: je 0x54ce42
0x54ce3c: fstp st(0)
0x54ce3e: fld dword ptr [esp + 4]
0x54ce42: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

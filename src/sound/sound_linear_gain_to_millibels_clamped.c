// sound_linear_gain_to_millibels_clamped  (Ghidra: FUN_0054cda0, still unnamed there; 0 callers
// in this module -- dead code as shipped, kept for completeness)
// address 0x54cda0, size 65 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Converts a linear gain value into a clamped
// logarithmic (DirectSound-style) volume unit."; disassembly (scratchpad/disasm/disasm.py)
// resolves Ghidra's bare __ftol() call: `fldlg2; fyl2x` computes log10(gain), multiplied by the
// double constant at 0x672b10 (== 2000.0, confirmed by reading the binary) and added to
// `bias_and_maximum` before truncation -- i.e. millibels = 2000*log10(gain) + bias_and_maximum
// (the 2x factor converts an amplitude ratio's decibels to power-style millibels, matching
// sound_gain_to_millibels's own doubling further down this module). The 0.0 compared against
// at 0x672ac0 confirmed by reading the binary.
// register convention: stack -> (gain, bias_and_maximum), ESI -> minimum (unaff_ESI).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call

// blam-cc: stack -> (gain, bias_and_maximum), ESI -> minimum
// Converts a linear gain to a millibel-like integer volume: 0 returns `minimum` unconditionally;
// otherwise `2000*log10(gain) + bias_and_maximum`, clamped to [minimum, bias_and_maximum].
/* The original converts with the game's __ftol (0x6391b4): FISTP of a chopped value into a 64-bit integer, of which
   only EAX is used. A NaN or out-of-range value becomes the integer indefinite 0x8000000000000000, whose low dword
   is 0 -- so log10 of a negative gain gives 0 (full volume), not the minimum volume a C cast would give. */
static int32_t ftol_low_dword(double value)
{
    if (value != value || value >= 9.2233720368547758e18 || value < -9.2233720368547758e18) return 0;
    return (int32_t)(long long)value;
}

int32_t sound_linear_gain_to_millibels_clamped(int32_t minimum, float gain, int32_t bias_and_maximum)
{
    int32_t result;

    if (gain != 0.0f) {
        result = ftol_low_dword(log10((double)gain) * 2000.0 + (float)bias_and_maximum);
        if (minimum <= result) {
            if (bias_and_maximum < result) {
                result = bias_and_maximum;
            }
            return result;
        }
    }
    return minimum;
}

#if 0
Original Ghidra decompilation (0x54cda0):

int FUN_0054cda0(float param_1,int param_2)

{
  int iVar1;
  int unaff_ESI;

  if (param_1 != 0.0) {
    log2((float10)param_1);
    iVar1 = __ftol();
    if (unaff_ESI <= iVar1) {
      if (param_2 < iVar1) {
        iVar1 = param_2;
      }
      return iVar1;
    }
  }
  return unaff_ESI;
}

Disassembly (0x54cda0..0x54cde0, capstone) resolving the __ftol() dataflow:

0x54cda0: fld dword ptr [0x672ac0]
0x54cda7: fld dword ptr [esp + 8]
0x54cdab: mov edi, dword ptr [esp + 0xc]
0x54cdaf: fucompp
0x54cdbc: fld dword ptr [esp + 8]
0x54cdc0: fldlg2
0x54cdc2: fxch st(1)
0x54cdc4: fyl2x
0x54cdc6: fmul qword ptr [0x672b10]
0x54cdcc: fiadd dword ptr [esp + 0xc]
0x54cdd0: call 0x6391b4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

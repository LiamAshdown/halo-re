// sound_linear_gain_to_attenuation  (Ghidra: sound_linear_gain_to_attenuation, already named)
// address 0x545710, size 71 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Converts a linear gain value to a clamped
//   logarithmic (centibel-style) attenuation value for DirectSound APIs."; the sibling function
//   src/sound/sound_linear_gain_to_millibels_clamped.c (0x54cda0) references the exact same two
//   globals (0x672ac0, 0x672b10) and its own disassembly resolves Ghidra's bare log2()+__ftol()
//   call sequence to `fldlg2; fyl2x; fmul qword ptr [0x672b10]` -- i.e. log10(gain) * 2000.0 (that
//   file's header explains the 2x scale: converting an amplitude-ratio decibel value to
//   power-style millibels). This function omits that sibling's trailing `fiadd` bias term (it has
//   no third parameter), and its floor/clamp constant -10000 matches
//   k_sound_minimum_volume (types/sound.h).
// register convention: plain __cdecl, gain and maximum as the two stack parameters Ghidra
//   recognizes directly.
// blam-cc: stack -> (gain, maximum)
// Phase-4 review (disassembly 0x545710..0x545756): the scaled log gets `maximum` added
// (fiadd) before truncation, like sound_gain_to_directsound_volume (0x54ee70); the draft only
// clamped with it. Every caller in the module passes maximum 0.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern double log10(double x); // FYL2X with LG2, Ghidra's log2()+scale pseudo-call; see file header

// blam-cc: stack -> (gain, maximum)
// Converts a linear gain to a centibel-like attenuation value (2000*log10(gain), truncated),
// clamped to [k_sound_minimum_volume, maximum]. A zero gain returns k_sound_minimum_volume
// unconditionally.
/* The original converts with the game's __ftol (0x6391b4): FISTP of a chopped value into a 64-bit integer, of which
   only EAX is used. A NaN or out-of-range value becomes the integer indefinite 0x8000000000000000, whose low dword
   is 0 -- so log10 of a negative gain gives 0 (full volume), not the minimum volume a C cast would give. */
static int32_t ftol_low_dword(double value)
{
    if (value != value || value >= 9.2233720368547758e18 || value < -9.2233720368547758e18) return 0;
    return (int32_t)(long long)value;
}

int32_t sound_linear_gain_to_attenuation(float gain, int32_t maximum)
{
    if (gain != 0.0f) {
        int32_t result = ftol_low_dword(log10((double)gain) * 2000.0 + (double)maximum); // fiadd [esp+0xc]

        if (k_sound_minimum_volume <= result) {
            if (maximum < result) {
                result = maximum;
            }
            return result;
        }
    }

    return k_sound_minimum_volume;
}

#if 0
Original Ghidra decompilation (0x545710):

int sound_linear_gain_to_attenuation(float param_1,int param_2)

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

Disassembly (0x545710..0x545757, capstone; phase-4 review):

0x545710: fld dword ptr [0x672ac0]
0x545716: push esi
0x545717: fld dword ptr [esp + 8]
0x54571b: mov esi, dword ptr [esp + 0xc]
0x54571f: fucompp 
0x545721: fnstsw ax
0x545723: test ah, 0x44
0x545726: jp 0x54572f
0x545728: mov eax, 0xffffd8f0
0x54572d: pop esi
0x54572e: ret 
0x54572f: fld dword ptr [esp + 8]
0x545733: fldlg2 
0x545735: fxch st(1)
0x545737: fyl2x 
0x545739: fmul qword ptr [0x672b10]
0x54573f: fiadd dword ptr [esp + 0xc]
0x545743: call 0x6391b4
0x545748: cmp eax, 0xffffd8f0
0x54574d: jl 0x545728
0x54574f: cmp eax, esi
0x545751: jle 0x545755
0x545753: mov eax, esi
0x545755: pop esi
0x545756: ret 
#endif

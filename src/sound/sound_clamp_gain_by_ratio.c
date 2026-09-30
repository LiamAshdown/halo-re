// sound_clamp_gain_by_ratio  (Ghidra: FUN_0054e660, still unnamed there)
// address 0x54e660, size 103 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Clamps a gain value against a scaled/divided
// comparison value, used when applying a looping sound's distance-based gain."
// register convention: three plain stack floats (Ghidra recognized all three directly).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "fn_sound.h"

// Clamps `gain` against `compare` scaled or divided by `ratio`: if compare < gain, clamps gain
// down to whichever of gain/(compare*ratio) is smaller; if compare > gain, clamps gain up to
// compare/ratio (if that is still >= gain). A zero ratio or compare == gain leaves gain
// unchanged.
float sound_clamp_gain_by_ratio(float gain, float compare, float ratio)
{
    float scaled;

    if (ratio != 0.0f && gain != compare) {
        if (compare < gain) {
            scaled = compare * ratio;
            if (scaled < gain) {
                return scaled;
            }
            return gain;
        }
        scaled = compare / ratio;
        if (gain <= scaled) {
            return scaled;
        }
    }
    return gain;
}

#if 0
Original Ghidra decompilation (0x54e660):

float10 FUN_0054e660(float param_1,float param_2,float param_3)

{
  float10 fVar1;

  if ((param_3 != 0.0) && (param_1 != param_2)) {
    if (param_2 < param_1) {
      fVar1 = (float10)param_2 * (float10)param_3;
      if (fVar1 < (float10)param_1) {
        return fVar1;
      }
      return (float10)param_1;
    }
    fVar1 = (float10)param_2 / (float10)param_3;
    if ((float10)param_1 <= fVar1) {
      return fVar1;
    }
  }
  return (float10)param_1;
}

Disassembly (0x54e660..0x54e6c7, capstone; phase-4 review):

0x54e660: fld dword ptr [0x672ac0]
0x54e666: fld dword ptr [esp + 0xc]
0x54e66a: fucompp 
0x54e66c: fnstsw ax
0x54e66e: test ah, 0x44
0x54e671: jnp 0x54e6c2
0x54e673: fld dword ptr [esp + 8]
0x54e677: fld dword ptr [esp + 4]
0x54e67b: fucompp 
0x54e67d: fnstsw ax
0x54e67f: test ah, 0x44
0x54e682: jnp 0x54e6c2
0x54e684: fld dword ptr [esp + 4]
0x54e688: fcomp dword ptr [esp + 8]
0x54e68c: fld dword ptr [esp + 8]
0x54e690: fnstsw ax
0x54e692: test ah, 0x41
0x54e695: jne 0x54e6af
0x54e697: fmul dword ptr [esp + 0xc]
0x54e69b: fld dword ptr [esp + 4]
0x54e69f: fcomp st(1)
0x54e6a1: fnstsw ax
0x54e6a3: test ah, 0x41
0x54e6a6: je 0x54e6c6
0x54e6a8: fstp st(0)
0x54e6aa: fld dword ptr [esp + 4]
0x54e6ae: ret 
0x54e6af: fdiv dword ptr [esp + 0xc]
0x54e6b3: fld dword ptr [esp + 4]
0x54e6b7: fcomp st(1)
0x54e6b9: fnstsw ax
0x54e6bb: test ah, 0x41
0x54e6be: jne 0x54e6c6
0x54e6c0: fstp st(0)
0x54e6c2: fld dword ptr [esp + 4]
0x54e6c6: ret 
#endif

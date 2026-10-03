// sound_reverb_size_scale  (Ghidra: FUN_00550c10, still unnamed there)
// address 0x550c10, size 91 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Normalizes a frequency-like environment parameter
// (roughly in the 20-20000 Hz range) into the scale expected by an EAX 3.0 reverb property, with
// special-cased boundary values."; called by sound_eax30_effect_apply_listener.c (0x550c70) per
// that function's own summary.
// register convention: plain stack float (Ghidra recognized param_1 directly).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"

// Maps SoundEnvironment +0x34 onto the value written to EAX 3.0 listener property 0x16
// (HF reference): 1000 at or below 20, 20000 at or above 20000, 5000 kept exactly, otherwise
// value * 5.005005e-05 * 19000 (constants at 0x00672aec..0x00672ad8, checked in the phase-4
// review).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
float sound_reverb_size_scale(float value)
{
    if (value <= 20.0f) {
        return 1000.0f;
    }
    if (value >= 20000.0f) {
        return 20000.0f;
    }
    if (value == 5000.0f) {
        return 5000.0f;
    }
    return value * 5.005005e-05f * 19000.0f;
}

#if 0
Original Ghidra decompilation (0x550c10):

float10 FUN_00550c10(float param_1)

{
  if (param_1 < 20.0 != (param_1 == 20.0)) {
    return (float10)1000.0;
  }
  if (20000.0 <= param_1) {
    return (float10)20000.0;
  }
  if (param_1 == 5000.0) {
    return (float10)5000.0;
  }
  return (float10)param_1 * (float10)5.005005e-05 * (float10)19000.0;
}

Disassembly (0x550c10..0x550c6b, capstone; phase-4 review):

0x550c10: fld dword ptr [esp + 4]
0x550c14: fcomp dword ptr [0x672aec]
0x550c1a: fnstsw ax
0x550c1c: test ah, 0x41
0x550c1f: jp 0x550c28
0x550c21: fld dword ptr [0x672ae8]
0x550c27: ret 
0x550c28: fld dword ptr [esp + 4]
0x550c2c: fcomp dword ptr [0x672ae4]
0x550c32: fnstsw ax
0x550c34: test ah, 1
0x550c37: jne 0x550c40
0x550c39: fld dword ptr [0x672ae4]
0x550c3f: ret 
0x550c40: fld dword ptr [0x672ae0]
0x550c46: fld dword ptr [esp + 4]
0x550c4a: fucompp 
0x550c4c: fnstsw ax
0x550c4e: test ah, 0x44
0x550c51: jp 0x550c5a
0x550c53: fld dword ptr [0x672ae0]
0x550c59: ret 
0x550c5a: fld dword ptr [esp + 4]
0x550c5e: fmul dword ptr [0x672adc]
0x550c64: fmul dword ptr [0x672ad8]
0x550c6a: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

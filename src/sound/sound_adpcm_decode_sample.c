// sound_adpcm_decode_sample  (Ghidra: FUN_0054e8c0, still unnamed there; 0 callers in this
// module -- reached only through the decode proc table, out of this module's static call graph)
// address 0x54e8c0, size 92 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Computes one clamped 16-bit delta-decoded sample from
// a selector byte and shifted prediction value, as used by an ADPCM-style codec."
// register convention: stack -> (selector, prediction, step), EDX -> unused pass-through high
// half of the 64-bit Ghidra return (in_EDX, preserved literally though this rewrite returns a
// plain 32-bit value since only the low half is ever meaningful for a 16-bit sample).
// UNSURE: this is generic ADPCM bit arithmetic with no module-specific struct fields; transcribed
// literally rather than re-derived from first principles.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"

// VERIFIED against disassembly 0x54e8c0..0x54e91a (2026-09-30) (the rcl/sbb bit selects were emulated against this formula
//   for 100000 random inputs with no difference); the add is now a wrapping unsigned add so the clamp tests see the
//   same int32 value the original does even when delta + prediction overflows.
// Decodes one ADPCM sample: forms a signed delta from `step` gated by the low 3 bits of
// `selector` (bit2 adds step, bit1 adds step/2, bit0 adds step/4, always adds step/8, negated
// when bit3 of `selector` is set), adds it to `prediction`, and clamps to a signed 16-bit range.
int32_t sound_adpcm_decode_sample(uint8_t selector, int32_t prediction, uint32_t step)
{
    uint32_t negate;
    int32_t delta;

    negate = (uint32_t)((selector >> 3 & 1) != 0);
    delta = (int32_t)((((-(uint32_t)((selector & 4) != 0) & step) +
                        (-(uint32_t)((selector & 2) != 0) & (step >> 1)) +
                        (-(uint32_t)((selector & 1) != 0) & (step >> 2)) + (step >> 3)) ^ -negate) +
                       negate);
    prediction = (int32_t)((uint32_t)delta + (uint32_t)prediction); // wrapping add (signed overflow is UB in C)

    if (prediction < 0x8000) {
        if (prediction < -0x8000) {
            prediction = -0x8000;
        }
    } else {
        prediction = 0x7fff;
    }
    return prediction;
}

#if 0
Original Ghidra decompilation (0x54e8c0):

undefined8 FUN_0054e8c0(byte param_1,int param_2,uint param_3)

{
  uint uVar1;
  undefined4 in_EDX;

  uVar1 = (uint)((param_1 >> 3 & 1) != 0);
  param_2 = ((-(uint)((param_1 & 4) != 0) & param_3) + (-(uint)((param_1 & 2) != 0) & param_3 >> 1)
             + (-(uint)((param_1 & 1) != 0) & param_3 >> 2) + (param_3 >> 3) ^ -uVar1) + uVar1 +
            param_2;
  if (param_2 < 0x8000) {
    if (param_2 < -0x8000) {
      param_2 = -0x8000;
    }
  }
  else {
    param_2 = 0x7fff;
  }
  return CONCAT44(in_EDX,param_2);
}

Disassembly (0x54e8c0..0x54e91c, capstone; phase-4 review):

0x54e8c0: push ebp
0x54e8c1: mov ebp, esp
0x54e8c3: push ebx
0x54e8c4: push ecx
0x54e8c5: push edx
0x54e8c6: xor eax, eax
0x54e8c8: mov ecx, dword ptr [ebp + 0x10]
0x54e8cb: mov bl, byte ptr [ebp + 8]
0x54e8ce: rcl bl, 6
0x54e8d1: sbb edx, edx
0x54e8d3: and edx, ecx
0x54e8d5: add eax, edx
0x54e8d7: shr ecx, 1
0x54e8d9: rcl bl, 1
0x54e8db: sbb edx, edx
0x54e8dd: and edx, ecx
0x54e8df: add eax, edx
0x54e8e1: shr ecx, 1
0x54e8e3: rcl bl, 1
0x54e8e5: sbb edx, edx
0x54e8e7: and edx, ecx
0x54e8e9: add eax, edx
0x54e8eb: shr ecx, 1
0x54e8ed: add eax, ecx
0x54e8ef: rcl bl, 6
0x54e8f2: sbb edx, edx
0x54e8f4: xor eax, edx
0x54e8f6: sub eax, edx
0x54e8f8: add eax, dword ptr [ebp + 0xc]
0x54e8fb: cmp eax, 0x7fff
0x54e900: jg 0x54e90e
0x54e902: cmp eax, 0xffff8000
0x54e907: jl 0x54e915
0x54e909: pop edx
0x54e90a: pop ecx
0x54e90b: pop ebx
0x54e90c: pop ebp
0x54e90d: ret 
0x54e90e: mov eax, 0x7fff
0x54e913: jmp 0x54e909
0x54e915: mov eax, 0xffff8000
0x54e91a: jmp 0x54e909
#endif

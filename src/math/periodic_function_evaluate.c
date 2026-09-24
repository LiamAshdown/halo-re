// periodic_function_evaluate  (Ghidra: periodic_function_evaluate, already named)
// address 0x4cc9b0, size 264 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: types/math.h periodic_function section (table size 0x400, mask 0x3ff, entry =
//   value*255, time scale t*25.6, wrapping mask 0xc0 for the two slide types). "type == 0"
//   returns the constant 1.0 without even checking whether the tables are initialized, matching
//   _periodic_function_one being case 0 in periodic_function_build_table @0x4ccdb0.
// register convention: type index in AX (in_AX, low half of EAX); time as the recognized stack
//   parameter (param_1).
//   // blam-cc: AX (low half of EAX) -> type, stack -> time
//
// VERIFIED against the disassembly at 0x4cc9b0 (objdump -d -M intel):
//   0x4cc9d4  fld QWORD [esp+0x14] / fmul QWORD ds:0x672d78   scaled = time * 25.600000381469727
//   0x4cc9de  fst QWORD [esp+0x14]                            scaled spilled
//   0x4cc9e2  fld QWORD ds:0x672af8 (1.0) / call 0x628cca     frac = fmod(scaled, 1.0)
//   0x4cc9f1  fsubr QWORD [esp+0x14] / fistp [esp+0xc]        index = lrint(scaled - frac)
//   0x4cca0f  and eax,0x3ff / movzx two bytes, second index (eax+1) & 0x3ff
//   0x4cca26  mov edx,1 / shl edx,cl / test dl,0xc0           the slide wrap test
//   0x4cca4c  fcomp ds:0x672d70 (0.75), fcom ds:0x672b8c (0.25), fadd ds:0x672ac4 (1.0)
// 0x628cca is the MSVC 7.1 CRT _CIfmod (x in ST(1), y in ST(0)) and the second operand is the
// double 1.0 at 0x672af8, so the earlier "unknown one-argument helper" reading is confirmed and
// now spelled fmod(x, 1.0). The index rounding is x87 fistp under the default control word,
// i.e. round-half-to-even; `lrint` below is the C spelling of that, not `round`.

#include "tags.h"
#include "math.h"

extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod: x in ST(1), y in ST(0)
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)

extern periodic_function_table *periodic_function_tables[12]; // 0x006b7aa8
extern uint8_t periodic_functions_initialized; // 0x006b7af0

// Samples the precomputed periodic (wave) function table selected by the type index in AX at
// the given time, returning an interpolated [0,1] value.
real periodic_function_evaluate(periodic_function_t type, double time)
{
    real frac_part;
    real sample_a, sample_b;
    uint32_t index;
    periodic_function_table *table;
    double scaled_time;

    if (type == 0) {
        return 1.0f;
    }
    if (periodic_functions_initialized == 0) {
        return 0.0f;
    }

    scaled_time = time * 25.600000381469727;
    frac_part = (real)fmod(scaled_time, 1.0);
    index = (uint32_t)(int32_t)lrint(scaled_time - (double)frac_part) & k_periodic_function_table_mask;
    table = periodic_function_tables[type];
    sample_a = (real)table->samples[index] * 0.003921569f;
    sample_b = (real)table->samples[(index + 1) & k_periodic_function_table_mask] * 0.003921569f;

    if (((1 << (type & 0x1f)) & k_periodic_function_wrapping_mask) == 0) {
        return sample_b * frac_part + (1.0f - frac_part) * sample_a;
    }
    if (0.75f < sample_a && sample_b < 0.25f) {
        sample_b = sample_b + 1.0f;
    }
    sample_a = sample_b * frac_part + (1.0f - frac_part) * sample_a;
    if (1.0f < sample_a) {
        return sample_a - 1.0f;
    }
    return sample_a;
}

#if 0
Original Ghidra decompilation (0x4cc9b0):

float10 periodic_function_evaluate(double param_1)

{
  float fVar1;
  short in_AX;
  uint uVar2;
  float10 fVar3;
  float10 fVar4;

  if (in_AX == 0) {
    return (float10)1.0;
  }
  if (DAT_006b7af0 == '\0') {
    fVar3 = (float10)0.0;
  }
  else {
    fVar3 = (float10)FUN_00628cca();
    fVar1 = (float)fVar3;
    uVar2 = (int)ROUND((float)((float10)(param_1 * 25.600000381469727) - fVar3)) & 0x3ff;
    fVar3 = (float10)*(byte *)((&DAT_006b7aa8)[in_AX] + uVar2) * (float10)0.003921569;
    fVar4 = (float10)*(byte *)((uVar2 + 1 & 0x3ff) + (&DAT_006b7aa8)[in_AX]) * (float10)0.003921569;
    if ((1 << ((byte)in_AX & 0x1f) & 0xc0U) == 0) {
      return fVar4 * (float10)fVar1 + ((float10)1.0 - (float10)fVar1) * fVar3;
    }
    if (((float10)0.75 < fVar3) && (fVar4 < (float10)0.25)) {
      fVar4 = fVar4 + (float10)1.0;
    }
    fVar3 = fVar4 * (float10)fVar1 + ((float10)1.0 - (float10)fVar1) * fVar3;
    if ((float10)1.0 < fVar3) {
      return fVar3 - (float10)1.0;
    }
  }
  return fVar3;
}
#endif

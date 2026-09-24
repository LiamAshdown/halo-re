// transition_function_evaluate  (Ghidra: FUN_004ccac0; renamed, Blam-style, not previously named)
// address 0x4ccac0, size 238 bytes
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md ("Samples one of the secondary periodic-function
//   tables at an explicit [0,1] phase rather than by elapsed time"); types/math.h
//   transition_function enum (6 cases, table = transition_function_tables). type==0 returns the
//   clamped phase directly without touching the tables, matching _transition_function_linear.
// register convention: transition type in CX (in_CX, low half of ECX); phase as the recognized
//   stack parameter (param_1).
//   // blam-cc: CX (low half of ECX) -> type, stack -> phase
//
// VERIFIED against the disassembly at 0x4ccac0 (objdump -d -M intel):
//   0x4ccb05  fmul ds:0x672d6c              x = clamped * 1023.0
//   0x4ccb16  fst [esp+0x10]                x spilled
//   0x4ccb1a  fld QWORD ds:0x672af8 (1.0) / call 0x628cca    frac = fmod(x, 1.0)
//   0x4ccb29  fld [esp+0x10] / fsub ds:0x672abc (0.5)        x - 0.5
//   0x4ccb3b  fistp [esp+0x8]               index = lrint(x - 0.5), x87 round-to-nearest-even
//   0x4ccb43  cmp ax,0x3ff / jne            the last entry is returned flat, so the lerp never
//                                           reads samples[1024]
//   0x4ccb63  movzx a=[esi+index], b=[esi+index+1], both * 0.003921569, lerp by frac
// Two corrections to the earlier reading:
//   * frac is fmod(x, 1.0) of the UNBIASED x -- the -0.5 is applied only to the value that
//     gets rounded to an index. The earlier version took the fractional part of (x - 0.5),
//     which shifts the interpolation weight by half a table entry everywhere.
//   * 0x628cca is the CRT _CIfmod (x in ST(1), y in ST(0)) and the second operand is the
//     double 1.0 read straight out of 0x672af8, so it is exactly frac(x), not an unknown.
// The index rounding is x87 fistp under the default control word, i.e. round-half-to-even;
// `lrint` below is the C spelling of that, not `round`.

#include "tags.h"
#include "math.h"

extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod: x in ST(1), y in ST(0)
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)

extern periodic_function_table *transition_function_tables[6]; // 0x006b7ad8
extern uint8_t periodic_functions_initialized; // 0x006b7af0

// Samples one of the secondary periodic-function tables at an explicit [0,1] phase rather than
// by elapsed time.
real transition_function_evaluate(transition_function_t type, real phase)
{
    real clamped;
    real frac_part;
    int16_t index;
    periodic_function_table *table;
    double x;

    clamped = phase;
    if (0.0f <= clamped) {
        if (1.0f < clamped) {
            clamped = 1.0f;
        }
    } else {
        clamped = 0.0f;
    }

    if (type == 0) {
        return clamped;
    }
    if (periodic_functions_initialized == 0) {
        return 0.0f;
    }

    table = transition_function_tables[type];
    x = (double)(clamped * 1023.0f);
    frac_part = (real)fmod(x, 1.0);
    index = (int16_t)(int32_t)lrint(x - 0.5);
    if (index != 0x3ff) {
        return (real)table->samples[index + 1] * 0.003921569f * frac_part +
               (1.0f - frac_part) * (real)table->samples[index] * 0.003921569f;
    }
    return (real)table->samples[0x3ff] * 0.003921569f;
}

#if 0
Original Ghidra decompilation (0x4ccac0):

float10 FUN_004ccac0(float param_1)

{
  int iVar1;
  short sVar2;
  short in_CX;
  float10 fVar3;
  float10 fVar4;

  fVar3 = (float10)param_1;
  if ((float10)0.0 <= fVar3) {
    if ((float10)1.0 < fVar3) {
      fVar3 = (float10)1.0;
    }
  }
  else {
    fVar3 = (float10)0.0;
  }
  if (in_CX != 0) {
    if (DAT_006b7af0 == '\0') {
      return (float10)0.0;
    }
    iVar1 = (&DAT_006b7ad8)[in_CX];
    fVar4 = (float10)FUN_00628cca();
    sVar2 = (short)(int)ROUND((float)(fVar3 * (float10)1023.0) - 0.5);
    if (sVar2 != 0x3ff) {
      return (float10)*(byte *)(sVar2 + iVar1 + 1) * (float10)0.003921569 * (float10)(float)fVar4 +
             ((float10)1.0 - (float10)(float)fVar4) *
             (float10)*(byte *)(sVar2 + iVar1) * (float10)0.003921569;
    }
    fVar3 = (float10)*(byte *)(iVar1 + 0x3ff) * (float10)0.003921569;
  }
  return fVar3;
}
#endif

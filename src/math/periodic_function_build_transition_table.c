// periodic_function_build_transition_table  (Ghidra: FUN_004cccb0; renamed, Blam-style, not previously named)
// address 0x4cccb0, size 229 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md ("Builds one of the six secondary periodic-function
//   byte tables"); types/math.h transition_function enum (6 cases matching FunctionType).
//   The six easing curves read out of the disassembly are exactly linear / t^0.5 / t^0.25 /
//   t^2 / t^4 / raised cosine, in that order, which is what confirms the enum order.
// register convention: transition type in BX, sign-extended (movsx ebx,bx at 0x4cccc2); output
//   byte table as the single stack parameter.
//   // blam-cc: EBX (low half, sign-extended) -> type, stack -> table
//
// VERIFIED against the disassembly at 0x4cccb0 (objdump -d -M intel) plus direct reads of every
// constant out of bin/halo.exe. Ghidra hides all of this because both pow() operands travel on
// the x87 stack and the *255 / +1 / *0.5 steps sit after the switch:
//   jump table at 0x4ccd98 = {0x4cccea, 0x4cccf2, 0x4ccd05, 0x4ccd18, 0x4ccd2b, 0x4ccd3e}
//   0x4cccd3  fild [esp+0xc] / fmul ds:0x672b74   t = i * 0.0009775171056389809  (== 1/1023)
//   case 0    fld t                                value = t                     (linear)
//   case 1    fld t / fld qword ds:0x672cf0 / call 0x6283c0   pow(t, 0.5)
//   case 2    fld t / fld qword ds:0x672d58 / call 0x6283c0   pow(t, 0.25)
//   case 3    fld t / fld qword ds:0x672d50 / call 0x6283c0   pow(t, 2.0)
//   case 4    fld t / fld qword ds:0x672d48 / call 0x6283c0   pow(t, 4.0)
//   case 5    fmul ds:0x672c24 (pi) / fsub ds:0x672d3c (pi/2) / fsin
//             / fadd ds:0x672ac4 (1.0) / fmul ds:0x672abc (0.5)
//   0x4ccd5e  fld ds:0x672b60 (255.0) / fmul st,st(1) / call 0x6391b4 (__ftol), then clamp 0..255
// 0x6283c0 is the MSVC 7.1 CRT _CIpow: base in ST(1), exponent in ST(0), so the operand order
// above is pow(t, exponent) and not the reverse.
//
// Three earlier readings of this function were wrong and are corrected here:
//   * case 0 is NOT dead/zero -- it stores t, giving the linear ramp 0..255.
//   * case 5 is the *raised* cosine (sin(...) + 1) * 0.5, not the raw sine; without the
//     +1 and *0.5 the first half of that table would clamp to 0.
//   * the *255 byte scale is real and applies to every case, including case 0.
// The exponents 0.5 / 0.25 / 2.0 / 4.0, previously guessed from the case names, are confirmed.
//
// The x87 stack is used as a one-slot carry across iterations: each case begins with
// `fstp st(0)` to drop the previous iteration's value (which survives because __ftol pops only
// the scaled copy), and a type outside 0..5 falls straight through to the scale step and
// re-emits the previous iteration's value. That is transcribed below as a `default` that
// leaves `value` unchanged.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// sin/pow are the C-library spellings of the x87 FSIN instruction and the CRT _CIpow this code
// calls (Ghidra's fsin() pseudo-call and FUN_006283c0); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double sin(double x);
extern double pow(double base, double exponent);
extern int __ftol(double value); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation

// Builds one of the six transition_function easing tables: 1024 bytes, entry i = curve(i/1023)*255.
void periodic_function_build_transition_table(transition_function_t type, uint8_t *table)
{
    int32_t i;
    real t;
    real value;
    int32_t scaled;

    value = 0.0f; // the FPU slot starts at 0.0 (fld of the zeroed spill at 0x4cccbd)

    for (i = 0; i < 1024; i++) {
        t = (real)i * 0.0009775171f; // i / 1023

        switch ((int32_t)type) {
        case _transition_function_linear:
            value = t;
            break;
        case _transition_function_early:
            value = (real)pow((double)t, 0.5);
            break;
        case _transition_function_very_early:
            value = (real)pow((double)t, 0.25);
            break;
        case _transition_function_late:
            value = (real)pow((double)t, 2.0);
            break;
        case _transition_function_very_late:
            value = (real)pow((double)t, 4.0);
            break;
        case _transition_function_cosine:
            value = ((real)sin((double)(t * 3.1415927f - 1.5707964f)) + 1.0f) * 0.5f;
            break;
        default:
            break; // out of range: the previous value is re-emitted, see header note
        }

        scaled = __ftol((double)(255.0f * value));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        table[i] = (uint8_t)scaled;
    }
}

#if 0
Original Ghidra decompilation (0x4cccb0):

void FUN_004cccb0(int param_1)

{
  int iVar1;
  undefined2 unaff_BX;
  int iVar2;
  int local_4;

  local_4 = 0;
  iVar2 = 0x400;
  do {
    switch(unaff_BX) {
    case 0:
      break;
    case 1:
      FUN_006283c0();
      break;
    case 2:
      FUN_006283c0();
      break;
    case 3:
      FUN_006283c0();
      break;
    case 4:
      FUN_006283c0();
      break;
    case 5:
      fsin((float10)((float)local_4 * 0.0009775171) * (float10)3.1415927 - (float10)1.5707964);
    }
    iVar1 = FUN_006391b4();
    if (iVar1 < 0) {
      iVar1 = 0;
    }
    else if (0xff < iVar1) {
      iVar1 = 0xff;
    }
    *(char *)(local_4 + param_1) = (char)iVar1;
    local_4 = local_4 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

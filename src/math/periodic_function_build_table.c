// periodic_function_build_table  (Ghidra: periodic_function_build_table, already named)
// address 0x4ccdb0, size 644 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: types/math.h periodic_function section: the 12-case switch lines up one for one
//   with the WaveFunction enum. The disassembly settles the order beyond doubt -- the
//   "_variable_period" cases (3, 5, 7) and spark (11) are exactly the cases that read the
//   noise-warped phase instead of the linear one, and the two slide cases (6, 7) are exactly
//   the ones the final pass refuses to normalize.
// register convention: __cdecl, both parameters on the stack; type is a 16-bit slot read with
//   movsx (0x4ccddb), so it is a signed short. The noise table is handed to
//   periodic_function_build_noise_table @0x4ccbb0 in EDX (lea edx,[esp+0x20] at 0x4ccdbe).
//
// VERIFIED against the disassembly at 0x4ccdb0 (objdump -d -M intel) plus direct reads of every
// constant and of the 12-entry jump table at 0x4cd040 out of bin/halo.exe.
//
// Two phases over a 0x2010-byte stack frame (mov eax,0x2010 / call __chkstk):
//   [esp+0x20]   real noise[1024]   filled by periodic_function_build_noise_table
//   [esp+0x1020] real wave[1024]    the unquantized samples
//   [esp+0x18] minimum, [esp+0x1c] maximum, seeded +FLT_MAX / -FLT_MAX
//
// Phase 1, per sample i (0x4cce00):
//   fild [esp+0x10] / fmul ds:0x672e9c / fstp [esp+0x14]        t = i * 0.02734374813735485
//   fld [esp+esi*4+0x20] / fmul ds:0x672e98 / fstp [esp+0x10]   u = noise[i] * 28.0
// (0.02734374813735485 is 28/1024, so t runs 0 .. 27.97 and both phases span 28 periods.)
// Every sample therefore has TWO phases available, and the jump table picks which one each
// case reads:
//   0x4cd040 = {0x4cce2c, 0x4cce39, 0x4cce46, 0x4cce59, 0x4cce98, 0x4cceb3,
//               0x4cce6c, 0x4cce82, 0x4cced8, 0x4ccf04, 0x4ccf04, 0x4ccf74}
//   case 0  0x4cce2c  fld ds:0x672ac4                      1.0
//   case 1  0x4cce39  fld ds:0x672ac0                      0.0
//   case 2  0x4cce46  t * 6.2831855, fcos                  cos(2*pi*t)
//   case 3  0x4cce59  u * 6.2831855, fcos                  cos(2*pi*u)      <- warped phase
//   case 4  0x4cce98  t, then the triangle fold at 0x4cceb9
//   case 5  0x4cceb3  u, then the triangle fold at 0x4cceb9  <- warped phase
//   case 6  0x4cce6c  t, fmod only
//   case 7  0x4cce82  u, fmod only                          <- warped phase
//   case 8  0x4cced8  seed = seed*0x19660d + 0x3c6ef35f, (seed>>16) * 1.5259022e-05
//   case 9  0x4ccf04  identical body to case 10; both read t
//   case 10 0x4ccf04
//   case 11 0x4ccf74  u, fmod, then fld st(0) / fmulp       frac(u) squared  <- warped phase
// 0x628cca is the MSVC 7.1 CRT _CIfmod (x in ST(1), y in ST(0)); every call site loads
// fld QWORD ds:0x672af8, which is the double 1.0, so it is exactly frac(x).
// The triangle fold at 0x4cceb9 is: f = frac(x); f < 0.5 ? 2*f : 1 - 2*(f - 0.5).
//
// Phase 2, quantization (0x4ccfba) -- the part Ghidra drops entirely:
//   mov edx,1 / mov ecx,edi / shl edx,cl / test dl,0xc0 / je
//     (1 << type) & 0xc0 nonzero, i.e. type 6 or 7 -> range = 0.0
//     otherwise                                    -> range = maximum - minimum
//   per entry: if (range != 0.0) value = (value - minimum) / range;
//              byte = clamp(__ftol(value * 255.0), 0, 255)
// So the running minimum/maximum are NOT dead: every table except the two slide sawtooths is
// rescaled to fill 0..255, and the range == 0.0 test is what keeps the constant tables
// (type 0 all-1.0, type 1 all-0.0) from dividing by zero. An earlier reading of this function
// called the min/max tracking dead code and quantized with a flat value*255; that is wrong for
// every wave whose natural range is not already [0,1] -- cos, for instance, would have had its
// entire negative half clamped to 0. The 0xc0 mask is the same one periodic_function_evaluate
// @0x4cc9b0 tests to decide which functions wrap, which is what ties it to slide.
//
// A type outside 0..11 falls through (cmp edi,0xb / ja 0x4ccf89) onto the still-live x87 value
// from the previous iteration; each case body starts with fstp st(0) to drop it. That
// carry-over is transcribed below as a default that leaves value unchanged.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// cos/sin are single x87 instructions in the original code (Ghidra's fcos()/fsin() pseudo-
// calls); declared locally instead of via <math.h> because -I types shadows that header name.
extern double cos(double x);
extern double sin(double x);
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod: x in ST(1), y in ST(0)
extern int __ftol(double value);        // 0x6391b4

extern random_seed random_seed_global; // 0x00719cd0
extern void periodic_function_build_noise_table(real *table); // 0x4ccbb0, table in EDX

// Builds one of the twelve primary periodic-function byte lookup tables.
void periodic_function_build_table(periodic_function_t type, uint8_t *out)
{
    real noise[1024];
    real wave[1024];
    real minimum, maximum;
    real range;
    real t; // linear phase, 0 .. 27.97
    real u; // noise-warped phase, noise[i] * 28
    real value;
    real frac;
    uint32_t seed;
    int32_t i;
    int32_t scaled;

    minimum = 3.4028235e+38f;
    maximum = -3.4028235e+38f;
    periodic_function_build_noise_table(noise);

    value = -3.4028235e+38f; // fld [esp+0x1c] at 0x4ccdd7 primes the carried x87 slot
    seed = random_seed_global;

    for (i = 0; i < 1024; i++) {
        t = (real)i * 0.027343748f;
        u = noise[i] * 28.0f;

        switch ((int32_t)type) {
        case _periodic_function_one:
            value = 1.0f;
            break;
        case _periodic_function_zero:
            value = 0.0f;
            break;
        case _periodic_function_cosine:
            value = (real)cos((double)(t * 6.2831855f));
            break;
        case _periodic_function_cosine_variable_period:
            value = (real)cos((double)(u * 6.2831855f));
            break;
        case _periodic_function_diagonal_wave:
        case _periodic_function_diagonal_wave_variable_period:
            frac = (real)fmod((double)((type == _periodic_function_diagonal_wave) ? t : u), 1.0);
            if (0.5f <= frac) {
                value = 1.0f - ((frac - 0.5f) + (frac - 0.5f));
            } else {
                value = frac + frac;
            }
            break;
        case _periodic_function_slide:
            value = (real)fmod((double)t, 1.0);
            break;
        case _periodic_function_slide_variable_period:
            value = (real)fmod((double)u, 1.0);
            break;
        case _periodic_function_noise:
            seed = seed * k_random_multiplier + k_random_increment;
            value = (real)(seed >> k_random_value_shift) * 1.5259022e-05f;
            random_seed_global = seed;
            break;
        case _periodic_function_jitter:
        case _periodic_function_wander: {
            real a = (real)cos((double)(t * 0.8975979f));
            real b = (real)cos((double)(t * 25.132742f));
            real c = (real)cos((double)(t * 43.9823f));
            real d = (real)sin((double)(t * 1.5707964f));
            real e = (real)sin((double)(t * 3.1415927f));
            real f = (real)cos((double)(t * 6.2831855f));
            value = f * e + (d * c + b * a) * 0.5f;
            break;
        }
        case _periodic_function_spark:
            value = (real)fmod((double)u, 1.0);
            value = value * value;
            break;
        default:
            break; // out of range: the previous sample is re-emitted, see header note
        }

        if (maximum < value) {
            maximum = value;
        }
        if (value < minimum) {
            minimum = value;
        }
        wave[i] = value;
    }

    // The two slide sawtooths are already in [0,1) and are deliberately not rescaled.
    if (((int32_t)1 << (int32_t)type) & k_periodic_function_wrapping_mask) {
        range = 0.0f;
    } else {
        range = maximum - minimum;
    }

    for (i = 0; i < 1024; i++) {
        value = wave[i];
        if (range != 0.0f) {
            value = (value - minimum) / range;
        }
        scaled = __ftol((double)(value * 255.0f));
        if (scaled < 0) {
            scaled = 0;
        } else if (0xff < scaled) {
            scaled = 0xff;
        }
        out[i] = (uint8_t)scaled;
    }
}

#if 0
Original Ghidra decompilation (0x4ccdb0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void __cdecl periodic_function_build_table(short type,uchar *out)

{
  float fVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;
  float10 fVar8;
  float10 fVar9;
  float10 fVar10;
  int local_2010;
  float local_2008;
  float local_2004;
  float local_2000 [1024];
  float afStack_1000 [1023];
  undefined4 uStack_4;

  uStack_4 = 0x4ccdba;
  local_2008 = 3.4028235e+38;
  local_2004 = -3.4028235e+38;
  FUN_004ccbb0();
  fVar5 = (float10)-3.4028235e+38;
  local_2010 = 0;
  iVar3 = 0x400;
  uVar4 = DAT_00719cd0;
  do {
    fVar1 = (float)local_2010 * 0.027343748;
    switch(type) {
    case 0:
      fVar5 = (float10)1.0;
      break;
    case 1:
      fVar5 = (float10)0.0;
      break;
    case 2:
      fVar5 = (float10)fcos((float10)fVar1 * (float10)6.2831855);
      break;
    case 3:
      fVar5 = (float10)fcos((float10)(local_2000[local_2010] * 28.0) * (float10)6.2831855);
      break;
    case 4:
      goto LAB_004cceb9;
    case 5:
LAB_004cceb9:
      fVar5 = (float10)FUN_00628cca();
      if ((float10)0.5 <= fVar5) {
        fVar5 = (float10)1.0 - ((fVar5 - (float10)0.5) + (fVar5 - (float10)0.5));
      }
      else {
        fVar5 = fVar5 + fVar5;
      }
      break;
    case 6:
      fVar5 = (float10)FUN_00628cca();
      break;
    case 7:
      fVar5 = (float10)FUN_00628cca();
      break;
    case 8:
      uVar4 = uVar4 * 0x19660d + 0x3c6ef35f;
      fVar5 = (float10)(uVar4 >> 0x10) * (float10)1.5259022e-05;
      DAT_00719cd0 = uVar4;
      break;
    case 9:
    case 10:
      fVar5 = (float10)fcos((float10)fVar1 * (float10)0.8975979);
      fVar6 = (float10)fcos((float10)fVar1 * (float10)25.132742);
      fVar7 = (float10)fcos((float10)fVar1 * (float10)43.9823);
      fVar8 = (float10)fsin((float10)fVar1 * (float10)1.5707964);
      fVar9 = (float10)fsin((float10)fVar1 * (float10)3.1415927);
      fVar10 = (float10)fcos((float10)fVar1 * (float10)6.2831855);
      fVar5 = fVar10 * fVar9 + (fVar8 * fVar7 + fVar6 * fVar5) * (float10)0.5;
      break;
    case 0xb:
      fVar5 = (float10)FUN_00628cca();
      fVar5 = fVar5 * fVar5;
    }
    if ((float10)local_2004 < fVar5) {
      local_2004 = (float)fVar5;
    }
    if (fVar5 < (float10)local_2008) {
      local_2008 = (float)fVar5;
    }
    afStack_1000[local_2010] = (float)fVar5;
    local_2010 = local_2010 + 1;
    iVar3 = iVar3 + -1;
    if (iVar3 == 0) {
      iVar3 = 0x400;
      do {
        iVar2 = FUN_006391b4();
        if (iVar2 < 0) {
          iVar2 = 0;
        }
        else if (0xff < iVar2) {
          iVar2 = 0xff;
        }
        *out = (uchar)iVar2;
        out = out + 1;
        iVar3 = iVar3 + -1;
      } while (iVar3 != 0);
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

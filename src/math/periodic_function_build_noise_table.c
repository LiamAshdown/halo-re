// periodic_function_build_noise_table  (Ghidra: FUN_004ccbb0; renamed, Blam-style, not previously named)
// address 0x4ccbb0, size 250 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/math_functions.md ("Builds a normalized cumulative noise/warp table used
//   to distort one of the periodic wave types"). Writes a running cumulative sum of a
//   random-weighted triple-cosine term into 1024 float entries, then divides every entry by the
//   final cumulative total so the table ends in roughly [0,1).
// register convention: output float table (1024 entries) in EDX (in_EDX), no stack arguments.
//   // blam-cc: EDX -> table
// UNSURE: this scratch table is 1024 raw floats, not a periodic_function_table (which holds
//   bytes); it is local to periodic_function_build_table @0x4ccdb0's case 3
//   (_periodic_function_cosine_variable_period) and is declared as a plain float array by the
//   caller, per that function's own header.

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// cos is a single x87 FCOS instruction in the original code (Ghidra's fcos() pseudo-call);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double cos(double x);

extern random_seed random_seed_global; // 0x00719cd0

// Builds a normalized cumulative noise/warp table used to distort one of the periodic wave
// types.
void periodic_function_build_noise_table(real *table)
{
    real cumulative;
    int32_t i;
    real c1, c2, c3;

    cumulative = 0.0f;
    for (i = 0; i < 1024; i++) {
        table[i] = cumulative;
        c1 = (real)cos((double)((real)i * 0.044792242f));
        c2 = (real)cos((double)((real)i * 0.03129321f));
        random_seed_global = (((random_seed_global * k_random_multiplier + k_random_increment) *
                                k_random_multiplier + k_random_increment) *
                               k_random_multiplier + k_random_increment) *
                              k_random_multiplier + k_random_increment;
        c3 = (real)cos((double)((real)i * 0.025157286f));
        cumulative = (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f *
                     (c3 + 1.0f + c2 + 1.0f + c1 + 1.0f + 0.25f) + cumulative + 0.25f;
    }
    for (i = 0; i < 1024; i++) {
        table[i] = (1.0f / cumulative) * table[i];
    }
}

#if 0
Original Ghidra decompilation (0x4ccbb0):

void FUN_004ccbb0(void)

{
  float *pfVar1;
  int iVar2;
  float *in_EDX;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;
  int local_4;

  fVar3 = (float10)0.0;
  local_4 = 0;
  iVar2 = 0x400;
  pfVar1 = in_EDX;
  do {
    *pfVar1 = (float)fVar3;
    fVar4 = (float10)fcos((float10)local_4 * (float10)0.044792242);
    pfVar1 = pfVar1 + 1;
    fVar5 = (float10)fcos((float10)local_4 * (float10)0.03129321);
    DAT_00719cd0 = (((DAT_00719cd0 * 0x19660d + 0x3c6ef35f) * 0x19660d + 0x3c6ef35f) * 0x19660d +
                   0x3c6ef35f) * 0x19660d + 0x3c6ef35f;
    fVar6 = (float10)fcos((float10)local_4 * (float10)0.025157286);
    local_4 = local_4 + 1;
    iVar2 = iVar2 + -1;
    fVar3 = (float10)(DAT_00719cd0 >> 0x10) * (float10)1.5259022e-05 *
            (fVar6 + (float10)1.0 + fVar5 + (float10)1.0 + fVar4 + (float10)1.0 + (float10)0.25) +
            fVar3 + (float10)0.25;
  } while (iVar2 != 0);
  iVar2 = 0x400;
  do {
    iVar2 = iVar2 + -1;
    *in_EDX = (float)(((float10)1.0 / fVar3) * (float10)*in_EDX);
    in_EDX = in_EDX + 1;
  } while (iVar2 != 0);
  return;
}
#endif

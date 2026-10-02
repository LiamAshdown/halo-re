// vector3d_lerp_by_mode_denominator  (Ghidra: FUN_004eb370; named per this rewrite)
// address 0x4eb370, size 289 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Linearly interpolates a 3D point between two
// endpoints using per-axis integer ratios, with the denominator field chosen by a mode flag.").
// register convention: EAX -> table (min/max plus two mode-selected denominators), ECX ->
// out_point, EDX -> ratios (3 ints).
//   // blam-cc: EAX -> table, ECX -> out_point, EDX -> ratios
// UNSURE: table's field names (min/max/denominator_a/denominator_b) and message_delta_vector3d_mode's
// exact meaning are inferred from usage, not independently re-derived; the struct is local to this
// file; folded into types/networking.h as vector3d_lerp_table by the 2026-09-20 review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t message_delta_vector3d_mode; // 0x0069b350, see message_delta_encode_vector3d.c



static real unsigned_int_to_float(int32_t value)
{
    real result = (real)value;
    if (value < 0) {
        result = result + 4.2949673e+09f;
    }
    return result;
}

// Interpolates out_point[i] = (table->maximum - table->minimum) * (ratios[i] / denom) +
// table->minimum for i in 0..2, where denom is table->denominator_mode0 or
// table->denominator_mode1 depending on message_delta_vector3d_mode.
void vector3d_lerp_by_mode_denominator(vector3d_lerp_table *table, real_vector3d *out_point,
    int32_t *ratios)
    // blam-cc: EAX -> table, ECX -> out_point, EDX -> ratios
{
    int32_t denom_field;
    real denom;
    real numerator;

    denom_field = message_delta_vector3d_mode == 0 ? table->denominator_mode0 : table->denominator_mode1;

    numerator = unsigned_int_to_float(ratios[0]);
    denom = unsigned_int_to_float(denom_field);
    out_point->i = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;

    numerator = unsigned_int_to_float(ratios[1]);
    denom = unsigned_int_to_float(denom_field);
    out_point->j = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;

    numerator = unsigned_int_to_float(ratios[2]);
    denom = unsigned_int_to_float(denom_field);
    out_point->k = (table->maximum - table->minimum) * (numerator / denom) + table->minimum;
}

#if 0
Original Ghidra decompilation (0x4eb370), from tools/pack.py 0x4eb370:

void FUN_004eb370(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;
  int *in_EDX;
  float fVar4;

  fVar2 = *in_EAX;
  fVar3 = (float)*in_EDX;
  if (DAT_0069b350 == 0) {
    if (*in_EDX < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    fVar1 = (float)(int)in_EAX[5];
    if ((int)in_EAX[5] < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    *in_ECX = (in_EAX[1] - fVar2) * (fVar3 / fVar1) + fVar2;
    fVar2 = (float)in_EDX[1];
    if (in_EDX[1] < 0) {
      fVar2 = fVar2 + 4.2949673e+09;
    }
    fVar3 = (float)(int)in_EAX[5];
    if ((int)in_EAX[5] < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    in_ECX[1] = (in_EAX[1] - *in_EAX) * (fVar2 / fVar3) + *in_EAX;
    fVar2 = *in_EAX;
    fVar3 = (float)in_EDX[2];
    if (in_EDX[2] < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    fVar4 = in_EAX[5];
    fVar1 = in_EAX[5];
  }
  else {
    if (*in_EDX < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    fVar1 = (float)(int)in_EAX[3];
    if ((int)in_EAX[3] < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    *in_ECX = (in_EAX[1] - fVar2) * (fVar3 / fVar1) + fVar2;
    fVar2 = (float)in_EDX[1];
    if (in_EDX[1] < 0) {
      fVar2 = fVar2 + 4.2949673e+09;
    }
    fVar3 = (float)(int)in_EAX[3];
    if ((int)in_EAX[3] < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    in_ECX[1] = (in_EAX[1] - *in_EAX) * (fVar2 / fVar3) + *in_EAX;
    fVar2 = *in_EAX;
    fVar3 = (float)in_EDX[2];
    if (in_EDX[2] < 0) {
      fVar3 = fVar3 + 4.2949673e+09;
    }
    fVar4 = in_EAX[3];
    fVar1 = in_EAX[3];
  }
  fVar1 = (float)(int)fVar1;
  if ((int)fVar4 < 0) {
    fVar1 = fVar1 + 4.2949673e+09;
  }
  in_ECX[2] = (in_EAX[1] - fVar2) * (fVar3 / fVar1) + fVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

// weapon_trigger_barrel_spread_offset  (Ghidra: FUN_004c54e0; named from
// out/phase4/items_functions.md, "Computes the rotated spread offset vector for one barrel of a
// multi-barrel weapon trigger, used when spawning projectiles")
// address 0x4c54e0, size 158 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/tags.h WeaponTrigger.distribution_function/.distribution_angle (used by the
//   caller, trigger_create_projectiles).
// register convention: vector to rotate in EAX, axis in ECX (both Ghidra-recognized
// parameters, matching vector3d_rotate_about_axis's own convention); barrel index in AX
// (in_AX); distribution_function type, distribution_angle and a flags word are
// Ghidra-recognized stack parameters.
// blam-cc: EAX -> v, ECX -> axis, AX -> barrel_index, stack -> (distribution_function,
//   distribution_angle, flags)
// UNSURE: `flags` bit 0 selects between two symmetric-placement formulas for the barrel index;
// its wider meaning (does it encode barrel count too?) is not established.

#include "tags.h"
#include "math.h"

// cos/sin are single x87 FCOS/FSIN instructions in the original code (Ghidra's fcos()/fsin()
// pseudo-calls); declared locally instead of via <math.h> because -I types shadows that header
// name with types/math.h.
extern double cos(double x);
extern double sin(double x);
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820

// Rotates `v` around `axis` by barrel_index's share of distribution_angle, for
// distribution_function == 1 (rotate); otherwise does nothing.
void weapon_trigger_barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index,
    int16_t distribution_function, real distribution_angle, uint32_t flags)
{
    real index;

    if ((flags & 1) == 0) {
        index = (real)(int16_t)(barrel_index >> 1) - 0.5f;
        if ((barrel_index & 1) != 0) {
            index = -index;
        }
    } else if (barrel_index == 0) {
        index = 0.0f;
    } else {
        int16_t half = (int16_t)(barrel_index - 1) >> 1;
        if (((barrel_index - 1) & 1) == 0) {
            index = (real)(-half);
        } else {
            index = (real)half;
        }
    }

    if (distribution_function == 1) {
        real angle = index * distribution_angle;
        real sin_angle = (real)sin((double)angle);
        real cos_angle = (real)cos((double)angle);
        vector3d_rotate_about_axis(v, axis, sin_angle, cos_angle);
    }
}

#if 0
Original Ghidra decompilation (0x4c54e0):

void FUN_004c54e0(undefined4 param_1,undefined4 param_2,short param_3,float param_4,uint param_5)

{
  short sVar1;
  ushort in_AX;
  float10 fVar2;
  float10 fVar3;

  if ((param_5 & 1) == 0) {
    fVar2 = (float10)(int)((short)in_AX >> 1) - (float10)0.5;
    if ((in_AX & 1) != 0) {
      fVar2 = -fVar2;
    }
  }
  else if (in_AX == 0) {
    fVar2 = (float10)0.0;
  }
  else {
    sVar1 = (short)(in_AX - 1) >> 1;
    if ((in_AX - 1 & 1) == 0) {
      fVar2 = (float10)(int)-sVar1;
    }
    else {
      fVar2 = (float10)(int)sVar1;
    }
  }
  if (param_3 == 1) {
    fVar3 = (float10)fcos(fVar2 * (float10)param_4);
    fVar2 = (float10)fsin(fVar2 * (float10)param_4);
    vector3d_rotate_about_axis((float)fVar2,(float)fVar3);
    return;
  }
  return;
}
#endif

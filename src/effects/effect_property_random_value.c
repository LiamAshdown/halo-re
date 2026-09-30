// effect_property_random_value  (Ghidra: particle_system_property_random_value; RENAMED per
// out/phase4/effects_types_notes.md's misattribution table: "0x451290
// particle_system_property_random_value -> effect_property_random_value" -- it reads
// Effect.a_scale/b_scale (effect +0x44/+0x48), not a pctl field)
// address 0x451290, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x451290..0x451309)
// evidence: types/effects.h effect (a_scale 0x44, b_scale 0x48); the arithmetic is the same
// {base_min, base_max} random-range shape as effect_random_scaled_range 0x44c840, but with two
// independent multiplier bit-sets (A and B) instead of one flags word.
// register convention: bit index in DL (in_DL), effect* in EBX (unaff_EBX), the A bit-set in ESI
// (unaff_ESI), the B bit-set in EDI (unaff_EDI); the seed pointer and the two bounds are Ghidra's
// own recognised stack parameters.
//   // blam-cc: EDX(low8) -> bit_index, EBX -> self, ESI -> a_bitset, EDI -> b_bitset,
//   //   stack -> (seed, base_min, base_max)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_effects.h"

// Returns a random value in [base_min, base_max), where base_min and (base_max - base_min) are
// each independently multiplied by self->a_scale when their bit_index bit is set in a_bitset,
// and by self->b_scale when it is set in b_bitset (both bit_index and bit_index + 1 are tested).
real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset,
    uint32_t b_bitset, random_seed *seed, real base_min, real base_max)
{
    real lower = base_min;
    real span;
    uint32_t mask = 1u << (bit_index & 0x1f);

    if ((a_bitset & mask) != 0) {
        lower = base_min * self->a_scale;
    }
    if ((b_bitset & mask) != 0) {
        lower = lower * self->b_scale;
    }

    span = base_max - base_min;
    mask = 1u << ((bit_index + 1) & 0x1f);
    if ((a_bitset & mask) != 0) {
        span = span * self->a_scale;
    }
    if ((b_bitset & mask) != 0) {
        span = span * self->b_scale;
    }

    *seed = *seed * k_random_multiplier + k_random_increment;
    return (real)(*seed >> k_random_value_shift) * 1.5259022e-05f * span + lower;
}

#if 0
Original Ghidra decompilation (0x451290):

/* WARNING: Removing unreachable block (ram,0x004512f6) */

float10 particle_system_property_random_value(uint *param_1,float param_2,float param_3)

{
  uint uVar1;
  byte in_DL;
  int unaff_EBX;
  uint unaff_ESI;
  uint unaff_EDI;
  float10 fVar2;
  float10 fVar3;

  fVar2 = (float10)param_2;
  uVar1 = 1 << (in_DL & 0x1f);
  if ((unaff_ESI & uVar1) != 0) {
    fVar2 = (float10)param_2 * (float10)*(float *)(unaff_EBX + 0x44);
  }
  if ((unaff_EDI & uVar1) != 0) {
    fVar2 = fVar2 * (float10)*(float *)(unaff_EBX + 0x48);
  }
  fVar3 = (float10)param_3 - (float10)param_2;
  uVar1 = 1 << (in_DL + 1 & 0x1f);
  if ((unaff_ESI & uVar1) != 0) {
    fVar3 = fVar3 * (float10)*(float *)(unaff_EBX + 0x44);
  }
  if ((unaff_EDI & uVar1) != 0) {
    fVar3 = fVar3 * (float10)*(float *)(unaff_EBX + 0x48);
  }
  uVar1 = *param_1 * 0x19660d + 0x3c6ef35f;
  *param_1 = uVar1;
  return (float10)(uVar1 >> 0x10) * (float10)1.5259022e-05 * fVar3 + fVar2;
}
#endif

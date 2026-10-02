// particle_system_roll_particle_state  (Ghidra: FUN_00454250, still unnamed there; named by
//   types/effects.h's own particle_state_values comment: "particle_system_roll_particle_state
//   0x454250 fills one from ParticleSystemTypeParticleState scale at 0x48, animation_rate at
//   0x50, rotation_rate at 0x58 and the colour pair at 0x60 and 0x70, all with one shared random
//   fraction for the colour")
// address 0x454250, size 343 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: types/tags.h ParticleSystemTypeParticleState (scale[2] +0x48, animation_rate[2]
//   +0x50, rotation_rate[2] +0x58, color_1 +0x60, color_2 +0x70, each ColorARGB in
//   alpha/red/green/blue order); types/effects.h particle_state_values (scale, animation_rate,
//   rotation_rate, ColorARGB color, in that order, size 0x1c == 7 floats, matching the 7 writes
//   here exactly). The LCG matches src/math/random_real_range_seeded.c, called four times
//   (scale, animation_rate, rotation_rate, and once for the whole color).
// register convention: state index in AX (in_AX); ParticleSystemTypeParticleState table pointer
//   in ECX (in_ECX, indexed by AX with stride 0x178); output pointer in EDX (in_EDX).
//   // blam-cc: in_AX -> index, in_ECX -> states, in_EDX -> out
// UNSURE: the random draw order is preserved exactly -- out->animation_rate, out->rotation_rate
//   and out->scale each get an independent draw (in that order), then the whole color gets one
//   more independent draw shared across all four channels; kept verbatim rather than reordered
//   to match declaration order.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern random_seed effect_random_seed; // 0x00719cd4

extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170

void particle_system_roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states,
    particle_state_values *out) // blam-cc: in_AX, in_ECX, in_EDX
{
    ParticleSystemTypeParticleState *state = &states[index];
    float color_fraction_bits;

    // First draw: consumed locally and reused below for the whole color.
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    color_fraction_bits = (float)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;

    out->animation_rate = random_real_range_seeded(&effect_random_seed,
        state->animation_rate[0], state->animation_rate[1]);
    out->rotation_rate = random_real_range_seeded(&effect_random_seed,
        state->rotation_rate[0], state->rotation_rate[1]);
    out->scale = random_real_range_seeded(&effect_random_seed,
        state->scale[0], state->scale[1]);

    out->color.alpha = (state->color_2.alpha - state->color_1.alpha) * color_fraction_bits +
                        state->color_1.alpha;
    out->color.red = (state->color_2.red - state->color_1.red) * color_fraction_bits +
                      state->color_1.red;
    out->color.green = (state->color_2.green - state->color_1.green) * color_fraction_bits +
                        state->color_1.green;
    out->color.blue = (state->color_2.blue - state->color_1.blue) * color_fraction_bits +
                       state->color_1.blue;
}

#if 0
Original Ghidra decompilation (0x454250):

void FUN_00454250(void)

{
  float fVar1;
  short in_AX;
  int iVar2;
  int in_ECX;
  uint uVar3;
  float *in_EDX;

  uVar3 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  iVar2 = in_AX * 0x178 + *(int *)(in_ECX + 0x78);
  DAT_00719cd4 = uVar3 * 0x19660d + 0x3c6ef35f;
  fVar1 = (float)(uVar3 >> 0x10) * 1.5259022e-05;
  in_EDX[1] = (*(float *)(iVar2 + 0x54) - *(float *)(iVar2 + 0x50)) *
              (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 + *(float *)(iVar2 + 0x50);
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  in_EDX[2] = (*(float *)(iVar2 + 0x5c) - *(float *)(iVar2 + 0x58)) *
              (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 + *(float *)(iVar2 + 0x58);
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  *in_EDX = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
            (*(float *)(iVar2 + 0x4c) - *(float *)(iVar2 + 0x48)) + *(float *)(iVar2 + 0x48);
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  in_EDX[3] = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
              (*(float *)(iVar2 + 0x70) - *(float *)(iVar2 + 0x60)) + *(float *)(iVar2 + 0x60);
  in_EDX[4] = (*(float *)(iVar2 + 0x74) - *(float *)(iVar2 + 100)) * fVar1 + *(float *)(iVar2 + 100)
  ;
  in_EDX[5] = (*(float *)(iVar2 + 0x78) - *(float *)(iVar2 + 0x68)) * fVar1 +
              *(float *)(iVar2 + 0x68);
  in_EDX[6] = (*(float *)(iVar2 + 0x7c) - *(float *)(iVar2 + 0x6c)) * fVar1 +
              *(float *)(iVar2 + 0x6c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

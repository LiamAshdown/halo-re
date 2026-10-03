// player_effect_apply_continuous_damage  (Ghidra: FUN_004567c0, still unnamed there; named
//   directly by out/phase4/effects_types_notes.md, whose section 9 ("player_effect is driven by
//   ContinuousDamageEffect, not DamageEffect") is this function's own field-by-field proof)
// address 0x4567c0, size 316 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: types/tags.h ContinuousDamageEffect (radius[2] +0x00, low_frequency_vibrate_frequency
//   +0x24, high_frequency_vibrate_frequency +0x28, camera_shaking_random_translation +0x44,
//   camera_shaking_random_rotation +0x48, camera_shaking_wobble_period +0x5c,
//   camera_shaking_wobble_weight +0x60); types/effects.h player_effect (low_frequency_vibrate
//   +0xcc, high_frequency_vibrate +0xd0, shake_translation +0xd4, shake_rotation +0xd8,
//   vibrate_ticks +0xdc); src/effects/decal_update_fade.c establishes game_time->game_time as
//   the current game tick; src/math/periodic_function_evaluate.c establishes that function's
//   (type, time) signature.
// register convention: ContinuousDamageEffect tag reference in EAX (in_EAX); local player index
//   in DX (in_DX); distance as the recognized stack parameter (param_1).
//   // blam-cc: in_EAX -> tag_reference, in_DX -> local_player_index, stack -> distance
// UNSURE: periodic_function_evaluate's type argument (AX) is dropped by Ghidra at this call
//   site; _periodic_function_cosine is used as the closest match to a smooth "wobble".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances;                          // 0x0087bc14
extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884
extern game_time_globals *game_time; // 0x006f1d6c

extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0

void player_effect_apply_continuous_damage(uint32_t tag_reference, int16_t local_player_index,
    float distance) // blam-cc: in_EAX, in_DX, stack
{
    ContinuousDamageEffect *effect =
        (ContinuousDamageEffect *)tag_instances[tag_reference & 0xffff].data;

    if (distance < effect->radius[1]) {
        player_effect *self = &player_effect_globals_pointer->players[local_player_index];
        float fraction = 1.0f - (distance - effect->radius[0]) /
                                 (effect->radius[0] - effect->radius[1]);
        float wobble, weighted, delta;

        fraction = (fraction < 0.0f) ? 0.0f : (1.0f < fraction ? 1.0f : fraction);

        wobble = (float)periodic_function_evaluate(_periodic_function_cosine,
            (double)((float)game_time->game_time / effect->camera_shaking_wobble_period));
        weighted = ((1.0f - effect->camera_shaking_wobble_weight) +
                    wobble * effect->camera_shaking_wobble_weight) * fraction;

        if (0 < self->vibrate_ticks) {
            self->vibrate_ticks = 0;
            self->low_frequency_vibrate = 0.0f;
            self->high_frequency_vibrate = 0.0f;
            self->shake_translation = 0.0f;
            self->shake_rotation = 0.0f;
        }

        delta = weighted * effect->camera_shaking_random_translation;
        if (delta < 0.0f) delta = 0.0f;
        self->shake_translation += delta;

        delta = weighted * effect->camera_shaking_random_rotation;
        if (delta < 0.0f) delta = 0.0f;
        self->shake_rotation += delta;

        self->low_frequency_vibrate += fraction * effect->low_frequency_vibrate_frequency;
        self->high_frequency_vibrate += fraction * effect->high_frequency_vibrate_frequency;
    }
}

#if 0
Original Ghidra decompilation (0x4567c0):

void FUN_004567c0(float param_1)

{
  float *pfVar1;
  float *pfVar2;
  uint in_EAX;
  short in_DX;
  int iVar3;
  float10 fVar4;
  float10 fVar5;

  pfVar2 = *(float **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_1 < pfVar2[1]) {
    iVar3 = in_DX * 0xec + DAT_006f1884;
    pfVar1 = (float *)(iVar3 + 0xcc);
    param_1 = 1.0 - (param_1 - *pfVar2) / (*pfVar2 - pfVar2[1]);
    if (0.0 <= param_1) {
      if (1.0 < param_1) {
        param_1 = 1.0;
      }
    }
    else {
      param_1 = 0.0;
    }
    fVar4 = (float10)periodic_function_evaluate
                               ((double)((float)*(int *)(DAT_006f1d6c + 0xc) / pfVar2[0x17]));
    fVar4 = (((float10)1.0 - (float10)pfVar2[0x18]) + fVar4 * (float10)pfVar2[0x18]) *
            (float10)param_1;
    if (0 < *(short *)(iVar3 + 0xdc)) {
      *(undefined2 *)(iVar3 + 0xdc) = 0;
      *pfVar1 = 0.0;
      *(undefined4 *)(iVar3 + 0xd0) = 0;
      *(undefined4 *)(iVar3 + 0xd4) = 0;
      *(undefined4 *)(iVar3 + 0xd8) = 0;
    }
    fVar5 = fVar4 * (float10)pfVar2[0x11];
    if (fVar5 <= (float10)0.0) {
      fVar5 = (float10)0.0;
    }
    *(float *)(iVar3 + 0xd4) = (float)(fVar5 + (float10)*(float *)(iVar3 + 0xd4));
    fVar4 = fVar4 * (float10)pfVar2[0x12];
    if (fVar4 <= (float10)0.0) {
      fVar4 = (float10)0.0;
    }
    *(float *)(iVar3 + 0xd8) = (float)(fVar4 + (float10)*(float *)(iVar3 + 0xd8));
    *pfVar1 = param_1 * pfVar2[9] + *pfVar1;
    *(float *)(iVar3 + 0xd0) = param_1 * pfVar2[10] + *(float *)(iVar3 + 0xd0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

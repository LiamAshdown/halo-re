// glow_particle_compute_fade
// address 0x4fd3a0, size 121 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd3a0 |
//   lightning_segment_compute_fade | glow_particle_compute_fade")
// rewrite confidence: 0.55
// evidence: types/objects.h glow (definition_tag 0x224), glow_particle (age 0x50, lifetime
//   0x52, fade 0x58); types/tags.h Glow.glow_flags bit 0x08 (fading_percentage tested here).
// register convention: Ghidra shows `int in_EAX` and `int in_ECX` with no assignment anywhere
//   in this function; by direct analogy with glow_particle_compute_color.c (0x4fd420, same
//   in_EAX/in_ECX shape, same +0x224/+0x50/+0x52 field reads) EAX is the glow instance and ECX
//   the particle.
// blam-cc: EAX -> entry, ECX -> particle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern tag_instance *tag_instances; // 0x0087bc14

void glow_particle_compute_fade(glow *entry /*EAX*/, glow_particle *particle /*ECX*/)
    // blam-cc: EAX -> entry, ECX -> particle
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;

    if ((tag[0x28] & 8) == 0) {
        particle->fade = 1.0f;
        return;
    }

    {
        float fade = 1.0f - (float)particle->age / (float)particle->lifetime;
        if (fade < 0.0f) {
            particle->fade = 0.0f;
            return;
        }
        if (fade > 1.0f) {
            fade = 1.0f;
        }
        particle->fade = fade;
    }
}

#if 0
Original Ghidra decompilation (0x4fd3a0):

void FUN_004fd3a0(void)

{
  float fVar1;
  int in_EAX;
  int in_ECX;

  if ((*(byte *)(*(int *)((*(uint *)(in_EAX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x28)
      & 8) == 0) {
    *(undefined4 *)(in_ECX + 0x58) = 0x3f800000;
    return;
  }
  fVar1 = 1.0 - (float)(int)*(short *)(in_ECX + 0x50) / (float)(int)*(short *)(in_ECX + 0x52);
  *(float *)(in_ECX + 0x58) = fVar1;
  if (fVar1 < 0.0) {
    *(undefined4 *)(in_ECX + 0x58) = 0;
    return;
  }
  if (1.0 < fVar1) {
    fVar1 = 1.0;
  }
  *(float *)(in_ECX + 0x58) = fVar1;
  return;
}
#endif

// glow_particle_compute_color
// address 0x4fd420, size 128 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd420 |
//   lightning_segment_compute_color | glow_particle_compute_color")
// rewrite confidence: 0.6
// evidence: types/objects.h glow (definition_tag 0x224), glow_particle (age 0x50, lifetime
//   0x52, base_color 0x38, render_color 0x44); types/tags.h Glow.glow_flags bit 0x20 (colour
//   fade tested here).
// register convention: same EAX=entry/ECX=particle shape as glow_particle_compute_fade.c
//   (0x4fd3a0), which this function is the direct color-fade companion of.
// blam-cc: EAX -> entry, ECX -> particle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

void glow_particle_compute_color(glow *entry /*EAX*/, glow_particle *particle /*ECX*/)
    // blam-cc: EAX -> entry, ECX -> particle
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;

    if ((tag[0x28] & 0x20) != 0) {
        float fade = 1.0f - (float)particle->age / (float)particle->lifetime;
        if (fade < 0.0f) {
            fade = 0.0f;
        }
        particle->render_color[0] = fade * particle->base_color[0];
        particle->render_color[1] = fade * particle->base_color[1];
        particle->render_color[2] = fade * particle->base_color[2];
        return;
    }
    particle->render_color[0] = particle->base_color[0];
    particle->render_color[1] = particle->base_color[1];
    particle->render_color[2] = particle->base_color[2];
}

#if 0
Original Ghidra decompilation (0x4fd420):

void FUN_004fd420(void)

{
  float fVar1;
  int in_EAX;
  int in_ECX;

  if ((*(byte *)(*(int *)((*(uint *)(in_EAX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x28)
      & 0x20) != 0) {
    fVar1 = 1.0 - (float)(int)*(short *)(in_ECX + 0x50) / (float)(int)*(short *)(in_ECX + 0x52);
    if (fVar1 < 0.0) {
      fVar1 = 0.0;
    }
    *(float *)(in_ECX + 0x44) = fVar1 * *(float *)(in_ECX + 0x38);
    *(float *)(in_ECX + 0x48) = fVar1 * *(float *)(in_ECX + 0x3c);
    *(float *)(in_ECX + 0x4c) = fVar1 * *(float *)(in_ECX + 0x40);
    return;
  }
  *(undefined4 *)(in_ECX + 0x44) = *(undefined4 *)(in_ECX + 0x38);
  *(undefined4 *)(in_ECX + 0x48) = *(undefined4 *)(in_ECX + 0x3c);
  *(undefined4 *)(in_ECX + 0x4c) = *(undefined4 *)(in_ECX + 0x40);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

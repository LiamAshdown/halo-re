// particle_current_radius  (Ghidra: FUN_004566f0, still unnamed there; named directly by
//   types/effects.h: "particle_current_radius 0x4566f0 (age over lifespan against
//   Particle.radius_animation at 0x74, times scale)")
// address 0x4566f0, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: types/effects.h particle.age (+0x14), lifespan (+0x18), scale (+0x5c),
//   definition_index (+0x04); types/tags.h Particle.radius_animation[2].
// register convention: particle handle in EAX (in_EAX).
//   // blam-cc: EAX -> particle_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_data;   // 0x0087abd0
extern tag_instance *tag_instances; // 0x0087bc14

// Interpolates a particle's current render radius between its Particle tag's radius_animation
// bounds by its lifetime fraction (age / lifespan), then scales by the particle's own random
// scale factor.
real particle_current_radius(datum_index particle_handle)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;

    return ((tag->radius_animation[1] - tag->radius_animation[0]) * (self->age / self->lifespan) +
            tag->radius_animation[0]) * self->scale;
}

#if 0
Original Ghidra decompilation (0x4566f0):

float10 FUN_004566f0(void)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar1 = *(int *)(DAT_0087abd0 + 0x34);
  iVar3 = (in_EAX & 0xffff) * 0x70;
  iVar2 = *(int *)((*(uint *)(iVar3 + iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  return (((float10)*(float *)(iVar2 + 0x78) - (float10)*(float *)(iVar2 + 0x74)) *
          ((float10)*(float *)(iVar3 + 0x14 + iVar1) / (float10)*(float *)(iVar3 + 0x18 + iVar1)) +
         (float10)*(float *)(iVar2 + 0x74)) * (float10)*(float *)(iVar3 + iVar1 + 0x5c);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

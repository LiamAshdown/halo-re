// particle_impact  (Ghidra: FUN_00456550, still unnamed there; named from its own summary in
//   out/phase4/effects_functions.md: "Triggers a particle's impact effect if one is defined,
//   otherwise deletes the particle")
// address 0x456550, size 78 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x456550..0x45659e)
// evidence: types/tags.h Particle.death_effect (TagDependency, tag_id lands at +0x64 in the raw
//   disassembly, matching the struct layout Particle.flags(0x00) + bitmap(0x04) + physics(0x14)
//   + material_effects(0x24) + pad(0x34) + lifespan(0x38) + fade_in/out(0x40/0x44) +
//   collision_effect(0x48) + death_effect(0x58), tag_id at +0xc of that dependency = 0x64).
// register convention: particle handle in EDI (unaff_EDI).
//   // blam-cc: unaff_EDI -> particle_handle
// UNSURE: the call to particle_impact_response_dispatch passes a literal 0 for its bundle
//   pointer in the raw decompile; Ghidra has clearly dropped a real register argument (the
//   dispatch function reads through it in the sound branch), and the fourcc it switches on is
//   likewise never visibly loaded here. Both are reconstructed from the death_effect tag
//   reference this function already resolved; see particle_impact_response_dispatch's own
//   header for the reconstruction.

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

extern void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index,
    real intensity); // 0x4565a0, EAX self, ECX fourcc, ESI definition_index, stack intensity
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module

// Fires the particle's death effect or sound (if its Particle tag has one) and then always
// deletes the particle.
void particle_impact(datum_index particle_handle)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;

    if (*(uint32_t *)&tag->death_effect.tag_id != 0xffffffffu) {
        // 0x456550..0x456589: ECX = the dependency's group (tag +0x58), ESI = its tag index (+0x64), EAX = self
        particle_impact_response_dispatch(self, *(tag_group *)&tag->death_effect.tag_fourcc,
            *(datum_index *)&tag->death_effect.tag_id, 0.0f);
    }

    datum_delete(particle_data, particle_handle);
}

#if 0
Original Ghidra decompilation (0x456550):

void FUN_00456550(void)

{
  uint unaff_EDI;

  if (*(int *)(*(int *)((*(uint *)((unaff_EDI & 0xffff) * 0x70 + *(int *)(DAT_0087abd0 + 0x34) + 4)
                        & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 100) != -1) {
    FUN_004565a0(0);
  }
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

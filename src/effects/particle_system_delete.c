// particle_system_delete  (Ghidra: particle_system_delete_453f60; renamed here per
//   out/phase4/effects_types_notes.md's misattribution table: "0x453f60 particle_system_delete_453f60
//   -- the real particle_system_delete", to free the plain name for callers)
// address 0x453f60, size 146 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: types/effects.h particle_system.type_states[4] (+0x58, first_particle at +0x3c of
//   each 0x40-byte entry) and particle_system_particle.next_particle (+0x04); types/tags.h
//   ParticleSystem.particle_types (TagReflexive); src/memory/datum_delete.c establishes the
//   (array, handle) argument order.
// register convention: none -- handle is the single Ghidra-recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;                // 0x0087bc14

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510

void particle_system_delete(datum_index handle)
{
    particle_system *system = &((particle_system *)particle_system_data->data)[handle & 0xffff];
    ParticleSystem *definition = (ParticleSystem *)tag_instances[system->definition_index & 0xffff].data;
    int32_t i;

    for (i = 0; i < (int32_t)definition->particle_types.count; i++) {
        datum_index particle_handle = system->type_states[i].first_particle;

        while (particle_handle != (datum_index)0xffffffff) {
            particle_system_particle *particle =
                &((particle_system_particle *)particle_system_particle_data->data)[particle_handle & 0xffff];
            datum_index next = particle->next_particle;

            datum_delete(particle_system_particle_data, particle_handle);
            particle_handle = next;
        }
    }

    datum_delete(particle_system_data, handle);
}

#if 0
Original Ghidra decompilation (0x453f60):

void particle_system_delete_453f60(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  int iVar7;

  iVar1 = *(int *)(DAT_0087abd4 + 0x34);
  iVar7 = (param_1 & 0xffff) * 0x158;
  iVar2 = *(int *)((*(uint *)(iVar7 + 8 + iVar1) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar6 = 0;
  if (0 < *(int *)(iVar2 + 0x5c)) {
    iVar5 = 0;
    iVar4 = DAT_0087abd8;
    do {
      uVar3 = *(uint *)(iVar5 * 0x40 + 0x94 + iVar7 + iVar1);
      while (uVar3 != 0xffffffff) {
        uVar3 = *(uint *)((uVar3 & 0xffff) * 0x80 + 4 + *(int *)(iVar4 + 0x34));
        iVar4 = datum_delete();
      }
      sVar6 = sVar6 + 1;
      iVar5 = (int)sVar6;
    } while (iVar5 < *(int *)(iVar2 + 0x5c));
  }
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

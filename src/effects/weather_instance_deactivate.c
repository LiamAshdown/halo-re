// weather_instance_deactivate  (Ghidra: FUN_00457f00, still unnamed there; named directly by
//   types/effects.h: "weather_instance_deactivate 0x457f00")
// address 0x457f00, size 177 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: types/effects.h weather_instance (definition_index +0x00, types[8] +0x1c),
//   weather_instance_type (particle_count +0x08, first_particle +0x0c), weather_particle
//   (next_particle +0x50); types/tags.h WeatherParticleSystem.particle_types (+0x24).
// register convention: weather instance index in AX (in_AX).
//   // blam-cc: in_AX -> instance_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern weather_instance weather_instances[1]; // 0x006b0ae4
extern data_array *weather_particle_data;     // 0x0087abcc
extern tag_instance *tag_instances;           // 0x0087bc14
extern int32_t weather_instance_count;        // 0x006b0ae0

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module

// Deactivates a weather instance: deletes every live particle across all of its particle type
// slots, then marks the instance free.
void weather_instance_deactivate(int16_t instance_index)
{
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)tag_instances[(uint16_t)instance->definition_index].data;
    int32_t i;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        weather_instance_type *slot = &instance->types[i];

        while (slot->first_particle != (datum_index)0xffffffff) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)slot->first_particle];
            datum_index next = p->next_particle;

            datum_delete(weather_particle_data, slot->first_particle);
            slot->particle_count -= 1;
            slot->first_particle = next;
        }
    }

    weather_instance_count--;
    instance->definition_index = (datum_index)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x457f00):

void FUN_00457f00(void)

{
  int iVar1;
  uint uVar2;
  short in_AX;
  int iVar3;
  short sVar4;
  int iVar5;
  uint *puVar6;

  puVar6 = (uint *)(&DAT_006b0ae4 + in_AX * 0x9c);
  iVar1 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = 0;
  sVar4 = 0;
  iVar3 = DAT_0087abcc;
  if (0 < *(int *)(iVar1 + 0x24)) {
    do {
      uVar2 = puVar6[iVar5 * 4 + 10];
      while (uVar2 != 0xffffffff) {
        uVar2 = *(uint *)((puVar6[iVar5 * 4 + 10] & 0xffff) * 0x54 + 0x50 + *(int *)(iVar3 + 0x34));
        iVar3 = datum_delete();
        *(short *)(puVar6 + iVar5 * 4 + 9) = (short)puVar6[iVar5 * 4 + 9] + -1;
        puVar6[iVar5 * 4 + 10] = uVar2;
      }
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(iVar1 + 0x24));
    DAT_006b0ae0 = DAT_006b0ae0 + -1;
    *puVar6 = 0xffffffff;
    return;
  }
  DAT_006b0ae0 = DAT_006b0ae0 + -1;
  *puVar6 = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

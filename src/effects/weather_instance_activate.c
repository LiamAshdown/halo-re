// weather_instance_activate  (Ghidra: FUN_00457e20, still unnamed there; named directly by
//   types/effects.h: "weather_instance_activate 0x457e20 seeds it [weather_instance_type]")
// address 0x457e20, size 210 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: types/effects.h weather_instance (definition_index +0x00, elapsed_time +0x04,
//   delta_time +0x08, intensity +0x0c, types[8] +0x1c), weather_instance_type (target_count
//   +0x00 random in WeatherParticleSystemParticleType.particle_count +0xa4, field_extent +0x04
//   from fade_out_end_distance +0x30, particle_count +0x08, first_particle +0x0c); types/tags.h
//   WeatherParticleSystem.particle_types (+0x24, count/pointer).
// register convention: definition tag index in EAX (in_EAX); weather instance index in DX
//   (in_DX, always 0 in retail per k_maximum_weather_instances); intensity as the recognized
//   stack parameter (param_1).
//   // blam-cc: in_EAX -> definition_index, in_DX -> instance_index, stack -> intensity

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "fn_effects.h"

extern weather_instance weather_instances[1]; // 0x006b0ae4
extern int32_t weather_instance_count;      // 0x006b0ae0
extern tag_instance *tag_instances;         // 0x0087bc14
extern random_seed effect_random_seed;      // 0x00719cd4

// Activates a weather instance for the given WeatherParticleSystem tag: resets its timers to the
// given intensity, and seeds each of its particle types with a random target particle count and
// its fade distance, with no particles yet.
void weather_instance_activate(datum_index definition_index, int16_t instance_index, real intensity)
{
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag = (WeatherParticleSystem *)tag_instances[(uint16_t)definition_index].data;
    int32_t i;

    instance->definition_index = definition_index;
    instance->intensity = intensity;
    instance->elapsed_time = 0.0f;
    instance->delta_time = 0.0f;
    weather_instance_count++;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + i;
        weather_instance_type *slot = &instance->types[i];

        slot->particle_count = 0;
        slot->first_particle = (datum_index)0xffffffff;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        slot->target_count = (type->particle_count[1] - type->particle_count[0]) *
            (real)(int16_t)(effect_random_seed >> 16) * (1.0f / 65536.0f) + type->particle_count[0];
        slot->field_extent = type->fade_out_end_distance;
    }
}

#if 0
Original Ghidra decompilation (0x457e20):

void FUN_00457e20(undefined4 param_1)

{
  float *pfVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  short in_DX;
  int iVar5;
  uint uVar6;
  short sVar7;

  iVar5 = in_DX * 0x9c;
  iVar2 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(uint *)(&DAT_006b0ae4 + iVar5) = in_EAX;
  *(undefined4 *)(&DAT_006b0af0 + iVar5) = param_1;
  *(undefined4 *)(&DAT_006b0ae8 + iVar5) = 0;
  *(undefined4 *)(&DAT_006b0aec + iVar5) = 0;
  DAT_006b0ae0 = DAT_006b0ae0 + 1;
  sVar7 = 0;
  if (0 < *(int *)(iVar2 + 0x24)) {
    iVar3 = 0;
    uVar6 = DAT_00719cd4;
    do {
      iVar4 = iVar3 * 0x25c + *(int *)(iVar2 + 0x28);
      pfVar1 = (float *)((int)(&DAT_006b0ae4 + iVar5) + (iVar3 * 4 + 7) * 4);
      *(undefined2 *)(pfVar1 + 2) = 0;
      uVar6 = uVar6 * 0x19660d + 0x3c6ef35f;
      pfVar1[3] = -NAN;
      sVar7 = sVar7 + 1;
      DAT_00719cd4 = uVar6;
      *pfVar1 = (*(float *)(iVar4 + 0xa8) - *(float *)(iVar4 + 0xa4)) *
                (float)(uVar6 >> 0x10) * 1.5259022e-05 + *(float *)(iVar4 + 0xa4);
      pfVar1[1] = *(float *)(iVar4 + 0x30);
      iVar3 = (int)sVar7;
    } while (iVar3 < *(int *)(iVar2 + 0x24));
  }
  return;
}
#endif

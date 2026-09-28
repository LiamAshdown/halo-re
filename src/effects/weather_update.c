// weather_update  (Ghidra: already named weather_update)
// address 0x53f5c0, size 663 bytes
// name confidence: 0.75   rewrite confidence: 0.55
// evidence: types/effects.h weather_particle_system_state (every field, "weather_update 0x53f5c0
//   owns every field"); types/tags.h ScenarioStructureBSPWeatherPalette (wind TagDependency
//   +0x80, wind_direction +0x90, wind_magnitude +0x9c), Wind (velocity[2] +0x00,
//   variation_area +0x08).
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern ScenarioStructureBSP *global_structure_bsp;
                                    // scenario's weather palette (ScenarioStructureBSPWeatherPalette)
extern int32_t weather_frame_counter;       // 0x00746f88
extern tag_instance *tag_instances;         // 0x0087bc14
extern random_seed effect_random_seed;      // 0x00719cd4
extern int16_t weather_particle_system_count; // 0x00746b84
extern weather_particle_system_state weather_wind_states[8]; // 0x00746b88

extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction
extern double sqrt(double x);
extern double cos(double x);
extern double sin(double x);

// Per-tick driver for the global weather system's wind: for each active row of the scenario's
// weather palette with a Wind tag, random-walks that row's magnitude/pitch/yaw perturbations and
// rebuilds its wind direction vector from the Wind tag's base direction and variation bounds.
void weather_update(void)
{
    int32_t palette_count = *(int32_t *)&global_structure_bsp->weather_palette.count;
    ScenarioStructureBSPWeatherPalette *palette =
        (ScenarioStructureBSPWeatherPalette *)global_structure_bsp->weather_palette.pointer;
    int32_t i;

    weather_frame_counter++;

    if (palette_count < 1) {
        weather_particle_system_count = (int16_t)palette_count;
        return;
    }

    for (i = 0; i < palette_count; i++) {
        ScenarioStructureBSPWeatherPalette *row = &palette[i];
        weather_particle_system_state *wind = &weather_wind_states[i];
        uint8_t *active = (uint8_t *)&weather_wind_states[0] + i * 0x20; // matches
                                    // weather_particle_system_state.active exactly

        if (*(uint32_t *)&row->wind.tag_id == 0xffffffffu) {
            *active = 0;
        } else {
            Wind *wind_tag = (Wind *)tag_instances[row->wind.tag_id.index].data;
            real yaw_base, pitch_base, yaw, pitch, cos_pitch;

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            wind->magnitude_walk += (int16_t)(effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->magnitude_walk = (wind->magnitude_walk < 0.0f) ? 0.0f :
                (wind->magnitude_walk > 1.0f ? 1.0f : wind->magnitude_walk);

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            wind->yaw_walk += (int16_t)(effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->yaw_walk = (wind->yaw_walk < -1.0f) ? -1.0f : (wind->yaw_walk > 1.0f ? 1.0f : wind->yaw_walk);

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            wind->pitch_walk += (int16_t)(effect_random_seed >> 16) >= 0 ? -0.01f : 0.01f;
            wind->pitch_walk = (wind->pitch_walk < -1.0f) ? -1.0f : (wind->pitch_walk > 1.0f ? 1.0f : wind->pitch_walk);

            wind->magnitude = (wind_tag->velocity[1] - wind_tag->velocity[0]) * wind->magnitude_walk +
                wind_tag->velocity[0];

            yaw_base = (real)atan2((double)row->wind_direction.j, (double)row->wind_direction.i);
            pitch_base = (real)atan2((double)row->wind_direction.k,
                (double)sqrt((double)(row->wind_direction.i * row->wind_direction.i +
                                       row->wind_direction.j * row->wind_direction.j)));

            pitch = wind->yaw_walk * wind_tag->variation_area.pitch * 0.5f + pitch_base; // UNSURE:
                                    // yaw_walk feeds the pitch perturbation and pitch_walk feeds
                                    // the yaw perturbation, preserved exactly as decompiled
            yaw = wind->pitch_walk * wind_tag->variation_area.yaw * 0.5f + yaw_base;

            cos_pitch = (real)cos((double)pitch);
            wind->direction_i = (real)cos((double)yaw) * cos_pitch;
            wind->direction_j = (real)sin((double)yaw) * cos_pitch;
            wind->direction_k = (real)sin((double)pitch);

            {
                real scale = row->wind_magnitude * wind->magnitude;
                wind->direction_i = scale * wind->direction_i;
                wind->direction_j = scale * wind->direction_j;
                wind->direction_k = scale * wind->direction_k;
            }

            *active = 1;
        }
    }

    weather_particle_system_count = (int16_t)palette_count;
}

#if 0
Original Ghidra decompilation (0x53f5c0):

void weather_update(void)

{
  float fVar1;
  float *pfVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  float10 fVar10;
  float10 fVar11;
  float10 fVar12;

  iVar5 = DAT_0087bc14;
  iVar4 = DAT_00746f9c;
  DAT_00746f88 = DAT_00746f88 + 1;
  iVar7 = 0;
  sVar6 = 0;
  uVar9 = DAT_00719cd4;
  if (*(int *)(DAT_00746f9c + 0x1b4) < 1) {
    DAT_00746b84 = *(undefined2 *)(DAT_00746f9c + 0x1b4);
    return;
  }
  do {
    iVar8 = iVar7 * 0xf0 + *(int *)(iVar4 + 0x1b8);
    if (*(uint *)(iVar8 + 0x8c) == 0xffffffff) {
      (&DAT_00746b88)[iVar7 * 0x20] = 0;
    }
    else {
      pfVar2 = *(float **)((*(uint *)(iVar8 + 0x8c) & 0xffff) * 0x20 + 0x14 + iVar5);
      uVar9 = uVar9 * 0x19660d + 0x3c6ef35f;
      if ((short)(((uVar9 >> 0x10) << 1) >> 0x10) == 0) {
        fVar1 = -0.01;
      }
      else {
        fVar1 = 0.01;
      }
      fVar1 = fVar1 + (float)(&DAT_00746b8c)[iVar7 * 8];
      (&DAT_00746b8c)[iVar7 * 8] = fVar1;
      if (0.0 <= fVar1) {
        if (1.0 < fVar1) {
          fVar1 = 1.0;
        }
      }
      else {
        fVar1 = 0.0;
      }
      (&DAT_00746b8c)[iVar7 * 8] = fVar1;
      uVar9 = uVar9 * 0x19660d + 0x3c6ef35f;
      if ((short)(((uVar9 >> 0x10) << 1) >> 0x10) == 0) {
        fVar3 = -0.01;
      }
      else {
        fVar3 = 0.01;
      }
      fVar3 = fVar3 + (float)(&DAT_00746b94)[iVar7 * 8];
      (&DAT_00746b94)[iVar7 * 8] = fVar3;
      if (-1.0 <= fVar3) {
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = -1.0;
      }
      (&DAT_00746b94)[iVar7 * 8] = fVar3;
      uVar9 = uVar9 * 0x19660d + 0x3c6ef35f;
      if ((short)(((uVar9 >> 0x10) << 1) >> 0x10) == 0) {
        fVar10 = (float10)-0.01;
      }
      else {
        fVar10 = (float10)0.01;
      }
      fVar10 = fVar10 + (float10)(float)(&DAT_00746b90)[iVar7 * 8];
      DAT_00719cd4 = uVar9;
      (&DAT_00746b90)[iVar7 * 8] = (float)fVar10;
      if ((float10)-1.0 <= fVar10) {
        if ((float10)1.0 < fVar10) {
          fVar10 = (float10)1.0;
        }
      }
      else {
        fVar10 = (float10)-1.0;
      }
      (&DAT_00746b90)[iVar7 * 8] = (float)fVar10;
      (&DAT_00746b98)[iVar7 * 8] = (pfVar2[1] - *pfVar2) * fVar1 + *pfVar2;
      fVar11 = (float10)fpatan((float10)*(float *)(iVar8 + 0x94),(float10)*(float *)(iVar8 + 0x90));
      fVar12 = (float10)fpatan((float10)*(float *)(iVar8 + 0x98),
                               SQRT((float10)*(float *)(iVar8 + 0x90) *
                                    (float10)*(float *)(iVar8 + 0x90) +
                                    (float10)*(float *)(iVar8 + 0x94) *
                                    (float10)*(float *)(iVar8 + 0x94)));
      fVar1 = (float)((float10)fVar3 * (float10)pfVar2[3] * (float10)0.5 + fVar12);
      fVar11 = fVar10 * (float10)pfVar2[2] * (float10)0.5 + fVar11;
      fVar10 = (float10)fcos((float10)fVar1);
      fVar12 = (float10)fcos(fVar11);
      (&DAT_00746b9c)[iVar7 * 8] = (float)(fVar12 * fVar10);
      fVar12 = (float10)fsin(fVar11);
      (&DAT_00746ba0)[iVar7 * 8] = (float)(fVar12 * fVar10);
      fVar10 = (float10)fsin((float10)fVar1);
      (&DAT_00746ba4)[iVar7 * 8] = (float)fVar10;
      fVar1 = *(float *)(iVar8 + 0x9c) * (float)(&DAT_00746b98)[iVar7 * 8];
      (&DAT_00746b9c)[iVar7 * 8] = fVar1 * (float)(&DAT_00746b9c)[iVar7 * 8];
      (&DAT_00746ba0)[iVar7 * 8] = fVar1 * (float)(&DAT_00746ba0)[iVar7 * 8];
      (&DAT_00746ba4)[iVar7 * 8] = fVar1 * (float)(&DAT_00746ba4)[iVar7 * 8];
      (&DAT_00746b88)[iVar7 * 0x20] = 1;
    }
    sVar6 = sVar6 + 1;
    iVar7 = (int)sVar6;
    iVar8 = *(int *)(iVar4 + 0x1b4);
  } while (iVar7 < iVar8);
  DAT_00746b84 = (short)iVar8;
  return;
}
#endif

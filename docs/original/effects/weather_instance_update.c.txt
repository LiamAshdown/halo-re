// weather_instance_update  (Ghidra: FUN_00458420, still unnamed there; named directly by
//   types/effects.h: "weather_instance_activate 0x457e20 and weather_instance_deactivate
//   0x457f00 own definition_index and the type array, weather_instance_update 0x458420 owns
//   elapsed_time and delta_time")
// address 0x458420, size 520 bytes
// name confidence: 0.6   rewrite confidence: 0.35 (see UNSURE)
// evidence: types/effects.h weather_instance (delta_time +0x08 "copied from 0x007c3110",
//   elapsed_time +0x04, intensity +0x0c), weather_particle (frame +0x2c, rotation +0x30,
//   rotation_rate +0x48, next_particle +0x50); types/tags.h WeatherParticleSystemParticleType
//   fade_in_start_height/_end_height (+0x34/+0x38) and fade_out_start_height/_end_height
//   (+0x3c/+0x40); src/objects/light_volume_render.c names 0x007c311c camera_position_z.
// register convention: weather instance index as the recognized stack parameter (param_1).
//   // blam-cc: stack -> instance_index
// UNSURE: FUN_00628cca's real signature disagrees across the codebase (void vs (value,
//   modulus)); called here as (frame, 1.0f) by analogy with src/units/vehicle_calculate_
//   steering_wheel_controls.c, wrapping the frame counter -- a guess, not evidence. See also
//   weather_instance_adjust_count.c's own header for the target-count multiplication this
//   function's call site does not visibly perform.
// reconciled: R45 weather_instance.delta_time is the render frame delta; 0x007c3110 extern renamed render_time_since_frame (render.h name)

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
extern float render_time_since_frame;          // 0x007c3110, render.h: seconds since the previous rendered frame
extern float camera_position_z;               // 0x007c311c

extern void weather_instance_adjust_count(int16_t instance_index, int16_t type_index,
    real target_value); // 0x457fc0, this module
extern void weather_particle_update(datum_index weather_particle_handle, int16_t type_index,
    int16_t instance_index); // 0x458630, this module
extern double fmod(double x, double y); // CRT fmod (0x628cca: _CIfmod, x87 fprem; name entry "fmod" at 0x006844f0)

// Per-tick update for one weather instance: advances its elapsed/delta time, and for each
// particle type slot, fades its target count in/out by camera height against the type's fade
// bounds, adjusts the live particle count toward that target, and advances every live particle's
// frame/rotation and physics.
void weather_instance_update(int16_t instance_index)
{
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)tag_instances[(uint16_t)instance->definition_index].data;
    int32_t i;

    instance->delta_time = render_time_since_frame;
    instance->elapsed_time = instance->delta_time + instance->elapsed_time;

    for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + i;
        weather_instance_type *slot = &instance->types[i];
        real fade_in, fade_out;
        datum_index particle_index;

        fade_in = (camera_position_z - type->fade_in_start_height) /
                  (type->fade_in_end_height - type->fade_in_start_height);
        fade_in = (fade_in < 0.0f) ? 0.0f : (fade_in > 1.0f ? 1.0f : fade_in);

        fade_out = (camera_position_z - type->fade_out_start_height) /
                   (type->fade_out_end_height - type->fade_out_start_height);
        fade_out = (fade_out < 0.0f) ? 0.0f : (fade_out > 1.0f ? 1.0f : fade_out);

        weather_instance_adjust_count(instance_index, (int16_t)i,
            (1.0f - fade_out) * fade_in * instance->intensity * slot->target_count); // UNSURE,
                                    // see file header

        particle_index = slot->first_particle;
        while (particle_index != (datum_index)0xffffffff) {
            weather_particle *p =
                &((weather_particle *)weather_particle_data->data)[(uint16_t)particle_index];

            p->frame = p->animation_rate * instance->delta_time + p->frame;
            p->frame = (float)fmod(p->frame, 1.0f); // UNSURE, see file header
            p->rotation = (real)((((particle_index & 1) != 0) ? -1 : 1)) * p->rotation_rate *
                instance->delta_time + p->rotation;

            weather_particle_update(particle_index, (int16_t)i, instance_index);

            particle_index = p->next_particle;
        }
    }
}

#if 0
Original Ghidra decompilation (0x458420):

void FUN_00458420(undefined4 param_1)

{
  int iVar1;
  uint uVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  float10 fVar10;

  iVar7 = (short)param_1 * 0x9c;
  iVar1 = *(int *)((*(uint *)(&DAT_006b0ae4 + iVar7) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(undefined4 *)(&DAT_006b0aec + iVar7) = DAT_007c3110;
  sVar8 = 0;
  *(float *)(&DAT_006b0ae8 + iVar7) =
       *(float *)(&DAT_006b0aec + iVar7) + *(float *)(&DAT_006b0ae8 + iVar7);
  if (0 < *(int *)(iVar1 + 0x24)) {
    iVar5 = 0;
    do {
      iVar6 = iVar5 * 0x25c + *(int *)(iVar1 + 0x28);
      if (0.0 <= (DAT_007c311c - *(float *)(iVar6 + 0x34)) /
                 (*(float *)(iVar6 + 0x38) - *(float *)(iVar6 + 0x34))) {
        if ((DAT_007c311c - *(float *)(iVar6 + 0x34)) /
            (*(float *)(iVar6 + 0x38) - *(float *)(iVar6 + 0x34)) <= 1.0) {
          fVar4 = (DAT_007c311c - *(float *)(iVar6 + 0x34)) /
                  (*(float *)(iVar6 + 0x38) - *(float *)(iVar6 + 0x34));
        }
        else {
          fVar4 = 1.0;
        }
      }
      else {
        fVar4 = 0.0;
      }
      if (0.0 <= (DAT_007c311c - *(float *)(iVar6 + 0x3c)) /
                 (*(float *)(iVar6 + 0x40) - *(float *)(iVar6 + 0x3c))) {
        if ((DAT_007c311c - *(float *)(iVar6 + 0x3c)) /
            (*(float *)(iVar6 + 0x40) - *(float *)(iVar6 + 0x3c)) <= 1.0) {
          fVar3 = (DAT_007c311c - *(float *)(iVar6 + 0x3c)) /
                  (*(float *)(iVar6 + 0x40) - *(float *)(iVar6 + 0x3c));
        }
        else {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = 0.0;
      }
      FUN_00457fc0(param_1,(1.0 - fVar3) * fVar4 * *(float *)(&DAT_006b0af0 + iVar7));
      uVar2 = *(uint *)((int)(&DAT_006b0ae4 + iVar7) + (iVar5 * 4 + 10) * 4);
      while (uVar2 != 0xffffffff) {
        iVar5 = *(int *)(DAT_0087abcc + 0x34);
        iVar6 = (uVar2 & 0xffff) * 0x54;
        iVar9 = iVar6 + iVar5;
        *(float *)(iVar9 + 0x2c) =
             *(float *)(iVar6 + 0x4c + iVar5) * *(float *)(&DAT_006b0aec + iVar7) +
             *(float *)(iVar6 + 0x2c + iVar5);
        fVar10 = (float10)FUN_00628cca();
        *(float *)(iVar9 + 0x2c) = (float)fVar10;
        *(float *)(iVar9 + 0x30) =
             (float)(int)((-(uint)((uVar2 & 1) != 0) & 0xfffffffe) + 1) * *(float *)(iVar9 + 0x48) *
             *(float *)(&DAT_006b0aec + iVar7) + *(float *)(iVar9 + 0x30);
        FUN_00458630(uVar2);
        uVar2 = *(uint *)(iVar9 + 0x50);
      }
      sVar8 = sVar8 + 1;
      iVar5 = (int)sVar8;
    } while (iVar5 < *(int *)(iVar1 + 0x24));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

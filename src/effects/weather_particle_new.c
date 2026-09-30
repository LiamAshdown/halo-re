// weather_particle_new  (Ghidra: FUN_00458070, still unnamed there; named directly by
//   types/effects.h: "weather_particle_new 0x458070 writes every field")
// address 0x458070, size 941 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: types/effects.h weather_particle (every field, established by this exact function)
//   and weather_instance_type.field_extent; types/tags.h WeatherParticleSystemParticleType
//   (acceleration_magnitude +0xcc, particle_radius +0xfc, animation_rate +0x104, rotation_rate
//   +0x10c, flags bit2 random_rotation, sprite_bitmap +0x194); src/effects/effect_random_
//   direction_from_table.c establishes FUN_004505e0's out-pointer convention.
// register convention: weather instance index in param_1, particle type index in param_2, both
//   Ghidra's own recognized stack parameters.
//   // blam-cc: stack -> (instance_index, type_index)
// VERIFIED against disassembly 0x458070..0x45840d (2026-09-30). FIXED: every random draw was converted with
//   (int16_t)(seed >> 16) * (1/65536) but the original does `fild` of the unsigned 16-bit value times
//   1.5259022e-05 (1/65535, the constant at 0x672b84), so draws >= 0x8000 came out negative; and the sprite
//   frame was truncated to an integer, the original keeps r * (1/65535) * sprite_count as a float.
//   effect_random_direction_from_table's output pointer (EAX) is the particle's own `acceleration` field
//   (0x45819f: eax = esi + 0x1c). No datum-index parity term exists in this function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *weather_particle_data;     // 0x0087abcc
extern weather_instance weather_instances[1]; // 0x006b0ae4
extern tag_instance *tag_instances;           // 0x0087bc14
extern random_seed effect_random_seed;        // 0x00719cd4

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module
extern void effect_random_direction_from_table(real_point3d *out); // 0x4505e0, this module
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t); // 0x43f6a0, EAX color1, ECX color0

// Creates one new weather particle (raindrop/snowflake) for the given weather instance and
// particle type slot: a random position inside the field box, zero velocity, a random-direction
// random-magnitude acceleration, a random rotation/sequence/frame/colour/alpha/radius/animation
// rate, and links it onto the slot's particle list.
datum_index weather_particle_new(int16_t instance_index, int16_t type_index)
{
    datum_index handle = datum_new(weather_particle_data);

    if (handle != (datum_index)0xffffffff) {
        weather_instance *instance = &weather_instances[instance_index];
        weather_instance_type *slot = &instance->types[type_index];
        WeatherParticleSystem *system_tag =
            (WeatherParticleSystem *)tag_instances[(uint16_t)instance->definition_index].data;
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)system_tag->particle_types.pointer + type_index;
        Bitmap *bitmap = (Bitmap *)tag_instances[type->sprite_bitmap.tag_id.index].data;
        weather_particle *p = &((weather_particle *)weather_particle_data->data)[(uint16_t)handle];

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.x = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.y = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->position.z = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * slot->field_extent;

        p->velocity.i = 0.0f;
        p->velocity.j = 0.0f;
        p->velocity.k = 0.0f;

        effect_random_direction_from_table((real_point3d *)&p->acceleration);
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        {
            real magnitude = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (type->acceleration_magnitude[1] - type->acceleration_magnitude[0]) +
                type->acceleration_magnitude[0];
            p->acceleration.i = magnitude * p->acceleration.i;
            p->acceleration.j = magnitude * p->acceleration.j;
            p->acceleration.k = magnitude * p->acceleration.k;
        }

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->radius = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->particle_radius[1] - type->particle_radius[0]) + type->particle_radius[0];
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->animation_rate = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->animation_rate[1] - type->animation_rate[0]) + type->animation_rate[0];
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->rotation_rate = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (type->rotation_rate[1] - type->rotation_rate[0]) + type->rotation_rate[0];

        if ((type->flags & 4) == 0) { // random_rotation
            p->rotation = 0.0f;
        } else {
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            p->rotation = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;
        }

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->sequence_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)bitmap->bitmap_group_sequence.count) >> 16);
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        {
            BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
            // 0x458345: fild count, fild draw, fmul 1/65535, fmul count -> a fractional frame (not truncated)
            p->frame = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (real)(int32_t)sequences[p->sequence_index].sprites.count;
        }

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        // 0x4583b4: EAX = type +0x148, ECX = type +0x138, stack (particle +0x38, type +0x20, t)
        color_interpolate((ColorRGB *)((uint8_t *)type + 0x148), (ColorRGB *)((uint8_t *)type + 0x138),
            (ColorRGB *)&p->color, *(uint32_t *)&((struct WeatherParticleSystemParticleType *)type)->flags,
            (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f);

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        p->alpha = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (*(real *)&((struct WeatherParticleSystemParticleType *)type)->color_upper_bound - *(real *)&((struct WeatherParticleSystemParticleType *)type)->color_lower_bound) +
            *(real *)&((struct WeatherParticleSystemParticleType *)type)->color_lower_bound;

        p->next_particle = slot->first_particle;
        slot->particle_count += 1;
        slot->first_particle = handle;
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x458070):

uint FUN_00458070(short param_1,short param_2)

{
  float fVar1;
  int iVar2;
  short sVar3;
  uint uVar4;
  uint *puVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined8 uVar10;

  uVar10 = datum_new();
  uVar4 = (uint)uVar10;
  if (uVar4 != 0xffffffff) {
    puVar5 = (uint *)(&DAT_006b0ae4 + param_1 * 0x9c);
    iVar8 = (int)param_2;
    iVar9 = iVar8 * 0x25c +
            *(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x28);
    iVar2 = *(int *)((*(uint *)(iVar9 + 0x1a0) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    iVar7 = (uVar4 & 0xffff) * 0x54 + *(int *)((int)((ulonglong)uVar10 >> 0x20) + 0x34);
    *(float *)(iVar7 + 4) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * (float)puVar5[iVar8 * 4 + 8];
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 8) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * (float)puVar5[iVar8 * 4 + 8];
    fVar1 = (float)puVar5[iVar8 * 4 + 8];
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar6 = DAT_00719cd4 >> 0x10;
    *(undefined4 *)(iVar7 + 0x18) = 0;
    *(undefined4 *)(iVar7 + 0x14) = 0;
    *(undefined4 *)(iVar7 + 0x10) = 0;
    *(float *)(iVar7 + 0xc) = (float)uVar6 * 1.5259022e-05 * fVar1;
    FUN_004505e0();
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    fVar1 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
            (*(float *)(iVar9 + 0xd0) - *(float *)(iVar9 + 0xcc)) + *(float *)(iVar9 + 0xcc);
    *(float *)(iVar7 + 0x1c) = fVar1 * *(float *)(iVar7 + 0x1c);
    *(float *)(iVar7 + 0x20) = fVar1 * *(float *)(iVar7 + 0x20);
    *(float *)(iVar7 + 0x24) = fVar1 * *(float *)(iVar7 + 0x24);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 0x44) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
         (*(float *)(iVar9 + 0x100) - *(float *)(iVar9 + 0xfc)) + *(float *)(iVar9 + 0xfc);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 0x4c) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
         (*(float *)(iVar9 + 0x108) - *(float *)(iVar9 + 0x104)) + *(float *)(iVar9 + 0x104);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 0x48) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
         (*(float *)(iVar9 + 0x110) - *(float *)(iVar9 + 0x10c)) + *(float *)(iVar9 + 0x10c);
    if ((*(byte *)(iVar9 + 0x20) & 4) == 0) {
      *(undefined4 *)(iVar7 + 0x30) = 0;
    }
    else {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar7 + 0x30) = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
    }
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    sVar3 = (short)((DAT_00719cd4 >> 0x10) * (int)*(short *)(iVar2 + 0x54) >> 0x10);
    *(short *)(iVar7 + 0x28) = sVar3;
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 0x2c) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
         (float)*(int *)(sVar3 * 0x40 + 0x34 + *(int *)(iVar2 + 0x58));
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    color_interpolate(iVar7 + 0x38,*(undefined4 *)(iVar9 + 0x20),
                      (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar7 + 0x34) =
         (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 *
         (*(float *)(iVar9 + 0x144) - *(float *)(iVar9 + 0x134)) + *(float *)(iVar9 + 0x134);
    *(uint *)(iVar7 + 0x50) = puVar5[iVar8 * 4 + 10];
    *(short *)(puVar5 + iVar8 * 4 + 9) = (short)puVar5[iVar8 * 4 + 9] + 1;
    puVar5[iVar8 * 4 + 10] = uVar4;
    return uVar4;
  }
  return 0xffffffff;
}
#endif

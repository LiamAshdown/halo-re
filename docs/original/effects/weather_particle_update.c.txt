// weather_particle_update  (Ghidra: FUN_00458630, still unnamed there; named directly by
//   types/effects.h: "weather_particle_update 0x458630 owns velocity, acceleration and the
//   position jitter")
// address 0x458630, size 686 bytes
// name confidence: 0.6   rewrite confidence: 0.3 (see UNSURE)
// evidence: types/effects.h weather_particle (acceleration +0x1c, velocity +0x10, position
//   +0x04, radius +0x44); types/tags.h WeatherParticleSystemParticleType.acceleration_magnitude
//   (+0xcc), acceleration_turning_rate (+0xd4), acceleration_change_rate (+0xd8), physics
//   TagDependency; src/physics/point_physics_tick.c establishes point_physics_tick's full
//   11 argument signature; src/math/vector3d_randomize_direction.c establishes
//   sphere_point_table / sphere_point_table_count.
// register convention: weather particle handle (low 16 bits) as the recognized ushort parameter
//   (param_1); weather particle type index in AX (in_AX); weather instance index in CX (in_CX).
//   // blam-cc: stack -> particle_handle, in_AX -> type_index, in_CX -> instance_index
// UNSURE: the tag struct's own field names (`acceleration_turning_rate` at +0xd4,
//   `acceleration_change_rate` at +0xd8) are used here for whatever the disassembly actually
//   reads at each offset, which is the reverse of how types/effects.h's own prose describes
//   their roles ("blended ... by acceleration_change_rate at +0xd4 and turned ... by
//   acceleration_turning_rate at +0xd8") -- the field NAMES are kept exactly as tags.h declares
//   them, offset for offset, regardless of that prose mismatch. `weather_instance.unknown_10`
//   is written here as a bsp_leaf_reference out-parameter (leaf_index at +0x10, cluster-ish
//   short pair at +0x14), which was not previously established. The two flag bits packed into
//   point_physics_tick's flags_arg (bit 0 always 1, bit 1 from weather_instance.in_sky) are
//   preserved exactly as computed without being certain of their meaning beyond what
//   point_physics_tick.c's own header already documents for that argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "physics.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *weather_particle_data;     // 0x0087abcc
extern weather_instance weather_instances[1]; // 0x006b0ae4
extern tag_instance *tag_instances;           // 0x0087bc14
extern real_point3d *sphere_point_table;      // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;      // 0x006b7af8, 1026
extern random_seed effect_random_seed;        // 0x00719cd4

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg,
    PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4,
    real_point3d *position, real_vector3d *wind, real_vector3d *out_normal,
    int16_t *out_material_type, real radius, real dt); // 0x50b530, physics module
extern void vector3d_positive_modulo(real reference); // 0x4588e0, math module (skipped in this pass, see
                                    // summary); UNSURE signature

// Per-tick update for one weather particle: randomly re-targets its acceleration vector's
// direction and magnitude toward the type's bounds, integrates velocity from it, runs a point
// physics tick to move and collide the particle, and adds a tiny deterministic positional jitter
// keyed on the particle's own handle.
void weather_particle_update(datum_index weather_particle_handle, int16_t type_index,
    int16_t instance_index) // blam-cc: stack, in_AX, in_CX
{
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *system_tag =
        (WeatherParticleSystem *)tag_instances[(uint16_t)instance->definition_index].data;
    WeatherParticleSystemParticleType *type =
        (WeatherParticleSystemParticleType *)system_tag->particle_types.pointer + type_index;
    weather_particle *p =
        &((weather_particle *)weather_particle_data->data)[(uint16_t)weather_particle_handle];

    if (type->acceleration_magnitude[0] != 0.0f || type->acceleration_magnitude[1] != 0.0f) {
        real length = vector3d_normalize_with_length(&p->acceleration);
        real old_weight = 1.0f - type->acceleration_turning_rate;
        real target_length;
        int16_t index;
        real_point3d *direction;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        target_length = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
            (2.0f * type->acceleration_change_rate) - type->acceleration_change_rate + length;
        if (target_length < type->acceleration_magnitude[0]) {
            target_length = type->acceleration_magnitude[0];
        } else if (target_length > type->acceleration_magnitude[1]) {
            target_length = type->acceleration_magnitude[1];
        }

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)sphere_point_table_count) >> 16);
        direction = &sphere_point_table[index];

        p->acceleration.i = old_weight * p->acceleration.i + direction->x * type->acceleration_turning_rate;
        p->acceleration.j = old_weight * p->acceleration.j + direction->y * type->acceleration_turning_rate;
        p->acceleration.k = old_weight * p->acceleration.k + direction->z * type->acceleration_turning_rate;

        p->acceleration.i = target_length * p->acceleration.i;
        p->acceleration.j = target_length * p->acceleration.j;
        p->acceleration.k = target_length * p->acceleration.k;

        p->velocity.i = instance->delta_time * p->acceleration.i + p->velocity.i;
        p->velocity.j = instance->delta_time * p->acceleration.j + p->velocity.j;
        p->velocity.k = instance->delta_time * p->acceleration.k + p->velocity.k;
    }

    {
        uint32_t flags_arg = (instance->in_sky != 0) ? 7u : 5u;
        PointPhysics *physics = (PointPhysics *)tag_instances[type->physics.tag_id.index].data;
        int16_t material_type;

        point_physics_tick(&p->velocity, flags_arg, physics,
            (bsp_leaf_reference *)((uint8_t *)instance + 0x10), (uint32_t)instance->cluster_index,
            &p->position, (real_vector3d *)0, (real_vector3d *)0, &material_type, p->radius,
            instance->delta_time);
    }

    {
        uint32_t seed = (uint16_t)weather_particle_handle * k_random_multiplier + k_random_increment;
        int16_t index = (int16_t)(((seed >> k_random_value_shift) *
            (uint32_t)(int32_t)sphere_point_table_count) >> 16);
        real_point3d *direction = &sphere_point_table[index];

        p->position.x = direction->x * 0.001f + p->position.x;
        p->position.y = direction->y * 0.001f + p->position.y;
        p->position.z = direction->z * 0.001f + p->position.z;
    }

    vector3d_positive_modulo(instance->types[type_index].field_extent);
}

#if 0
Original Ghidra decompilation (0x458630):

void FUN_00458630(ushort param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short in_AX;
  int iVar5;
  int iVar6;
  int iVar7;
  short in_CX;
  uint uVar8;
  int iVar9;
  int iVar10;
  float10 fVar11;

  iVar9 = in_CX * 0x9c;
  iVar5 = in_AX * 0x25c +
          *(int *)(*(int *)((*(uint *)(&DAT_006b0ae4 + iVar9) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                           ) + 0x28);
  iVar10 = (uint)param_1 * 0x54 + *(int *)(DAT_0087abcc + 0x34);
  if ((*(float *)(iVar5 + 0xcc) != 0.0) || (*(float *)(iVar5 + 0xd0) != 0.0)) {
    pfVar1 = (float *)(iVar10 + 0x1c);
    fVar11 = (float10)vector3d_normalize_with_length();
    fVar2 = 1.0 - *(float *)(iVar5 + 0xd4);
    uVar8 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    fVar11 = (float10)(uVar8 >> 0x10) * (float10)1.5259022e-05 *
             ((float10)*(float *)(iVar5 + 0xd8) - -(float10)*(float *)(iVar5 + 0xd8)) +
             -(float10)*(float *)(iVar5 + 0xd8) + fVar11;
    if ((float10)*(float *)(iVar5 + 0xcc) <= fVar11) {
      if ((float10)*(float *)(iVar5 + 0xd0) < fVar11) {
        fVar11 = (float10)*(float *)(iVar5 + 0xd0);
      }
    }
    else {
      fVar11 = (float10)*(float *)(iVar5 + 0xcc);
    }
    DAT_00719cd4 = uVar8 * 0x19660d + 0x3c6ef35f;
    iVar6 = (int)(short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10);
    iVar7 = DAT_006b7af4 + iVar6 * 0xc;
    fVar3 = *(float *)(iVar7 + 4);
    fVar4 = *(float *)(iVar7 + 8);
    *pfVar1 = fVar2 * *pfVar1 + *(float *)(DAT_006b7af4 + iVar6 * 0xc) * *(float *)(iVar5 + 0xd4);
    *(float *)(iVar10 + 0x20) = fVar2 * *(float *)(iVar10 + 0x20) + fVar3 * *(float *)(iVar5 + 0xd4)
    ;
    *(float *)(iVar10 + 0x24) = fVar2 * *(float *)(iVar10 + 0x24) + fVar4 * *(float *)(iVar5 + 0xd4)
    ;
    *pfVar1 = (float)(fVar11 * (float10)*pfVar1);
    *(float *)(iVar10 + 0x20) = (float)(fVar11 * (float10)*(float *)(iVar10 + 0x20));
    *(float *)(iVar10 + 0x24) = (float)(fVar11 * (float10)*(float *)(iVar10 + 0x24));
    fVar2 = *(float *)(&DAT_006b0aec + iVar9);
    *(float *)(iVar10 + 0x10) = fVar2 * *pfVar1 + *(float *)(iVar10 + 0x10);
    *(float *)(iVar10 + 0x14) = fVar2 * *(float *)(iVar10 + 0x20) + *(float *)(iVar10 + 0x14);
    *(float *)(iVar10 + 0x18) = fVar2 * *(float *)(iVar10 + 0x24) + *(float *)(iVar10 + 0x18);
  }
  pfVar1 = (float *)(iVar10 + 4);
  FUN_0050b530(((&DAT_006b0afe)[iVar9] != '\0') * '\x02' + '\x05',
               *(undefined4 *)((*(uint *)(iVar5 + 0xb8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
               &DAT_006b0af4 + iVar9,*(undefined2 *)(&DAT_006b0afc + iVar9),pfVar1,0,0,0,
               *(undefined4 *)(iVar10 + 0x44),*(undefined4 *)(&DAT_006b0aec + iVar9));
  iVar7 = (int)(short)(((short)param_1 * 0x19660d + 0x3c6ef35fU >> 0x10) * (int)DAT_006b7af8 >> 0x10
                      );
  iVar5 = DAT_006b7af4 + iVar7 * 0xc;
  fVar2 = *(float *)(iVar5 + 4);
  fVar3 = *(float *)(iVar5 + 8);
  *pfVar1 = *(float *)(DAT_006b7af4 + iVar7 * 0xc) * 0.001 + *pfVar1;
  *(float *)(iVar10 + 8) = fVar2 * 0.001 + *(float *)(iVar10 + 8);
  *(float *)(iVar10 + 0xc) = fVar3 * 0.001 + *(float *)(iVar10 + 0xc);
  FUN_004588e0(*(uint *)((int)(&DAT_006b0ae4 + iVar9) + (in_AX * 4 + 8) * 4));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif

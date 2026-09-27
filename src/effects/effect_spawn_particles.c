// effect_spawn_particles  (Ghidra: particle_system_spawn_particles; RENAMED per
// out/phase4/effects_types_notes.md's misattribution table: "0x451f90
// particle_system_spawn_particles -> effect_spawn_particles (one EffectEvent, not a pctl
// system)")
// address 0x451f90, size 2600 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN from objdump 0x451f90..0x4529b8; was 0.2: the densest function in the
//   batch; several callee argument lists are only partly visible at their call sites and are
//   reconstructed from the locals that consume their results rather than read directly. See the
//   grouped UNSURE notes below.)
// evidence: types/effects.h effect (event_index 0x4e, event_time 0x50, event_duration 0x54,
//   previous_event_fraction 0x58, color 0x18, velocity 0x24, location.leaf_index 0x10,
//   first_person_weapon_index 0x4c, object_index 0x3c, tint_source 0x30, a_scale 0x44,
//   b_scale 0x48, location_markers[32] 0x5c) and effect_location_marker (marker_index 0x02,
//   transform 0x08, real_matrix4x3 scale/forward/left/up/position at +0/+4/+0x10/+0x1c/+0x28
//   relative to transform); types/tags.h EffectEvent.particles (0x38 count / 0x3c pointer) and
//   EffectParticle (violence_mode 0x02, create 0x04, location 0x08, relative_offset 0x14,
//   relative_direction_vector 0x20, particle_type.tag_id 0x64, distribution_function 0x68,
//   count[2] 0x6c, distribution_radius[2] 0x70, tint_lower_bound/tint_upper_bound 0xb0/0xc0,
//   a_scales_values/b_scales_values 0xe0/0xe4 -- bit 7 distribution_radius, bit 8
//   distribution_radius_delta, bits 3-4 (0x18, ">>3&3") select the colour pair per the raw
//   `color_interpolate(&out, uVar17>>3&3, t)` call, bit 2 (0x4) applies the effect tint);
//   types/objects.h object_header/object.nodes; this module's effect_distribution_function_evaluate
//   0x453290 (this is the exact "difference of two evaluations" mechanism its own file header
//   describes), effect_marker_next 0x453180, particle_new 0x455740 (particle_creation_data);
//   src/math/matrix4x3_transform_point.c and matrix4x3_transform_normal.c for the two rotation
//   shapes duplicated inline here (no direct call to either exists in the callee list); this
//   module's own effect_random_direction_vector.c establishes the sphere_point_table /
//   sphere_point_table_count globals reused inline here.
// register convention: __cdecl, this function's own single argument (the effect handle) is
//   Ghidra's own recognized stack parameter, re-typed here as `effect *self` directly (the raw
//   decompile treats it as a byte offset base throughout, exactly like this module's other
//   effect_* functions).
//   // blam-cc: stack -> self
// UNSURE (grouped, in order of appearance):
//  1. `local_18 = (short)__ftol()` and `sVar15 = (short)__ftol()` each truncate the *float*
//     result of effect_distribution_function_evaluate directly into a 16-bit count with no
//     visible multiplication in between. types/effects.h's own note on `previous_event_fraction`
//     ("a spawn count for a tick is the difference of the distribution function evaluated at the
//     two fractions") only makes sense if each evaluation is scaled by
//     self->particle_counts[particle_index] (the per-type count effect_update rolls at event
//     start) before truncation; modeled that way here, but the multiply itself is not visible in
//     the decompile.
//  2. the "if (particle_debug_override == 1) { count = __ftol(); }" sequence mirrors the
//     identical shape already accepted in src/effects/particle_system_spawn.c
//     ("if (DAT_0069c566 == 1) { local_37c = __ftol(); }"); modeled the same way, as an opaque
//     debug override whose real source value cannot be recovered here.
//  3. RESOLVED by the phase-4 integration pass. The nine arguments here are
//     (seed, relative_direction_vector, &out_direction, &out_velocity, velocity[0] 0x84,
//     velocity[1] 0x88, velocity_cone_angle 0x8c, a_scales_values, b_scales_values), and
//     0x451310's own decompile agrees: its param_3/param_4 are written as 3-float vectors, so
//     they are outputs, and min/max are param_5/param_6. effect_random_velocity_vector.c has
//     been corrected to that signature and this file now calls it directly; the second
//     call-site-local prototype is gone.
//  4. the exact particle_creation_data field each of the three rotated vectors lands in
//     (position at +0x10 is solid -- it is the only one added to a `local_24/20/1c` sphere point
//     scaled by the distribution-radius roll, matching real_point3d position exactly) but
//     `unknown_1c` (+0x1c) and `velocity` (+0x28) for the other two are inferred only from
//     declaration order, not from any struct-typed write.
//  5. the tint_source callback's third argument and color_interpolate's exact colour-pair
//     encoding are taken from existing sibling files (effect_resolve_marker_transform-adjacent
//     evidence in types/effects.h's tint_source note, and object_lights_update_all.c's
//     color_interpolate signature) rather than independently re-derived here. The *selector* is
//     now read correctly: the raw code loads one dword at +0x64 and uses `>> 3 & 3` and `& 4`
//     from it, and +0x64 is EffectParticle.flags, not a_scales_values (+0xe0).
//  6. particle_new's creation block is assembled in Ghidra's locals local_9c..local_60 and the
//     call takes its pointer in a register, so the mapping of those locals onto
//     particle_creation_data fields is inference. position (+0x10) is solid; unknown_1c (+0x1c)
//     and velocity (+0x28) are ordering only.
//
// Fixed by the phase-4 integration pass, against tools/pack.py 0x451f90:
//   * the distribution-radius roll advances the RNG TWICE. The first advance drives the radius
//     fraction, the second selects the sphere_point_table sample. The earlier draft advanced it
//     once and used the same word for both.
//   * out_direction is rotated through the marker and node transforms WITHOUT the transform's
//     `scale`; only out_velocity is pre-scaled by it. The earlier draft scaled both.
//   * effect.velocity * 30 is added to out_velocity, not to out_direction, and out_direction is
//     passed through unchanged. The earlier draft added it to out_direction and then copied
//     that over out_velocity.
//   * the colour pair selector and the tint-apply bit come from EffectParticle.flags (+0x64).
//   * the ColorARGB alpha is lerped from tint_lower_bound.alpha / tint_upper_bound.alpha; the
//     earlier draft left alpha unwritten.
//   * scenario_location_get_water_and_weather takes &self->location; the earlier draft wrote `self + 0x10`, which on a
//     typed `effect *` is 0x10 * 0xfc bytes past the record.
//   * the "stay attached to marker" branch (flags bit 0) hands particle_new the vectors as they
//     were BEFORE the node transform, which is what the original's local reuse produces.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header

extern uint8_t particle_spawn_debug_mode;   // 0x0069c565, UNSURE name: nonzero enables this
                                            // function at all; ==1 additionally forces a debug
                                            // spawn-count override (see UNSURE 2)
extern tag_instance *tag_instances;         // 0x0087bc14
extern data_array *object_data;             // 0x008603b0
extern random_seed effect_random_seed;      // 0x00719cd4
extern uint8_t *first_person_weapon_globals; // 0x006b2d98, stride 0x1ea0
extern real_point3d *sphere_point_table;    // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;    // 0x006b7af8
extern const real_point3d *global_origin3d_pointer; // 0x00696714

extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern real effect_distribution_function_evaluate(EffectDistributionFunction_t type,
    real fraction); // 0x453290, this module
extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker,
    int32_t mode); // 0x453180, this module
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack (leaf, weather_index_out)
extern void particle_new(particle_creation_data *creation_data); // 0x455740, this module
extern int32_t __ftol(void); // 0x6391b4, MSVC runtime float-to-int truncation, UNSURE
extern real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset,
    uint32_t b_bitset, random_seed *seed, real base_min, real base_max); // 0x451290, this module
extern void effect_random_velocity_vector(effect *self, random_seed *seed,
    real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity,
    real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
    // 0x451310, this module; self in EAX, see UNSURE 3

// Rotates a point by a marker's real_matrix4x3 transform (scale, then forward/left/up, then
// translate) -- the same formula as src/math/matrix4x3_transform_point.c, duplicated inline
// because the disassembly never calls it directly at either of the two sites that need it.
static void effect_spawn_particles_transform_point(real_point3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    if (m->scale != 1.0f) {
        x *= m->scale;
        y *= m->scale;
        z *= m->scale;
    }
    out->x = x * m->forward.i + y * m->left.i + z * m->up.i + m->position.x;
    out->y = x * m->forward.j + y * m->left.j + z * m->up.j + m->position.y;
    out->z = x * m->forward.k + y * m->left.k + z * m->up.k + m->position.z;
}

// Rotates a direction by a marker's real_matrix4x3 (scale, then forward/left/up, no translate) --
// the same formula as src/math/matrix4x3_transform_normal.c, duplicated inline for the same
// reason as above.
static void effect_spawn_particles_transform_normal(real_vector3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    if (m->scale != 1.0f) {
        x *= m->scale;
        y *= m->scale;
        z *= m->scale;
    }
    out->i = x * m->forward.i + y * m->left.i + z * m->up.i;
    out->j = x * m->forward.j + y * m->left.j + z * m->up.j;
    out->k = x * m->forward.k + y * m->left.k + z * m->up.k;
}

// The same rotation WITHOUT the matrix scale. The original applies `scale` to the velocity
// vector before rotating it through both the marker and the node transform, but never to the
// direction vector -- at either site. Kept as its own helper so the asymmetry is visible.
static void effect_spawn_particles_rotate_unscaled(real_vector3d *out, real x, real y, real z,
    real_matrix4x3 *m)
{
    out->i = x * m->forward.i + y * m->left.i + z * m->up.i;
    out->j = x * m->forward.j + y * m->left.j + z * m->up.j;
    out->k = x * m->forward.k + y * m->left.k + z * m->up.k;
}

// For the current event's EffectParticle list, rolls how many of each type should spawn this
// tick (the difference of the distribution function evaluated at this tick's and last tick's
// event fraction, scaled by the type's already-rolled particle count), then for every qualifying
// marker of that type's EffectLocation spawns that many individual particles: each one's
// position/orientation is built by transforming a randomized offset and two randomized
// direction/velocity-like vectors through the marker's transform (and, if the marker names a
// real node or first-person-weapon marker, through that node's transform too), its colour is
// interpolated from the type's tint bounds, and the result is hCommitted through particle_new.
void effect_spawn_particles(effect *self)
{
    // REWRITTEN (objdump 0x451f90..0x4529b8). The creation record handed to particle_new (EDI) is filled as the
    //   binary lays it out at [esp+0x24]: +0x00 the Particle tag (EffectParticle +0x60), +0x04 object, +0x08 marker,
    //   +0x0a first person weapon (a word), +0x0c first person marker, +0x0d create == 2, +0x0e create == 1,
    //   +0x10 position, +0x1c direction, +0x28 velocity, +0x34 the tint source vector, +0x40 initial angle,
    //   +0x44 angular velocity roll (bit 3), +0x48 radius roll (bit 9), +0x4c alpha, +0x50 rgb. The draft dropped
    //   the direction, angle, create flags and first person flag, rolled both properties with bit 0 and the wrong
    //   ranges, and replaced the debug halving with a bare __ftol().
    Effect *tag;
    EffectEvent *event;
    real previous_fraction;
    real current_fraction;
    int16_t type_index;

    if (particle_spawn_debug_mode == 0) {
        return;
    }
    tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
    event = &((EffectEvent *)tag->events.pointer)[self->event_index];
    previous_fraction = self->previous_event_fraction;
    current_fraction = (self->event_duration > 0.0f) ? self->event_time / self->event_duration : 1.0f;

    for (type_index = 0; (int32_t)type_index < (int32_t)event->particles.count; type_index++) {
        uint8_t *pt = (uint8_t *)event->particles.pointer + (int32_t)type_index * 0xe8;
        int16_t location = *(int16_t *)(pt + 0x08);
        int16_t violence_mode = *(int16_t *)(pt + 0x02);
        uint16_t create = *(uint16_t *)(pt + 0x04);
        real count_scale;
        int16_t current_count;
        int16_t spawn_count;
        datum_index marker_handle;
        effect_location_marker *entry;

        if (location < 0 || (int32_t)location >= (int32_t)tag->locations.count) {
            continue;
        }
        if ((((uint8_t *)self)[2] >> 6 & 1) != 0 ? violence_mode == 1 : violence_mode == 2) {
            continue;
        }
        count_scale = (real)(int32_t)self->particle_counts[type_index];
        current_count = (int16_t)(int32_t)(effect_distribution_function_evaluate(
            (EffectDistributionFunction_t)*(uint16_t *)(pt + 0x68), current_fraction) * count_scale);
        spawn_count = (int16_t)((uint16_t)current_count - (int32_t)(effect_distribution_function_evaluate(
            (EffectDistributionFunction_t)*(uint16_t *)(pt + 0x68), previous_fraction) * count_scale));
        if (particle_spawn_debug_mode == 1) {
            spawn_count = (int16_t)(int32_t)((real)(int32_t)spawn_count * 0.5f);
        }
        if (spawn_count <= 0) {
            continue;
        }

        marker_handle = self->location_markers[location];
        for (entry = effect_marker_next(self, &marker_handle, create); entry != 0;
             entry = effect_marker_next(self, &marker_handle, create)) {
            uint16_t remaining;

            if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0 &&
                *(int32_t *)(first_person_weapon_globals + self->first_person_weapon_index * 0x1ea0 + 8) == -1) {
                continue;
            }
            remaining = (uint16_t)spawn_count;
            do {
                uint32_t a_bits = *(uint32_t *)(pt + 0xe0);
                uint32_t b_bits = *(uint32_t *)(pt + 0xe4);
                real radius0 = *(real *)(pt + 0x70);
                real base_radius = radius0;
                real radius_span;
                real radius;
                uint32_t radius_word;
                int16_t sample_index;
                real_point3d sample;
                real_matrix4x3 *m = &entry->transform;
                particle_creation_data record;
                real_point3d position;       // the node-space results the detached path uses
                real_vector3d direction;
                real_vector3d velocity;
                uint8_t create_ok;
                real frac;
                uint32_t flags;

                if ((a_bits & 0x80) != 0) {
                    base_radius *= self->a_scale;
                }
                if ((b_bits & 0x80) != 0) {
                    base_radius *= self->b_scale;
                }
                radius_span = *(real *)(pt + 0x74) - radius0;
                if ((a_bits & 0x100) != 0) {
                    radius_span *= self->a_scale;
                }
                if ((b_bits & 0x100) != 0) {
                    radius_span *= self->b_scale;
                }
                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                radius_word = effect_random_seed;
                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                sample_index = (int16_t)(((effect_random_seed >> 16) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);
                sample = sphere_point_table[sample_index];
                radius = (real)(int32_t)(radius_word >> 16) * 1.5259022e-05f * radius_span + base_radius;

                // marker space: record.position = marker * (offset * scale) + sample * radius
                effect_spawn_particles_transform_point(&record.position, *(real *)(pt + 0x14),
                    *(real *)(pt + 0x18), *(real *)(pt + 0x1c), m);
                record.position.x += sample.x * radius;
                record.position.y += sample.y * radius;
                record.position.z += sample.z * radius;
                {
                    real_vector3d raw_direction, raw_velocity;

                    effect_random_velocity_vector(self, &effect_random_seed, (real_vector3d *)(pt + 0x20),
                        &raw_direction, &raw_velocity, *(real *)(pt + 0x84), *(real *)(pt + 0x88),
                        *(real *)(pt + 0x8c), a_bits, (uint8_t)b_bits);
                    effect_spawn_particles_rotate_unscaled((real_vector3d *)&record.unknown_1c, raw_direction.i, raw_direction.j,
                        raw_direction.k, m);
                    effect_spawn_particles_transform_normal(&record.velocity, raw_velocity.i, raw_velocity.j,
                        raw_velocity.k, m);
                }

                if (entry->marker_index != 0xffff) {
                    real_matrix4x3 *node;
                    int16_t node_index = (int16_t)(entry->marker_index & 0x7fff);

                    if ((entry->marker_index & 0x8000) != 0) {
                        node = (real_matrix4x3 *)(first_person_weapon_globals + 0x108c +
                            self->first_person_weapon_index * 0x1ea0 + node_index * 0x34);
                    } else {
                        uint8_t *owner = (uint8_t *)((object_header *)object_data->data)[(uint16_t)self->object_index].data;

                        node = (real_matrix4x3 *)(owner + *(int16_t *)(owner + 0x1f2) + node_index * 0x34);
                    }
                    effect_spawn_particles_transform_point(&position, record.position.x, record.position.y,
                        record.position.z, node);
                    effect_spawn_particles_rotate_unscaled(&direction, record.unknown_1c.x, record.unknown_1c.y,
                        record.unknown_1c.z, node);
                    effect_spawn_particles_transform_normal(&velocity, record.velocity.i, record.velocity.j,
                        record.velocity.k, node);
                } else {
                    position = record.position;
                    direction = *(real_vector3d *)&record.unknown_1c;
                    velocity = record.velocity;
                }

                switch (*(int16_t *)(pt + 0x00)) { // create_in, jump table 0x4529c0
                case 0:
                    create_ok = 1;
                    break;
                case 1: // air only
                    create_ok = !scenario_location_get_water_and_weather(&position, &self->location, 0);
                    break;
                case 2: // water only
                    create_ok = scenario_location_get_water_and_weather(&position, &self->location, 0);
                    break;
                default:
                    create_ok = 0;
                    break;
                }
                if (!create_ok) {
                    continue; // 0x45295e: the do/while condition still counts this particle
                }

                record.definition_index = *(datum_index *)(pt + 0x60);
                flags = *(uint32_t *)(pt + 0x64);
                if ((flags & 1) != 0) {
                    // stay attached to the marker: marker-space vectors, the owner and the node
                    record.object_index = self->object_index;
                    record.marker_index = (entry->marker_index == 0xffff) ? -1 : (int16_t)(entry->marker_index & 0x7fff);
                    record.gravity = *(real_vector3d *)global_origin3d_pointer;
                } else {
                    if (self->tint_source.proc != 0) {
                        void (*tint_proc)(real_vector3d *, real_point3d *, void *) =
                            (void (*)(real_vector3d *, real_point3d *, void *))(uintptr_t)self->tint_source.proc;
                        tint_proc(&record.gravity, &position, (void *)(uintptr_t)self->tint_source.data);
                    } else {
                        record.gravity = *(real_vector3d *)global_origin3d_pointer;
                    }
                    record.position = position;
                    *(real_vector3d *)&record.unknown_1c = direction;
                    record.object_index = 0xffffffff;
                    record.marker_index = -1; // left stale by the binary; unused without an object
                    record.velocity.i = self->velocity.i * 30.0f + velocity.i;
                    record.velocity.j = self->velocity.j * 30.0f + velocity.j;
                    record.velocity.k = self->velocity.k * 30.0f + velocity.k;
                }
                record.scale = effect_property_random_value(9, self, a_bits, b_bits, &effect_random_seed,
                    *(real *)(pt + 0xa0), *(real *)(pt + 0xa4));
                record.unknown_44 = effect_property_random_value(3, self, *(uint32_t *)(pt + 0xe0),
                    *(uint32_t *)(pt + 0xe4), &effect_random_seed, *(real *)(pt + 0x90), *(real *)(pt + 0x94));
                if ((pt[0x64] & 2) != 0) {
                    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                    record.unknown_40 = (real)(int32_t)(effect_random_seed >> 16) * 1.5259022e-05f * 6.2831855f;
                } else {
                    record.unknown_40 = 0.0f;
                }
                if ((*(uint32_t *)(pt + 0xe0) & 0x800) == 0 && (*(uint32_t *)(pt + 0xe4) & 0x800) == 0) {
                    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                    frac = (real)(int32_t)(effect_random_seed >> 16) * 1.5259022e-05f;
                } else {
                    frac = ((*(uint32_t *)(pt + 0xe0) & 0x800) != 0) ? self->a_scale : 1.0f;
                    if ((*(uint32_t *)(pt + 0xe4) & 0x800) != 0) {
                        frac *= self->b_scale;
                    }
                }
                flags = *(uint32_t *)(pt + 0x64);
                color_interpolate((ColorRGB *)(pt + 0xc4), (ColorRGB *)(pt + 0xb4),
                    (ColorRGB *)&record.color.red, (flags >> 3) & 3, frac);
                record.color.alpha = (1.0f - frac) * *(real *)(pt + 0xb0) + frac * *(real *)(pt + 0xc0);
                if ((flags & 4) != 0) {
                    record.color.red *= self->color.red;
                    record.color.green *= self->color.green;
                    record.color.blue *= self->color.blue;
                }
                *(int16_t *)&record.first_person_weapon_index = self->first_person_weapon_index;
                record.first_person = (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0);
                record.unknown_0d = (create == 2);
                record.unknown_0e = (create == 1);
                particle_new(&record);
            } while (--remaining != 0);
        }
    }
    self->previous_event_fraction = current_fraction;
}

#if 0
Original Ghidra decompilation (0x451f90):

void __cdecl particle_system_spawn_particles(int particle_system)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  undefined2 uVar5;
  ushort uVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  char cVar14;
  short sVar15;
  ushort uVar16;
  uint uVar17;
  float *pfVar18;
  int iVar19;
  int iVar20;
  undefined2 *puVar21;
  bool bVar22;
  float10 fVar23;
  char local_c9;
  uint local_c4;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  undefined4 local_84;
  undefined4 local_80;
  undefined4 local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  int local_4c;
  undefined4 local_48;
  float local_44;
  float local_40;
  float local_3c;
  int local_38;
  float local_34;
  float local_30;
  float local_2c;
  int local_28;
  float local_24;
  float local_20;
  float local_1c;
  short local_18;
  undefined4 local_14;
  float local_10;
  float local_c;
  float local_8;

  if (DAT_0069c565 != '\0') {
    local_28 = *(int *)((*(uint *)(particle_system + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar20 = *(short *)(particle_system + 0x4e) * 0x44 + *(int *)(local_28 + 0x38);
    local_14 = *(undefined4 *)(particle_system + 0x58);
    if (*(float *)(particle_system + 0x54) <= 0.0) {
      local_5c = 1.0;
    }
    else {
      local_5c = *(float *)(particle_system + 0x50) / *(float *)(particle_system + 0x54);
    }
    iVar19 = 0;
    local_38 = 0;
    local_4c = iVar20;
    if (0 < *(int *)(iVar20 + 0x38)) {
      do {
        puVar21 = (undefined2 *)(iVar19 * 0xe8 + *(int *)(iVar20 + 0x3c));
        sVar4 = puVar21[4];
        if ((-1 < sVar4) && ((int)sVar4 < *(int *)(local_28 + 0x28))) {
          if ((*(byte *)(particle_system + 2) >> 6 & 1) == 0) {
            bVar22 = puVar21[1] == 2;
          }
          else {
            bVar22 = puVar21[1] == 1;
          }
          iVar20 = local_4c;
          if (!bVar22) {
            uVar5 = puVar21[0x34];
            transition_function_evaluate(uVar5,local_5c);
            local_18 = __ftol();
            transition_function_evaluate(uVar5,local_14);
            sVar15 = __ftol();
            uVar16 = local_18 - sVar15;
            if (DAT_0069c565 == '\x01') {
              uVar16 = __ftol();
            }
            iVar20 = local_4c;
            if (0 < (short)uVar16) {
              local_48 = *(undefined4 *)(particle_system + 0x5c + sVar4 * 4);
              iVar19 = FUN_00453180(particle_system,&local_48,puVar21[2]);
              iVar20 = local_4c;
              while (local_4c = iVar20, iVar19 != 0) {
                if (((*(short *)(iVar19 + 2) == -1) || (-1 < *(short *)(iVar19 + 2))) ||
                   (*(int *)(*(short *)(particle_system + 0x4c) * 0x1ea0 + 8 + DAT_006b2d98) != -1))
                {
                  local_c4 = (uint)uVar16;
                  do {
                    fVar1 = *(float *)(puVar21 + 0x38);
                    local_10 = *(float *)(puVar21 + 0x3a);
                    fVar7 = fVar1;
                    if ((char)*(uint *)(puVar21 + 0x70) < '\0') {
                      fVar7 = fVar1 * *(float *)(particle_system + 0x44);
                    }
                    if ((char)*(uint *)(puVar21 + 0x72) < '\0') {
                      fVar7 = fVar7 * *(float *)(particle_system + 0x48);
                    }
                    fVar1 = local_10 - fVar1;
                    if ((*(uint *)(puVar21 + 0x70) & 0x100) != 0) {
                      fVar1 = fVar1 * *(float *)(particle_system + 0x44);
                    }
                    if ((*(uint *)(puVar21 + 0x72) & 0x100) != 0) {
                      fVar1 = fVar1 * *(float *)(particle_system + 0x48);
                    }
                    uVar17 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
                    DAT_00719cd4 = uVar17 * 0x19660d + 0x3c6ef35f;
                    pfVar18 = (float *)(DAT_006b7af4 +
                                       (short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10) *
                                       0xc);
                    local_24 = *pfVar18;
                    local_20 = pfVar18[1];
                    local_1c = pfVar18[2];
                    fVar7 = (float)(uVar17 >> 0x10) * 1.5259022e-05 * fVar1 + fVar7;
                    fVar1 = *(float *)(puVar21 + 10);
                    fVar2 = *(float *)(puVar21 + 0xc);
                    fVar3 = *(float *)(puVar21 + 0xe);
                    if (*(int *)(iVar19 + 8) != 0x3f800000) {
                      fVar1 = fVar1 * *(float *)(iVar19 + 8);
                      fVar2 = fVar2 * *(float *)(iVar19 + 8);
                      fVar3 = fVar3 * *(float *)(iVar19 + 8);
                    }
                    local_c = fVar2 * *(float *)(iVar19 + 0x18) +
                              fVar1 * *(float *)(iVar19 + 0xc) + fVar3 * *(float *)(iVar19 + 0x24);
                    local_8 = fVar2 * *(float *)(iVar19 + 0x1c) +
                              fVar1 * *(float *)(iVar19 + 0x10) + fVar3 * *(float *)(iVar19 + 0x28);
                    fVar9 = local_24 * fVar7 + *(float *)(iVar19 + 0x30) + local_c;
                    fVar8 = local_20 * fVar7 + *(float *)(iVar19 + 0x34) + local_8;
                    fVar1 = local_1c * fVar7 + *(float *)(iVar19 + 0x38) +
                            fVar2 * *(float *)(iVar19 + 0x20) +
                            fVar1 * *(float *)(iVar19 + 0x14) + fVar3 * *(float *)(iVar19 + 0x2c);
                    FUN_00451310(&DAT_00719cd4,puVar21 + 0x10,&local_9c,&local_90,
                                 *(undefined4 *)(puVar21 + 0x42),*(undefined4 *)(puVar21 + 0x44),
                                 *(undefined4 *)(puVar21 + 0x46),*(undefined4 *)(puVar21 + 0x70),
                                 *(undefined4 *)(puVar21 + 0x72));
                    fVar2 = local_94 * *(float *)(iVar19 + 0x24) +
                            local_98 * *(float *)(iVar19 + 0x18) +
                            local_9c * *(float *)(iVar19 + 0xc);
                    fVar7 = local_94 * *(float *)(iVar19 + 0x28) +
                            local_98 * *(float *)(iVar19 + 0x1c) +
                            local_9c * *(float *)(iVar19 + 0x10);
                    local_94 = local_94 * *(float *)(iVar19 + 0x2c) +
                               local_9c * *(float *)(iVar19 + 0x14) +
                               local_98 * *(float *)(iVar19 + 0x20);
                    if (*(int *)(iVar19 + 8) != 0x3f800000) {
                      local_90 = local_90 * *(float *)(iVar19 + 8);
                      local_8c = local_8c * *(float *)(iVar19 + 8);
                      local_88 = local_88 * *(float *)(iVar19 + 8);
                    }
                    uVar6 = *(ushort *)(iVar19 + 2);
                    fVar10 = local_90 * *(float *)(iVar19 + 0xc) +
                             local_8c * *(float *)(iVar19 + 0x18) +
                             local_88 * *(float *)(iVar19 + 0x24);
                    fVar3 = local_90 * *(float *)(iVar19 + 0x10) +
                            local_8c * *(float *)(iVar19 + 0x1c) +
                            local_88 * *(float *)(iVar19 + 0x28);
                    local_88 = local_90 * *(float *)(iVar19 + 0x14) +
                               local_8c * *(float *)(iVar19 + 0x20) +
                               local_88 * *(float *)(iVar19 + 0x2c);
                    local_58 = fVar9;
                    local_54 = fVar8;
                    local_50 = fVar1;
                    local_44 = fVar2;
                    local_40 = fVar7;
                    local_3c = local_94;
                    local_34 = fVar10;
                    local_30 = fVar3;
                    local_2c = local_88;
                    if (uVar6 != 0xffff) {
                      if ((short)uVar6 < 0) {
                        pfVar18 = (float *)((short)(uVar6 & 0x7fff) * 0x34 + 0x108c +
                                           *(short *)(particle_system + 0x4c) * 0x1ea0 +
                                           DAT_006b2d98);
                      }
                      else {
                        iVar20 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                         (*(uint *)(particle_system + 0x3c) & 0xffff) * 0xc);
                        pfVar18 = (float *)((int)*(short *)(iVar20 + 0x1f2) +
                                           (short)(uVar6 & 0x7fff) * 0x34 + iVar20);
                      }
                      if (*pfVar18 != 1.0) {
                        fVar9 = fVar9 * *pfVar18;
                        fVar8 = fVar8 * *pfVar18;
                        fVar1 = fVar1 * *pfVar18;
                      }
                      local_3c = fVar2 * pfVar18[3] + local_94 * pfVar18[9] + fVar7 * pfVar18[6];
                      fVar11 = fVar10;
                      fVar12 = local_88;
                      fVar13 = fVar3;
                      if (*pfVar18 != 1.0) {
                        fVar11 = fVar10 * *pfVar18;
                        fVar13 = fVar3 * *pfVar18;
                        fVar12 = local_88 * *pfVar18;
                      }
                      local_2c = fVar11 * pfVar18[3] + fVar13 * pfVar18[6] + fVar12 * pfVar18[9];
                      local_58 = fVar9 * pfVar18[1] + fVar8 * pfVar18[4] + fVar1 * pfVar18[7] +
                                 pfVar18[10];
                      local_54 = fVar9 * pfVar18[2] + fVar8 * pfVar18[5] + fVar1 * pfVar18[8] +
                                 pfVar18[0xb];
                      local_50 = fVar9 * pfVar18[3] + fVar8 * pfVar18[6] + fVar1 * pfVar18[9] +
                                 pfVar18[0xc];
                      local_44 = fVar2 * pfVar18[1] + local_94 * pfVar18[7] + fVar7 * pfVar18[4];
                      local_40 = fVar2 * pfVar18[2] + local_94 * pfVar18[8] + fVar7 * pfVar18[5];
                      local_34 = fVar11 * pfVar18[1] + fVar13 * pfVar18[4] + fVar12 * pfVar18[7];
                      local_30 = fVar11 * pfVar18[2] + fVar13 * pfVar18[5] + fVar12 * pfVar18[8];
                    }
                    local_9c = fVar2;
                    local_98 = fVar7;
                    local_90 = fVar10;
                    local_8c = fVar3;
                    switch(*puVar21) {
                    case 0:
                      local_c9 = '\x01';
                      goto LAB_00452671;
                    case 1:
                      cVar14 = FUN_0053ed60(particle_system + 0x10,0);
                      local_c9 = '\x01' - (cVar14 != '\0');
                      break;
                    case 2:
                      local_c9 = FUN_0053ed60(particle_system + 0x10,0);
                      break;
                    case 3:
                      local_c9 = '\0';
                      goto LAB_0045295e;
                    }
                    if (local_c9 != '\0') {
LAB_00452671:
                      if ((*(byte *)(puVar21 + 0x32) & 1) == 0) {
                        if (*(code **)(particle_system + 0x34) == (code *)0x0) {
                          local_84 = *(undefined4 *)PTR_DAT_00696714;
                          local_80 = *(undefined4 *)(PTR_DAT_00696714 + 4);
                          local_7c = *(undefined4 *)(PTR_DAT_00696714 + 8);
                        }
                        else {
                          (**(code **)(particle_system + 0x34))
                                    (&local_84,&local_58,*(undefined4 *)(particle_system + 0x30));
                        }
                        local_90 = *(float *)(particle_system + 0x24) * 30.0 + local_34;
                        local_9c = local_44;
                        local_98 = local_40;
                        local_8c = *(float *)(particle_system + 0x28) * 30.0 + local_30;
                        local_94 = local_3c;
                        local_88 = *(float *)(particle_system + 0x2c) * 30.0 + local_2c;
                      }
                      else {
                        local_84 = *(undefined4 *)PTR_DAT_00696714;
                        local_80 = *(undefined4 *)(PTR_DAT_00696714 + 4);
                        local_7c = *(undefined4 *)(PTR_DAT_00696714 + 8);
                      }
                      fVar23 = (float10)particle_system_property_random_value
                                                  (&DAT_00719cd4,*(undefined4 *)(puVar21 + 0x50),
                                                   *(undefined4 *)(puVar21 + 0x52));
                      local_70 = (float)fVar23;
                      fVar23 = (float10)particle_system_property_random_value
                                                  (&DAT_00719cd4,*(undefined4 *)(puVar21 + 0x48),
                                                   *(undefined4 *)(puVar21 + 0x4a));
                      local_74 = (float)fVar23;
                      if ((*(byte *)(puVar21 + 0x32) & 2) == 0) {
                        local_78 = 0.0;
                      }
                      else {
                        DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
                        local_78 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
                      }
                      if (((*(uint *)(puVar21 + 0x70) & 0x800) == 0) &&
                         ((*(uint *)(puVar21 + 0x72) & 0x800) == 0)) {
                        DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
                        fVar1 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
                      }
                      else {
                        fVar1 = 1.0;
                        if ((*(uint *)(puVar21 + 0x70) & 0x800) != 0) {
                          fVar1 = *(float *)(particle_system + 0x44);
                        }
                        if ((*(uint *)(puVar21 + 0x72) & 0x800) != 0) {
                          fVar1 = fVar1 * *(float *)(particle_system + 0x48);
                        }
                      }
                      uVar17 = *(uint *)(puVar21 + 0x32);
                      color_interpolate(&local_68,uVar17 >> 3 & 3,fVar1);
                      local_6c = fVar1 * *(float *)(puVar21 + 0x60) +
                                 (1.0 - fVar1) * *(float *)(puVar21 + 0x58);
                      if ((uVar17 & 4) != 0) {
                        local_68 = local_68 * *(float *)(particle_system + 0x18);
                        local_64 = local_64 * *(float *)(particle_system + 0x1c);
                        local_60 = local_60 * *(float *)(particle_system + 0x20);
                      }
                      FUN_00455740();
                    }
LAB_0045295e:
                    local_c4 = local_c4 - 1;
                  } while (local_c4 != 0);
                }
                iVar19 = FUN_00453180(particle_system,&local_48,puVar21[2]);
                iVar20 = local_4c;
              }
            }
          }
        }
        local_38 = local_38 + 1;
        iVar19 = (int)(short)local_38;
      } while (iVar19 < *(int *)(iVar20 + 0x38));
    }
    *(float *)(particle_system + 0x58) = local_5c;
  }
  return;
}
#endif

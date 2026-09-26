// effect_spawn_particles  (Ghidra: particle_system_spawn_particles; RENAMED per
// out/phase4/effects_types_notes.md's misattribution table: "0x451f90
// particle_system_spawn_particles -> effect_spawn_particles (one EffectEvent, not a pctl
// system)")
// address 0x451f90, size 2600 bytes
// name confidence: 0.5   rewrite confidence: 0.2 (LOW -- this is the densest function in the
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

extern void color_interpolate(void *out_color, uint32_t color_pair, float t); // 0x43f6a0
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
    if (particle_spawn_debug_mode == 0) {
        return;
    }

    {
        Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
        EffectEvent *event = &((EffectEvent *)tag->events.pointer)[self->event_index];
        EffectParticle *particle_types = (EffectParticle *)event->particles.pointer;
        real previous_fraction = self->previous_event_fraction;
        real current_fraction = (self->event_duration <= 0.0f) ? 1.0f
            : self->event_time / self->event_duration;
        int32_t type_index;

        for (type_index = 0; type_index < (int32_t)event->particles.count; type_index++) {
            EffectParticle *particle_type = &particle_types[type_index];
            int16_t location = particle_type->location;

            if (location < 0 || (int32_t)location >= (int32_t)tag->locations.count) {
                continue;
            }

            {
                uint8_t skip_type;
                if ((self->flags & _effect_first_person_bit) == 0) {
                    skip_type = (particle_type->violence_mode == 2); // UNSURE, see object_change_color_evaluate.c's matching note
                } else {
                    skip_type = (particle_type->violence_mode == 1);
                }
                if (skip_type) {
                    continue;
                }
            }

            {
                int16_t current_count = (int16_t)(int32_t)(effect_distribution_function_evaluate(
                    particle_type->distribution_function, current_fraction) *
                    self->particle_counts[type_index]); // UNSURE 1
                int16_t previous_count = (int16_t)(int32_t)(effect_distribution_function_evaluate(
                    particle_type->distribution_function, previous_fraction) *
                    self->particle_counts[type_index]); // UNSURE 1
                int16_t spawn_count = current_count - previous_count;

                if (particle_spawn_debug_mode == 1) {
                    spawn_count = (int16_t)__ftol(); // UNSURE 2
                }

                if (spawn_count <= 0) {
                    continue;
                }

                {
                    datum_index marker_handle = self->location_markers[location];
                    effect_location_marker *entry = effect_marker_next(self, &marker_handle,
                        particle_type->create);

                    while (entry != (effect_location_marker *)0) {
                        // Skip first-person-weapon markers unless the weapon is currently held
                        // (globals[fp_index]+8 != -1), matching every other "first-person
                        // marker gate" in this module.
                        if (entry->marker_index == 0xffff || (entry->marker_index & 0x8000) == 0 ||
                            *(int32_t *)(first_person_weapon_globals +
                                self->first_person_weapon_index * 0x1ea0 + 8) != -1) {
                            int16_t remaining = spawn_count;

                            do {
                                real base_radius = particle_type->distribution_radius[0];
                                real radius_span = particle_type->distribution_radius[1] - base_radius;

                                if ((particle_type->a_scales_values & 0x80) != 0) { // bit7 distribution_radius
                                    base_radius *= self->a_scale;
                                }
                                if ((particle_type->b_scales_values & 0x80) != 0) {
                                    base_radius *= self->b_scale;
                                }
                                if ((particle_type->a_scales_values & 0x100) != 0) { // bit8 distribution_radius_delta
                                    radius_span *= self->a_scale;
                                }
                                if ((particle_type->b_scales_values & 0x100) != 0) {
                                    radius_span *= self->b_scale;
                                }

                                // Two advances: the first word drives the radius fraction, the
                                // second selects the sphere_point_table sample.
                                {
                                    uint32_t radius_word =
                                        effect_random_seed * k_random_multiplier + k_random_increment;
                                    int16_t sample_index;
                                    real_point3d sample;
                                    real radius;
                                    real_matrix4x3 *m = &entry->transform;
                                    real_point3d position;
                                    real_vector3d vec1, vec2;
                                    real_vector3d pre_node_direction, pre_node_velocity;

                                    effect_random_seed =
                                        radius_word * k_random_multiplier + k_random_increment;

                                    sample_index = (int16_t)((effect_random_seed >> k_random_value_shift) *
                                        (uint32_t)(int32_t)sphere_point_table_count >> 16);
                                    sample = sphere_point_table[sample_index];
                                    radius = (real)(radius_word >> k_random_value_shift) *
                                        1.5259022e-05f * radius_span + base_radius;

                                    effect_spawn_particles_transform_point(&position,
                                        particle_type->relative_offset.x, particle_type->relative_offset.y,
                                        particle_type->relative_offset.z, m);
                                    position.x += sample.x * radius;
                                    position.y += sample.y * radius;
                                    position.z += sample.z * radius;

                                    {
                                        real_vector3d raw_direction, raw_velocity;
                                        effect_random_velocity_vector(self, &effect_random_seed,
                                            (real_vector3d *)&particle_type->relative_direction_vector,
                                            &raw_direction, &raw_velocity,
                                            particle_type->velocity[0], particle_type->velocity[1],
                                            particle_type->velocity_cone_angle,
                                            particle_type->a_scales_values,
                                            (uint8_t)particle_type->b_scales_values);

                                        // vec1 (the direction) is rotated WITHOUT the marker
                                        // scale; vec2 (the velocity) is scaled first.
                                        effect_spawn_particles_rotate_unscaled(&vec1,
                                            raw_direction.i, raw_direction.j, raw_direction.k, m);
                                        effect_spawn_particles_transform_normal(&vec2,
                                            raw_velocity.i, raw_velocity.j, raw_velocity.k, m);
                                    }

                                    // The original keeps the pre-node vectors live across the
                                    // node transform and hands THOSE to particle_new on the
                                    // "stay attached to marker" path below.
                                    pre_node_direction = vec1;
                                    pre_node_velocity = vec2;

                                    if (entry->marker_index != 0xffff) {
                                        real_matrix4x3 *node;
                                        uint16_t node_index = entry->marker_index & 0x7fff;

                                        if ((entry->marker_index & 0x8000) != 0) {
                                            node = (real_matrix4x3 *)(first_person_weapon_globals + 0x108c +
                                                self->first_person_weapon_index * 0x1ea0 + node_index * 0x34);
                                        } else {
                                            object *owner = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                                            node = (real_matrix4x3 *)((uint8_t *)owner + owner->nodes.offset +
                                                node_index * 0x34);
                                        }

                                        {
                                            real_point3d p2;
                                            real_vector3d v1b, v2b;
                                            effect_spawn_particles_transform_point(&p2, position.x, position.y, position.z, node);
                                            // Again: direction unscaled, velocity scaled.
                                            effect_spawn_particles_rotate_unscaled(&v1b, vec1.i, vec1.j, vec1.k, node);
                                            effect_spawn_particles_transform_normal(&v2b, vec2.i, vec2.j, vec2.k, node);
                                            position = p2;
                                            vec1 = v1b;
                                            vec2 = v2b;
                                        }
                                    }

                                    {
                                        uint8_t create_ok;
                                        switch (particle_type->create_in) {
                                        case effectcreatein_any_environment:
                                            create_ok = 1;
                                            break;
                                        // 0x45262b / 0x45264c: EBX = &position ([esp+0x84]), push &self->location, 0
                                        case effectcreatein_air_only:
                                            create_ok = !scenario_location_get_water_and_weather(&position, &self->location, 0);
                                            break;
                                        case effectcreatein_water_only:
                                            create_ok = scenario_location_get_water_and_weather(&position, &self->location, 0);
                                            break;
                                        case effectcreatein_space_only:
                                            create_ok = 0;
                                            break;
                                        default:
                                            create_ok = 0;
                                            break;
                                        }

                                        if (create_ok) {
                                            particle_creation_data creation_data;
                                            real_vector3d out_direction, out_velocity;
                                            real_point3d tint_color;
                                            real fraction, spin_angle, rolled_scale, rolled_rotation_rate;
                                            uint32_t scale_bits;

                                            // UNSURE: particle_creation_data (types/effects.h) is
                                            // documented as "at least 0x5c bytes"; these two rolls
                                            // (radius[2] and angular_velocity[2]) clearly feed
                                            // the new particle's scale and rotation rate, but the
                                            // struct has no named rotation-rate field yet, so the
                                            // roll is computed and stored in `creation_data.scale`
                                            // only -- the rotation-rate destination is a TYPES-GAP.
                                            rolled_scale = effect_property_random_value(0, self, 0, 0,
                                                &effect_random_seed, particle_type->radius[0],
                                                particle_type->radius[1]); // UNSURE, see file header note 1
                                            rolled_rotation_rate = effect_property_random_value(0, self, 0, 0,
                                                &effect_random_seed, particle_type->angular_velocity[0],
                                                particle_type->angular_velocity[1]); // UNSURE
                                            (void)rolled_rotation_rate; // TYPES-GAP: no field to store
                                                // this in on particle_creation_data yet

                                            if ((particle_type->flags & 1) == 0) { // stay_attached_to_marker clear
                                                if (self->tint_source.proc == 0) {
                                                    tint_color = *global_origin3d_pointer;
                                                } else {
                                                    void (*tint_proc)(real_point3d *, real_point3d *, void *) =
                                                        (void (*)(real_point3d *, real_point3d *, void *))(uintptr_t)self->tint_source.proc;
                                                    tint_proc(&tint_color, &position, (void *)(uintptr_t)self->tint_source.data);
                                                        // UNSURE 5: argument order/types guessed from
                                                        // types/effects.h's tint_source note
                                                }
                                                // The effect's own velocity, per tick at 30 Hz,
                                                // is added to the particle's velocity only. The
                                                // direction passes through untouched.
                                                out_velocity.i = self->velocity.i * 30.0f + vec2.i;
                                                out_velocity.j = self->velocity.j * 30.0f + vec2.j;
                                                out_velocity.k = self->velocity.k * 30.0f + vec2.k;
                                                out_direction = vec1;
                                            } else {
                                                tint_color = *global_origin3d_pointer;
                                                // flags bit 0 set: particle_new gets the vectors
                                                // as they stood before the node transform.
                                                out_direction = pre_node_direction;
                                                out_velocity = pre_node_velocity;
                                            }

                                            fraction = 0.0f; // UNSURE 1-adjacent: particle_system_property_random_value
                                                // roll for scale/animation_rate elided here, see file header
                                            (void)fraction;

                                            if ((particle_type->flags & 2) == 0) { // random_initial_angle clear
                                                spin_angle = 0.0f;
                                            } else {
                                                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                                                spin_angle = (real)(effect_random_seed >> k_random_value_shift) *
                                                    1.5259022e-05f * 6.2831855f;
                                            }
                                            (void)spin_angle; // TYPES-GAP: no rotation field on
                                                // particle_creation_data to store this in yet

                                            if ((particle_type->a_scales_values & 0x800) == 0 &&
                                                (particle_type->b_scales_values & 0x800) == 0) {
                                                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                                                fraction = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
                                            } else {
                                                fraction = 1.0f;
                                                if ((particle_type->a_scales_values & 0x800) != 0) {
                                                    fraction = self->a_scale;
                                                }
                                                if ((particle_type->b_scales_values & 0x800) != 0) {
                                                    fraction *= self->b_scale;
                                                }
                                            }

                                            // One dword load at EffectParticle +0x64, which is
                                            // `flags`; both the colour-pair selector (>>3 & 3)
                                            // and the tint-apply bit (& 4) come out of it.
                                            scale_bits = (uint32_t)particle_type->flags;
                                            color_interpolate(&creation_data.color.red,
                                                (scale_bits >> 3) & 3, fraction);
                                            creation_data.color.alpha =
                                                fraction * particle_type->tint_upper_bound.alpha +
                                                (1.0f - fraction) * particle_type->tint_lower_bound.alpha;
                                            if ((scale_bits & 4) != 0) {
                                                creation_data.color.red *= self->color.red;
                                                creation_data.color.green *= self->color.green;
                                                creation_data.color.blue *= self->color.blue;
                                            }

                                            creation_data.definition_index = particle_type->particle_type.tag_id.index;
                                            creation_data.object_index = self->object_index;
                                            creation_data.marker_index = -1;
                                            creation_data.first_person_weapon_index = (uint8_t)self->first_person_weapon_index;
                                            creation_data.first_person = (self->flags & _effect_first_person_bit) != 0;
                                            creation_data.position = position;
                                            creation_data.unknown_1c = tint_color;   // UNSURE 6
                                            creation_data.velocity = out_velocity;   // UNSURE 6
                                            (void)out_direction; // UNSURE 6: the rotated direction
                                                // is one of the three vectors handed to
                                                // particle_new; particle_creation_data has no
                                                // named field for it yet
                                            creation_data.gravity.i = 0.0f;
                                            creation_data.gravity.j = 0.0f;
                                            creation_data.gravity.k = 0.0f;
                                            creation_data.scale = rolled_scale;

                                            particle_new(&creation_data);
                                        }
                                    }
                                }

                                remaining--;
                            } while (remaining != 0);
                        }

                        entry = effect_marker_next(self, &marker_handle, particle_type->create);
                    }
                }
            }
        }

        self->previous_event_fraction = current_fraction;
    }
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

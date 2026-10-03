#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * Random-draw helpers used by the effect and particle spawners; every member keeps the original draw order.
 */
class effect_random {
public:
    static float evaluate(EffectDistributionFunction_t type, float fraction);
    static void direction_from_table(real_point3d *out);
    static void direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset);
    static real fraction();
    static int16_t int_between(int16_t minimum, int16_t maximum);
    static real scaled_range(uint32_t flags, real scale, real base_min, real base_max, uint8_t bit_index);
    static uint32_t uint16();
    static void velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
};

/**
 * A view over an effect record in the effect data array: markers, events, placement and particle spawning.
 */
class effect_view {
public:
    explicit effect_view(effect *value) : record(value) {}

    void event_apply(EffectPart *part, effect_location_marker *marker, real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale);
    static void environment_probe(uint32_t definition_index, int16_t location_index, real_point3d *marker_position, uint32_t sound_param);
    static void from_node_table(int16_t entry_index, uint8_t *context, object_marker *out);
    datum_index create(int16_t location_index, object_marker *resolved_marker, uint8_t first_person);
    effect_location_marker * next(datum_index *marker, int32_t mode);
    static int32_t node_table_resolver(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
    real property_random_value(uint8_t bit_index, uint32_t a_bitset, uint32_t b_bitset, random_seed *seed, real base_min, real base_max);
    static void reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index);
    void rebuild_markers(effect_marker_resolver resolve_marker);
    static void release_first_person_markers(int16_t first_person_weapon_index);
    real_matrix4x3 * resolve_marker_transform(int16_t marker);
    void set_placement(const ColorRGB *color, const effect_tint_source *tint_source, real a_scale, real b_scale);
    void spawn_particles();
    void change_color_evaluate();

    effect *record;
};

/**
 * An effect addressed by its datum index: creation, update, stop and the module-wide passes.
 */
class effect_ref {
public:
    explicit effect_ref(datum_index value) : datum(value) {}

    static uint32_t check_object_collisions();
    void destroy();
    static uint8_t first_person_screen_timer_active(datum_index object_index);
    static datum_index create(datum_index definition_index, datum_index creator_object_index, uint8_t force_create);
    static datum_index new_at_texture_coordinate(datum_index definition_index, datum_index object_index, int16_t change_color_index, int16_t u, int16_t v);
    static datum_index new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
    static datum_index new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t marker_count, uint32_t marker_names, uint32_t marker_positions, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
    static datum_index new_with_color(datum_index definition_index, datum_index creator_object_index, const real_vector3d *velocity, uint16_t marker_count, uint32_t marker_names, real_point3d *position, uint32_t marker_forwards, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source, uint8_t force_create);
    void start_event(int16_t event_index);
    void stop(uint8_t stop_immediately);
    effect * try_and_get();
    void update(real dt);
    static void refresh_structure_locations();
    static void update_all(real delta_time);

    datum_index datum;
};

}
}

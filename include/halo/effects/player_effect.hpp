#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * A view over a local player effect record: camera shake, camera impulse and screen flash setters.
 */
class player_effect_view {
public:
    explicit player_effect_view(player_effect *value) : record(value) {}

    void set_camera_impulse(int16_t local_player_index, real *descriptor, real *direction, real intensity_falloff, real duration_scale);
    void set_camera_shake(player_camera_shake *descriptor, float intensity_falloff, float duration_scale);
    void set_screen_flash(player_screen_flash *descriptor, float intensity_falloff, float duration_scale);

    player_effect *record;
};

/**
 * Player damage and screen effects addressed by player datum index, plus the per-local-player builders.
 */
class player_effect_ref {
public:
    explicit player_effect_ref(datum_index value) : datum(value) {}

    static void apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin);
    static void apply_continuous_damage(uint32_t tag_reference, int16_t local_player_index, float distance);
    void apply_generic_damage_feedback(float fraction);
    static void build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index);
    static void build_screen_flash(uint32_t *out, int16_t local_player_index);
    static void clear_dead_players();
    static void fade_damage_indicators(int16_t local_player_index, uint32_t *out_previous_indicators);
    void mark_damage_direction(const damage_data *dd, const real_vector3d *direction, float random_blend, float damage_amount);
    static void mark_damage_direction_dispatch(void **context);
    static void random_shake_offset(real_matrix4x3 *out, real magnitude, real angle);
    void send_network_update(const real_vector3d *direction, const damage_data *dd, float random_blend, float damage_amount);
    void set_screen_flash_for_player(player_screen_flash *descriptor, float intensity_falloff);
    static int32_t locality_for_object(datum_index weapon_object_index);

    datum_index datum;
};

}
}

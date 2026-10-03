#include "halo/effects/effects.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern int32_t *player_control_globals_ptr;
extern random_seed effect_random_seed;
extern double fabs(double x);
extern const real *global_up3d_pointer;
extern double cos(double x);
extern double sin(double x);
extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b);
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b);
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
extern void game_engine_update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta);
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward);
extern player_globals *local_player_globals;
}

namespace halo::effects {

/**
 * Replaces a local player's active camera impulse with a new one built from `descriptor` and
 * `direction` when the new one out-prioritizes (or has run longer than) the current one, then
 * always feeds a second, independent computation into game_engine_update_local_player_look.
 * REWRITTEN from objdump. Raw offsets into player_effect: +0x00 impulse direction, +0x0c impulse rotation, +0x50 the
 * 13-float impulse copied from the descriptor (+0x50 duration in ticks, +0x60/+0x64 magnitude min/max, +0x68
 * intensity), +0xe0 ticks, +0xe8 flags.
 * A new impulse replaces the running one when it outlasts it, is stronger, or is as strong and longer. Its
 * direction is the angle between the (x, y, 0) damage direction and the player's (pitch, yaw) look vector, both
 * normalized copies (the caller's vector is left alone). The rotation axis is up x direction, rotated about the
 *
 * @address 0x4579b0
 */
void player_effect_view::set_camera_impulse(int16_t local_player_index, real *descriptor, real *direction, real intensity_falloff, real duration_scale)
{
    player_effect * self = record;
    uint8_t *fx = (uint8_t *)self;
    real *impulse = (real *)(fx + 0x50);
    real_vector3d *impulse_direction = (real_vector3d *)(fx + 0x00);
    real_vector3d *impulse_rotation = (real_vector3d *)(fx + 0x0c);
    real_vector3d *up = (real_vector3d *)global_up3d_pointer;
    real duration_ticks = duration_scale * 30.0f;
    real ticks = (real)((struct player_effect *)fx)->impulse_ticks;
    real blended = (1.0f - descriptor[6]) * intensity_falloff + descriptor[6];
    real *look_globals = (real *)((uint8_t *)player_control_globals_ptr + local_player_index * 0x40);

    if (impulse[0] > ticks || blended > ((struct player_effect *)fx)->impulse.intensity ||
        (!(blended < ((struct player_effect *)fx)->impulse.intensity) && duration_ticks * descriptor[0] > ticks)) {
        real_vector3d flat_direction;
        real_vector3d look;
        real a = look_globals[7];
        real b = look_globals[8];

        flat_direction.i = direction[0];
        flat_direction.j = direction[1];
        flat_direction.k = 0.0f;
        vector3d_normalize_with_length(&flat_direction);
        look.i = (real)cos((double)a) * (real)cos((double)b);
        look.j = (real)sin((double)a) * (real)cos((double)b);
        look.k = 0.0f;
        vector3d_normalize_with_length(&look);

        if ((real)fabs((double)(flat_direction.i * flat_direction.i + flat_direction.j * flat_direction.j +
                                flat_direction.k * flat_direction.k - 1.0f)) < 9.9999997e-05 &&
            (real)fabs((double)(look.i * look.i + look.j * look.j + look.k * look.k - 1.0f)) < 9.9999997e-05) {
            real angle = vector2d_angle_between((real_vector2d *)&flat_direction, (real_vector2d *)&look);
            real magnitude;
            real random_angle;
            int i;

            for (i = 0; i < 13; i++) {
                impulse[i] = descriptor[i];
            }
            impulse[0] = duration_ticks * impulse[0];
            ((struct player_effect *)fx)->impulse.intensity = blended;
            ((struct player_effect *)fx)->impulse_ticks = (int16_t)(int32_t)impulse[0];

            impulse_direction->k = 0.0f;
            impulse_direction->i = (real)cos((double)angle);
            impulse_direction->j = (real)sin((double)angle);

            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            magnitude = (real)(int32_t)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                (((struct player_effect *)fx)->impulse.magnitude_maximum - ((struct player_effect *)fx)->impulse.magnitude_minimum) + ((struct player_effect *)fx)->impulse.magnitude_minimum;
            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
            random_angle = (real)(int32_t)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;

            vector3d_cross_product(impulse_rotation, up, impulse_direction);
            vector3d_normalize_with_length(impulse_rotation);
            vector3d_rotate_about_axis(impulse_rotation, impulse_direction, (real)sin((double)random_angle),
                (real)cos((double)random_angle));
            impulse_rotation->i = magnitude * impulse_rotation->i;
            impulse_rotation->j = magnitude * impulse_rotation->j;
            impulse_rotation->k = magnitude * impulse_rotation->k;
            *(uint8_t *)&((struct player_effect *)fx)->flags |= 2;
        }
    }

    {
        real blended_b = (1.0f - descriptor[9]) * intensity_falloff + descriptor[9];
        datum_index player_handle = (local_player_index != -1 && local_player_index < 1) ?
            *(datum_index *)&local_player_globals->local_players[local_player_index] : (datum_index)k_datum_index_none;
        real_vector3d forward;
        real_vector3d left;
        real yaw_delta;
        real pitch_delta;

        player_compute_view_forward_vector(player_handle, &look_globals[7], &forward);
        left.i = forward.k * up->j - forward.j * up->k;
        left.j = forward.i * up->k - forward.k * up->i;
        left.k = forward.j * up->i - forward.i * up->j;
        yaw_delta = (left.j * direction[1] + left.k * direction[2] + left.i * direction[0]) * descriptor[8] * blended_b;
        pitch_delta = (forward.j * direction[1] + forward.i * direction[0] + forward.k * direction[2]) * descriptor[8] *
            blended_b;
        game_engine_update_local_player_look(local_player_index, yaw_delta, pitch_delta);
    }
}

}

namespace halo::effects {

void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index, real *descriptor, real *direction, real intensity_falloff, real duration_scale)
{
    halo::effects::player_effect_view(self).set_camera_impulse(local_player_index, descriptor, direction, intensity_falloff, duration_scale);
}

}

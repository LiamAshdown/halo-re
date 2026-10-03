#include "halo/core/lcg.hpp"
#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/core/libm.hpp"

void player_effect_build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index);
static auto &player_effect_globals_pointer = halo::link::ref<player_effect_globals *>(halo::effects::vars().player_effect_globals_pointer);
static auto &k_render_identity_matrix_ptr = halo::link::ref<real_matrix4x3 *>(halo::effects::vars().k_render_identity_matrix_ptr);

namespace halo::effects {

/**
 * File-local helper used by player_effect_build_camera_shake_matrix.
 */
static real shake_random_signed(void)
{
    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(int32_t)(halo::math::globals().effect_random_seed >> 16) * halo::k_unit_word_scale * 2.0f - 1.0f;
}

/**
 * REWRITTEN from objdump. Scripted shake (globals +0x120 bit 0): identity, then while ticks (+0x11c) remain,
 * t = intensity * (bit 1 ? ticks / duration : 1 - ticks / duration) and ticks count down; once they run out
 * with bit 1 set the shake ends (bit 0 cleared). The rotation comes from euler angles with random +-1 draws
 * times the translation amplitudes (+0x10c..+0x114) and t, and the position from three more draws times
 * +0x104 / +0x100 / +0x108. Per player (stride 0xec): while +0xe0 ticks remain (or bit 1 of +0xe8 forces full),
 * t = transition(+0x54, 1 - (+0x50 - ticks) / +0x50) * +0x68, the matrix rotates about cross(+0x00, up) by
 * t * +0x58 and is placed at t * +0x0c + t * +0x5c * (+0x00). Otherwise it's identity. While +0xe2 ticks remain
 * (or bit 2 forces full), a second matrix gets t2 = transition(+0x88, 1 - (+0x84 - ticks) / +0x84) * +0xac,
 * w = (periodic(+0xa0, (+0x84 - ticks) / +0xa4) * +0xa8 + 1 - +0xa8) * t2, and two random shake offsets:
 *
 * @address 0x457390
 */
void player_effect_ref::build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index)
{
    player_effect_globals *globals = player_effect_globals_pointer;
    int16_t dt;

    if (local_player_index == -1) {
        return;
    }
    if ((globals->scripted_shake_flags & 1) != 0) {
        real t = globals->scripted_shake_intensity;
        int16_t ticks = globals->scripted_shake_ticks;

        *out = *k_render_identity_matrix_ptr;
        if (ticks > 0) {
            real fraction = (real)(int32_t)ticks / (real)(int32_t)globals->scripted_shake_duration;

            if ((globals->scripted_shake_flags & 2) == 0) {
                fraction = 1.0f - fraction;
            }
            t = fraction * t;
            globals->scripted_shake_ticks = (int16_t)(ticks - halo::game::globals().game_time->ticks_this_frame);
        } else if ((globals->scripted_shake_flags & 2) != 0) {
            globals->scripted_shake_flags &= ~(uint32_t)1;
        }
        if ((globals->scripted_shake_flags & 1) == 0) {
            return;
        }
        if (!(t >= 0.0f)) {
            t = 0.0f;
        } else if (!(t <= 1.0f)) {
            t = 1.0f;
        }
        {
            real a1 = shake_random_signed();
            real a2 = shake_random_signed();
            real a3 = shake_random_signed();

            halo::math::matrix4x3_from_euler_angles(*out, a3 * globals->scripted_shake_rotation[0] * t, a2 * globals->scripted_shake_rotation[1] * t,
                a1 * globals->scripted_shake_rotation[2] * t);
        }
        {
            real b4 = shake_random_signed();
            real b5 = shake_random_signed();
            real b6 = shake_random_signed();

            out->position.x = b6 * globals->scripted_shake_translation[1] * t;
            out->position.y = b5 * globals->scripted_shake_translation[0] * t;
            out->position.z = b4 * globals->scripted_shake_translation[2] * t;
        }
        return;
    }

    {
        player_effect *self = &globals->players[local_player_index];
        int16_t ticks = self->impulse_ticks;
        real t;

        if (ticks <= 0 && (self->flags & _player_effect_camera_impulse_bit) == 0) {
            *out = *k_render_identity_matrix_ptr;
        } else {
            real_vector3d axis;
            real angle;
            real k;
            real_matrix4x3 rotation;

            if ((self->flags & _player_effect_camera_impulse_bit) != 0) {
                t = 1.0f;
            } else {
                real duration = self->impulse.duration;

                t = halo::math::transition_function_evaluate(self->impulse.transition_function,
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * self->impulse.intensity;
            }
            self->flags &= ~(uint32_t)_player_effect_camera_impulse_bit;
            halo::math::vector3d_cross_product(axis, self->impulse_direction, *halo::math::globals().global_up3d_pointer);
            angle = t * self->impulse.rotation_angle;
            halo::math::matrix4x3_from_axis_angle(rotation, axis, (real)halo::libm::sin((double)angle), (real)halo::libm::cos((double)angle));
            k = t * self->impulse.translation_scale;
            rotation.position.x = t * self->impulse_rotation.i + k * self->impulse_direction.i;
            rotation.position.y = t * self->impulse_rotation.j + k * self->impulse_direction.j;
            rotation.position.z = t * self->impulse_rotation.k + k * self->impulse_direction.k;
            self->impulse_ticks = (int16_t)(self->impulse_ticks - halo::game::globals().game_time->ticks_this_frame);
            *out = rotation;
        }

        ticks = self->shake_ticks;
        if (ticks > 0 || (self->flags & _player_effect_camera_shake_bit) != 0) {
            real_matrix4x3 second = *k_render_identity_matrix_ptr;
            real t2;
            real w;
            real a;
            real b;

            if ((self->flags & _player_effect_camera_shake_bit) != 0) {
                t2 = 1.0f;
            } else {
                real duration = self->shake.duration;

                t2 = halo::math::transition_function_evaluate(self->shake.transition_function,
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * self->shake.intensity;
            }
            w = halo::math::periodic_function_evaluate(self->shake.wobble_function,
                (double)((self->shake.duration - (real)(int32_t)self->shake_ticks) / self->shake.wobble_period));
            w = (w * self->shake.wobble_weight + (1.0f - self->shake.wobble_weight)) * t2;
            a = w * self->shake.random_translation;
            if (!(a > 0.0f)) {
                a = 0.0f;
            }
            b = w * self->shake.random_rotation;
            if (!(b > 0.0f)) {
                b = 0.0f;
            }
            self->flags &= ~(uint32_t)_player_effect_camera_shake_bit;
            halo::effects::player_effect_random_shake_offset(&second, a + self->shake_translation, b + self->shake_rotation);
            dt = halo::game::globals().game_time->ticks_this_frame;
            self->vibrate_ticks = (int16_t)(self->vibrate_ticks + dt);
            if (self->vibrate_ticks > 0) {
                self->vibrate_ticks = 0;
                self->low_frequency_vibrate = 0.0f;
                self->high_frequency_vibrate = 0.0f;
                self->shake_translation = 0.0f;
                self->shake_rotation = 0.0f;
            }
            halo::effects::player_effect_random_shake_offset(&second, a, b);
            self->shake_ticks = (int16_t)(self->shake_ticks - dt);
            halo::math::globals().matrix4x3_multiply_procedure(out, &second, out);
        }
    }
}

}

namespace halo::effects {

void player_effect_build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index)
{
    halo::effects::player_effect_ref::build_camera_shake_matrix(out, local_player_index);
}

}

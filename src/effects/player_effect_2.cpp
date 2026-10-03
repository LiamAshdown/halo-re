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
    uint8_t *g = (uint8_t *)globals;
    int16_t dt;

    if (local_player_index == -1) {
        return;
    }
    if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 1) != 0) {
        real t = ((struct player_effect_globals *)g)->scripted_shake_intensity;
        int16_t ticks = ((struct player_effect_globals *)g)->scripted_shake_ticks;

        *out = *k_render_identity_matrix_ptr;
        if (ticks > 0) {
            real fraction = (real)(int32_t)ticks / (real)(int32_t)((struct player_effect_globals *)g)->scripted_shake_duration;

            if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 2) == 0) {
                fraction = 1.0f - fraction;
            }
            t = fraction * t;
            ((struct player_effect_globals *)g)->scripted_shake_ticks = (int16_t)(ticks - halo::game::globals().game_time->ticks_this_frame);
        } else if ((((struct player_effect_globals *)g)->scripted_shake_flags & 2) != 0) {
            ((struct player_effect_globals *)g)->scripted_shake_flags &= ~(uint32_t)1;
        }
        if ((*(uint8_t *)&((struct player_effect_globals *)g)->scripted_shake_flags & 1) == 0) {
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

            halo::math::matrix4x3_from_euler_angles(*out, a3 * *(real *)(g + 0x10c) * t, a2 * *(real *)(g + 0x110) * t,
                a1 * *(real *)(g + 0x114) * t);
        }
        {
            real b4 = shake_random_signed();
            real b5 = shake_random_signed();
            real b6 = shake_random_signed();

            out->position.x = b6 * *(real *)(g + 0x104) * t;
            out->position.y = b5 * *(real *)(g + 0x100) * t;
            out->position.z = b4 * *(real *)(g + 0x108) * t;
        }
        return;
    }

    {
        uint8_t *self = g + (int32_t)local_player_index * 0xec;
        int16_t ticks = *(int16_t *)(self + 0xe0);
        real t;

        if (ticks <= 0 && (self[0xe8] & 2) == 0) {
            *out = *k_render_identity_matrix_ptr;
        } else {
            real_vector3d axis;
            real angle;
            real k;
            real_matrix4x3 rotation;

            if ((self[0xe8] & 2) != 0) {
                t = 1.0f;
            } else {
                real duration = *(real *)(self + 0x50);

                t = halo::math::transition_function_evaluate(*(int16_t *)(self + 0x54),
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * *(real *)(self + 0x68);
            }
            self[0xe8] &= 0xfd;
            halo::math::vector3d_cross_product(axis, *(real_vector3d *)self, *halo::math::globals().global_up3d_pointer);
            angle = t * *(real *)(self + 0x58);
            halo::math::matrix4x3_from_axis_angle(rotation, axis, (real)halo::libm::sin((double)angle), (real)halo::libm::cos((double)angle));
            k = t * *(real *)(self + 0x5c);
            rotation.position.x = t * *(real *)(self + 0x0c) + k * *(real *)(self + 0x00);
            rotation.position.y = t * *(real *)(self + 0x10) + k * *(real *)(self + 0x04);
            rotation.position.z = t * *(real *)(self + 0x14) + k * *(real *)(self + 0x08);
            *(int16_t *)(self + 0xe0) = (int16_t)(*(int16_t *)(self + 0xe0) - halo::game::globals().game_time->ticks_this_frame);
            *out = rotation;
        }

        ticks = *(int16_t *)(self + 0xe2);
        if (ticks > 0 || (self[0xe8] & 4) != 0) {
            real_matrix4x3 second = *k_render_identity_matrix_ptr;
            real t2;
            real w;
            real a;
            real b;

            if ((self[0xe8] & 4) != 0) {
                t2 = 1.0f;
            } else {
                real duration = *(real *)(self + 0x84);

                t2 = halo::math::transition_function_evaluate((int16_t)*(uint16_t *)(self + 0x88),
                    1.0f - (duration - (real)(int32_t)ticks) / duration) * *(real *)(self + 0xac);
            }
            w = halo::math::periodic_function_evaluate(*(int16_t *)(self + 0xa0),
                (double)((*(real *)(self + 0x84) - (real)(int32_t)*(int16_t *)(self + 0xe2)) / *(real *)(self + 0xa4)));
            w = (w * *(real *)(self + 0xa8) + (1.0f - *(real *)(self + 0xa8))) * t2;
            a = w * *(real *)(self + 0x8c);
            if (!(a > 0.0f)) {
                a = 0.0f;
            }
            b = w * *(real *)(self + 0x90);
            if (!(b > 0.0f)) {
                b = 0.0f;
            }
            self[0xe8] &= 0xfb;
            halo::effects::player_effect_random_shake_offset(&second, a + *(real *)(self + 0xd4), b + *(real *)(self + 0xd8));
            dt = halo::game::globals().game_time->ticks_this_frame;
            *(int16_t *)(self + 0xdc) = (int16_t)(*(int16_t *)(self + 0xdc) + dt);
            if (*(int16_t *)(self + 0xdc) > 0) {
                *(int16_t *)(self + 0xdc) = 0;
                *(real *)(self + 0xcc) = 0.0f;
                *(real *)(self + 0xd0) = 0.0f;
                *(real *)(self + 0xd4) = 0.0f;
                *(real *)(self + 0xd8) = 0.0f;
            }
            halo::effects::player_effect_random_shake_offset(&second, a, b);
            *(int16_t *)(self + 0xe2) = (int16_t)(*(int16_t *)(self + 0xe2) - dt);
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

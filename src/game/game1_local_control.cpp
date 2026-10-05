/**
 * Local player control input digitisation and look vector helpers.
 */

#include "halo/math/constants.hpp"
#include "halo/game/constants.hpp"
#include "tags.h"
#include "halo/networking/game_mode.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "units.h"
#include "game.h"
#include "crt.h"
#include <string.h>
#include "interface.h"
#include "main.h"

#include "halo/game/game1_local_control.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"

static constexpr float k_degrees_to_radians = 0.017453292f;
static constexpr float k_seconds_per_tick = 0.033333335f;
static constexpr float k_look_epsilon = 9.999999747378752e-05f;

static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &local_player_input_states = halo::link::ref<local_player_input_state [k_maximum_local_players]>(halo::game::vars().local_player_input_states);
static auto &look_yaw_rate_setting = halo::link::ref<real [k_maximum_local_players]>(halo::game::vars().look_yaw_rate_setting);
static auto &look_pitch_rate_setting = halo::link::ref<real [k_maximum_local_players]>(halo::game::vars().look_pitch_rate_setting);
static auto &look_aim_assist_enabled = halo::link::ref<uint8_t>(halo::game::vars().look_aim_assist_enabled);
static auto &look_rate_doubler_zoom_inverts = halo::link::ref<uint8_t>(halo::game::vars().look_rate_doubler_zoom_inverts);
static auto &look_rate_doubler_enabled = halo::link::ref<uint8_t>(halo::game::vars().look_rate_doubler_enabled);
static auto &game_engine_input_source_flag = halo::link::ref<uint8_t>(halo::game::vars().game_engine_input_source_flag);
static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);

namespace halo::game::engine1 {

/**
 * File-local helper of LocalControl: control input absolute.
 */
real LocalControl::control_input_absolute(real value)
{
    return value < 0.0f ? -value : value;
}

/**
 * Game_engine_build_local_player_control_input (0x4710b0) zeroes all eight dwords and fills them, then tail-
 * calls game_engine_digitize_control_input (0x472760) with this record in EDX.
 *
 * Original register convention: EDI -> table.
 *
 * @address 0x4710b0
 */
void LocalControl::build_local_player_control_input(int16_t local_player_index, real delta_time, player_control_input *out)
{
    datum_index player_index = k_datum_index_none;
    local_player_control *control;
    player *plr;
    local_player_input_state *input;
    GlobalsPlayerControl *player_control;
    GlobalsPlayerInformation *player_information;
    real yaw_rate;
    real pitch_rate;
    real look_x;
    real look_y;
    real yaw_delta = 0.0f;
    real pitch_delta = 0.0f;
    real abs_look_x;
    int8_t buttons[0x13];
    int32_t i;

    if (local_player_index != -1 && local_player_index < k_maximum_local_players) {
        player_index = local_player_globals->local_players[local_player_index];
    }

    out->throttle_x = 0.0f;
    out->throttle_y = 0.0f;
    out->primary_trigger = 0.0f;
    out->yaw_delta = 0.0f;
    out->pitch_delta = 0.0f;
    out->action = 0;
    out->melee = 0;
    out->pad_16 = 0;
    out->control_flags = 0;
    out->button_flags = 0;

    if (player_index == k_datum_index_none) {
        halo::game::game_engine_digitize_control_input(out);
        return;
    }

    control = &player_control_globals_ptr->local_players[local_player_index];
    plr = halo::game::player_at(player_index);
    player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
    player_information = (GlobalsPlayerInformation *)global_globals->player_information.pointer;
    input = &local_player_input_states[plr->local_player_index];

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_local) {
        input->throttle_x = halo::game::control_axis_sign(input->throttle_x);
        input->throttle_y = halo::game::control_axis_sign(input->throttle_y);
    }

    yaw_rate = 0.0f;
    pitch_rate = 0.0f;
    if (plr->unit != k_datum_index_none) {
        object *unit_object = halo::game::object_at(plr->unit);
        unit_data *unit = halo::game::unit_data_of(unit_object);

        yaw_rate = look_yaw_rate_setting[local_player_index] *
                   k_degrees_to_radians * k_seconds_per_tick;
        pitch_rate = look_pitch_rate_setting[local_player_index] *
                     k_degrees_to_radians * k_seconds_per_tick;

        if (unit_object->parent_object != k_datum_index_none && unit->vehicle_seat_index != -1) {
            object *parent =
                halo::game::object_at(unit_object->parent_object);
            Unit *parent_definition =
                (Unit *)halo::game::tag_data_at(parent->definition_tag);
            UnitSeat *seat =
                &((UnitSeat *)parent_definition->seats.pointer)[unit->vehicle_seat_index];

            if (seat->yaw_rate > 0.0f) {
                yaw_rate = seat->yaw_rate * k_degrees_to_radians * k_seconds_per_tick;
            }
            if (seat->pitch_rate > 0.0f) {
                pitch_rate = seat->pitch_rate * k_degrees_to_radians * k_seconds_per_tick;
            }
        }
    }

    out->throttle_x = input->throttle_x;
    out->throttle_y = input->throttle_y;

    {
        real abs_y = control_input_absolute(input->look_y);
        real abs_x = control_input_absolute(input->look_x);
        real scale = 1.0f;

        if (abs_y > 0.1f && abs_x > 0.1f) {
            real a, b;

            if (abs_y <= abs_x) {
                a = abs_y / abs_x;
                b = 1.0f;
            } else {
                b = abs_x / abs_y;
                a = 1.0f;
            }
            scale = (real)halo::libm::sqrt((double)(a * a + b * b));
        }

        look_x = scale * input->look_x;
        if (look_x < -1.0f) { look_x = -1.0f; } else if (look_x > 1.0f) { look_x = 1.0f; }
        look_y = scale * input->look_y;
        if (look_y < -1.0f) { look_y = -1.0f; } else if (look_y > 1.0f) { look_y = 1.0f; }
    }

    if ((player_control_globals_ptr->flags & 1) == 0 && game_time->paused == 0) {
        if (input->look_is_analog == 0) {
            real scale = 1.0f;

            out->throttle_x = input->throttle_x;
            out->throttle_y = input->throttle_y;
            if (plr->unit != k_datum_index_none && control->desired_zoom_level != -1) {
                scale = 1.0f / halo::units::unit_get_active_weapon_scale(plr->unit, control->desired_zoom_level);
            }
            if (plr->unit != k_datum_index_none) {
                unit_data *unit = halo::game::unit_data_of(halo::game::object_at(plr->unit));

                scale = (1.0f - unit->stun * player_information->stun_turning_penalty) * scale;
            }
            out->yaw_delta = scale * input->look_x;
            out->pitch_delta = scale * input->look_y;
            if (out->yaw_delta < -4.5f) { out->yaw_delta = -4.5f; }
            else if (out->yaw_delta > 4.5f) { out->yaw_delta = 4.5f; }
            if (out->pitch_delta < -2.3f) { out->pitch_delta = -2.3f; }
            else if (out->pitch_delta > 2.3f) { out->pitch_delta = 2.3f; }

            control->nameplate_target = halo::game::camera_observer_get_target_id(
                (datum_index *)&control->nameplate_weight, local_player_index);
        } else {
            real look_yaw_pitch[2];
            real look_yaw_pitch_rate[2];
            real rate_multiplier;
            int32_t doubler;

            doubler = (input->buttons[0x0b] != 0 && look_rate_doubler_zoom_inverts != 0)
                          ? (look_rate_doubler_enabled == 0 ? 1 : 0)
                          : (int32_t)look_rate_doubler_enabled;
            rate_multiplier = (real)(doubler + 1);

            yaw_delta = rate_multiplier * yaw_rate *
                halo::game::response_curve_evaluate((int16_t)player_control->look_function.count, look_x,
                                        (real *)player_control->look_function.pointer);
            pitch_delta = rate_multiplier * pitch_rate *
                halo::game::response_curve_evaluate((int16_t)player_control->look_function.count, look_y,
                                        (real *)player_control->look_function.pointer);

            if (plr->unit != k_datum_index_none && control->desired_zoom_level != -1) {
                real inverse_scale =
                    1.0f / halo::units::unit_get_active_weapon_scale(plr->unit, control->desired_zoom_level);

                yaw_delta = yaw_delta * inverse_scale;
                pitch_delta = inverse_scale * pitch_delta;
            }
            if (plr->unit != k_datum_index_none) {
                unit_data *unit = halo::game::unit_data_of(halo::game::object_at(plr->unit));
                real stun_scale =
                    1.0f - unit->stun * player_information->stun_turning_penalty;

                yaw_delta = yaw_delta * stun_scale;
                pitch_delta = stun_scale * pitch_delta;
            }

            abs_look_x = control_input_absolute(look_x);
            if (abs_look_x < player_control->look_peg_threshold) {
                control->look_acceleration_timer = 0.0f;
            } else {
                real fraction =
                    control->look_acceleration_timer / player_control->look_acceleration_time;

                if (fraction < 0.0f) { fraction = 0.0f; } else if (fraction > 1.0f) { fraction = 1.0f; }
                yaw_delta = ((player_control->look_acceleration_scale - 1.0f) * fraction + 1.0f) *
                            yaw_delta;
                control->look_acceleration_timer = delta_time + control->look_acceleration_timer;
            }

            control->nameplate_target = halo::game::camera_observer_get_target_angles(
                &control->nameplate_weight, &control->aim_assist_weight,
                look_yaw_pitch, look_yaw_pitch_rate, local_player_index);

            if (look_aim_assist_enabled != 0 && control->aim_assist_weight > 0.0f &&
                (abs_look_x > k_look_epsilon ||
                 control_input_absolute(look_y) > k_look_epsilon ||
                 control_input_absolute(out->throttle_x) > k_look_epsilon ||
                 control_input_absolute(out->throttle_y) > k_look_epsilon)) {
                real time_scale = halo::game::game_engine_get_time_scale();
                real friction = player_control->magnetism_friction;
                real adhesion = player_control->magnetism_adhesion;
                real keep;
                real magnetism_yaw;
                real magnetism_pitch;

                if (friction < 0.0f) { friction = 0.0f; } else if (friction > 1.0f) { friction = 1.0f; }
                keep = 1.0f - friction * control->aim_assist_weight;
                if (adhesion < 0.0f) { adhesion = 0.0f; } else if (adhesion > 1.0f) { adhesion = 1.0f; }
                adhesion = adhesion * control->aim_assist_weight;

                if (halo::main::globals().game_globals->players_are_double_speed != 0) {
                    time_scale = time_scale * 0.5f;
                }
                magnetism_yaw = look_yaw_pitch_rate[0] * time_scale;
                magnetism_pitch = time_scale * look_yaw_pitch_rate[1];
                if (magnetism_yaw < -0.10471976f) { magnetism_yaw = -0.10471976f; }
                else if (magnetism_yaw > 0.10471976f) { magnetism_yaw = 0.10471976f; }
                if (magnetism_pitch < -0.05235988f) { magnetism_pitch = -0.05235988f; }
                else if (magnetism_pitch > 0.05235988f) { magnetism_pitch = 0.05235988f; }

                yaw_delta = keep * yaw_delta + magnetism_yaw * adhesion;
                pitch_delta = keep * pitch_delta + magnetism_pitch * adhesion;
            }

            out->yaw_delta = yaw_delta * delta_time * halo::game::k_ticks_per_second_f;
            out->pitch_delta = delta_time * halo::game::k_ticks_per_second_f * pitch_delta;
        }
    } else {
        out->yaw_delta = 0.0f;
        out->pitch_delta = 0.0f;
    }

    for (i = 0; i < 0x13; i += 1) {
        buttons[i] = 0;
    }
    {
        uint16_t both = (uint16_t)(control->suppressed_until_released & control->suppressed_buttons);

        if (both != 0) {
            for (i = 0; i < 0x13; i += 1) {
                if ((both & (uint32_t)(1 << i)) != 0 && input->buttons[i] == 0) {
                    uint16_t clear = (uint16_t)~(uint32_t)(1 << i);

                    control->suppressed_buttons = (uint16_t)(control->suppressed_buttons & clear);
                    control->suppressed_until_released =
                        (uint16_t)(control->suppressed_until_released & clear);
                }
            }
        }
    }
    for (i = 0; i < 0x13; i += 1) {
        if ((control->suppressed_buttons & (uint32_t)(1 << i)) == 0) {
            buttons[i] = input->buttons[i];
        }
    }

    if (buttons[0x0a] == 0) { out->control_flags &= ~0x1u; } else { out->control_flags |= 0x1u; }
    if (buttons[0x00] == 0) { out->control_flags &= ~0x2u; } else { out->control_flags |= 0x2u; }
    if (buttons[0x02] == 0) { out->control_flags &= ~0x40u; } else { out->control_flags |= 0x40u; }
    if (buttons[0x05] == 0) { out->control_flags &= ~0x10u; } else { out->control_flags |= 0x10u; }
    if (buttons[0x0d] == 0) { out->control_flags &= ~0x400u; } else { out->control_flags |= 0x400u; }
    if (buttons[0x07] == 0) { out->control_flags &= ~0x800u; } else { out->control_flags |= 0x800u; }

    if (buttons[0x06] == 0) { out->control_flags &= ~0x3000u; } else { out->control_flags |= 0x3000u; }

    if (buttons[0x0e] == 0 &&
        (int16_t)(uint16_t)(uint8_t)buttons[0x02] < player_control->minimum_weapon_swap_ticks) {
        out->control_flags &= ~0x4000u;
    } else {
        out->control_flags |= 0x4000u;
    }
    if (buttons[0x04] == 0) { out->control_flags &= ~0x80u; } else { out->control_flags |= 0x80u; }
    if (buttons[0x0b] == 0) { out->button_flags &= ~0x4u; } else { out->button_flags |= 0x4u; }
    if (buttons[0x03] == 0) { out->button_flags &= ~0x1u; } else { out->button_flags |= 0x1u; }
    if (buttons[0x01] == 0) { out->button_flags &= ~0x2u; } else { out->button_flags |= 0x2u; }

    out->primary_trigger = (out->control_flags & 0x800u) != 0 ? 1.0f : 0.0f;

    if ((control->suppressed_buttons & 0x200u) == 0) {
        out->melee = input->buttons[0x09];
    }
    if ((control->suppressed_buttons & 0x004u) == 0) {
        out->action = input->buttons[0x02];
    }

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client && (out->control_flags & 0x800u) != 0 &&
        plr->unit != k_datum_index_none) {
        unit_data *unit = halo::game::unit_data_of(halo::game::object_at(plr->unit));

        if (unit->current_weapon_index != -1) {
            object *weapon_object = ((object_header *)halo::objects::globals().object_data->data)
                [unit->weapons[unit->current_weapon_index] & halo::k_datum_slot_mask].data;
            weapon_data *weapon =
                (weapon_data *)((uint8_t *)weapon_object + k_item_extension_offset);

            if ((weapon->flags & 1) != 0) {
                out->control_flags &= ~0x800u;
            }
        }
    }

    {
        real magnitude = out->throttle_x * out->throttle_x + out->throttle_y * out->throttle_y;

        if (magnitude > 1.0f) {
            real inverse = 1.0f / (real)halo::libm::sqrt((double)magnitude);

            out->throttle_x = inverse * out->throttle_x;
            out->throttle_y = inverse * out->throttle_y;
        }
    }
    halo::game::game_engine_digitize_control_input(out);
}

/**
 * Thin wrapper around the look-vector computation player_compute_view_forward_vector for a given local-
 * player/unit context.
 *
 * Original register convention: EAX -> out_forward, CX -> local_player_index.
 *
 * @address 0x471f40
 */
void LocalControl::compute_local_player_look_vector(real_vector3d *out_forward, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index unit = k_datum_index_none;

    if (local_player_index != -1 && local_player_index < 1) {
        unit = local_player_globals->local_players[local_player_index];
    }
    halo::game::player_compute_view_forward_vector(unit, &look->yaw, out_forward);
}

/**
 * Converts a facing vector into the yaw and pitch of the given local player.
 *
 * Original register convention: EAX -> facing, CX -> local_player_index.
 *
 * @address 0x470d80
 */
void LocalControl::compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    real horizontal;

    look->yaw = (real)halo::libm::atan2((double)facing->j, (double)facing->i);
    horizontal = (real)halo::libm::sqrt((double)(facing->i * facing->i + facing->j * facing->j));
    look->pitch = (real)halo::libm::atan2((double)facing->k, (double)horizontal);
    if (look->yaw < 0.0f) {
        look->yaw = look->yaw + halo::math::k_two_pi;
    }
}

/**
 * Converts a raw unit-control input structure into digitized action-flag bits accumulated in the global local-
 * player control header.
 *
 * Original register convention: EDX -> input.
 *
 * @address 0x472760
 */
void LocalControl::digitize_control_input(player_control_input *input)
{
    uint32_t *flags = &player_control_globals_ptr->action_flags;
    uint32_t control_flags = input->control_flags;
    uint32_t button_flags = input->button_flags;

    if ((input->melee != 0 || local_player_input_states[0].buttons[halo::game::fields::k_input_action_accept] != 0) && halo::saved_games::globals().game_state_write_in_progress == 0 &&
        *(int8_t *)(cinematic_globals_ptr + 10) != 0) {
        split_screen_quit_prompt_string = halo::k_word_none;
        halo::networking::globals().join_error_reason = 0;
        halo::main::globals().main_globals.revert_map_if_allowed = 1;
    }

    if (control_flags & 0x40) { *flags |= _player_action_jump; }
    if (control_flags & 0x02) { *flags |= _player_action_flashlight; }
    if (input->action != 0) { *flags |= _player_action_action; }
    if (input->melee != 0) { *flags |= _player_action_melee; }
    if (input->primary_trigger > 0.0f) { *flags |= _player_action_primary_trigger; }
    if (control_flags & 0x2000) { *flags |= _player_action_reload; }
    if (button_flags & 0x04) { *flags |= _player_action_exchange_weapon; }

    if (input->pitch_delta > 0.0f) { *flags |= _player_action_look_up; }
    else if (input->pitch_delta < 0.0f) { *flags |= _player_action_look_down; }

    if (input->yaw_delta > 0.0f) { *flags |= _player_action_look_left; }
    else if (input->yaw_delta < 0.0f) { *flags |= _player_action_look_right; }

    if (input->throttle_x > 0.0f) { *flags |= _player_action_forward; }
    else if (input->throttle_x < 0.0f) { *flags |= _player_action_backward; }

    if (input->throttle_y > 0.0f) { *flags |= _player_action_left; }
    else if (input->throttle_y < 0.0f) { *flags |= _player_action_right; }

    {
        uint32_t *latched = &player_control_globals_ptr->action_flags_latched;
        uint32_t *edge = &player_control_globals_ptr->action_flags_edge;

        if ((*latched & 1) == 0) {
            uint32_t v = *edge;

            if (v & 1) {
                if ((input->control_flags & 0x40) == 0) {
                    v = v & ~1u;
                } else {
                    v = v | 1;
                }
                *edge = v;
                input->control_flags &= ~0x40u;
            }
        } else {
            input->control_flags &= ~0x40u;
        }

        if (game_engine_input_source_flag != 0) {
            int32_t clear_button = 1;

            if ((*latched & 4) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    clear_button = 0;
                } else {
                    if ((input->control_flags & 2) == 0) {
                        v = v & ~4u;
                    } else {
                        v = v | 4;
                    }
                    *edge = v;
                }
            }
            if (clear_button) {
                input->control_flags &= ~2u;
            }

            if ((*latched & 8) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    return;
                }
                if (input->button_flags & 2) {
                    *edge = v | 4;
                    input->button_flags &= ~2u;
                    return;
                }
                *edge = v & ~4u;
            }
            input->button_flags &= ~2u;
            return;
        }

        {
            int32_t clear_button = 1;

            if ((*latched & 4) == 0) {
                uint32_t v = *edge;

                if ((v & 4) == 0) {
                    clear_button = 0;
                } else {
                    if ((input->control_flags & 0x40) == 0) {
                        v = v & ~4u;
                    } else {
                        v = v | 4;
                    }
                    *edge = v;
                }
            }
            if (clear_button) {
                input->control_flags &= ~0x40u;
            }
        }

        if ((*latched & 8) == 0) {
            uint32_t v = *edge;

            if ((v & 8) == 0) {
                return;
            }
            if (input->button_flags & 1) {
                *edge = v | 8;
                input->button_flags &= ~1u;
                return;
            }
            *edge = v & ~8u;
        }
        input->button_flags &= ~1u;
    }
}

/**
 * Returns the maximum look-pitch angle for a unit, using a zoom-weapon-specific value when the unit is holding
 * a scoped weapon, otherwise a fixed default.
 *
 * Original register convention: AX -> local_player_index.
 *
 * @address 0x471f90
 */
real LocalControl::get_max_look_pitch(int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    real result = 1.2217305f;

    if (look->unit != k_datum_index_none) {
        void *base = *(void **)((uint8_t *)halo::objects::globals().object_data->data +
            (uint32_t)(uint16_t)look->unit * halo::objects::globals().object_data->size + 8);
        object *o = (object *)base;
        unit_data *u = halo::game::unit_data_of(base);
        uint8_t *tag_data = (uint8_t *)halo::cache::globals().tag_instances[(uint16_t)o->definition_tag].data;

        if (u->current_weapon_index != -1 && u->weapons[u->current_weapon_index] != k_datum_index_none) {
            result = halo::items::weapon_clamp_zoom_fov(u->weapons[u->current_weapon_index], look->desired_zoom_level,
                                           *(real *)(tag_data + 0x1a0));
        } else {
            result = *(real *)(tag_data + 0x1a0);
        }
    }
    return result;
}

/**
 * Initializes the look state of a local player from the facing of a unit.
 *
 * Original register convention: EDX -> unit, AX -> local_player_index.
 *
 * @address 0x470e80
 */
void LocalControl::init_player_look_state_from_object(datum_index unit, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];

    memset(look, 0, sizeof(*look));
    look->unit = unit;
    look->desired_weapon_index = -1;
    look->desired_grenade_index = -1;
    look->desired_zoom_level = -1;
    look->autolevelling_active = 0;
    look->nameplate_target = k_datum_index_none;
    look->pitch_maximum = 1.4906585f;
    look->pitch_minimum = -1.4906585f;
    look->suppressed_buttons = 0;
    look->suppressed_until_released = 0;

    if (unit != k_datum_index_none) {
        unit_data *u = *(unit_data **)((uint8_t *)halo::objects::globals().object_data->data +
            (uint32_t)(uint16_t)unit * halo::objects::globals().object_data->size + 8);
        real horizontal;

        look->yaw = (real)halo::libm::atan2((double)u->desired_facing_vector.j, (double)u->desired_facing_vector.i);
        horizontal = (real)halo::libm::sqrt((double)(u->desired_facing_vector.i * u->desired_facing_vector.i +
            u->desired_facing_vector.j * u->desired_facing_vector.j));
        look->pitch = (real)halo::libm::atan2((double)u->desired_facing_vector.k, (double)horizontal);
        if (look->yaw < 0.0f) {
            look->yaw = look->yaw + halo::math::k_two_pi;
        }
        look->desired_weapon_index = u->desired_weapon_index;
        look->desired_grenade_index = (int16_t)u->desired_grenade_index;
        look->desired_zoom_level = (int16_t)u->desired_zoom_level;

        if (!halo::libm::is_nan((double)look->pitch) && look->pitch <= 1.4922565f && -1.4922565f <= look->pitch) {
            halo::libm::is_nan((double)look->yaw);
        }
    }
}

}

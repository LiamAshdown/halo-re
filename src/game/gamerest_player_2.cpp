#include "halo/game/gamerest_player.hpp"
#include "halo/scenario/api.hpp"
#include "interface.h"
#include "main.h"
#include <string.h>
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &network_message_scratch = halo::link::ref<uint8_t>(halo::game::vars().network_message_scratch);
extern "C" {
extern ScenarioPlayerStartingLocation *game_get_player_starting_location(int16_t index);
extern real *game_engine_get_player_color(uint32_t player_index, real *out_rgb);
extern void object_placement_data_set_change_colors(real *color, object_placement_data *placement);
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index);
extern void game_engine_apply_player_grenade_counts(uint32_t player_index);
extern void game_engine_send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle, int32_t value, int32_t machine_index);
extern double cos(double x);
extern double sin(double x);
}
static auto &reference_axis_006696728 = halo::link::ref<real_vector3d *>(halo::game::vars().reference_axis_006696728);
static auto &global_down3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_down3d_pointer);
extern "C" {
extern double fcos(double radians);
extern double fsin(double radians);
}
static auto &k_random_scale_65536 = halo::link::ref<float>(halo::ai::vars().k_random_scale_65536);
static auto &sqrt_pow_exponent = halo::link::ref<double>(halo::game::vars().sqrt_pow_exponent);
extern "C" {
extern double pow(double base, double exponent);
}
static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &local_player_count = halo::link::ref<int16_t>(halo::game::vars().local_player_count);
static auto &player_control_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().player_control_globals_ptr);
static auto &hud_weapon_state = halo::link::ref<uint8_t *>(halo::ui::vars().hud_weapon_state);
static auto &hud_unit_meters = halo::link::ref<uint8_t *>(halo::ui::vars().hud_unit_meters);
static auto &update_server_queues = halo::link::ref<data_array *>(halo::game::vars().update_server_queues);
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &server_maximum_queued_client_updates = halo::link::ref<int32_t>(halo::game::vars().server_maximum_queued_client_updates);
static auto &server_maximum_pending_client_update_ticks = halo::link::ref<int32_t>(halo::game::vars().server_maximum_pending_client_update_ticks);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);
static auto &global_007102d8 = halo::link::ref<uint8_t>(halo::game::vars().global_007102d8);
static auto &main_game_globals = halo::link::ref<uint8_t *>(halo::game::vars().main_game_globals);
extern "C" {
extern void console_print_va(const char *format, ...);
}

namespace {
static void player_respawn_drop_lights(datum_index object_index)
{
    uint8_t *header = (uint8_t *)halo::objects::globals().object_data->data + (object_index & 0xffff) * 0xc;
    uint8_t *obj = *(uint8_t **)(header + 8);
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;

    if (*(int32_t *)&((Object *)tag)->model.tag_id == -1) {
        return;
    }
    if (*(uint8_t *)&((object *)obj)->flags & 1) {
        halo::objects::object_for_each_light_attachment(object_index, 0, 1);
    }
    if (*(int32_t *)&((Object *)tag)->model.tag_id != -1) {
        ((object *)obj)->flags &= ~1u;
        header = (uint8_t *)halo::objects::globals().object_data->data + (object_index & 0xffff) * 0xc;
        header[2] |= 2;
    }
}
static object *object_from_index(datum_index object_index)
{
    return ((object_header *)halo::objects::globals().object_data->data)[object_index & 0xffff].data;
}
static int32_t player_update_queue_count(const circular_queue *queue)
{
    if (queue->read_index < queue->write_index) {
        return queue->write_index - queue->read_index;
    }
    if (queue->write_index < queue->read_index) {
        return queue->write_index - queue->read_index + queue->capacity;
    }
    return 0;
}
}

namespace halo::game {

/**
 * Drops a pending-delete object's light attachments, as both halves of the revert branch do.
 *
 * @address 0x477ea0
 */
void PlayerView::respawn()
{
    uint8_t *p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;

    if (current_game_engine == 0 && ((player *)p)->local_player_index != -1) {
        datum_index *slot = (datum_index *)&local_player_globals->local_player_units[((player *)p)->local_player_index];
        datum_index existing_unit = *slot;

        *slot = k_datum_index_none;
        if (existing_unit != k_datum_index_none) {
            uint8_t *unit = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (existing_unit & 0xffff) * 0xc + 8);

            if ((unit[0x106] & 4) == 0) {
                datum_index held_weapon = k_datum_index_none;
                int16_t weapon_index = ((unit_object *)unit)->unit.current_weapon_index;

                if (weapon_index != -1) {
                    held_weapon = *(datum_index *)(unit + 0x2f8 + weapon_index * 4);
                }
                halo::objects::object_mark_pending_delete(existing_unit);
                player_respawn_drop_lights(existing_unit);
                LocalPlayers::set_controlled_unit(existing_unit, ((player *)p)->local_player_index);
                if (held_weapon != k_datum_index_none) {
                    player_respawn_drop_lights(held_weapon);
                }
                goto reset_player_state;
            }
            halo::objects::object_delete(existing_unit);
        }
    }

    if (halo::networking::globals().game_mode == 2 || halo::networking::globals().game_mode == 0) {
        int16_t location_index = PlayerView(player_index).pick_random_starting_location();
        datum_index unit_tag;
        ScenarioPlayerStartingLocation *location;
        object_placement_data placement;
        real color_buffer[3];
        real color[3];
        real *player_color;
        real facing;
        datum_index new_unit;
        uint8_t *unit;

        if (location_index == -1) {
            goto reset_player_state;
        }
        unit_tag = *(datum_index *)((uint8_t *)global_globals->player_information.pointer + 0xc);
        if (unit_tag == k_datum_index_none) {
            goto reset_player_state;
        }
        location = halo::game::game_get_player_starting_location(location_index);
        if (current_game_engine != 0) {
            unit_tag = *(datum_index *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x1c);
        }
        halo::objects::object_placement_data_initialize(&placement, unit_tag, k_datum_index_none);
        placement.position = *(real_point3d *)location;
        facing = ((struct ScenarioPlayerStartingLocation *)location)->facing;
        placement.forward.i = (real)cos(facing);
        placement.forward.j = (real)sin(facing);
        placement.forward.k = 0.0f;
        placement.up = *halo::math::globals().global_up3d_pointer;
        player_color = halo::game::game_engine_get_player_color(player_index, color_buffer);
        color[0] = player_color[0];
        color[1] = player_color[1];
        color[2] = player_color[2];
        halo::game::object_placement_data_set_change_colors(color, &placement);

        new_unit = halo::objects::object_new_with_datum_role_control(&placement, 3);
        if (new_unit == k_datum_index_none) {
            goto reset_player_state;
        }
        unit = (uint8_t *)halo::objects::object_try_and_get(new_unit, 3);
        if (unit == 0) {
            goto reset_player_state;
        }
        p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
        ((unit_object *)unit)->base.owner_linkage = player_index;
        ((unit_object *)unit)->base.owner_team = *(int16_t *)&((player *)p)->team;
        *(uint32_t *)&((unit_object *)unit)->unit.controlling_player = player_index;
        ((player *)p)->unit = new_unit;
        halo::units::unit_refresh_targeting_flag_and_weapons(new_unit, 1);
        if (((player *)p)->local_player_index != -1) {
            halo::game::game_engine_init_player_look_state_from_object(new_unit, ((player *)p)->local_player_index);
        }
        if (current_game_engine == 0) {
            int32_t profile_count = *(int32_t *)&halo::scenario::globals().scenario->player_starting_profile.count;

            if (profile_count > 1 && ((player *)p)->deaths > 0) {
                LocalPlayerUnit(((player *)p)->unit).apply_starting_profile(1, 1);
            } else if (profile_count != 0) {
                LocalPlayerUnit(((player *)p)->unit).apply_starting_profile(0, 1);
            }
        }
        if (halo::networking::globals().game_mode == 2) {
            int32_t team = ((player *)p)->team;
            int32_t encoded_bits;

            halo::game::game_engine_apply_player_grenade_counts(player_index);
            *(uint32_t *)&((unit_object *)unit)->base.network_role = 0;
            halo::objects::object_type_override_call_0x68(new_unit);
            encoded_bits = halo::units::unit_build_network_update(new_unit, (int32_t)&network_message_scratch, 0x7ff8);
            if (encoded_bits > 0) {
                halo::networking::network_session_broadcast_to_flagged(encoded_bits, (network_server_globals *)halo::networking::globals().server,
                    1, &network_message_scratch, 1, 0, 0, 3);
            }
            *(uint32_t *)(p + 0x68) = 0;
            halo::game::game_engine_send_unit_weapon_loadout(new_unit, player_index, team, -1);
        }
    }

reset_player_state:
    p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    *(uint32_t *)(p + 0x68) = 0;
    *(uint16_t *)&((player *)p)->interaction_type = 0;
    ((player *)p)->interaction_object = k_datum_index_none;
    if (((player *)p)->local_player_index != -1) {
        halo::camera::observer_new(&halo::camera::globals().observers[((player *)p)->local_player_index]);
    }
}

/**
 * Computes a forward-facing unit vector from (yaw, pitch) into *out_forward. Then, if
 * player_handle's unit is attached to a parent object (e.g. seated in a vehicle) and that
 * parent's seat definition does not have the third_person_camera flag set, re-derives an
 * orthonormal basis from the parent's up vector and out_forward (falling back to a second
 * reference axis if the first choice of cross-product axis turns out to be degenerate), and
 * transforms out_forward in place through that basis -- i.e. reorients the raw view vector into
 * the vehicle's own coordinate frame for a first-person seat.
 *
 * @address 0x473d70
 */
void PlayerView::compute_view_forward_vector(real *yaw_pitch, real_vector3d *out_forward)
{
    datum_index player_handle = player_index;
    player *plr;
    object *unit_obj;
    object *parent_obj;
    Unit *parent_definition;
    UnitSeat *seat;
    unit_data *unit;
    real_vector3d cross_result;
    real_matrix4x3 basis;
    real length;

    out_forward->i = (float)(fcos(yaw_pitch[1]) * fcos(yaw_pitch[0]));
    out_forward->j = (float)(fcos(yaw_pitch[1]) * fsin(yaw_pitch[0]));
    out_forward->k = (float)fsin(yaw_pitch[1]);

    plr = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    if (plr->unit == (datum_index)-1) {
        return;
    }

    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[plr->unit & 0xffff].data;
    if (unit_obj->parent_object == (datum_index)-1) {
        return;
    }

    parent_obj = halo::objects::object_try_and_get(unit_obj->parent_object, _object_mask_vehicle);
    if (parent_obj == (object *)0) {
        return;
    }

    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    parent_definition = (Unit *)halo::cache::globals().tag_instances[parent_obj->definition_tag & 0xffff].data;
    seat = &((UnitSeat *)parent_definition->seats.pointer)[unit->vehicle_seat_index];
    if ((seat->flags & 0x10) != 0) {
        return;
    }

    halo::math::vector3d_cross_product(cross_result, *reference_axis_006696728, parent_obj->up);
    length = halo::math::vector3d_normalize_with_length(cross_result);
    if (length == 0.0f) {
        halo::math::vector3d_cross_product(cross_result, *global_down3d_pointer, parent_obj->up);
        halo::math::vector3d_normalize_with_length(cross_result);
    }
    halo::math::matrix4x3_from_forward_up(parent_obj->up, cross_result, basis);
    halo::math::matrix4x3_transform_normal(*out_forward, *out_forward, basis);
}

/**
 * blam-cc: stack -> player_index
 * Walks Scenario::player_starting_locations, scoring every entry whose game-type list matches
 * the active game engine (or, outside multiplayer, whose list is entirely wildcard/zero) and
 * that has no vehicle parked on it, by sqrt(random 0..1) * suitability, and returns the index of
 * the highest-scoring entry, or -1 if the scenario has no starting locations at all (or if every
 * candidate scored 0, since best_index starts at -1 and the comparison is strict).
 * NOTE: the original guards the per-iteration location pointer with "if (i >= 0 && i < count)"
 * and passes NULL when that fails, which cannot happen for i in [0, count); the do/while below
 * drops the dead guard and walks the array directly.
 *
 * @address 0x4776d0
 */
int16_t PlayerView::pick_random_starting_location()
{
    datum_index player_handle = player_index;
    int16_t count;
    int16_t index;
    int16_t best_index;
    float best_score;
    ScenarioPlayerStartingLocation *location;
    int32_t current_engine_index;

    count = (int16_t)halo::scenario::globals().scenario->player_starting_locations.count;
    index = 0;
    best_index = -1;
    best_score = 0.0f;

    if (0 < count) {
        location = (ScenarioPlayerStartingLocation *)halo::scenario::globals().scenario->player_starting_locations.pointer;
        do {
            float suitability;
            uint8_t matches;

            current_engine_index = (current_game_engine != 0) ? current_game_engine->index : -1;
            matches = halo::game::netgame_equipment_game_type_matches(&location->type_0, 4, current_engine_index);
            if (!matches) {
                suitability = 0.0f;
            } else if (halo::game::game_engine_location_blocked_by_vehicle(
                           (real_point3d *)&location->position) != 0) {
                suitability = 0.0f;
            } else {
                suitability = halo::game::game_engine_rate_player_starting_location(location, player_handle);
            }

            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            {
                float unit_random = (float)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536;
                float weight = (float)pow((double)unit_random, sqrt_pow_exponent) * suitability;
                if (best_score < weight) {
                    best_score = weight;
                    best_index = index;
                }
            }

            index = index + 1;
            location = location + 1;
        } while (index < count);
        return best_index;
    }
    return -1;
}

/**
 * blam-cc: EAX -> player_handle, stack -> slot
 * For slot 0 only, sets unit_flags bit 0x10 on the player's unit and resets unit::unknown_422
 * to 0.
 *
 * @address 0x479d90
 */
void KillStreak::begin(int16_t slot)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    unit_data *unit = (unit_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);

    if (slot == 0) {
        unit->flags = unit->flags | _unit_flag_unknown_10;
        unit->active_camouflage_regrowth = 0;
    }
}

/**
 * blam-cc: EAX -> player_handle, stack -> slot
 * For slot 0 only, sets unit_flags bit 0x20 on the player's unit.
 *
 * @address 0x479de0
 */
void KillStreak::continue_streak(int16_t slot)
{
    if (slot == 0) {
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        unit_data *unit = (unit_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);
        unit->flags = unit->flags | 0x20;
    }
}

/**
 * Implements the original `players_rebind_local_player_after_load`.
 *
 * @address 0x4765f0
 */
void Players::rebind_local_player_after_load()
{
    int16_t local = profile_slot_id[0];
    int16_t slot;
    datum_index handle = k_datum_index_none;
    player *entry;
    uint8_t *control;

    if (local == -1) {
        local = 0;
    }
    if (local < 1 && local_player_globals->local_players[local] != k_datum_index_none) {
        return;
    }
    if (local_player_count != 1) {
        return;
    }
    for (slot = 0; slot < 4; slot++) {
        if (slot < 1) {
            handle = local_player_globals->local_players[slot];
            if (handle != k_datum_index_none) {
                break;
            }
        }
    }
    if (slot >= 4) {
        return;
    }

    entry = (player *)((uint8_t *)player_data->data + (handle & 0xffff) * 0x200);
    {
        datum_index old = local_player_globals->local_players[slot];

        if (old != k_datum_index_none) {
            ((player *)((uint8_t *)player_data->data + (old & 0xffff) * 0x200))->local_player_index = -1;
        }
        local_player_globals->local_players[slot] = k_datum_index_none;
    }

    control = player_control_globals_ptr + 0x10 + slot * 0x40;
    memset(control, 0, 0x40);
    *(int32_t *)(control + 0x00) = -1;
    *(int16_t *)(control + 0x20) = -1;
    *(int16_t *)(control + 0x22) = -1;
    *(int16_t *)(control + 0x24) = -1;
    *(int32_t *)(control + 0x28) = -1;
    control[0x26] = 0;
    *(uint32_t *)(control + 0x3c) = 0x3fbf0243;
    *(uint32_t *)(control + 0x38) = 0xbfbf0243;
    *(int16_t *)(control + 0x08) = 0;
    *(int16_t *)(control + 0x0a) = 0;

    halo::game::game_set_local_player(handle, local);
    halo::game::game_engine_init_player_look_state_from_object(entry->unit, local);

    memmove(hud_weapon_state + local * 0x28, hud_weapon_state + slot * 0x28, 0x28);
    memmove(hud_weapon_state + 0x28 + local * 0x50, hud_weapon_state + 0x28 + slot * 0x50, 0x50);
    memmove(hud_unit_meters + local * 0x58, hud_unit_meters + slot * 0x58, 0x58);
}

/**
 * Replays remote players' queued control updates on the host until each player's backlog is
 * within the allowed number of records and ticks.
 *
 * @address 0x4768c0
 */
void Players::server_catchup_on_client_updates()
{
    data_iterator player_iter;
    player *plr;

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;

    for (plr = (player *)halo::memory::data_iterator_next(&player_iter); plr != 0;
         plr = (player *)halo::memory::data_iterator_next(&player_iter)) {
        player_update_queue *queue;

        if (plr->local_player_index != -1) {
            continue;
        }
        queue = &((update_server_queue *)update_server_queues->data)[player_iter.index & 0xffff].queue;

        for (;;) {
            player_update_record record;
            player_action action;
            int32_t source_id;
            object *unit_obj;
            unit_data *unit;
            unit_control_data control;

            if (player_update_queue_count(&queue->queue) <= server_maximum_queued_client_updates) {
                int32_t pending_ticks = 0;
                int32_t i = queue->queue.read_index;
                int32_t write_index = queue->queue.write_index;

                while (i != write_index) {
                    pending_ticks += ((player_update_record *)queue->queue.records[i])->references_remaining;
                    i = (i + 1) % k_player_update_history_count;
                }
                if (pending_ticks <= server_maximum_pending_client_update_ticks) {
                    break;
                }
            }

            halo::game::player_update_queue_pop_current(&record, queue);
            action = record.action;
            source_id = record.references_remaining != 0 ? -1 : (int32_t)record.field0;

            if (plr->unit == (datum_index)-1) {
                continue;
            }
            unit_obj = object_from_index(plr->unit);
            unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            if ((unit->flags & 0x40) == 0) {
                continue;
            }

            if (local_player_globals->input_disabled == 0) {
                if (unit->current_weapon_index != -1) {
                    datum_index weapon_index = unit->weapons[unit->current_weapon_index];
                    if (weapon_index != (datum_index)-1) {
                        Weapon *weapon = (Weapon *)halo::cache::globals().tag_instances[object_from_index(weapon_index)->definition_tag & 0xffff].data;
                        if ((weapon->weapon_flags & 0x8) != 0) {
                            action.weapon_index = unit->current_weapon_index;
                        }
                    }
                }
                control.control_flags = (uint16_t)action.control_flags;
                PlayerView(player_iter.index).compute_view_forward_vector(&action.desired_yaw, &control.aiming_vector);
                control.facing_vector = control.aiming_vector;
                control.looking_vector = control.aiming_vector;
                control.throttle.i = action.throttle_x;
                control.throttle.j = action.throttle_y;
                control.throttle.k = 0.0f;
                control.primary_trigger = action.primary_trigger;
                control.weapon_index = action.weapon_index;
                control.grenade_index = action.grenade_index;
                control.zoom_level = action.zoom_level;
                control.animation_state = 3;
                control.aiming_speed = 0;
                halo::units::unit_apply_control_block(plr->unit, &control, source_id);
            } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                control.weapon_index = -1;
                control.grenade_index = -1;
                control.zoom_level = -1;
                control.throttle = *(const real_vector3d *)global_origin3d_pointer;
                control.facing_vector = unit->desired_facing_vector;
                control.aiming_vector = unit->desired_aiming_vector;
                control.looking_vector = unit->desired_looking_vector;
                control.animation_state = 3;
                control.aiming_speed = 0;
                control.control_flags = 0;
                control.primary_trigger = 0.0f;
                halo::units::unit_apply_control_block(plr->unit, &control, source_id);
            }

            if (PlayerView(player_iter.index).unit_has_parent() && halo::networking::globals().client_vehicle_ack_enabled != 0) {
                halo::objects::object_update(unit_obj->parent_object);
            } else {
                halo::units::unit_update(plr->unit);
                halo::units::biped_update(plr->unit);
            }
        }
    }
}

/**
 * Per-tick BSP-switch-trigger handling. First decrements player_globals::respawn_stagger and,
 * once it reaches 0, clears a render-state flag. Then for every player: advances its screen-fade
 * state (a countdown that either ticks down or, once triggered, drives player_effect_apply_generic_damage_feedback with the
 * fraction remaining), and once the fade completes and the player's unit does not have object
 * flags bit 0x20 set, announces game-over-style kill feed lines to every player while hosting
 * and sets that flag. Advances the player's kill-streak timers if it has a unit. Then, if the
 * player's unit's root ancestor's object flags do not have bit 0x200000 set, scans every
 * bsp_switch_trigger_volume whose source matches the currently active structure BSP: if the
 * player is standing in its named trigger volume, validates the destination BSP index (logging
 * an error for an out-of-range or same-as-current one) and otherwise stages it in
 * split_screen_quit_prompt_string and calls hud_display_loading_message to perform the switch. Finally resets the
 * player's pending interaction and dispatches the appropriate nearby-interaction scan
 * (player_update_nearby_interactions_secondary while this machine is a network client, player_update_nearby_interactions_primary otherwise).
 *
 * @address 0x4749a0
 */
void StructureBsp::switch_structure_bsp()
{
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    datum_index walk, root;
    object *root_obj;

    if (local_player_globals->respawn_stagger > 0) {
        local_player_globals->respawn_stagger = local_player_globals->respawn_stagger - 1;
        if (local_player_globals->respawn_stagger == 0) {
            main_game_globals[2] = 0;
        }
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        player_handle = player_iter.index;

        {

            int32_t *fade_ticks = (int32_t *)((uint8_t *)plr + 0xcc);

            if (plr->telefrag_danger == 0) {
                if (*fade_ticks > 0) {
                    *fade_ticks = *fade_ticks - 1;
                }
            } else if (*fade_ticks < 0x5a) {
                halo::effects::player_effect_apply_generic_damage_feedback(player_handle, (real)*fade_ticks * 0.011111111f);
            } else if (plr->unit != (datum_index)-1) {
                object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[plr->unit & 0xffff].data;
                if ((*((uint8_t *)unit_obj + 0x106) & 0x20) == 0) {
                    if (halo::networking::globals().game_mode == 2) {

                        halo::game::chimera__kill_feed(player_handle, 0x1f, (uint32_t)0xffffffff, 1, 0);
                    }
                    *((uint8_t *)unit_obj + 0x106) = *((uint8_t *)unit_obj + 0x106) | 0x20;
                }
            }
        }
        plr->telefrag_danger = 0;

        if (plr->unit != (datum_index)-1) {
            halo::game::player_kill_streak_tick(player_handle);
        }

        if (plr->unit != (datum_index)-1) {
            walk = plr->unit;
            do {
                root = walk;
                walk = ((object_header *)halo::objects::globals().object_data->data)[root & 0xffff].data->parent_object;
            } while (walk != (datum_index)-1);
            root_obj = ((object_header *)halo::objects::globals().object_data->data)[root & 0xffff].data;

            if ((root_obj->flags & 0x200000) == 0 && halo::scenario::globals().scenario->bsp_switch_trigger_volumes.count > 0) {
                ScenarioBSPSwitchTriggerVolume *volumes =
                    (ScenarioBSPSwitchTriggerVolume *)halo::scenario::globals().scenario->bsp_switch_trigger_volumes.pointer;
                int32_t count = halo::scenario::globals().scenario->bsp_switch_trigger_volumes.count;
                int32_t i;

                for (i = 0; i < count; i = i + 1) {
                    ScenarioBSPSwitchTriggerVolume *entry = &volumes[i];
                    if (entry->source == (uint16_t)halo::scenario::globals().structure_bsp_index && plr->unit != (datum_index)-1 &&
                        halo::scenario::scenario_query::trigger_volume_contains_point((int16_t)entry->trigger_volume, (real_point3d *)(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (plr->unit & 0xffff) * 0xc + 8) + 0xa0)) != 0) {

                        int16_t destination = (int16_t)entry->destination;

                        {
                            uint8_t *stage_byte = (uint8_t *)local_player_globals + 0x17;
                            uint8_t low = (uint8_t)(*stage_byte & 0xf);
                            *stage_byte = low;
                            *stage_byte = (uint8_t)((((uint8_t)plr->local_player_index ^ low) & 0xf) ^ low);
                        }
                        local_player_globals->bsp_switch_trigger_volume_index = (int16_t)i;
                        if (destination < 0 || destination >= halo::scenario::globals().scenario->structure_bsps.count) {
                            halo::main::console_print_va("tried to switch to invalid structure-bsp %d", (int32_t)destination);
                        } else if (destination == halo::scenario::globals().structure_bsp_index) {
                            halo::main::console_print_va("tried to switch to current structure-bsp %d", (int32_t)destination);
                        } else {

                            split_screen_quit_prompt_string = (uint16_t)destination;
                            halo::interface::hud_display_loading_message(1);
                        }
                    }
                }
            }
        }

        plr->interaction_object = (datum_index)-1;
        plr->interaction_type = 0;

        if (halo::networking::globals().game_mode == 1) {
            halo::game::player_update_nearby_interactions_secondary(player_handle);
        } else {
            halo::game::player_update_nearby_interactions_primary(player_handle);
        }

        plr = (player *)halo::memory::data_iterator_next(&player_iter);
    }

    {
        uint8_t *stage = (uint8_t *)local_player_globals + 0x17;
        if ((*stage & 0xf) != 0xf) {
            *stage = (uint8_t)(((*stage & 0xf0) + 0x10) ^ (*stage & 0xf));
            if ((*stage & 0xf0) > 0xc0) {
                *stage = 0xf;
            }
        }
    }

    if (local_player_globals->no_player_has_a_unit == 0) {
        if (global_007102d8 != 0) {
            global_007102d8 = 0;
        }
    } else if (current_game_engine == 0 && global_007102d8 == 0) {
        halo::networking::globals().join_error_reason = 0;
        halo::main::globals().main_globals.lost_map = 1;
        global_007102d8 = 1;
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::PlayerView::respawn; forwards to the C++ implementation.
 * blam-cc: EAX -> player_index
 * blam-cc: EAX
 * blam-cc: EAX
 * blam-cc: EAX, stack
 * blam-cc: EAX, stack
 * blam-cc: EDX
 *
 * @address 0x477ea0
 */
void player_respawn(uint32_t player_index)
{
    halo::game::PlayerView(player_index).respawn();
}

/**
 * C entry point for halo::game::PlayerView::compute_view_forward_vector; forwards to the C++ implementation.
 * register convention: EAX -> player_handle, ECX -> yaw_pitch (2 floats: yaw then pitch),
 * ESI -> out_forward.
 * // blam-cc: EAX -> player_handle, ECX -> yaw_pitch, ESI -> out_forward
 * blam-cc: EAX -> player_handle, ECX -> yaw_pitch, ESI -> out_forward
 *
 * @address 0x473d70
 */
void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward)
{
    halo::game::PlayerView(player_handle).compute_view_forward_vector(yaw_pitch, out_forward);
}

/**
 * C entry point for halo::game::PlayerView::pick_random_starting_location; forwards to the C++ implementation.
 * register convention: none (this function's argument, the respawning player's index, is its
 * own single stack parameter; nothing arrives in a register).
 * // blam-cc: stack -> player_index
 * blam-cc: EDX -> point
 * blam-cc: EAX -> location,
 * blam-cc: stack -> player_index
 *
 * @address 0x4776d0
 */


/**
 * C entry point for halo::game::KillStreak::begin; forwards to the C++ implementation.
 * register convention: a player handle in EAX (in_EAX); `slot` is this function's own stack
 * parameter.
 * // blam-cc: EAX -> player_handle, stack -> slot
 * blam-cc: EAX -> player_handle, stack -> slot
 *
 * @address 0x479d90
 */
void player_kill_streak_begin(int16_t slot, uint32_t player_handle)
{
    halo::game::KillStreak(player_handle).begin(slot);
}

/**
 * C entry point for halo::game::KillStreak::continue_streak; forwards to the C++ implementation.
 * register convention: a player handle in EAX (in_EAX); `slot` is this function's own stack
 * parameter.
 * // blam-cc: EAX -> player_handle, stack -> slot
 * blam-cc: EAX -> player_handle, stack -> slot
 *
 * @address 0x479de0
 */
void player_kill_streak_continue(int16_t slot, uint32_t player_handle)
{
    halo::game::KillStreak(player_handle).continue_streak(slot);
}

/**
 * C entry point for halo::game::Players::rebind_local_player_after_load; forwards to the C++ implementation.
 * blam-cc: no arguments
 *
 * @address 0x4765f0
 */
void players_rebind_local_player_after_load(void)
{
    halo::game::Players::rebind_local_player_after_load();
}

/**
 * C entry point for halo::game::Players::server_catchup_on_client_updates; forwards to the C++ implementation.
 * register convention: plain __cdecl, no parameters.
 * blam-cc: (no arguments)
 * blam-cc: EDI -> iterator
 * blam-cc: EAX -> out, EBX -> queue
 * blam-cc: EAX, ECX, ESI
 * blam-cc: EAX -> unit_index, EDX -> control, stack -> source_id
 * blam-cc: ECX
 *
 * @address 0x4768c0
 */
void players_server_catchup_on_client_updates(void)
{
    halo::game::Players::server_catchup_on_client_updates();
}

/**
 * C entry point for halo::game::StructureBsp::switch_structure_bsp; forwards to the C++ implementation.
 * blam-cc: EDI -> iterator
 * blam-cc: EAX -> player_handle
 * blam-cc: EDI -> player_handle
 * blam-cc: EDI -> player_handle
 *
 * @address 0x4749a0
 */
void main_switch_structure_bsp(void)
{
    halo::game::StructureBsp::switch_structure_bsp();
}

}

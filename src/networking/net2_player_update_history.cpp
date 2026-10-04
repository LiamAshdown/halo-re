#include <stddef.h>
/**
 * @file src/networking/net2_player_update_history.cpp
 * Player update history ring, queue and replay.
 */
#include "tags.h"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include "win32.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include "crt.h"
#include "halo/networking/net2_player_update_history.hpp"
#include "halo/memory/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/main/api.hpp"
#include "halo/platform/memory.hpp"

static auto &local_player_name_filter = halo::link::ref<uint16_t [0x400]>(halo::game::vars().local_player_name_filter);
static auto &player_update_log_categories_default = halo::link::ref<uint32_t>(halo::networking::vars().player_update_log_categories_default);
static auto &player_update_log_categories_filtered = halo::link::ref<uint32_t>(halo::networking::vars().player_update_log_categories_filtered);
static auto &player_update_log_flags = halo::link::ref<uint8_t>(halo::main::vars().player_update_log_flags);
static auto &player_update_history_log_path = halo::link::ref<char *>(halo::networking::vars().player_update_history_log_path);
static auto &player_update_log_file_mode_string = halo::link::ref<char []>(halo::networking::vars().player_update_log_file_mode_string);
static auto &network_client = halo::link::ref<network_client_globals *>(halo::networking::vars().network_client);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);


namespace halo::networking {

int32_t PlayerUpdateHistory::advance(int16_t player_index)
{
    data_iterator iter;
    void *element;

    iter.data = halo::game::globals().player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    for (element = halo::memory::data_iterator_next(&iter); element != 0; element = halo::memory::data_iterator_next(&iter)) {
        if ((uint16_t)iter.index == (uint16_t)player_index) {
            return (int32_t)iter.index;
        }
    }
    return -1;
}

uint8_t PlayerUpdateHistory::add(datum_index unit_index, player_update_history *history,
    int32_t tick_count, player_action control, int32_t *out_update_id)
{
    player_update_history_node *node;
    player_update_history_node *walk;
    object *unit_obj;
    object *vehicle_obj;
    unit_data *unit_ext;
    biped_data *biped_ext;
    unit_data *vehicle_ext;
    int32_t count;
    int32_t tick_sum;
    uint32_t next_id;

    count = 0;
    walk = history->head;
    if (walk != 0) {
        do {
            walk = walk->next;
            count = count + 1;
        } while (walk != 0);
        if (0x3f < count) {
            count = 0;
            tick_sum = 0;
            for (walk = history->head; walk != 0; walk = walk->next) {
                count = count + 1;
                tick_sum = tick_sum + walk->tick_count;
            }
            halo::networking::player_update_history_log_write(1, 0,
                "[%d]: Player update history overflow, [%d] updates == [%d] ticks.\n",
                halo::game::globals().game_time->game_time, count, tick_sum);
            *out_update_id = -1;
            return 0;
        }
    }

    node = (player_update_history_node *)halo::platform::heap_allocate(0, sizeof(player_update_history_node));
    node->update_id = history->next_update_id;
    node->tick_count = tick_count;
    memcpy(node->control, &control, sizeof(node->control));
    node->next = 0;

    next_id = (history->next_update_id + 1) & 0x8000003f;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xffffffc0) + 1;
    }
    history->next_update_id = next_id;

    unit_obj = halo::game::object_at(unit_index);
    unit_ext = &reinterpret_cast<unit_object *>(unit_obj)->unit;
    biped_ext = &reinterpret_cast<biped_object *>(unit_obj)->biped;

    node->vehicle_object = unit_obj->parent_object;

    node->unit_state.position = unit_obj->position;
    node->unit_state.velocity = unit_obj->velocity;
    node->unit_state.forward = unit_obj->forward;
    node->unit_state.animation_graph = unit_obj->animation_graph;
    node->unit_state.animation_index = unit_obj->animation_index;
    node->unit_state.animation_frame = unit_obj->animation_frame;
    node->unit_state.interpolation_frame_index = unit_obj->interpolation_frame_index;
    node->unit_state.node_function_count = unit_obj->node_function_count;
    memcpy(node->unit_state.animation_state, &unit_ext->animation_state_flags, 0x48);
    memcpy(node->unit_state.seat_acceleration, &unit_ext->seat_acceleration_last_position, 0x30);
    node->unit_state.biped_flags = biped_ext->flags;
    node->unit_state.stop_moving_ticks = biped_ext->stop_moving_ticks;
    node->unit_state.airborne_ticks = biped_ext->airborne_ticks;
    node->unit_state.slipping_ticks = biped_ext->slipping_ticks;
    node->unit_state.jump_ticks = biped_ext->jump_ticks;
    node->unit_state.landing_type = biped_ext->landing_type;
    node->unit_state.crouch_fraction = biped_ext->crouch_fraction;
    node->unit_state.ground_normal = biped_ext->ground_normal;
    node->unit_state.ground_plane_distance = biped_ext->ground_plane_distance;
    node->unit_state.landing_ticks = biped_ext->landing_ticks;
    node->unit_state.landing_duration_ticks = biped_ext->landing_duration_ticks;
    node->unit_state.movement_state = biped_ext->movement_state;
    node->unit_state.ground_surface_index = biped_ext->ground_surface_index;

    if (halo::game::player_unit_has_parent(unit_ext->controlling_player)) {
        vehicle_obj = halo::game::object_at(unit_obj->parent_object);
        node->has_vehicle = 1;
        node->vehicle_state.position = vehicle_obj->position;
        node->vehicle_state.velocity = vehicle_obj->velocity;
        node->vehicle_state.angular_velocity = vehicle_obj->angular_velocity;
        memcpy(reinterpret_cast<uint8_t *>(&node->vehicle_state) + offsetof(vehicle_state_snapshot, body_024), (uint8_t *)vehicle_obj + 0x04, 0x1f0);
        vehicle_ext = &reinterpret_cast<unit_object *>(vehicle_obj)->unit;
        node->vehicle_state.driver_seat_power = vehicle_ext->driver_seat_power;
        node->vehicle_state.gunner_seat_power = vehicle_ext->gunner_seat_power;
        node->vehicle_state.pad_21c = 0;
        memcpy(node->vehicle_state.tail, (uint8_t *)vehicle_obj + 0x4cc, 0xf4);
    } else {
        node->has_vehicle = 0;
    }

    if (history->tail != 0) {
        history->tail->next = node;
    }
    history->tail = node;
    if (history->head == 0) {
        history->head = node;
    }

    count = 0;
    tick_sum = 0;
    for (walk = history->head; walk != 0; walk = walk->next) {
        count = count + 1;
        tick_sum = tick_sum + walk->tick_count;
    }
    if (node->update_id % 10 == 0) {
        halo::networking::player_update_history_log_write(1, 0,
            "[%d]: Added through update [%d]. [%d]/[%d]updates == [%d] ticks\n",
            halo::game::globals().game_time->game_time, node->update_id, count,
            0x40, tick_sum);
    }
    if (count == 0x40) {
        halo::networking::player_update_history_log_write(1, 0,
            "[%d]: Warning...Update history is now full, [%d] updates.\n",
            halo::game::globals().game_time->game_time, 0x40);
    }

    *out_update_id = node->update_id;
    return 1;
}

void PlayerUpdateHistory::destroy(player_update_history *history)
{
    player_update_history_node *node;
    player_update_history_node *next;

    node = history->head;
    while (node != 0) {
        next = node->next;
        halo::platform::heap_free(node);
        node = next;
    }
    history->head = 0;
    history->tail = 0;
    halo::platform::heap_free(history);
}

player_update_history_node * PlayerUpdateHistory::find_and_prune(player_update_history *history,
    int32_t target_id, uint8_t prune)
{
    player_update_history_node *node;
    player_update_history_node *after_match;
    player_update_history_node *walk;
    player_update_history_node *next;
    int32_t matched_id;

    node = history->head;
    after_match = 0;
    walk = node;
    if (node != 0) {
        for (;;) {
            after_match = walk->next;
            if (walk->update_id == target_id) {
                break;
            }
            walk = after_match;
            if (walk == 0) {
                return 0;
            }
        }
        if (prune == 1) {
            walk = 0;
            do {
                if (node == 0) {
                    break;
                }
                matched_id = node->update_id;
                walk = node->next;
                halo::platform::heap_free(node);
                node = walk;
            } while (matched_id != target_id);
            history->head = walk;
            if (walk == 0) {
                history->tail = 0;
                return after_match;
            }
        }
    }
    return after_match;
}

void PlayerUpdateHistory::free_all(player_update_history *history)
{
    player_update_history_node *node;
    player_update_history_node *next;

    node = history->head;
    while (node != 0) {
        next = node->next;
        halo::platform::heap_free(node);
        node = next;
    }
    history->head = 0;
    history->tail = 0;
}

void PlayerUpdateHistory::log_set_name_filter(char *name)
{
    int32_t length;
    int32_t i;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x800) {
        length = 0x3ff;
    }
    if ((uint32_t)(length * 2 + 2) <= 0x800) {
        local_player_name_filter[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            local_player_name_filter[i] = (uint16_t)(uint8_t)name[i];
        }
    }
}

int32_t PlayerUpdateHistory::play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack)
{
    void * (*const memcpy)(void *dest, const void *src, int32_t count) = reinterpret_cast<void * (*)(void *dest, const void *src, int32_t count)>(&::memcpy);
    player_update_history_node *node;
    object *unit_obj;
    object *vehicle_obj;
    unit_data *unit_ext;
    biped_data *biped_ext;
    unit_data *vehicle_ext;
    datum_index parent_object;
    uint8_t in_vehicle_check;
    int32_t updates_this_call;
    int32_t ticks_this_call;
    int32_t remaining_ticks;
    int32_t result;
    real original_x, original_y, original_z;
    real server_start_x, server_start_y, server_start_z;
    real client_start_x, client_start_y, client_start_z;
    real end_x, end_y, end_z;
    real end2_x, end2_y, end2_z;
    real dx, dz;
    real distance;
    int32_t first_replayed_update_id = 0;
    int32_t last_replayed_update_id = 0;

    vehicle_obj = 0;
    node = halo::networking::player_update_history_find_and_prune(history, prune_target_id, prune);
    history->statistics[0] = history->statistics[0] + 1;
    history->statistics[3] = 0;
    history->statistics[4] = 0;

    if (unit_index == k_datum_index_none) {
        result = 0;
        halo::networking::player_update_history_log_write(1, 0, "Ignoring update [%d] due to unit_index == NONE", prune_target_id);
        if (node != 0) {
            return result;
        }
    } else if (node != 0) {
        unit_obj = halo::game::object_at(unit_index);
        unit_ext = &reinterpret_cast<unit_object *>(unit_obj)->unit;
        biped_ext = &reinterpret_cast<biped_object *>(unit_obj)->biped;
        parent_object = unit_obj->parent_object;

        if (parent_object != k_datum_index_none) {
            in_vehicle_check = halo::units::unit_seat_flag_bit2(parent_object, unit_ext->vehicle_seat_index);
            if (in_vehicle_check != 1) {

                return in_vehicle_check;
            }
            if (vehicle_ack == 0) {
                return 0;
            }
            if (node->has_vehicle != 1) {
                return (int32_t)vehicle_ack;
            }
            vehicle_obj = halo::game::object_at(parent_object);
            vehicle_ext = &reinterpret_cast<unit_object *>(vehicle_obj)->unit;
            if (reinterpret_cast<vehicle_object *>(vehicle_obj)->vehicle.collision_update_pending != 0) {
                reinterpret_cast<vehicle_object *>(vehicle_obj)->vehicle.collision_update_pending = 0;
                return (int32_t)vehicle_obj;
            }
        }
        if (unit_ext->animation_state == _unit_animation_state_seat_exit ||
            unit_ext->animation_state == _unit_animation_state_seat_enter) {
            return (int32_t)vehicle_obj;
        }

        if (vehicle_obj == 0) {
            original_x = unit_obj->position.x;
            original_y = unit_obj->position.y;
            original_z = unit_obj->position.z;
            client_start_x = node->unit_state.position.x;
            client_start_y = node->unit_state.position.y;
            client_start_z = node->unit_state.position.z;
        } else {
            original_x = vehicle_obj->position.x;
            original_y = vehicle_obj->position.y;
            original_z = vehicle_obj->position.z;
            client_start_x = node->vehicle_state.position.x;
            client_start_y = node->vehicle_state.position.y;
            client_start_z = node->vehicle_state.position.z;
        }

        unit_obj->velocity = node->unit_state.velocity;
        unit_obj->forward = node->unit_state.forward;
        unit_obj->animation_graph = node->unit_state.animation_graph;
        unit_obj->animation_index = node->unit_state.animation_index;
        unit_obj->animation_frame = node->unit_state.animation_frame;
        unit_obj->interpolation_frame_index = node->unit_state.interpolation_frame_index;
        unit_obj->node_function_count = node->unit_state.node_function_count;
        memcpy(&unit_ext->animation_state_flags, node->unit_state.animation_state, 0x48);
        memcpy(&unit_ext->seat_acceleration_last_position, node->unit_state.seat_acceleration, 0x30);
        biped_ext->flags = node->unit_state.biped_flags;
        biped_ext->stop_moving_ticks = node->unit_state.stop_moving_ticks;
        biped_ext->airborne_ticks = node->unit_state.airborne_ticks;
        biped_ext->slipping_ticks = node->unit_state.slipping_ticks;
        biped_ext->jump_ticks = node->unit_state.jump_ticks;
        biped_ext->landing_type = node->unit_state.landing_type;
        biped_ext->crouch_fraction = node->unit_state.crouch_fraction;
        biped_ext->ground_normal = node->unit_state.ground_normal;
        biped_ext->ground_plane_distance = node->unit_state.ground_plane_distance;
        biped_ext->landing_ticks = node->unit_state.landing_ticks;
        biped_ext->landing_duration_ticks = node->unit_state.landing_duration_ticks;
        biped_ext->movement_state = node->unit_state.movement_state;
        biped_ext->ground_surface_index = node->unit_state.ground_surface_index;

        if (vehicle_obj == 0) {
            unit_obj->position.x = server_x;
            unit_obj->position.y = server_y;
            unit_obj->position.z = server_z;
            updates_this_call = 0;
            ticks_this_call = 0;
        } else {
            vehicle_obj->velocity = node->vehicle_state.velocity;
            vehicle_obj->angular_velocity = node->vehicle_state.angular_velocity;
            vehicle_obj->forward = node->vehicle_state.forward;
            vehicle_obj->up = node->vehicle_state.up;
            vehicle_ext->driver_seat_power = node->vehicle_state.driver_seat_power;
            vehicle_ext->gunner_seat_power = node->vehicle_state.gunner_seat_power;
            memcpy((uint8_t *)vehicle_obj + 0x4cc, node->vehicle_state.tail, 0xf4);
            {
                real_point3d server_position = {server_x, server_y, server_z};

                halo::units::unit_propagate_position_delta_to_children(&server_position, parent_object);
            }
            vehicle_obj->velocity = vehicle_ack->vehicle.velocity;
            vehicle_obj->angular_velocity = vehicle_ack->vehicle.angular_velocity;
            vehicle_obj->forward = vehicle_ack->vehicle.forward;
            vehicle_obj->up = vehicle_ack->vehicle.up;
            updates_this_call = 0;
            ticks_this_call = 0;
        }

        first_replayed_update_id = node->update_id;
        do {
            const player_action &action = *reinterpret_cast<const player_action *>(node->control);
            unit_control_data control_block;

            memset(&control_block, 0, sizeof(control_block));
            halo::game::player_compute_view_forward_vector(reinterpret_cast<unit_object *>(unit_obj)->unit.controlling_player,
                const_cast<real *>(&action.desired_yaw), &control_block.aiming_vector);
            control_block.facing_vector = control_block.aiming_vector;
            control_block.looking_vector = control_block.aiming_vector;
            control_block.animation_state = 3;
            control_block.aiming_speed = 0;
            control_block.control_flags = (uint16_t)action.control_flags;
            control_block.weapon_index = action.weapon_index;
            control_block.grenade_index = action.grenade_index;
            control_block.zoom_level = action.zoom_level;
            control_block.throttle.i = action.throttle_x;
            control_block.throttle.j = action.throttle_y;
            control_block.throttle.k = 0.0f;
            control_block.primary_trigger = action.primary_trigger;
            halo::units::unit_apply_control_block(unit_index, &control_block, -1);
            remaining_ticks = node->tick_count;
            updates_this_call = updates_this_call + 1;
            if (0 < remaining_ticks) {
                ticks_this_call = ticks_this_call + remaining_ticks;
                do {
                    halo::units::globals().updates_suppressed = 1;
                    if (vehicle_obj == 0) {
                        halo::units::unit_update(unit_index);
                        halo::units::biped_update(unit_index);
                    } else {
                        halo::objects::object_update(parent_object);
                    }
                    remaining_ticks = remaining_ticks - 1;
                    halo::units::globals().updates_suppressed = 0;
                } while (remaining_ticks != 0);
            }
            last_replayed_update_id = node->update_id;
            node = node->next;
        } while (node != 0);

        if (vehicle_obj == 0) {
            end_x = unit_obj->position.x;
            end_y = unit_obj->position.y;
            end_z = unit_obj->position.z;
        } else {
            end_x = vehicle_obj->position.x;
            end_y = vehicle_obj->position.y;
            end_z = vehicle_obj->position.z;
        }

        halo::networking::player_update_history_log_write(1, 0, "       Original Pos: [%f] [%f] [%f]",
            (double)original_x, (double)original_y, (double)original_z);
        halo::networking::player_update_history_log_write(1, 0, "Server Starting Pos: [%f] [%f] [%f]",
            (double)server_x, (double)server_y, (double)server_z);
        halo::networking::player_update_history_log_write(1, 0, "Client Starting Pos: [%f] [%f] [%f]",
            (double)client_start_x, (double)client_start_y, (double)client_start_z);
        halo::networking::player_update_history_log_write(1, 0, "         Ending Pos: [%f] [%f] [%f]",
            (double)end_x, (double)end_y, (double)end_z);
        halo::networking::player_update_history_log_write(1, 0, "         Difference: [%f]",
            (double)halo::libm::sqrt((double)((end_x - original_x) * (end_x - original_x) +
                (end_y - original_y) * (end_y - original_y) + (end_z - original_z) * (end_z - original_z))));
        halo::networking::player_update_history_log_write(1, 0, "        Ran updates: [%d] -> [%d], [%d] updates == [%d] ticks",
            first_replayed_update_id, last_replayed_update_id, updates_this_call, ticks_this_call);

        if (vehicle_obj == 0) {
            end2_x = unit_obj->position.x;
            end2_y = unit_obj->position.y;
            end2_z = unit_obj->position.z;
        } else {
            end2_x = vehicle_obj->position.x;
            end2_y = vehicle_obj->position.y;
            end2_z = vehicle_obj->position.z;
        }

        result = history->statistics[2] + ticks_this_call;
        history->statistics[1] = history->statistics[1] + updates_this_call;
        history->statistics[2] = result;
        history->statistics[4] = ticks_this_call;
        result = result / history->statistics[0];
        history->statistics[3] = updates_this_call;
        *(float *)&history->statistics[7] = (float)result;
        dz = end2_z - original_z;
        distance = (real)halo::libm::sqrt((double)((end2_x - original_x) * (end2_x - original_x) +
                        (end2_y - original_y) * (end2_y - original_y) + dz * dz)) +
            *(float *)&history->statistics[5];
        *(float *)&history->statistics[5] = distance;
        *(float *)&history->statistics[6] = distance / (float)history->statistics[0];
        return result;
    }

    result = 0;
    halo::networking::player_update_history_log_write(1, 0, "Ignoring update [%d] due to starting_update == NULL", prune_target_id);
    return result;
}

void PlayerUpdateHistory::play_for_update_index(datum_index player_index)
{
    player *plr;

    plr = (player *)((uint8_t *)halo::game::globals().player_data->data + (uint16_t)player_index * halo::game::globals().player_data->size);
    halo::networking::player_update_history_play(1, plr->baseline_update_id, network_client->update_history, plr->unit,
        *(float *)&plr->unknown_f0, *(float *)&plr->unknown_f4, *(float *)&plr->unknown_f8, 0);

}

void PlayerUpdateHistory::play_local_player(int32_t target_update_id)
{
    data_iterator iter;
    player *candidate;
    datum_index unit_index;
    player_update_history_node *node;
    player_update_history_node *after_match;
    int32_t node_id;

    unit_index = k_datum_index_none;
    iter.data = halo::game::globals().player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    while (candidate != 0 && candidate->local_player_index == -1) {
        candidate = (player *)halo::memory::data_iterator_next(&iter);
    }
    if (candidate != 0) {
        unit_index = candidate->unit;
    }

    if (network_client == 0) {
        return;
    }
    node = (network_client->update_history)->head;
    if (node == 0) {
        return;
    }
    do {
        node_id = node->update_id;
        after_match = node->next;
        node = after_match;
        if (node_id == target_update_id) {
            break;
        }
        if (node == 0) {
            return;
        }
    } while (1);
    if (after_match != 0) {
        halo::networking::player_update_history_play(0, 0, network_client->update_history,
            unit_index, after_match->unit_state.position.x,
            after_match->unit_state.position.y,
            after_match->unit_state.position.z, 0);
    }
}

void PlayerUpdateHistory::flush_by_name(char *name)
{
    uint16_t filter_name[0x400];
    int32_t length;
    int32_t i;
    data_iterator iter;
    player *candidate;
    int32_t index;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x800) {
        length = 0x3ff;
    }
    if ((uint32_t)(length * 2 + 2) <= 0x800) {
        filter_name[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            filter_name[i] = (uint16_t)(uint8_t)name[i];
        }
    }

    iter.data = halo::game::globals().player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    while (candidate != 0) {
        if (wcscmp((wchar_t *)candidate->name, (wchar_t *)filter_name) == 0) {
            for (index = candidate->update_history.queue.read_index;
                 index != candidate->update_history.queue.write_index;
                 index = (index + 1) % 0x78) {

            }
            halo::game::players_find_local_owned_unclear();
        }
        candidate = (player *)halo::memory::data_iterator_next(&iter);
    }
}

int32_t PlayerUpdateHistory::offset_from_head(player *plr, int32_t new_update_id)
{
    circular_queue *queue;
    int32_t used;
    player_update_record *head_record;
    int32_t head_id;

    queue = &plr->update_history.queue;
    if (queue->read_index < queue->write_index) {
        used = -queue->read_index;
    } else {
        if (queue->read_index <= queue->write_index) {
            return -1;
        }
        used = queue->capacity - queue->read_index;
    }
    if (queue->write_index + used < 1) {
        return -1;
    }
    if (queue->read_index == queue->write_index) {
        head_record = 0;
    } else {
        head_record = (player_update_record *)queue->records[queue->read_index];
    }
    head_id = head_record->field0;
    if (head_id <= new_update_id) {
        if (new_update_id <= head_id) {
            return 0;
        }
        return new_update_id - head_id;
    }
    return (new_update_id - head_id) + 0x40;
}

void PlayerUpdateHistory::remote_player_action_update_apply(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    uint8_t is_baseline;
    remote_player_action_state decoded;
    remote_player_action_state previous;

    header = (remote_player_update_header *)decode_context[0x11];
    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        memset(&decoded, 0, sizeof(decoded));
        is_baseline = 1;
        if (halo::networking::message_delta_decode_compound_field((void **)decode_context, &decoded) != 1) {
            return;
        }
    } else {
        player *candidate;

        is_baseline = 0;
        candidate = 0;
        if (remapped_index != -1) {
            int16_t index = (int16_t)remapped_index;
            if (index >= 0 && index < halo::game::globals().player_data->maximum_count) {
                player *maybe = (player *)((uint8_t *)halo::game::globals().player_data->data
                    + (int32_t)halo::game::globals().player_data->size * (int32_t)index);
                int16_t salt = (int16_t)((uint32_t)remapped_index >> 16);
                if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)
                    && maybe->local_player_index == -1) {
                    candidate = maybe;
                }
            }
        }
        if (candidate != 0) {
            memcpy(&previous, &candidate->unknown_f0, sizeof(previous));
        }

        decoded = previous;
        state->bits_read += halo::networking::message_delta_read_changed_subfields(state, (uint8_t *)(decode_context + 1),
            (int32_t)&previous, (int32_t)&decoded);
        state->changed = 1;
    }

    remapped_index = header->player_index;
    if (remapped_index != -1) {
        int16_t index = (int16_t)remapped_index;
        if (index >= 0 && index < halo::game::globals().player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)halo::game::globals().player_data->data
                + (int32_t)halo::game::globals().player_data->size * (int32_t)index);
            int16_t salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)
                && maybe->local_player_index == -1) {
                halo::networking::handle_remote_player_action_update(&decoded, header, is_baseline);
            }
        }
    }
}

}  // namespace halo::networking

namespace halo::networking {
void player_update_history_log_printf_filtered(player *target_player, int32_t unused_arg,
    const char *format, ...)
{
    char buffer[0x400];
    va_list args;

    if (wcscmp((wchar_t *)target_player->name, (wchar_t *)local_player_name_filter) != 0) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    player_update_history_log_write(0, 1, buffer);
}

void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask, const char *format, ...)
{
    uint32_t mask;
    char buffer[0x400];
    va_list args;
    FILE *file;

    mask = use_filtered_mask != 0 ? player_update_log_categories_filtered : player_update_log_categories_default;
    if ((mask & category_flags) != category_flags) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    if ((player_update_log_flags & 2) == 0) {
        return;
    }
    file = (FILE *)fopen(player_update_history_log_path, player_update_log_file_mode_string);
    if (file == 0) {
        return;
    }
    fprintf(file, buffer);
    fclose(file);
}

int32_t player_data_iterator_advance(int16_t player_index)
{
    return halo::networking::PlayerUpdateHistory::advance(player_index);
}

uint8_t player_update_history_add(datum_index unit_index, player_update_history *history,
    int32_t tick_count, player_action control, int32_t *out_update_id)
{
    return halo::networking::PlayerUpdateHistory::add(unit_index, history, tick_count, control, out_update_id);
}

void player_update_history_destroy(player_update_history *history)
{
    halo::networking::PlayerUpdateHistory::destroy(history);
}

player_update_history_node * player_update_history_find_and_prune(player_update_history *history,
    int32_t target_id, uint8_t prune)
{
    return halo::networking::PlayerUpdateHistory::find_and_prune(history, target_id, prune);
}

void player_update_history_free_all(player_update_history *history)
{
    halo::networking::PlayerUpdateHistory::free_all(history);
}

void player_update_history_log_set_name_filter(char *name)
{
    halo::networking::PlayerUpdateHistory::log_set_name_filter(name);
}

int32_t player_update_history_play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack)
{
    return halo::networking::PlayerUpdateHistory::play(prune, prune_target_id, history, unit_index, server_x, server_y, server_z, vehicle_ack);
}

void player_update_history_play_for_update_index(datum_index player_index)
{
    halo::networking::PlayerUpdateHistory::play_for_update_index(player_index);
}

void player_update_history_play_local_player(int32_t target_update_id)
{
    halo::networking::PlayerUpdateHistory::play_local_player(target_update_id);
}

void player_update_queue_flush_by_name(char *name)
{
    halo::networking::PlayerUpdateHistory::flush_by_name(name);
}

int32_t player_update_queue_offset_from_head(player *plr, int32_t new_update_id)
{
    return halo::networking::PlayerUpdateHistory::offset_from_head(plr, new_update_id);
}

void player_update_remote_player_action_update_apply(int32_t **decode_context)
{
    halo::networking::PlayerUpdateHistory::remote_player_action_update_apply(decode_context);
}

}

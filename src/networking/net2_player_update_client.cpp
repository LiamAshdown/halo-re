/**
 * @file src/networking/net2_player_update_client.cpp
 * Client-side player update ingestion.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include <string.h>
#include "objects.h"
#include "units.h"
#include "halo/networking/net2_player_update_client.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern data_array * player_data;
extern game_time_globals * game_time;
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern void message_delta_decode_compound_field_staged(void *decode_context);
extern uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id);
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...);
extern void player_update_history_play_for_update_index(void *update_history, int32_t update_id);
extern void * object_network_id_table;
extern network_client_globals * network_client;
extern datum_index players_find_local_owned_unclear(void);
extern void player_update_history_play(uint8_t flag, uint32_t control_ec, void *update_history,
    datum_index unit, float x, float y, float z, void *control_ptr);
extern network_id_table * machine_table;
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state,
    void *field_bindings, const void *previous, void *destination);
extern void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline);
extern uint8_t message_delta_decode_compound_field_forced(void *decode_context, void *destination, int32_t force_changed);
extern uint8_t is_remote_player_update_in_order(player *target_player, uint8_t control_sequence,
    int32_t update_id);
extern int32_t player_update_queue_offset_from_head(player *target_player, int32_t update_id);
extern uint8_t position_update_queue_push(circular_queue *queue, real x, real y, real z,
    int32_t tick, int32_t sequence);
extern int32_t circular_queue_count(circular_queue *queue);
extern void unit_snap_position_if_far(real_point3d *new_position, object *obj,
    datum_index unit_index);
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...);
extern uint8_t circular_queue_push(circular_queue *queue, void *source);
void player_update_client_local_player_update_from_network(int32_t *decode_context);
void player_update_client_local_player_vehicle_update_from_network(int32_t *decode_context);
void player_update_client_remote_player_action_update_from_network(int32_t **decode_context);
void player_update_client_remote_player_position_delta_from_network(int32_t **decode_context);
void player_update_client_remote_player_position_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, real x, real y, real z);
void player_update_client_remote_player_total_biped_update_from_network(int32_t **decode_context);
void player_update_client_remote_player_total_vehicle_update_from_network(int32_t **decode_context);
void player_update_client_remote_player_vehicle_position_delta_from_network(int32_t **decode_context);
void player_update_client_remote_player_vehicle_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, vehicle_update_body vehicle);
}


namespace halo::networking {

void PlayerUpdateClient::local_player_update_from_network(int32_t *decode_context)
{
    int32_t mode;
    int32_t *record_ctx;
    local_player_update_ack ack;
    data_iterator iter;
    player *candidate;

    record_ctx = (int32_t *)(uintptr_t)decode_context[0];
    mode = record_ctx[0];
    if (mode != 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }
    if (message_delta_decode_compound_field(decode_context, &ack) != 1) {
        return;
    }
    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    if (candidate == 0) {
        return;
    }
    while (candidate->local_player_index == -1) {
        candidate = (player *)halo::memory::data_iterator_next(&iter);
        if (candidate == 0) {
            return;
        }
    }
    if (is_local_player_update_in_order(candidate->last_update_id, ack.update_id) != 1) {
        return;
    }
    player_update_history_log_write(1, 0, "[%d]: Received ack for update [%d].\n",
        game_time->game_time, ack.baseline_id);
    candidate->last_update_id = ack.update_id;
    candidate->baseline_update_id = ack.baseline_id;
    candidate->unknown_f0 = *(int32_t *)&ack.position.x;
    candidate->unknown_f4 = *(int32_t *)&ack.position.y;
    candidate->unknown_f8 = *(int32_t *)&ack.position.z;
    player_update_history_play_for_update_index(0, 0);
}

void PlayerUpdateClient::local_player_vehicle_update_from_network(int32_t *decode_context)
{
    int32_t mode;
    int32_t *record_ctx;
    local_player_vehicle_update_ack ack;
    real_vector3d temp;
    datum_index vehicle_handle;
    player *candidate;

    record_ctx = (int32_t *)(uintptr_t)decode_context[0];
    mode = record_ctx[0];
    if (mode != 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }
    if (message_delta_decode_compound_field(decode_context, &ack) != 1) {
        return;
    }
    if (ack.vehicle.parent_or_tag != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)object_network_id_table + 0x28);
        ack.vehicle.parent_or_tag = table_base[ack.vehicle.parent_or_tag];
    } else {
        ack.vehicle.parent_or_tag = -1;
    }

    halo::math::vector3d_cross_product(temp, ack.vehicle.up, ack.vehicle.forward);
    halo::math::vector3d_cross_product(ack.vehicle.up, ack.vehicle.forward, temp);
    halo::math::vector3d_normalize_with_length(ack.vehicle.forward);
    halo::math::vector3d_normalize_with_length(ack.vehicle.up);

    vehicle_handle = players_find_local_owned_unclear();
    candidate = (player *)halo::memory::datum_get(vehicle_handle, player_data);
    if (candidate == 0) {
        return;
    }
    if (is_local_player_update_in_order(candidate->last_update_id, ack.update_id) == 1) {
        player_update_history_log_write(1, 0, "[%d]: Received vehicle ack for update [%d].\n",
            game_time->game_time, ack.baseline_id);
        candidate->last_update_id = ack.update_id;
        candidate->baseline_update_id = ack.baseline_id;
        candidate->unknown_f0 = *(int32_t *)&ack.vehicle.position.x;
        candidate->unknown_f4 = *(int32_t *)&ack.vehicle.position.y;
        candidate->unknown_f8 = *(int32_t *)&ack.vehicle.position.z;
        player_update_history_play(1, candidate->baseline_update_id,
            network_client->update_history, candidate->unit,
            ack.vehicle.position.x, ack.vehicle.position.y, ack.vehicle.position.z, &ack);
        return;
    }
    player_update_history_log_write(1, 0,
        "[%d]: Threw away local player vehicle ack [%d] (%d), previous ack [%d] (%d).\n",
        game_time->game_time, ack.baseline_id,
        candidate->baseline_update_id, ack.update_id, candidate->last_update_id);
}

void PlayerUpdateClient::remote_player_action_update_from_network(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_action_state staged;

    header = (remote_player_update_header *)decode_context[0x11];
    memset(&staged, 0, sizeof(staged));

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }

    if (candidate != 0) {
        state = (message_delta_decode_state *)decode_context[0];
        if (state->incremental != 0) {
            const void *control_record = &candidate->unknown_f0;

            memcpy(&staged, control_record, sizeof(staged));
            state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
                control_record, &staged);
            state->changed = 1;
            handle_remote_player_action_update(&staged, header, 0);
            return;
        }
        if (message_delta_decode_compound_field(decode_context, &staged) == 1) {
            handle_remote_player_action_update(&staged, header, 1);
        }
        return;
    }
    message_delta_decode_compound_field_staged(decode_context);
}

void PlayerUpdateClient::remote_player_position_delta_from_network(int32_t **decode_context)
{
    remote_player_update_header *header;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    message_delta_decode_state *state;
    real_point3d position;

    header = (remote_player_update_header *)decode_context[0x11];
    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }
    if (candidate == 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        if (message_delta_decode_compound_field(decode_context, &position) != 1) {
            return;
        }
        *(real *)&candidate->position_baseline_x = position.x;
        *(real *)&candidate->position_baseline_y = position.y;
        *(real *)&candidate->position_baseline_z = position.z;
    } else {
        position.x = *(real *)&candidate->position_baseline_x;
        position.y = *(real *)&candidate->position_baseline_y;
        position.z = *(real *)&candidate->position_baseline_z;
        if (message_delta_decode_compound_field_forced(decode_context, &position, 0) != 1) {
            return;
        }
    }
    player_update_client_remote_player_position_update_from_network(
        remapped_index, header->update_id, header->baseline_id,
        position.x, position.y, position.z);
}

void PlayerUpdateClient::remote_player_position_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, real x, real y, real z)
{
    int16_t index;
    int16_t salt;
    player *target;
    int32_t on_update_id;
    int32_t distance;

    if (player_index == -1) {
        return;
    }
    index = (int16_t)player_index;
    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    target = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
    salt = (int16_t)((uint32_t)player_index >> 16);
    if (target->identifier == 0 || (salt != 0 && target->identifier != salt)) {
        return;
    }

    if (is_remote_player_update_in_order(target, (uint8_t)control_sequence, update_id) != 1) {
        return;
    }

    if (target->last_position_update_id != -1) {
        if (target->update_history.queue.read_index == target->update_history.queue.write_index) {
            on_update_id = -1;
        } else {
            player_update_record *oldest = (player_update_record *)
                target->update_history.queue.records[target->update_history.queue.read_index];
            on_update_id = (int32_t)oldest->field0;
        }

        distance = player_update_queue_offset_from_head(target, update_id);
        if (distance >= 0 && distance < 0x20) {
            int32_t position_count;
            int32_t action_count;
            int32_t write_index;
            int32_t read_index;

            if (position_update_queue_push(&target->position_updates, x, y, z, update_id,
                    distance) == 0) {
                player_update_history_log_printf_filtered(target, 1,
                    "[%d]: Remote player position_queue overflow.\n",
                    game_time->game_time);
            }

            write_index = target->position_updates.write_index;
            read_index = target->position_updates.read_index;
            if (write_index > read_index) {
                position_count = write_index - read_index;
            } else if (write_index < read_index) {
                position_count = (write_index - read_index) + target->position_updates.capacity;
            } else {
                position_count = 0;
            }

            write_index = target->update_history.queue.write_index;
            read_index = target->update_history.queue.read_index;
            if (write_index > read_index) {
                action_count = write_index - read_index;
            } else if (write_index < read_index) {
                action_count = (write_index - read_index) + target->update_history.queue.capacity;
            } else {
                action_count = 0;
            }

            player_update_history_log_printf_filtered(target, 1,
                "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions",
                update_id, on_update_id, distance, action_count, position_count);
            target->position_update_ignored_count = 0;
        } else {
            int32_t out_of_range_count;

            out_of_range_count = target->position_update_ignored_count + 1;
            target->position_update_ignored_count = out_of_range_count;
            if (out_of_range_count <= 2) {
                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Ignoring [%d (%d)] ",
                    update_id, on_update_id, distance, action_count, position_count,
                    out_of_range_count, 2);

            } else {
                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Applying immediately",
                    update_id, on_update_id, distance, action_count, position_count);
                target->position_update_ignored_count = 0;

                if (target->unit != -1) {
                    object *unit = halo::objects::object_try_and_get(target->unit, 3);

                    if (unit != 0 && unit->parent_object == -1 && unit->network_role == 1) {
                        real_point3d new_position;
                        real snap_distance;

                        new_position.x = x;
                        new_position.y = y;
                        new_position.z = z;
                        snap_distance = halo::math::vector3d_distance(new_position, unit->position);
                        if (snap_distance <= 1.0f) {
                            player_update_history_log_printf_filtered(target, 1,
                                "Apply immediately saved by tolerance [%f] (%f)",
                                (double)snap_distance, 1.0);
                        } else {
                            player_update_history_log_printf_filtered(target, 1,
                                "Apply immediately dist: [%f] (%f)",
                                (double)snap_distance, 1.0);
                            unit_snap_position_if_far(&new_position, unit, target->unit);
                        }
                    }
                }
            }
        }
        target->last_remote_update_id = (int32_t)(uint8_t)control_sequence;
    }
    target->last_position_update_id = update_id;
}

void PlayerUpdateClient::remote_player_total_biped_update_from_network(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_biped_update_state decoded;
    remote_player_biped_update_state previous;
    uint8_t is_baseline;
    const char *mode;

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data
                + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }

    if (candidate == 0 || candidate->local_player_index != -1) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        uint8_t decoded_ok;

        memset(&decoded, 0, sizeof(decoded));
        decoded_ok = message_delta_decode_compound_field(decode_context, &decoded);
        if (decoded_ok == 1) {
            *(real *)&candidate->position_baseline_x = decoded.position.x;
            *(real *)&candidate->position_baseline_y = decoded.position.y;
            *(real *)&candidate->position_baseline_z = decoded.position.z;
        }
        is_baseline = 1;
        if (decoded_ok != 1) {
            return;
        }
        mode = "stateless";
    } else {
        memcpy(&previous.action, &candidate->unknown_f0, sizeof(previous.action));
        previous.position.x = *(real *)&candidate->position_baseline_x;
        previous.position.y = *(real *)&candidate->position_baseline_y;
        previous.position.z = *(real *)&candidate->position_baseline_z;
        decoded = previous;
        state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
            &previous, &decoded);
        state->changed = 1;
        is_baseline = 0;
        mode = "incremental";
    }

    player_update_history_log_printf_filtered(candidate, 3,
        "Received %s total biped update [%d].", mode, header->update_id);
    handle_remote_player_action_update(&decoded.action, header, is_baseline);
    player_update_client_remote_player_position_update_from_network(
        header->player_index, header->update_id, header->control_sequence,
        decoded.position.x, decoded.position.y, decoded.position.z);
}

void PlayerUpdateClient::remote_player_total_vehicle_update_from_network(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_vehicle_update_state decoded;
    remote_player_vehicle_update_state previous;
    uint8_t is_baseline;
    const char *mode;

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data
                + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }
    if (candidate == 0 || candidate->local_player_index != -1) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        uint8_t decoded_ok;

        memset(&decoded, 0, sizeof(decoded));
        decoded_ok = message_delta_decode_compound_field(decode_context, &decoded);
        if (decoded_ok == 1) {
            real_vector3d temp;

            halo::math::vector3d_cross_product(temp, decoded.vehicle.up, decoded.vehicle.forward);
            halo::math::vector3d_cross_product(decoded.vehicle.up, decoded.vehicle.forward, temp);
            halo::math::vector3d_normalize_with_length(decoded.vehicle.forward);
            halo::math::vector3d_normalize_with_length(decoded.vehicle.up);
            memcpy(&candidate->vehicle_baseline, &decoded.vehicle, sizeof(decoded.vehicle));
        }
        is_baseline = 1;
        if (decoded_ok != 1) {
            return;
        }
        mode = "stateless";
    } else {
        memcpy(&previous.action, &candidate->unknown_f0, sizeof(previous.action));
        memcpy(&previous.vehicle, &candidate->vehicle_baseline, sizeof(previous.vehicle));
        decoded = previous;
        state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
            &previous, &decoded);
        state->changed = 1;
        is_baseline = 0;
        mode = "incremental";
    }

    player_update_history_log_printf_filtered(candidate, 3,
        "Received %s total vehicle update [%d].", mode, header->update_id);
    handle_remote_player_action_update(&decoded.action, header, is_baseline);
    player_update_client_remote_player_vehicle_update_from_network(
        header->player_index, header->update_id, header->control_sequence, decoded.vehicle);
}

void PlayerUpdateClient::remote_player_vehicle_position_delta_from_network(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    vehicle_update_body decoded;

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data
                + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }
    if (candidate == 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        real_vector3d temp;

        if (message_delta_decode_compound_field(decode_context, &decoded) != 1) {
            return;
        }
        halo::math::vector3d_cross_product(temp, decoded.up, decoded.forward);
        halo::math::vector3d_cross_product(decoded.up, decoded.forward, temp);
        halo::math::vector3d_normalize_with_length(decoded.forward);
        halo::math::vector3d_normalize_with_length(decoded.up);
        memcpy(&candidate->vehicle_baseline, &decoded, sizeof(decoded));
    } else {
        memcpy(&decoded, &candidate->vehicle_baseline, sizeof(decoded));
        if (message_delta_decode_compound_field_forced(decode_context, &decoded, 0) != 1) {
            return;
        }
    }

    player_update_client_remote_player_vehicle_update_from_network(
        remapped_index, header->update_id, header->baseline_id, decoded);
}

void PlayerUpdateClient::remote_player_vehicle_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, vehicle_update_body vehicle)
{
    int16_t index;
    int16_t salt;
    player *target;
    int32_t remapped_vehicle;
    real_vector3d temp;
    int32_t on_update_id;
    int32_t distance;

    if (player_index == -1) {
        return;
    }
    index = (int16_t)player_index;
    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    target = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
    salt = (int16_t)((uint32_t)player_index >> 16);
    if (target->identifier == 0 || (salt != 0 && target->identifier != salt)) {
        return;
    }

    remapped_vehicle = -1;
    if (vehicle.parent_or_tag != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)object_network_id_table + 0x28);
        remapped_vehicle = table_base[vehicle.parent_or_tag];
    }
    vehicle.parent_or_tag = remapped_vehicle;

    temp.i = vehicle.forward.j * vehicle.up.k - vehicle.forward.k * vehicle.up.j;
    temp.j = vehicle.forward.k * vehicle.up.i - vehicle.forward.i * vehicle.up.k;
    temp.k = vehicle.forward.i * vehicle.up.j - vehicle.forward.j * vehicle.up.i;
    vehicle.up.i = temp.j * vehicle.forward.k - temp.k * vehicle.forward.j;
    vehicle.up.j = temp.k * vehicle.forward.i - vehicle.forward.k * temp.i;
    vehicle.up.k = vehicle.forward.j * temp.i - temp.j * vehicle.forward.i;
    halo::math::vector3d_normalize_with_length(vehicle.forward);
    halo::math::vector3d_normalize_with_length(vehicle.up);

    if (is_remote_player_update_in_order(target, (uint8_t)control_sequence, update_id) != 1) {
        return;
    }

    if (target->last_vehicle_update_id != -1) {
        if (target->update_history.queue.read_index == target->update_history.queue.write_index) {
            on_update_id = -1;
        } else {
            player_update_record *oldest = (player_update_record *)
                target->update_history.queue.records[target->update_history.queue.read_index];
            on_update_id = (int32_t)oldest->field0;
        }

        distance = player_update_queue_offset_from_head(target, update_id);
        if (distance >= 0 && distance < 0x20) {
            vehicle_update_record record;
            int32_t vehicle_count;
            int32_t action_count;
            int32_t write_index;
            int32_t read_index;

            record.tick = update_id;
            record.sequence = distance;
            record.body = vehicle;
            if (circular_queue_push(&target->vehicle_updates, &record) == 0) {
                player_update_history_log_printf_filtered(target, 1,
                    "[%d]: Remote player vehicle_update_queue overflow.\n",
                    game_time->game_time);
            }

            write_index = target->vehicle_updates.write_index;
            read_index = target->vehicle_updates.read_index;
            if (write_index > read_index) {
                vehicle_count = write_index - read_index;
            } else if (write_index < read_index) {
                vehicle_count = (target->vehicle_updates.capacity - read_index) + write_index;
            } else {
                vehicle_count = 0;
            }

            write_index = target->update_history.queue.write_index;
            read_index = target->update_history.queue.read_index;
            if (write_index > read_index) {
                action_count = write_index - read_index;
            } else if (write_index < read_index) {
                action_count = (write_index - read_index) + target->update_history.queue.capacity;
            } else {
                action_count = 0;
            }

            player_update_history_log_printf_filtered(target, 1,
                "Received vehicle_update update [%d], on [%d] (%d). [%d] actions, "
                "[%d] vehicle updates",
                update_id, on_update_id, distance, action_count, vehicle_count);
            target->vehicle_update_ignored_count = 0;
        } else {
            int32_t out_of_range_count;

            out_of_range_count = target->vehicle_update_ignored_count + 1;
            target->vehicle_update_ignored_count = out_of_range_count;
            if (out_of_range_count <= 1) {

                int32_t position_count = circular_queue_count(&target->position_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Ignoring [%d (%d)] ",
                    update_id, on_update_id, distance, action_count, position_count,
                    out_of_range_count, 1);

            } else {
                int32_t vehicle_count = circular_queue_count(&target->vehicle_updates);
                int32_t action_count = circular_queue_count(&target->update_history.queue);

                player_update_history_log_printf_filtered(target, 1,
                    "Received pos update [%d], on [%d] (%d). [%d] actions, [%d] positions "
                    "***Applying immediately",
                    update_id, on_update_id, distance, action_count, vehicle_count);
                target->vehicle_update_ignored_count = 0;

                if (target->unit != -1) {
                    object *unit = halo::objects::object_try_and_get(target->unit, 3);

                    if (unit != 0 && unit->parent_object == vehicle.parent_or_tag) {
                        object *vehicle_object = halo::objects::object_try_and_get(vehicle.parent_or_tag, 3);

                        if (vehicle_object != 0) {
                            halo::units::unit_propagate_position_delta_to_children(&vehicle.position, vehicle.parent_or_tag);
                            vehicle_object->velocity = vehicle.velocity;
                            vehicle_object->angular_velocity = vehicle.angular_velocity;
                            vehicle_object->forward = vehicle.forward;
                            vehicle_object->up = vehicle.up;
                        }
                    }
                }
            }
        }
        target->last_remote_update_id = (int32_t)(uint8_t)control_sequence;
    }
    target->last_vehicle_update_id = update_id;
}

}  // namespace halo::networking

extern "C" {
void player_update_client_local_player_update_from_network(int32_t *decode_context)
{
    halo::networking::PlayerUpdateClient::local_player_update_from_network(decode_context);
}

void player_update_client_local_player_vehicle_update_from_network(int32_t *decode_context)
{
    halo::networking::PlayerUpdateClient::local_player_vehicle_update_from_network(decode_context);
}

void player_update_client_remote_player_action_update_from_network(int32_t **decode_context)
{
    halo::networking::PlayerUpdateClient::remote_player_action_update_from_network(decode_context);
}

void player_update_client_remote_player_position_delta_from_network(int32_t **decode_context)
{
    halo::networking::PlayerUpdateClient::remote_player_position_delta_from_network(decode_context);
}

void player_update_client_remote_player_position_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, real x, real y, real z)
{
    halo::networking::PlayerUpdateClient::remote_player_position_update_from_network(player_index, update_id, control_sequence, x, y, z);
}

void player_update_client_remote_player_total_biped_update_from_network(int32_t **decode_context)
{
    halo::networking::PlayerUpdateClient::remote_player_total_biped_update_from_network(decode_context);
}

void player_update_client_remote_player_total_vehicle_update_from_network(int32_t **decode_context)
{
    halo::networking::PlayerUpdateClient::remote_player_total_vehicle_update_from_network(decode_context);
}

void player_update_client_remote_player_vehicle_position_delta_from_network(int32_t **decode_context)
{
    halo::networking::PlayerUpdateClient::remote_player_vehicle_position_delta_from_network(decode_context);
}

void player_update_client_remote_player_vehicle_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, vehicle_update_body vehicle)
{
    halo::networking::PlayerUpdateClient::remote_player_vehicle_update_from_network(player_index, update_id, control_sequence, vehicle);
}

}

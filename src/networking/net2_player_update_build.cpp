/**
 * @file src/networking/net2_player_update_build.cpp
 * Builders and ordering checks for player update packets.
 */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include <stdint.h>
#include "units.h"
#include <string.h>
#include "halo/networking/net2_player_update_build.hpp"

extern "C" {
extern int32_t network_ack_resend_interval_ms;
extern game_time_globals * game_time;
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern void network_player_update_history_log_write(const char *format, ...);
extern data_array * object_data;
extern network_id_table * object_network_id_table;
extern int32_t network_vehicle_ack_resend_interval_ms;
extern int32_t hash_table_get(hash_table *table, uint32_t key);
extern data_array * player_data;
extern void * machine_table;
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority);
extern double sin(double x);
extern double cos(double x);
extern uint8_t network_broadcast_event_feed_mode;
extern int32_t network_action_resend_interval_ms;
extern int32_t network_action_resend_interval_ms_alt;
extern void * data_iterator_next(data_iterator *iterator);
extern network_machine * network_machine_find_by_id(network_server_globals *server,
    int32_t machine_id);
extern void network_event_feed_queue_append(uint8_t *queue, uint32_t *key,
    uint32_t *payload);
extern int32_t network_transform_resend_interval_ms;
extern int32_t network_vehicle_transform_resend_interval_ms_alt;
extern int32_t network_attachment_transform_resend_interval_ms_alt;
extern network_server_globals * network_server;
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);
extern uint8_t player_unit_has_parent(datum_index player_handle);
extern double atan2(double y, double x);
extern double sqrt(double x);
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...);
extern uint8_t circular_queue_push(circular_queue *queue, void *record);
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...);
int32_t build_local_player_position_update(uint8_t *out_changed, player *plr);
int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr);
void build_player_full_resync_update(uint32_t player_index);
void build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control);
void build_remote_player_transform_update(uint32_t player_index, player_action *control,
    int32_t network_key);
void build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key);
void build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key);
void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline);
uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id);
uint8_t is_remote_player_update_in_order(player *plr, uint8_t new_update_id, int32_t update_id);
}


namespace halo::networking {

int32_t PlayerUpdateBuilder::build_local_player_position_update(uint8_t *out_changed, player *plr)
{
    int32_t encoded_size;
    uint8_t *plr_bytes;
    local_player_update_ack ack;
    void *ack_ptr;
    uint32_t next_id;
    uint32_t logged_id;

    encoded_size = 0;
    *out_changed = 0;
    plr_bytes = (uint8_t *)plr;
    if (-1 < *(int32_t *)(plr_bytes + 0xf4) && *(int32_t *)(plr_bytes + 0xf4) < 0x40) {
        ack.update_id = *(uint8_t *)(plr_bytes + 0xe8);
        ack.baseline_id = *(uint8_t *)(plr_bytes + 0xf4);
        *(uint32_t *)&ack.position.x = *(uint32_t *)(plr_bytes + 0xf8);
        *(uint32_t *)&ack.position.y = *(uint32_t *)(plr_bytes + 0xfc);
        *(uint32_t *)&ack.position.z = *(uint32_t *)(plr_bytes + 0x100);
        if (*(int32_t *)(plr_bytes + 0xec) == -1 ||
            *(int32_t *)(plr_bytes + 0xf0) + network_ack_resend_interval_ms <=
                game_time->game_time) {
            ack_ptr = &ack;
            encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x23, 0, &ack_ptr, 0, 1, '\0');
            *out_changed = 0;
            next_id = (*(uint32_t *)(plr_bytes + 0xe8) + 1) & 0x8000001f;
            if ((int32_t)next_id < 0) {
                next_id = (next_id - 1 | 0xffffffe0) + 1;
            }
            logged_id = ack.baseline_id;
            *(uint32_t *)(plr_bytes + 0xe8) = next_id;
            network_player_update_history_log_write("[%d]: [%d]:\t Acked [%d]\n", GetTickCount(),
                game_time->game_time, logged_id);
            *(int32_t *)(plr_bytes + 0xec) = *(int32_t *)(plr_bytes + 0xf4);
            *(int32_t *)(plr_bytes + 0xf0) = game_time->game_time;
        }
    }
    return encoded_size;
}

int32_t PlayerUpdateBuilder::build_local_player_vehicle_update(uint8_t *out_changed, player *plr)
{
    uint8_t *plr_bytes;
    local_player_vehicle_update_ack ack;
    object *unit_obj;
    object *vehicle_obj;
    datum_index parent_object;
    int32_t network_hash;
    int32_t encoded_size;
    uint32_t next_id;
    uint32_t logged_id;
    void *ack_ptr;

    *out_changed = 0;
    plr_bytes = (uint8_t *)plr;
    if (*(int32_t *)(plr_bytes + 0xf4) < 0 || 0x3f < *(int32_t *)(plr_bytes + 0xf4)) {
        return 0;
    }
    ack.update_id = *(uint8_t *)(plr_bytes + 0xe8);
    ack.baseline_id = *(uint8_t *)(plr_bytes + 0xf4);
    unit_obj = ((object_header *)object_data->data)[*(uint32_t *)(plr_bytes + 0x34) & 0xffff].data;
    parent_object = unit_obj->parent_object;
    vehicle_obj = ((object_header *)object_data->data)[parent_object & 0xffff].data;
    network_hash = 0;
    if (parent_object != (datum_index)-1) {
        network_hash = hash_table_get(&object_network_id_table->id_to_index,
            parent_object);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ack.vehicle.parent_or_tag = network_hash;
    *(uint32_t *)&ack.vehicle.position.x = *(uint32_t *)(plr_bytes + 0xf8);
    *(uint32_t *)&ack.vehicle.position.y = *(uint32_t *)(plr_bytes + 0xfc);
    *(uint32_t *)&ack.vehicle.position.z = *(uint32_t *)(plr_bytes + 0x100);
    ack.vehicle.velocity = vehicle_obj->velocity;
    ack.vehicle.angular_velocity = vehicle_obj->angular_velocity;
    ack.vehicle.forward = vehicle_obj->forward;
    ack.vehicle.up = vehicle_obj->up;

    if (*(int32_t *)(plr_bytes + 0xec) != -1 &&
        (uint32_t)game_time->game_time <
            (uint32_t)(*(int32_t *)(plr_bytes + 0xf0) + network_vehicle_ack_resend_interval_ms)) {
        return 0;
    }
    ack_ptr = &ack;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x24, 0, &ack_ptr, 0, 1, '\0');
    *out_changed = 0;
    next_id = (*(uint32_t *)(plr_bytes + 0xe8) + 1) & 0x8000001f;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xffffffe0) + 1;
    }
    logged_id = ack.baseline_id;
    *(uint32_t *)(plr_bytes + 0xe8) = next_id;
    network_player_update_history_log_write("[%d]: [%d]:\t Acked vehicle [%d]\n", GetTickCount(),
        game_time->game_time, logged_id);
    *(int32_t *)(plr_bytes + 0xec) = *(int32_t *)(plr_bytes + 0xf4);
    *(int32_t *)(plr_bytes + 0xf0) = game_time->game_time;
    return encoded_size;
}

void PlayerUpdateBuilder::build_player_full_resync_update(uint32_t player_index)
{
    uint8_t *cache;
    uint32_t staged12[12];
    uint32_t staged16[16];
    int32_t network_hash;
    struct { uint8_t update_id; uint8_t baseline_id; } header;
    int32_t body[3];
    int32_t encoded_size;
    void *items_ptr;
    void *previous_ptr;
    int32_t i;

    cache = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;

    header.update_id = *(uint8_t *)(cache + 0x128);
    header.baseline_id = *(uint8_t *)(cache + 300);
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    for (i = 0; i < 12; i = i + 1) {
        staged12[i] = *(uint32_t *)(cache + 0x130 + i * 4);
    }
    items_ptr = staged12;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x25, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = *(uint8_t *)(cache + 0x16c);
    header.baseline_id = 0;
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    body[0] = *(int32_t *)(cache + 0x170);
    body[1] = *(int32_t *)(cache + 0x174);
    body[2] = *(int32_t *)(cache + 0x178);
    items_ptr = body;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x27, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = *(uint8_t *)(cache + 0x16c);
    header.baseline_id = 0;
    network_hash = 0;
    if (player_index != 0xffffffff) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), player_index);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    for (i = 0; i < 16; i = i + 1) {
        staged16[i] = *(uint32_t *)(cache + 0x188 + i * 4);
    }
    items_ptr = staged16;
    previous_ptr = &network_hash;
    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x28, (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 1);
}

void PlayerUpdateBuilder::run_build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control)
{
    uint8_t *cache;
    uint32_t network_hash;
    uint8_t staged_update_id;
    uint32_t staged[12];
    real direction_x, direction_y, direction_z;
    uint32_t now;
    uint8_t is_full;
    uint8_t skip_delta;
    int32_t encoded_size;
    void *items_ptr;
    void *previous_ptr;
    int32_t previous_offset;
    uint32_t next_id;
    int32_t i;
    data_iterator iter;
    player *candidate;
    network_machine *machine;

    cache = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    staged_update_id = update_id_byte;

    if (network_broadcast_event_feed_mode == 0) {
        network_hash = 0;
        if (player_index != 0xffffffff) {
            network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), network_key);
            if (network_hash == 0xffffffff) {
                network_hash = 0;
            }
        }
    } else {
        network_hash = player_index;
    }

    ((uint8_t *)staged)[0] = (uint8_t)update_id_byte;
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)&control)[i];
    }

    direction_x = (real)(cos((double)control.desired_pitch) * cos((double)control.desired_yaw));
    direction_y = (real)(cos((double)control.desired_pitch) * sin((double)control.desired_yaw));
    direction_z = (real)sin((double)control.desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;

    now = (uint32_t)game_time->game_time;
    if (now < (uint32_t)(network_action_resend_interval_ms + *(int32_t *)(cache + 0x124)) &&
        *(int32_t *)(cache + 0x124) != -1) {
        if (now < (uint32_t)(*(int32_t *)(cache + 0x120) + network_action_resend_interval_ms_alt)) {
            is_full = 0;
            goto encode;
        }
        skip_delta = 1;
        *(uint32_t *)(cache + 0x120) = now;
    } else {
        for (i = 0; i < 12; i = i + 1) {
            *(uint32_t *)(cache + 0x130 + i * 4) = staged[i];
        }
        *(int32_t *)(cache + 0x124) = game_time->game_time;
        *(int32_t *)(cache + 0x120) = game_time->game_time;
        next_id = (*(uint8_t *)(cache + 300) + 1) & 0x80000001;
        skip_delta = 1;
        *(uint32_t *)(cache + 0x128) = staged_update_id;
        if ((int32_t)next_id < 0) {
            next_id = (next_id - 1 | 0xfffffffe) + 1;
        }
        staged_update_id = (uint8_t)next_id;
        *(uint8_t *)(cache + 300) = (uint8_t)next_id;
    }
    is_full = 1;

encode:
    if (network_broadcast_event_feed_mode == 0) {
        if (is_full) {
            if (skip_delta != 1) {
                items_ptr = (void *)(cache + 0x130);
                previous_ptr = &network_hash;
                previous_offset = 0;
                encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, skip_delta, 0x25,
                    (int32_t)&previous_ptr, &items_ptr, previous_offset, 1, skip_delta);
            } else {
                items_ptr = staged;
                previous_ptr = &network_hash;
                encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, skip_delta, 0x25,
                    (int32_t)&items_ptr, &previous_ptr, 0, 1, skip_delta);
            }
            if (0 < encoded_size) {
                iter.data = player_data;
                iter.next_index = 0;
                iter.index = k_datum_index_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                candidate = (player *)data_iterator_next(&iter);
                while (candidate != 0) {

                    if (candidate->local_player_index == -1) {
                        machine = network_machine_find_by_id(0, 0);
                        if (machine != 0 &&
                            ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 1 & 1) != 0) &&
                            ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 2 & 1) != 0)) {
                            network_session_send_to_machine(1, 0, encoded_size, is_full, 0, 0, 1);
                        }
                    }
                    candidate = (player *)data_iterator_next(&iter);
                }
            }
        }
    } else if (is_full) {
        network_event_feed_queue_append((uint8_t *)staged, 0, 0);
    }
}

void PlayerUpdateBuilder::build_remote_player_transform_update(uint32_t player_index, player_action *control,
    int32_t network_key)
{
    int32_t (*const build_remote_player_vehicle_update)(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key) = reinterpret_cast<int32_t (*)(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)>(&::build_remote_player_vehicle_update);
    int32_t (*const build_remote_player_vehicle_attachment_update)(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key) = reinterpret_cast<int32_t (*)(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)>(&::build_remote_player_vehicle_attachment_update);
    uint8_t *plr;
    uint8_t *cache;
    int32_t update_id;
    object *unit_obj;
    int32_t encoded_size;
    uint8_t is_full;
    uint32_t now;
    data_iterator iter;
    player *candidate;
    int32_t i;
    int16_t *machine_id_slot;
    network_machine *machine;

    plr = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    update_id = *(int32_t *)(plr + 0xf4);
    cache = plr;
    encoded_size = 0;

    if (-1 < update_id && update_id < 0x40) {
        unit_obj = object_try_and_get(*(datum_index *)(plr + 0x34), _object_mask_unit);
        if (unit_obj != 0) {
            if (unit_obj->parent_object == (datum_index)-1) {
                now = (uint32_t)game_time->game_time;
                if (now < (uint32_t)(network_transform_resend_interval_ms + *(int32_t *)(cache + 0x168)) &&
                    *(int32_t *)(cache + 0x168) != -1) {
                    if (now < (uint32_t)(*(int32_t *)(cache + 0x164) +
                            network_vehicle_transform_resend_interval_ms_alt)) {
                        goto fallback;
                    }
                    is_full = 0;
                } else {
                    is_full = 1;
                }
                encoded_size = build_remote_player_vehicle_update(cache, 0, is_full, is_full,
                    control, network_key);
            } else {
                if (player_unit_has_parent(*(datum_index *)(plr + 0x34)) != 1) {
                    goto fallback;
                }
                now = (uint32_t)game_time->game_time;
                if (now < (uint32_t)(network_transform_resend_interval_ms + *(int32_t *)(cache + 0x180)) &&
                    *(int32_t *)(cache + 0x180) != -1) {
                    if (now < (uint32_t)(*(int32_t *)(cache + 0x17c) +
                            network_attachment_transform_resend_interval_ms_alt)) {
                        goto fallback;
                    }
                    is_full = 0;
                } else {
                    is_full = 1;
                }
                encoded_size = build_remote_player_vehicle_attachment_update(cache, 0, is_full,
                    is_full, control, network_key);
            }

            if (0 < encoded_size) {
                iter.data = player_data;
                iter.next_index = 0;
                iter.index = k_datum_index_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                candidate = (player *)data_iterator_next(&iter);
                while (candidate != 0) {
                    if (player_index != 0xffffffff && candidate->local_player_index == -1) {
                        machine_id_slot = (int16_t *)((uint8_t *)network_server + 0x3c4);
                        for (i = 0; i < 0x10; i = i + 1) {
                            if (machine_id_slot[i * 0x30] ==
                                (int16_t)*(char *)&((struct player *)candidate)->machine_index) {
                                machine = (network_machine *)((uint8_t *)network_server + 0x3b8 +
                                    i * 0x60);
                                if (((*(uint16_t *)((uint8_t *)machine + 0xe) >> 1 & 1) != 0) &&
                                    ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 2 & 1) != 0)) {
                                    network_session_send_to_machine(1, 0, encoded_size, 1, 0, 0, 1);
                                }
                                break;
                            }
                        }
                    }
                    candidate = (player *)data_iterator_next(&iter);
                }
                now = (*(uint32_t *)(cache + 0x160) + 1) & 0x80000007;
                if ((int32_t)now < 0) {
                    now = (now - 1 | 0xfffffff8) + 1;
                }
                *(uint32_t *)(cache + 0x160) = now;
                return;
            }
            return;
        }
    }

fallback:
    build_remote_player_action_update(player_index, network_key, 0, *control);
}

void PlayerUpdateBuilder::run_build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)
{
    int32_t network_hash;
    uint32_t staged[12];
    uint8_t staged_update_id;
    uint32_t next_id;
    real direction_x, direction_y, direction_z;
    int32_t i;
    object *unit_obj;
    object *vehicle_obj;
    datum_index parent_object;
    int32_t vehicle_hash;
    uint32_t vehicle_state[16];
    int32_t previous_state[12];
    void *items_ptr;
    void *previous_ptr;

    staged_update_id = update_id;
    network_hash = 0;
    if (network_key != -1) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), network_key);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ((uint8_t *)staged)[0] = flags;
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)control)[i];
    }

    direction_x = (real)(cos((double)control->desired_pitch) * cos((double)control->desired_yaw));
    direction_y = (real)(cos((double)control->desired_pitch) * sin((double)control->desired_yaw));
    direction_z = (real)sin((double)control->desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;

    unit_obj = ((object_header *)object_data->data)[*(uint32_t *)(cache + 0x34) & 0xffff].data;
    parent_object = unit_obj->parent_object;
    vehicle_obj = ((object_header *)object_data->data)[parent_object & 0xffff].data;
    vehicle_hash = 0;
    if (parent_object != (datum_index)-1) {
        vehicle_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), parent_object);
        if (vehicle_hash == -1) {
            vehicle_hash = 0;
        }
    }
    vehicle_state[0] = vehicle_hash;
    vehicle_state[1] = *(uint32_t *)(cache + 0xf8);
    vehicle_state[2] = *(uint32_t *)(cache + 0xfc);
    vehicle_state[3] = *(uint32_t *)(cache + 0x100);
    *(real_vector3d *)&vehicle_state[4] = vehicle_obj->velocity;
    *(real_vector3d *)&vehicle_state[7] = vehicle_obj->angular_velocity;
    *(real_vector3d *)&vehicle_state[10] = vehicle_obj->forward;
    *(real_vector3d *)&vehicle_state[13] = vehicle_obj->up;

    if (is_full == '\x01') {

        for (i = 0; i < 12; i = i + 1) {
            *(uint32_t *)(cache + 0x130 + i * 4) = staged[i];
        }

        *(int32_t *)(cache + 0x124) = game_time->game_time;
        next_id = (*(uint8_t *)(cache + 300) + 1) & 0x80000001;
        *(int32_t *)(cache + 0x120) = game_time->game_time;
        *(uint32_t *)(cache + 0x128) = update_id;
        if ((int32_t)next_id < 0) {
            next_id = (next_id - 1 | 0xfffffffe) + 1;
        }
        staged_update_id = (uint8_t)next_id;
        *(uint8_t *)(cache + 300) = (uint8_t)next_id;
        previous_ptr = &network_hash;
        items_ptr = staged;
        message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x2a, (int32_t)&previous_ptr, &items_ptr, 0, 1, '\0');
        for (i = 0; i < 16; i = i + 1) {
            *(uint32_t *)(cache + 0x188 + i * 4) = vehicle_state[i];
        }
        *(int32_t *)(cache + 0x180) = game_time->game_time;
        *(uint32_t *)(cache + 0x184) = staged_update_id;
        *(int32_t *)(cache + 0x17c) = game_time->game_time;
        return;
    }

    for (i = 0; i < 12; i = i + 1) {
        previous_state[i] = *(int32_t *)(cache + 0x130 + i * 4);
    }
    items_ptr = staged;
    previous_ptr = previous_state;
    message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x2a, (int32_t)&previous_ptr, &items_ptr,
        (int32_t)vehicle_state, 1, '\x01');
    *(int32_t *)(cache + 0x120) = game_time->game_time;
    *(int32_t *)(cache + 0x17c) = game_time->game_time;
}

void PlayerUpdateBuilder::run_build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key)
{
    int32_t network_hash;
    uint32_t staged[27];
    uint8_t staged_update_id;
    uint32_t next_id;
    real direction_x, direction_y, direction_z;
    int32_t i;
    int32_t previous_state[15];
    void *items_ptr;
    void *previous_ptr;

    staged_update_id = update_id;
    network_hash = 0;
    if (network_key != -1) {
        network_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0x0c), network_key);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ((uint8_t *)staged)[0] = flags;
    for (i = 0; i < 8; i = i + 1) {
        staged[i] = ((uint32_t *)control)[i];
    }

    direction_x = (real)(cos((double)control->desired_pitch) * cos((double)control->desired_yaw));
    direction_y = (real)(cos((double)control->desired_pitch) * sin((double)control->desired_yaw));
    direction_z = (real)sin((double)control->desired_pitch);
    *(real *)&staged[9] = direction_x;
    *(real *)&staged[10] = direction_y;
    *(real *)&staged[11] = direction_z;
    staged[12] = *(uint32_t *)(cache + 0xf8);
    staged[13] = *(uint32_t *)(cache + 0xfc);
    staged[14] = *(uint32_t *)(cache + 0x100);

    if (is_full == '\x01') {
        for (i = 0; i < 12; i = i + 1) {
            staged[15 + i] = *(uint32_t *)(cache + 0x130 + i * 4);
        }
        *(int32_t *)(cache + 0x124) = game_time->game_time;
        next_id = (*(uint8_t *)(cache + 300) + 1) & 0x80000001;
        *(int32_t *)(cache + 0x120) = game_time->game_time;
        *(uint32_t *)(cache + 0x128) = update_id;
        if ((int32_t)next_id < 0) {
            next_id = (next_id - 1 | 0xfffffffe) + 1;
        }
        staged_update_id = (uint8_t)next_id;
        *(uint8_t *)(cache + 300) = (uint8_t)next_id;
        previous_ptr = &network_hash;
        items_ptr = staged;
        message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x29, (int32_t)&previous_ptr, &items_ptr, 0, 1, '\0');
        *(uint32_t *)(cache + 0x170) = staged[12];
        *(uint32_t *)(cache + 0x174) = staged[13];
        *(uint32_t *)(cache + 0x178) = staged[14];
        *(int32_t *)(cache + 0x168) = game_time->game_time;
        *(uint32_t *)(cache + 0x16c) = staged_update_id;
        *(int32_t *)(cache + 0x164) = game_time->game_time;
        return;
    }

    for (i = 0; i < 12; i = i + 1) {
        previous_state[i] = *(int32_t *)(cache + 0x130 + i * 4);
    }
    previous_state[12] = *(int32_t *)(cache + 0x170);
    previous_state[13] = *(int32_t *)(cache + 0x174);
    previous_state[14] = *(int32_t *)(cache + 0x178);
    items_ptr = staged;
    previous_ptr = previous_state;
    message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x29, (int32_t)&previous_ptr, &items_ptr,
        (int32_t)&previous_state, 1, '\x01');
    *(int32_t *)(cache + 0x120) = game_time->game_time;
    *(int32_t *)(cache + 0x164) = game_time->game_time;
}

void PlayerUpdateBuilder::handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline)
{
    int32_t handle;
    int16_t index;
    int16_t salt;
    int32_t stride_offset;
    player *candidate;
    uint8_t baseline_id;
    uint8_t action_index;

    handle = header->player_index;
    candidate = 0;
    if (handle != -1) {
        index = (int16_t)handle;
        if (index >= 0 && index < player_data->maximum_count) {
            salt = (int16_t)((uint32_t)handle >> 16);
            stride_offset = (int32_t)player_data->size * (int32_t)index;
            candidate = (player *)((uint8_t *)player_data->data + stride_offset);
            if (candidate->identifier == 0 || (salt != 0 && candidate->identifier != salt)) {
                candidate = 0;
            }
        }
    }
    if (candidate == 0) {
        return;
    }

    if (is_baseline == 1) {
        baseline_id = header->baseline_id;
        candidate->baseline_update_id = baseline_id;
        memcpy(&candidate->unknown_f0, control_source, sizeof(*control_source));
    } else {
        int32_t distance;

        baseline_id = header->baseline_id;
        if ((uint32_t)baseline_id != candidate->baseline_update_id) {
            player_update_history_log_printf_filtered(candidate, 2,
                "[%d]: Threw away remote player action update with base baseline, [%d] != [%d].",
                game_time->game_time, baseline_id, candidate->baseline_update_id);
            return;
        }
        action_index = header->update_id;
        if ((int32_t)action_index <= candidate->last_update_id) {
            distance = ((int32_t)action_index - candidate->last_update_id) + 0x40;
        } else {
            distance = (int32_t)action_index - candidate->last_update_id;
        }
        if (distance >= 0x20) {
            return;
        }
    }

    action_index = header->update_id;
    if (candidate->last_update_id != -1) {
        float x = control_source->direction.i;
        float y = control_source->direction.j;
        float z = control_source->direction.k;

        control_source->yaw = (float)atan2(y, x);
        control_source->pitch = (float)atan2(z, sqrt(x * x + y * y));

        control_source->unknown_20 = (uint16_t)stride_offset;
        player_update_history_log_printf_filtered(candidate, 2, "Received action [%d]", action_index);

        if (candidate->last_update_id != -1) {
            uint32_t record[11];
            uint8_t first_byte = (uint8_t)control_source->flags;

            record[0] = action_index;
            record[1] = first_byte;
            record[2] = first_byte;
            memcpy(&record[3], &control_source->unknown_04, sizeof(uint32_t) * 8);
            if (!circular_queue_push((circular_queue *)&candidate->update_history, record)) {
                player_update_history_log_printf_filtered(candidate, 2,
                    "[%d]: Remote player action_queue overflow.\n",
                    game_time->game_time);
            }
        }
    }
    candidate->last_update_id = action_index;
}

uint8_t PlayerUpdateBuilder::is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id)
{
    if (current_update_id != -1) {
        if (current_update_id < new_update_id) {
            if (0xf < new_update_id - current_update_id) {
                player_update_history_log_write(1, 0,
                    "[%d]a: Threw away local player ack [%d] (%d).\n",
                    game_time->game_time);
                return 0;
            }
        } else if (0xf < (new_update_id - current_update_id) + 0x20) {
            player_update_history_log_write(1, 0,
                "[%d]b: Threw away local player ack [%d] (%d).\n",
                game_time->game_time);
            return 0;
        }
    }
    return 1;
}

uint8_t PlayerUpdateBuilder::is_remote_player_update_in_order(player *plr, uint8_t new_update_id, int32_t update_id)
{
    void (*const player_update_history_log_printf_filtered)(player *target_player, int32_t unused_arg,
    const char *format, ...) = reinterpret_cast<void (*)(player *target_player, int32_t unused_arg,
    const char *format, ...)>(&::player_update_history_log_printf_filtered);
    int32_t previous_id;
    int32_t delta;
    uint8_t new_id_byte;

    previous_id = plr->last_remote_update_id;
    if (previous_id == -1) {
        return 1;
    }
    new_id_byte = (uint8_t)new_update_id;
    delta = new_id_byte - previous_id;
    if (previous_id < new_id_byte) {
        if (3 < delta) {
            player_update_history_log_printf_filtered(plr, 1,
                "[%d]a: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
                game_time->game_time, update_id, plr->last_position_update_id, (int32_t)new_id_byte, previous_id);
            return 0;
        }
    } else if (3 < delta + 8) {
        player_update_history_log_printf_filtered(plr, 1,
            "[%d]b: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
            game_time->game_time, update_id, plr->last_position_update_id, (int32_t)new_id_byte, previous_id);
        return 0;
    }
    return 1;
}

}  // namespace halo::networking

extern "C" {
int32_t build_local_player_position_update(uint8_t *out_changed, player *plr)
{
    return halo::networking::PlayerUpdateBuilder::build_local_player_position_update(out_changed, plr);
}

int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr)
{
    return halo::networking::PlayerUpdateBuilder::build_local_player_vehicle_update(out_changed, plr);
}

void build_player_full_resync_update(uint32_t player_index)
{
    halo::networking::PlayerUpdateBuilder::build_player_full_resync_update(player_index);
}

void build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control)
{
    halo::networking::PlayerUpdateBuilder::run_build_remote_player_action_update(player_index, network_key, update_id_byte, control);
}

void build_remote_player_transform_update(uint32_t player_index, player_action *control,
    int32_t network_key)
{
    halo::networking::PlayerUpdateBuilder::build_remote_player_transform_update(player_index, control, network_key);
}

void build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)
{
    halo::networking::PlayerUpdateBuilder::run_build_remote_player_vehicle_attachment_update(cache, update_id, flags, is_full, control, network_key);
}

void build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key)
{
    halo::networking::PlayerUpdateBuilder::run_build_remote_player_vehicle_update(cache, update_id, flags, is_full, control, network_key);
}

void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline)
{
    halo::networking::PlayerUpdateBuilder::handle_remote_player_action_update(control_source, header, is_baseline);
}

uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id)
{
    return halo::networking::PlayerUpdateBuilder::is_local_player_update_in_order(current_update_id, new_update_id);
}

uint8_t is_remote_player_update_in_order(player *plr, uint8_t new_update_id, int32_t update_id)
{
    return halo::networking::PlayerUpdateBuilder::is_remote_player_update_in_order(plr, new_update_id, update_id);
}

}

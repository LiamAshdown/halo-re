/**
 * @file src/networking/net2_player_update_build.cpp
 * Builders and ordering checks for player update packets.
 */
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
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
#include "halo/memory/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/units/api.hpp"
#include "halo/platform/time.hpp"

static auto &network_ack_resend_interval_ms = halo::link::ref<int32_t>(halo::networking::vars().network_ack_resend_interval_ms);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &network_vehicle_ack_resend_interval_ms = halo::link::ref<int32_t>(halo::networking::vars().network_vehicle_ack_resend_interval_ms);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &network_broadcast_event_feed_mode = halo::link::ref<uint8_t>(halo::networking::vars().network_broadcast_event_feed_mode);
static auto &network_action_resend_interval_ms = halo::link::ref<int32_t>(halo::networking::vars().network_action_resend_interval_ms);
static auto &network_action_resend_interval_ms_alt = halo::link::ref<int32_t>(halo::networking::vars().network_action_resend_interval_ms_alt);
static auto &network_transform_resend_interval_ms = halo::link::ref<int32_t>(halo::networking::vars().network_transform_resend_interval_ms);
static auto &network_vehicle_transform_resend_interval_ms_alt = halo::link::ref<int32_t>(halo::networking::vars().network_vehicle_transform_resend_interval_ms_alt);
static auto &network_attachment_transform_resend_interval_ms_alt = halo::link::ref<int32_t>(halo::networking::vars().network_attachment_transform_resend_interval_ms_alt);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);


namespace halo::networking {

using halo::game::remote_player_update_cache;
using halo::game::remote_update_cache;

int32_t PlayerUpdateBuilder::local_player_position_update(uint8_t *out_changed, player *plr)
{
    int32_t encoded_size;
    local_player_update_ack ack;
    void *ack_ptr;
    uint32_t next_id;
    uint32_t logged_id;

    encoded_size = 0;
    *out_changed = 0;
    if (-1 < static_cast<int32_t>(plr->unknown_f4) && static_cast<int32_t>(plr->unknown_f4) < 0x40) {
        ack.update_id = (uint8_t)plr->last_update_id;
        ack.baseline_id = (uint8_t)plr->unknown_f4;
        *(uint32_t *)&ack.position.x = static_cast<uint32_t>(plr->unknown_f8);
        *(uint32_t *)&ack.position.y = static_cast<uint32_t>(plr->unknown_fc);
        *(uint32_t *)&ack.position.z = static_cast<uint32_t>(plr->unknown_100);
        if (static_cast<int32_t>(plr->baseline_update_id) == -1 ||
            plr->unknown_f0 + network_ack_resend_interval_ms <=
                halo::game::globals().game_time->game_time) {
            ack_ptr = &ack;
            encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::local_player_update), 0, &ack_ptr, 0, 1, '\0');
            *out_changed = 0;
            next_id = (static_cast<uint32_t>(plr->last_update_id) + 1) & 0x8000001f;
            if ((int32_t)next_id < 0) {
                next_id = (next_id - 1 | 0xffffffe0) + 1;
            }
            logged_id = ack.baseline_id;
            plr->last_update_id = static_cast<int32_t>(next_id);
            halo::networking::network_player_update_history_log_write("[%d]: [%d]:\t Acked [%d]\n", halo::platform::tick_milliseconds(),
                halo::game::globals().game_time->game_time, logged_id);
            plr->baseline_update_id = plr->unknown_f4;
            plr->unknown_f0 = halo::game::globals().game_time->game_time;
        }
    }
    return encoded_size;
}

int32_t PlayerUpdateBuilder::local_player_vehicle_update(uint8_t *out_changed, player *plr)
{
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
    if (static_cast<int32_t>(plr->unknown_f4) < 0 || 0x3f < static_cast<int32_t>(plr->unknown_f4)) {
        return 0;
    }
    ack.update_id = (uint8_t)plr->last_update_id;
    ack.baseline_id = (uint8_t)plr->unknown_f4;
    unit_obj = halo::game::object_at(static_cast<uint32_t>(plr->unit));
    parent_object = unit_obj->parent_object;
    vehicle_obj = halo::game::object_at(parent_object);
    network_hash = 0;
    if (parent_object != k_datum_index_none) {
        network_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index,
            parent_object);
        if (network_hash == -1) {
            network_hash = 0;
        }
    }
    ack.vehicle.parent_or_tag = network_hash;
    *(uint32_t *)&ack.vehicle.position.x = static_cast<uint32_t>(plr->unknown_f8);
    *(uint32_t *)&ack.vehicle.position.y = static_cast<uint32_t>(plr->unknown_fc);
    *(uint32_t *)&ack.vehicle.position.z = static_cast<uint32_t>(plr->unknown_100);
    ack.vehicle.velocity = vehicle_obj->velocity;
    ack.vehicle.angular_velocity = vehicle_obj->angular_velocity;
    ack.vehicle.forward = vehicle_obj->forward;
    ack.vehicle.up = vehicle_obj->up;

    if (static_cast<int32_t>(plr->baseline_update_id) != -1 &&
        (uint32_t)halo::game::globals().game_time->game_time <
            (uint32_t)(plr->unknown_f0 + network_vehicle_ack_resend_interval_ms)) {
        return 0;
    }
    ack_ptr = &ack;
    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::local_player_vehicle_update), 0, &ack_ptr, 0, 1, '\0');
    *out_changed = 0;
    next_id = (static_cast<uint32_t>(plr->last_update_id) + 1) & 0x8000001f;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xffffffe0) + 1;
    }
    logged_id = ack.baseline_id;
    plr->last_update_id = static_cast<int32_t>(next_id);
    halo::networking::network_player_update_history_log_write("[%d]: [%d]:\t Acked vehicle [%d]\n", halo::platform::tick_milliseconds(),
        halo::game::globals().game_time->game_time, logged_id);
    plr->baseline_update_id = plr->unknown_f4;
    plr->unknown_f0 = halo::game::globals().game_time->game_time;
    return encoded_size;
}

void PlayerUpdateBuilder::player_full_resync_update(uint32_t player_index)
{
    player *cache;
    uint32_t staged12[12];
    uint32_t staged16[16];
    // The original keeps one 12-byte remote_player_update_header on its stack (0x4e7d90, esp+0x14) and points the
    // encoder's header array at it; the player hash, update id and baseline id are its first three fields.
    remote_player_update_header header = {};
    int32_t body[3];
    int32_t encoded_size;
    void *items_ptr;
    void *previous_ptr;
    int32_t i;

    cache = halo::game::player_at(player_index);
    remote_player_update_cache &c = remote_update_cache(cache);

    header.update_id = (uint8_t)c.action_update_id;
    header.baseline_id = c.action_baseline_id;
    header.player_index = 0;
    if (player_index != halo::k_dword_none) {
        header.player_index = halo::objects::hash_table_get(&machine_table->id_to_index, player_index);
        if (header.player_index == -1) {
            header.player_index = 0;
        }
    }
    for (i = 0; i < 12; i = i + 1) {
        staged12[i] = c.action_baseline[i];
    }
    items_ptr = staged12;
    previous_ptr = &header;
    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::remote_player_action_update), (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    halo::networking::network_session_send_to_machine((int32_t)player_index, network_server, 1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = (uint8_t)c.biped_update_id;
    header.baseline_id = 0;
    header.player_index = 0;
    if (player_index != halo::k_dword_none) {
        header.player_index = halo::objects::hash_table_get(&machine_table->id_to_index, player_index);
        if (header.player_index == -1) {
            header.player_index = 0;
        }
    }
    body[0] = static_cast<int32_t>(c.biped_baseline[0]);
    body[1] = static_cast<int32_t>(c.biped_baseline[1]);
    body[2] = static_cast<int32_t>(c.biped_baseline[2]);
    items_ptr = body;
    previous_ptr = &header;
    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::remote_player_position_delta), (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    halo::networking::network_session_send_to_machine((int32_t)player_index, network_server, 1, network_message_scratch, encoded_size, 1, 0, 0, 1);

    header.update_id = (uint8_t)c.biped_update_id;
    header.baseline_id = 0;
    header.player_index = 0;
    if (player_index != halo::k_dword_none) {
        header.player_index = halo::objects::hash_table_get(&machine_table->id_to_index, player_index);
        if (header.player_index == -1) {
            header.player_index = 0;
        }
    }
    for (i = 0; i < 16; i = i + 1) {
        staged16[i] = c.vehicle_baseline[i];
    }
    items_ptr = staged16;
    previous_ptr = &header;
    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::remote_player_vehicle_position_delta), (int32_t)&previous_ptr,
        &items_ptr, 0, 1, '\0');
    halo::networking::network_session_send_to_machine((int32_t)player_index, network_server, 1, network_message_scratch, encoded_size, 1, 0, 0, 1);
}

// The remote player builders below hash the player's own datum through the machine table, stage a 12-dword action
// item (control-sequence byte, 8 control dwords, a unit aim direction), and hand the encoder three pointer arrays:
// one per item for the 12-byte remote_player_update_header (the static fields), one for the item itself, and in delta
// mode one for the baseline it is diffed against. Getting any of those arrays wrong sends garbage ids.
static int32_t remote_player_hash(uint32_t player_index)
{
    int32_t hash = 0;

    if (player_index != halo::k_dword_none) {
        hash = halo::objects::hash_table_get(&machine_table->id_to_index, static_cast<int32_t>(player_index));
        if (hash == -1) {
            hash = 0;
        }
    }
    return hash;
}

// staged[0] carries the control-sequence byte, staged[1..8] the eight control dwords and staged[9..11] the aim
// direction rebuilt from the control's yaw and pitch.
static void stage_remote_player_action(uint32_t *staged, uint8_t control_sequence, const player_action *control)
{
    staged[0] = 0;
    ((uint8_t *)staged)[0] = control_sequence;
    memcpy(&staged[1], control, 8 * sizeof(uint32_t));
    *(real *)&staged[9] = (real)(halo::libm::cos((double)control->desired_pitch) * halo::libm::cos((double)control->desired_yaw));
    *(real *)&staged[10] = (real)(halo::libm::cos((double)control->desired_pitch) * halo::libm::sin((double)control->desired_yaw));
    *(real *)&staged[11] = (real)halo::libm::sin((double)control->desired_pitch);
}

// Starts a new action baseline from staged: records it on the cache, stamps both action ticks, remembers update_id
// and returns the next baseline id (the ids alternate 0/1).
static uint8_t begin_action_baseline(remote_player_update_cache &c, const uint32_t *staged, uint8_t update_id)
{
    uint32_t next_id;

    memcpy(c.action_baseline, staged, sizeof(c.action_baseline));
    c.action_full_tick = halo::game::globals().game_time->game_time;
    c.action_delta_tick = halo::game::globals().game_time->game_time;
    c.action_update_id = update_id;
    next_id = (c.action_baseline_id + 1) & 0x80000001;
    if ((int32_t)next_id < 0) {
        next_id = (next_id - 1 | 0xfffffffe) + 1;
    }
    c.action_baseline_id = (uint8_t)next_id;
    return (uint8_t)next_id;
}

// A full update (previous == 0) is encoded stateless with force 0; a delta one against previous with force 1.
static int32_t encode_remote_player_update(int32_t message, remote_player_update_header *header, void *item,
    void *previous)
{
    void *header_ptr = header;
    void *item_ptr = item;
    void *previous_ptr = previous;
    uint8_t delta = previous != 0;

    return halo::networking::message_delta_encode_message((int32_t)network_message_scratch,
        halo::k_network_message_scratch_size, delta, message, (int32_t)&header_ptr, &item_ptr,
        delta ? (int32_t)&previous_ptr : 0, 1, (char)delta);
}

void PlayerUpdateBuilder::remote_player_action_update(uint32_t player_index, uint32_t update_id,
    uint8_t control_sequence, player_action control)
{
    player *cache;
    remote_player_update_header header = {};
    uint32_t staged[12];
    uint32_t now;
    uint8_t full = 1;
    int32_t encoded_size;
    data_iterator iter;
    player *candidate;
    network_machine *machine;

    cache = halo::game::player_at(player_index);
    remote_player_update_cache &c = remote_update_cache(cache);

    header.player_index = network_broadcast_event_feed_mode == 0 ? remote_player_hash(player_index)
                                                                   : static_cast<int32_t>(player_index);
    header.update_id = (uint8_t)update_id;
    header.baseline_id = c.action_baseline_id;
    // The original header is 8 bytes followed directly by staged[0], so the encoder reads this byte from there.
    header.control_sequence = control_sequence;
    stage_remote_player_action(staged, control_sequence, &control);

    now = (uint32_t)halo::game::globals().game_time->game_time;
    if (now < (uint32_t)(network_action_resend_interval_ms + c.action_full_tick) && c.action_full_tick != -1) {
        if (now < (uint32_t)(c.action_delta_tick + network_action_resend_interval_ms_alt)) {
            return;
        }
        full = 0;
        c.action_delta_tick = static_cast<int32_t>(now);
    } else {
        header.baseline_id = begin_action_baseline(c, staged, (uint8_t)update_id);
    }

    if (network_broadcast_event_feed_mode != 0) {
        halo::networking::network_event_feed_queue_append(
            full ? halo::game::fields::network_event_feed_a : halo::game::fields::network_event_feed_b,
            reinterpret_cast<uint32_t *>(&header), staged);
        return;
    }

    encoded_size = encode_remote_player_update(
        halo::networking::message_id(halo::networking::delta_message::remote_player_action_update), &header, staged,
        full ? 0 : c.action_baseline);
    if (encoded_size <= 0) {
        return;
    }
    iter.data = halo::game::globals().player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    while (candidate != 0) {
        if (iter.index != player_index && candidate->local_player_index == -1) {
            int32_t machine_id = *(int8_t *)&candidate->machine_index;

            machine = halo::networking::network_machine_find_by_id(network_server, machine_id);
            if (machine != 0 && ((machine->flags >> 1 & 1) != 0) && ((machine->flags >> 2 & 1) != 0)) {
                halo::networking::network_session_send_to_machine(machine_id, network_server, 1,
                    network_message_scratch, encoded_size, full, 0, 0, 1);
            }
        }
        candidate = (player *)halo::memory::data_iterator_next(&iter);
    }
}

void PlayerUpdateBuilder::remote_player_transform_update(uint32_t player_index, uint32_t update_id,
    uint8_t control_sequence, player_action control)
{
    player *plr;
    uint8_t *cache;
    object *unit_obj;
    int32_t encoded_size;
    uint8_t is_full;
    uint32_t now;
    data_iterator iter;
    player *candidate;
    int32_t i;
    network_machine *machine;

    plr = halo::game::player_at(player_index);
    cache = reinterpret_cast<uint8_t *>(plr);
    remote_player_update_cache &c = remote_update_cache(plr);

    if (-1 < static_cast<int32_t>(plr->unknown_f4) && static_cast<int32_t>(plr->unknown_f4) < 0x40) {
        unit_obj = halo::objects::object_try_and_get(plr->unit, _object_mask_unit);
        if (unit_obj != 0) {
            bool fall_back = false;

            encoded_size = 0;
            now = (uint32_t)halo::game::globals().game_time->game_time;
            if (unit_obj->parent_object == k_datum_index_none) {
                is_full = 1;
                if (now < (uint32_t)(network_transform_resend_interval_ms + c.biped_full_tick) && c.biped_full_tick != -1) {
                    fall_back = now < (uint32_t)(c.biped_delta_tick + network_vehicle_transform_resend_interval_ms_alt);
                    is_full = 0;
                }
                if (!fall_back) {
                    encoded_size = halo::networking::build_remote_player_vehicle_update(cache, (uint8_t)update_id,
                        control_sequence, is_full, &control, (int32_t)player_index);
                }
            } else if (halo::game::player_unit_has_parent(player_index) != 1) {
                fall_back = true;
            } else {
                is_full = 1;
                if (now < (uint32_t)(network_transform_resend_interval_ms + c.vehicle_full_tick) && c.vehicle_full_tick != -1) {
                    fall_back = now < (uint32_t)(c.vehicle_delta_tick + network_attachment_transform_resend_interval_ms_alt);
                    is_full = 0;
                }
                if (!fall_back) {
                    encoded_size = halo::networking::build_remote_player_vehicle_attachment_update(cache,
                        (uint8_t)update_id, control_sequence, is_full, &control, (int32_t)player_index);
                }
            }

            if (!fall_back && 0 < encoded_size) {
                iter.data = halo::game::globals().player_data;
                iter.next_index = 0;
                iter.index = k_datum_index_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                candidate = (player *)halo::memory::data_iterator_next(&iter);
                while (candidate != 0) {
                    if (iter.index != player_index && candidate->local_player_index == -1) {
                        for (i = 0; i < 0x10; i = i + 1) {
                            if (network_server->machines[i].machine_id == (int16_t)*(int8_t *)&candidate->machine_index) {
                                machine = &network_server->machines[i];
                                if (((machine->flags >> 1 & 1) != 0) && ((machine->flags >> 2 & 1) != 0)) {
                                    halo::networking::network_session_send_to_machine(machine->machine_id, network_server, 1,
                                        network_message_scratch, encoded_size, is_full, 0, 0, 1);
                                }
                                break;
                            }
                        }
                    }
                    candidate = (player *)halo::memory::data_iterator_next(&iter);
                }
                now = (c.position_counter + 1) & 0x80000007;
                if ((int32_t)now < 0) {
                    now = (now - 1 | 0xfffffff8) + 1;
                }
                c.position_counter = now;
                return;
            }
        }
    }

    halo::networking::build_remote_player_action_update(player_index, update_id, control_sequence, control);
}

int32_t PlayerUpdateBuilder::remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t control_sequence, char is_full, player_action *control, int32_t network_key)
{
    // One 28-dword item: the 12 action dwords, then the vehicle's network id, the player's three cached position
    // dwords and the vehicle's velocity, angular velocity, forward and up (0x4e86f0).
    uint32_t staged[28];
    uint32_t previous[28];
    remote_player_update_header header = {};
    object *unit_obj;
    object *vehicle_obj;
    datum_index parent_object;
    int32_t vehicle_hash;
    int32_t encoded_size;
    player *plr = (player *)cache;

    remote_player_update_cache &c = remote_update_cache(plr);
    header.player_index = remote_player_hash(static_cast<uint32_t>(network_key));
    header.update_id = update_id;
    header.baseline_id = c.action_baseline_id;
    header.control_sequence = (uint8_t)c.position_counter;
    stage_remote_player_action(staged, control_sequence, control);

    unit_obj = halo::game::object_at(static_cast<uint32_t>(plr->unit));
    parent_object = unit_obj->parent_object;
    vehicle_obj = halo::game::object_at(parent_object);
    vehicle_hash = 0;
    if (parent_object != k_datum_index_none) {
        // Vehicles are named by the object network id table, not the machine table the player hash uses.
        vehicle_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, parent_object);
        if (vehicle_hash == -1) {
            vehicle_hash = 0;
        }
    }
    staged[12] = static_cast<uint32_t>(vehicle_hash);
    staged[13] = static_cast<uint32_t>(plr->unknown_f8);
    staged[14] = static_cast<uint32_t>(plr->unknown_fc);
    staged[15] = static_cast<uint32_t>(plr->unknown_100);
    *(real_vector3d *)&staged[16] = vehicle_obj->velocity;
    *(real_vector3d *)&staged[19] = vehicle_obj->angular_velocity;
    *(real_vector3d *)&staged[22] = vehicle_obj->forward;
    *(real_vector3d *)&staged[25] = vehicle_obj->up;

    if (is_full == '\x01') {
        header.baseline_id = begin_action_baseline(c, staged, update_id);
        encoded_size = encode_remote_player_update(
            halo::networking::message_id(halo::networking::delta_message::remote_player_vehicle_update), &header, staged, 0);
        memcpy(c.vehicle_baseline, &staged[12], sizeof(c.vehicle_baseline));
        c.vehicle_full_tick = halo::game::globals().game_time->game_time;
        c.vehicle_update_id = update_id;
        c.vehicle_delta_tick = halo::game::globals().game_time->game_time;
        return encoded_size;
    }

    memcpy(previous, c.action_baseline, sizeof(c.action_baseline));
    memcpy(&previous[12], c.vehicle_baseline, sizeof(c.vehicle_baseline));
    encoded_size = encode_remote_player_update(
        halo::networking::message_id(halo::networking::delta_message::remote_player_vehicle_update), &header, staged, previous);
    c.action_delta_tick = halo::game::globals().game_time->game_time;
    c.vehicle_delta_tick = halo::game::globals().game_time->game_time;
    return encoded_size;
}

int32_t PlayerUpdateBuilder::remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t control_sequence,
    char is_full, player_action *control, int32_t network_key)
{
    // One 15-dword item: the 12 action dwords, then the player's three cached position dwords (0x4e84d0; it sends
    // remote_player_biped_update despite the name).
    uint32_t staged[15];
    uint32_t previous[15];
    remote_player_update_header header = {};
    int32_t encoded_size;
    player *plr = (player *)cache;

    remote_player_update_cache &c = remote_update_cache(plr);
    header.player_index = remote_player_hash(static_cast<uint32_t>(network_key));
    header.update_id = update_id;
    header.baseline_id = c.action_baseline_id;
    header.control_sequence = (uint8_t)c.position_counter;
    stage_remote_player_action(staged, control_sequence, control);
    staged[12] = static_cast<uint32_t>(plr->unknown_f8);
    staged[13] = static_cast<uint32_t>(plr->unknown_fc);
    staged[14] = static_cast<uint32_t>(plr->unknown_100);

    if (is_full == '\x01') {
        header.baseline_id = begin_action_baseline(c, staged, update_id);
        encoded_size = encode_remote_player_update(
            halo::networking::message_id(halo::networking::delta_message::remote_player_biped_update), &header, staged, 0);
        memcpy(c.biped_baseline, &staged[12], sizeof(c.biped_baseline));
        c.biped_full_tick = halo::game::globals().game_time->game_time;
        c.biped_update_id = update_id;
        c.biped_delta_tick = halo::game::globals().game_time->game_time;
        return encoded_size;
    }

    memcpy(previous, c.action_baseline, sizeof(c.action_baseline));
    memcpy(&previous[12], c.biped_baseline, sizeof(c.biped_baseline));
    encoded_size = encode_remote_player_update(
        halo::networking::message_id(halo::networking::delta_message::remote_player_biped_update), &header, staged, previous);
    c.action_delta_tick = halo::game::globals().game_time->game_time;
    c.biped_delta_tick = halo::game::globals().game_time->game_time;
    return encoded_size;
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
        if (index >= 0 && index < halo::game::globals().player_data->maximum_count) {
            salt = (int16_t)((uint32_t)handle >> 16);
            stride_offset = (int32_t)halo::game::globals().player_data->size * (int32_t)index;
            candidate = (player *)((uint8_t *)halo::game::globals().player_data->data + stride_offset);
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
            halo::networking::player_update_history_log_printf_filtered(candidate, 2,
                "[%d]: Threw away remote player action update with base baseline, [%d] != [%d].",
                halo::game::globals().game_time->game_time, baseline_id, candidate->baseline_update_id);
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

        control_source->yaw = (float)halo::libm::atan2(y, x);
        control_source->pitch = (float)halo::libm::atan2(z, halo::libm::sqrt(x * x + y * y));

        control_source->unknown_20 = (uint16_t)stride_offset;
        halo::networking::player_update_history_log_printf_filtered(candidate, 2, "Received action [%d]", action_index);

        if (candidate->last_update_id != -1) {
            uint32_t record[11];
            uint8_t first_byte = (uint8_t)control_source->flags;

            record[0] = action_index;
            record[1] = first_byte;
            record[2] = first_byte;
            memcpy(&record[3], &control_source->unknown_04, sizeof(uint32_t) * 8);
            if (!halo::game::circular_queue_push((circular_queue *)&candidate->update_history, record)) {
                halo::networking::player_update_history_log_printf_filtered(candidate, 2,
                    "[%d]: Remote player action_queue overflow.\n",
                    halo::game::globals().game_time->game_time);
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
                halo::networking::player_update_history_log_write(1, 0,
                    "[%d]a: Threw away local player ack [%d] (%d).\n",
                    halo::game::globals().game_time->game_time);
                return 0;
            }
        } else if (0xf < (new_update_id - current_update_id) + 0x20) {
            halo::networking::player_update_history_log_write(1, 0,
                "[%d]b: Threw away local player ack [%d] (%d).\n",
                halo::game::globals().game_time->game_time);
            return 0;
        }
    }
    return 1;
}

uint8_t PlayerUpdateBuilder::is_remote_player_update_in_order(player *plr, uint8_t new_update_id, int32_t update_id)
{
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
            halo::networking::player_update_history_log_printf_filtered(plr, 1,
                "[%d]a: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
                halo::game::globals().game_time->game_time, update_id, plr->last_position_update_id, (int32_t)new_id_byte, previous_id);
            return 0;
        }
    } else if (3 < delta + 8) {
        halo::networking::player_update_history_log_printf_filtered(plr, 1,
            "[%d]b: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
            halo::game::globals().game_time->game_time, update_id, plr->last_position_update_id, (int32_t)new_id_byte, previous_id);
        return 0;
    }
    return 1;
}

}  // namespace halo::networking

namespace halo::networking {
int32_t build_local_player_position_update(uint8_t *out_changed, player *plr)
{
    return halo::networking::PlayerUpdateBuilder::local_player_position_update(out_changed, plr);
}

int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr)
{
    return halo::networking::PlayerUpdateBuilder::local_player_vehicle_update(out_changed, plr);
}

void build_player_full_resync_update(uint32_t player_index)
{
    halo::networking::PlayerUpdateBuilder::player_full_resync_update(player_index);
}

void build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control)
{
    halo::networking::PlayerUpdateBuilder::remote_player_action_update(player_index, network_key, update_id_byte, control);
}

void build_remote_player_transform_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control)
{
    halo::networking::PlayerUpdateBuilder::remote_player_transform_update(player_index, network_key, update_id_byte, control);
}

int32_t build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key)
{
    return halo::networking::PlayerUpdateBuilder::remote_player_vehicle_attachment_update(cache, update_id, flags, is_full, control, network_key);
}

int32_t build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key)
{
    return halo::networking::PlayerUpdateBuilder::remote_player_vehicle_update(cache, update_id, flags, is_full, control, network_key);
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

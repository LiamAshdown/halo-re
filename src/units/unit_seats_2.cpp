#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::units {

namespace unit_detach_child_at_named_seat_local {

static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = halo::objects::object_record_bytes(object_index);
    uint8_t *vehicle = halo::objects::object_record_bytes(vehicle_index);
    uint8_t *nodes = self + ((unit_object *)self)->base.nodes.offset;
    uint8_t *seat = (uint8_t *)((struct Unit *)halo::objects::tag_record_bytes(*(datum_index *)vehicle))->seats.pointer + ((unit_object *)self)->unit.vehicle_seat_index * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(halo::objects::tag_record_bytes(*(datum_index *)&((struct Unit *)halo::objects::tag_record_bytes(*(datum_index *)self))->base.model.tag_id) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index && (uint8_t)((struct unit_object *)vehicle)->unit.animation_state != 0x25 &&
        ((unit_object *)self)->base.parent_object != k_datum_index_none) {
        halo::units::UnitView(((unit_object *)self)->base.parent_object).try_set_animation_state(0x25);
    }
    ((unit_object *)self)->unit.last_parent_object_index = vehicle_index;
    ((unit_object *)self)->unit.last_seat_change_tick = halo::game::globals().game_time->game_time;
    if (((unit_object *)self)->unit.driver_unit_index == object_index) {
        ((unit_object *)self)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)self)->unit.gunner_unit_index == object_index) {
        ((unit_object *)self)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((unit_object *)self)->base.position.x;
    position.y = offset.y + ((unit_object *)self)->base.position.y;
    position.z = offset.z + ((unit_object *)self)->base.position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = halo::objects::object_record_bytes(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((unit_object *)self)->base.forward.i = basis.forward;
    *(real_vector3d *)&((unit_object *)self)->base.up.i = basis.up;
    {
        uint8_t *object = halo::objects::object_record_bytes(object_index);
        uint8_t *object_tag = halo::objects::tag_record_bytes(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
            halo::objects::object_header_of(object_index).flags |= 2;
        }
    }
    ((unit_object *)self)->unit.vehicle_seat_index = -1;
    ((struct unit_object *)self)->unit.base_animation_state = 2;
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    UnitView(vehicle_index).recompute_seat_occupants();
    UnitView(object_index).pick_and_ready_next_weapon();
    {
        int8_t request[2] = { 0x14, 0 };

        UnitView(object_index).update_animation_state_machine(request);
    }
    *(real_point3d *)(self + ((unit_object *)self)->base.node_function_values.offset + 0x10) = default_translation;
    if (((unit_object *)self)->base.type == 0) {
        UnitView(object_index).reset_orientation_and_find_position(vehicle_index);
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (UnitView(vehicle_index).all_seats_unoccupied() == 1) {
        uint8_t *empty = (uint8_t *)halo::objects::object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = halo::game::globals().game_time->game_time;
        }
    }
    if (halo::networking::globals().game_mode == 1) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(((unit_object *)self)->unit.controlling_player, halo::game::globals().player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}

static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = ((unit_object *)self)->unit.controlling_player;
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (halo::networking::globals().game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    player = (uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (halo::networking::globals().client != 0) {
        halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
    }
}

typedef struct unit_seat_iterator {
    uint32_t type_mask;
    uint8_t flags_mask;
    uint8_t unknown_05;
    int16_t index;
    datum_index handle;
    uint32_t signature;
} unit_seat_iterator;

}

/**
 * Engine function unit_detach_child_at_named_seat.
 *
 * @address 0x56ab50
 */
int16_t UnitView::detach_child_at_named_seat(char *seat_marker_name)
{
    using namespace unit_detach_child_at_named_seat_local;
    uint32_t unit_index = datum_handle;
    int16_t count = 0;
    uint8_t *unit_tag;
    uint8_t any_seat;
    unit_seat_iterator iterator;
    uint8_t *child;

    if (unit_index == k_datum_index_none) {
        return 0;
    }
    unit_tag = halo::objects::tag_record_bytes(*(datum_index *)halo::objects::object_record_bytes(unit_index));
    any_seat = (uint8_t)(seat_marker_name == 0 || seat_marker_name[0] == 0);
    iterator.signature = 0x86868686;
    iterator.type_mask = 3;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;
    for (child = (uint8_t *)halo::objects::object_iterator_next((object_iterator *)&iterator); child != 0;
         child = (uint8_t *)halo::objects::object_iterator_next((object_iterator *)&iterator)) {
        datum_index child_index;
        uint8_t *self;
        char label[0x100];
        char *c;

        if (*(datum_index *)(child + 0x11c) != unit_index) {
            continue;
        }
        strcpy(label, (char *)((uint8_t *)((struct Unit *)unit_tag)->seats.pointer + *(int16_t *)(child + 0x2f0) * 0x11c + 4));
        for (c = label; *c != 0; c++) {
            *c = (char)tolower((uint8_t)*c);
        }
        if (!any_seat && strstr(label, seat_marker_name) == 0) {
            continue;
        }
        child_index = iterator.handle;
        self = (uint8_t *)halo::objects::object_try_and_get(child_index, 3);
        if (self == 0 || halo::networking::globals().game_mode == 1 || ((unit_object *)self)->base.parent_object == k_datum_index_none ||
            ((unit_object *)self)->unit.vehicle_seat_index == -1) {
            continue;
        }
        if (((unit_object *)self)->base.type == 1) {
            uint8_t *obj = halo::objects::object_record_bytes(child_index);
            datum_index vehicle_index = ((unit_object *)obj)->base.parent_object;

            if (vehicle_index != k_datum_index_none && ((unit_object *)obj)->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(child_index, vehicle_index);
            }
            biped_free_local_player_history(halo::objects::object_record_bytes(child_index));
            continue;
        }
        if (!::halo::units::unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            uint8_t *self_tag = halo::objects::tag_record_bytes(*(datum_index *)self);
            datum_index graph = *(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id;
            uint8_t *seat_block = *(uint8_t **)(halo::objects::tag_record_bytes(graph) + 0x10) + (int8_t)(uint8_t)((struct unit_object *)self)->unit.animation_definition_index * 0x64;

            if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                datum_index vehicle_index = ((unit_object *)self)->base.parent_object;
                uint8_t *object;
                uint8_t *object_tag;

                if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == child_index) {
                    UnitView((int32_t)vehicle_index).notify_weapon_removed();
                }
                UnitView(child_index).set_custom_animation(*(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id, halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
                object = halo::objects::object_record_bytes(child_index);
                object_tag = halo::objects::tag_record_bytes(*(datum_index *)object);
                if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                    if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                        halo::objects::object_for_each_light_attachment(child_index, 0, 1);
                    }
                    if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                        clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                        halo::objects::object_header_of(child_index).flags |= 2;
                    }
                }
                ((struct unit_object *)self)->unit.animation_state = 0x1b;
                halo::ai::actor_notify_weapon_pickup_once(child_index);
                if (((unit_object *)self)->base.network_role == 0) {
                    ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)child_index);
                }
                count++;
            }
        }
    }
    return count;
}

}

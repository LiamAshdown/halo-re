#include "halo/units/seat_detach.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "game.h"
#include "networking.h"
#include "halo/units/animation_states.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/game/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/math/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/units/api.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/link.hpp"
#include <string.h>

namespace halo::units {

/**
 * Releases the local player update history a biped had been keeping when it leaves a seat or dies in one.
 */
void biped_free_local_player_history(unit_object *self)
{
    datum_index player_index = self->unit.controlling_player;
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);

    if (halo::networking::globals().game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    player *record = reinterpret_cast<player *>(static_cast<uint8_t *>(halo::game::globals().player_data->data) + halo::game::globals().player_data->size * index);
    if (record->identifier == 0 || (salt != 0 && record->identifier != salt) || record->local_player_index == -1) {
        return;
    }
    if (halo::networking::globals().client != 0) {
        halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
    }
}

/**
 * Detaches a biped from its seat in a vehicle: places it at the seat marker offset, copies the node orientation, clears the seat
 * bookkeeping on both units and readies the next weapon.
 */
void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    unit_object *self = halo::objects::object_as<unit_object>(object_index);
    unit_object *vehicle = halo::objects::object_as<unit_object>(vehicle_index);
    real_matrix4x3 *nodes = halo::objects::object_block<real_matrix4x3>(self->base, self->base.nodes);
    UnitSeat *seat = &halo::objects::block_element<UnitSeat>(halo::objects::tag_as<Unit>(vehicle->base.definition_tag)->seats, self->unit.vehicle_seat_index);
    GBXModel *model = halo::objects::tag_as<GBXModel>(halo::objects::tag_handle(halo::objects::tag_as<Unit>(self->base.definition_tag)->base.model));
    ModelNode *model_nodes = halo::objects::block_elements<ModelNode>(model->nodes);
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, seat->marker_name.string, &marker, 1);
    offset.x = nodes->position.x - marker.node_transform.position.x;
    offset.y = nodes->position.y - marker.node_transform.position.y;
    offset.z = nodes->position.z - marker.node_transform.position.z;
    default_translation = *reinterpret_cast<real_point3d *>(&model_nodes->default_translation);
    if (vehicle->unit.driver_unit_index == object_index && (uint8_t)vehicle->unit.animation_state != animation_state_value(unit_animation_state_id::unknown_25) &&
        self->base.parent_object != k_datum_index_none) {
        UnitView(self->base.parent_object).try_set_animation_state(animation_state_value(unit_animation_state_id::unknown_25));
    }
    self->unit.last_parent_object_index = vehicle_index;
    self->unit.last_seat_change_tick = halo::game::globals().game_time->game_time;
    if (self->unit.driver_unit_index == object_index) {
        self->unit.driver_unit_index = k_datum_index_none;
    }
    if (self->unit.gunner_unit_index == object_index) {
        self->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + self->base.position.x;
    position.y = offset.y + self->base.position.y;
    position.z = offset.z + self->base.position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        object *reloaded = halo::objects::object_as<object>(object_index);

        halo::math::matrix4x3_multiply(halo::objects::object_block<real_matrix4x3>(*reloaded, reloaded->nodes),
            reinterpret_cast<real_matrix4x3 *>(&model_nodes->scale), &basis);
    }
    *(real_vector3d *)&self->base.forward.i = basis.forward;
    *(real_vector3d *)&self->base.up.i = basis.up;
    {
        object *record = halo::objects::object_as<object>(object_index);
        Object *object_tag = halo::objects::tag_as<Object>(record->definition_tag);

        if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1 && test_flag(record->flags, objects::object_flag::no_collision)) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
            clear_flag(record->flags, objects::object_flag::no_collision);
            halo::objects::object_header_of(object_index).flags |= 2;
        }
    }
    self->unit.vehicle_seat_index = -1;
    self->unit.base_animation_state = _unit_base_animation_state_stand;
    if (vehicle->unit.driver_unit_index == object_index) {
        vehicle->unit.driver_unit_index = k_datum_index_none;
    }
    if (vehicle->unit.gunner_unit_index == object_index) {
        vehicle->unit.gunner_unit_index = k_datum_index_none;
    }
    UnitView(vehicle_index).recompute_seat_occupants();
    UnitView(object_index).pick_and_ready_next_weapon();
    {
        int8_t request[2] = { 0x14, 0 };

        UnitView(object_index).update_animation_state_machine(request);
    }
    halo::objects::object_block<real_orientation>(self->base, self->base.node_function_values)->translation = default_translation;
    if (self->base.type == 0) {
        UnitView(object_index).reset_orientation_and_find_position(vehicle_index);
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (UnitView(vehicle_index).all_seats_unoccupied() == 1) {
        vehicle_object *empty = reinterpret_cast<vehicle_object *>(halo::objects::object_try_and_get(vehicle_index, 2));

        if (empty != 0) {
            empty->vehicle.network_update_tick = halo::game::globals().game_time->game_time;
        }
    }
    if (halo::networking::globals().game_mode == 1) {
        player *record = reinterpret_cast<player *>(halo::memory::datum_get(self->unit.controlling_player, halo::game::globals().player_data));

        if (record != 0 && record->local_player_index == -1) {
            record->position_updates.read_index = 0;
            record->position_updates.write_index = 0;
            record->vehicle_updates.read_index = 0;
            record->vehicle_updates.write_index = 0;
        }
    }
}

}  // namespace halo::units

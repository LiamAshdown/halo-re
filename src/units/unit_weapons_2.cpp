#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"

static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);

namespace halo::units {

namespace unit_spawn_with_starting_weapons_local {

typedef struct vehicle_network_create_message {
    datum_index definition;
    int32_t network_key;
    int16_t owner_team;
    int16_t pad_0a;
    int32_t machine_key;
    int32_t creator_key;
    int32_t weapon_keys[4];
    real_point3d position;
    real_vector3d forward;
    real_vector3d up;
    real_vector3d velocity;
    real_vector3d angular_velocity;
    uint8_t network_epoch;
    uint8_t pad_61[3];
} vehicle_network_create_message;

}

/**
 * Engine function unit_spawn_with_starting_weapons.
 *
 * @address 0x572110
 */
void unit_spawn_with_starting_weapons(void *command_record)
{
    using namespace unit_spawn_with_starting_weapons_local;
    vehicle_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t *keys;
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index vehicle_index;
    vehicle_object *vehicle;
    int32_t i;

    if (*(int32_t *)*(int32_t **)command_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)command_record);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)command_record, &message) != 1) {
        return;
    }
    halo::math::vector3d_cross_product(side, message.up, message.forward);
    halo::math::vector3d_cross_product(message.up, message.forward, side);
    halo::math::vector3d_normalize_with_length(message.forward);
    halo::math::vector3d_normalize_with_length(message.up);
    if (message.creator_key != 0) {
        creator = ((int32_t *)object_network_id_table->handles)[message.creator_key];
    }
    if (message.machine_key != 0) {
        machine = (*(int32_t **)&machine_table->handles)[message.machine_key];
    }
    memset(placement, 0, sizeof(placement));
    ((struct object_placement_data *)placement)->definition_tag = message.definition;
    *(int32_t *)&((struct object_placement_data *)placement)->owner_linkage = machine;
    *(int32_t *)&((struct object_placement_data *)placement)->role = creator;
    ((struct object_placement_data *)placement)->owner_team = message.owner_team;
    memcpy(placement + 0x18, &message.position, 12);
    memcpy(placement + 0x34, &message.forward, 12);
    memcpy(placement + 0x40, &message.up, 12);
    vehicle_index = halo::objects::object_new_with_datum_role_control((object_placement_data *)placement, 1);
    if (vehicle_index == k_datum_index_none) {
        return;
    }
    halo::networking::network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)vehicle_index);
    vehicle = reinterpret_cast<vehicle_object *>(halo::objects::object_record_bytes(vehicle_index));
    memcpy(&vehicle->vehicle.network_baseline_position, &message.position, 12);
    memcpy(&vehicle->vehicle.network_baseline_velocity, &message.velocity, 12);
    memcpy(&vehicle->vehicle.network_baseline_angular_velocity, &message.angular_velocity, 12);
    memcpy(&vehicle->vehicle.network_baseline_forward, &message.forward, 12);
    memcpy(&vehicle->vehicle.network_baseline_up, &message.up, 12);
    vehicle->vehicle.network_epoch = message.network_epoch;
    vehicle->vehicle.network_position_pending = 1;
    vehicle->vehicle.network_update_sequence = 0;
    halo::objects::object_set_position_and_recalculate((real_point3d *)&vehicle->vehicle.network_baseline_position, vehicle_index);
    memcpy(&vehicle->base.velocity, &vehicle->vehicle.network_baseline_velocity, 12);
    memcpy(&vehicle->base.angular_velocity, &vehicle->vehicle.network_baseline_angular_velocity, 12);
    memcpy(reinterpret_cast<uint8_t *>(vehicle) + 0x74, reinterpret_cast<uint8_t *>(vehicle) + 0x550, 12);
    memcpy(reinterpret_cast<uint8_t *>(vehicle) + 0x80, reinterpret_cast<uint8_t *>(vehicle) + 0x55c, 12);
    vehicle->unit.network_update_applied = 1;
    keys = (int32_t *)object_network_id_table->handles;
    for (i = 0; i < 4; i++) {
        int32_t weapon = message.weapon_keys[i] != 0 ? keys[message.weapon_keys[i]] : -1;

        if (weapon != -1) {
            ::halo::units::unit_pickup_weapon(0, (uint32_t)weapon, vehicle_index);
        } else {
            ((int32_t *)&vehicle->unit.weapons)[i] = -1;
        }
    }
}

}

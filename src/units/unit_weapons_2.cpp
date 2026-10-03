#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern void unit_invalidate_local_player_zoom_level(void);
extern network_id_table *object_network_id_table;
extern network_id_table *machine_table;
extern uint8_t network_object_index_cache[];
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context);
extern uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key);
}

namespace halo::units {

/**
 * Engine function unit_clear_weapon_switch_state.
 *
 * Original register convention: see file header.
 *
 * @address 0x565a70
 */
void halo::units::unit_clear_weapon_switch_state(unit_data *unit, uint8_t skip_notify, datum_index sound_definition_index)
{
    if (!skip_notify) {
        halo::sound::sound_start_unspatialized(sound_definition_index, 1.0f);
    }
    unit->zoom_level = -1;
    unit->desired_zoom_level = -1;
    unit->integrated_night_vision_power = 0.0f;
    unit_invalidate_local_player_zoom_level();
}

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
void halo::units::unit_spawn_with_starting_weapons(void *command_record)
{
    using namespace unit_spawn_with_starting_weapons_local;
    vehicle_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t *keys;
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index vehicle_index;
    uint8_t *vehicle;
    int32_t i;

    if (*(int32_t *)*(int32_t **)command_record != 0) {
        message_delta_decode_compound_field_staged(command_record);
        return;
    }
    if (message_delta_decode_compound_field(command_record, &message) != 1) {
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
    network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)vehicle_index);
    vehicle = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(vehicle_index)].data;
    memcpy(vehicle + 0x52c, &message.position, 12);
    memcpy(vehicle + 0x538, &message.velocity, 12);
    memcpy(vehicle + 0x544, &message.angular_velocity, 12);
    memcpy(vehicle + 0x550, &message.forward, 12);
    memcpy(vehicle + 0x55c, &message.up, 12);
    vehicle[0x526] = message.network_epoch;
    vehicle[0x525] = 1;
    vehicle[0x527] = 0;
    halo::objects::object_set_position_and_recalculate((real_point3d *)(vehicle + 0x52c), vehicle_index);
    memcpy(vehicle + 0x68, vehicle + 0x538, 12);
    memcpy(vehicle + 0x8c, vehicle + 0x544, 12);
    memcpy(vehicle + 0x74, vehicle + 0x550, 12);
    memcpy(vehicle + 0x80, vehicle + 0x55c, 12);
    ((struct unit_object *)vehicle)->unit.network_update_applied = 1;
    keys = (int32_t *)object_network_id_table->handles;
    for (i = 0; i < 4; i++) {
        int32_t weapon = message.weapon_keys[i] != 0 ? keys[message.weapon_keys[i]] : -1;

        if (weapon != -1) {
            ::halo::units::unit_pickup_weapon(0, (uint32_t)weapon, vehicle_index);
        } else {
            ((int32_t *)&((struct unit_object *)vehicle)->unit.weapons)[i] = -1;
        }
    }
}

}

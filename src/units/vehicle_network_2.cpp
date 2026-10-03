#include "halo/units/unit.hpp"
#include "win32.h"

extern "C" {
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern network_id_table *object_network_id_table;
extern uint8_t network_client_vehicle_ack_enabled;
extern int64_t performance_frequency;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int32_t hash_table_get(hash_table *table, int32_t key);
extern int32_t message_delta_encode_message(void *buffer, int32_t bit_budget, int32_t flag, int32_t message_type, void *changed, void *items, void *types, int32_t count, char force_changed);
extern int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
extern int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);
}

namespace halo::units {

namespace vehicle_encode_network_update_local {

typedef struct vehicle_network_update_header {
    int32_t network_key;
    uint8_t unknown_526;
    uint8_t update_sequence;
    uint8_t is_delta;
    uint8_t pad_07;
    int32_t timestamp_milliseconds;
} vehicle_network_update_header;

typedef struct vehicle_network_update_baseline {
    uint8_t object_flag_5;
    uint8_t pad_01[3];
    real_point3d position;
    real_vector3d velocity;
    real_vector3d angular_velocity;
    real_vector3d forward;
    real_vector3d up;
} vehicle_network_update_baseline;

}

/**
 * Encodes a network update for one vehicle into `buffer`: a full baseline (full_update == 1: header plus
 * position/velocity/angular velocity/orientation plus the vehicle's delta record at 0x528) or a delta (header
 * plus the delta record). Returns the encoded bit count, or 0 when nothing was written. Advances
 * network_update_sequence (wrapping 0xff to 0) when the encoder wrote anything.
 *
 * @address 0x5724d0
 */
int32_t VehicleView::encode_network_update(void *buffer, int32_t bit_budget, int32_t full_update)
{
    using namespace vehicle_encode_network_update_local;
    datum_index vehicle_index = datum_handle;
    object *obj;
    vehicle_data *vehicle;
    unit_data *unit;
    vehicle_network_update_header header;
    vehicle_network_update_baseline baseline;
    void *slots[3];
    large_integer counter;
    int32_t message_type;
    int32_t key;
    int32_t result;

    if (UnitView(vehicle_index).any_flagged_seat_occupied() && full_update != 0 &&
        network_client_vehicle_ack_enabled) {
        return 0;
    }

    obj = object_try_and_get(vehicle_index, 2);
    if (obj == 0) {
        return 0;
    }
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    key = 0;
    if (vehicle_index != (datum_index)0xffffffff) {
        key = hash_table_get(&object_network_id_table->id_to_index, (int32_t)vehicle_index);
        if (key == -1) {
            key = 0;
        }
    }
    header.network_key = key;
    header.unknown_526 = vehicle->unknown_526;
    header.update_sequence = vehicle->network_update_sequence;
    header.is_delta = (uint8_t)(full_update == 0);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    header.timestamp_milliseconds = __alldiv(
        __allmul((int32_t)counter.parts.low_part, counter.parts.high_part, 1000, 0),
        (int32_t)performance_frequency, (int32_t)(performance_frequency >> 32));

    message_type = object_type_definitions[obj->type]->network_delta_message_type;

    if (full_update == 1) {
        baseline.object_flag_5 = (uint8_t)((obj->flags >> 5) & 1);
        baseline.position = obj->position;
        baseline.velocity = obj->velocity;
        baseline.angular_velocity = obj->angular_velocity;
        baseline.forward = obj->forward;
        baseline.up = obj->up;
        slots[0] = &vehicle->network_delta_sequence;
        slots[1] = &baseline;
        slots[2] = &header;
        result = message_delta_encode_message(buffer, bit_budget, 1, message_type,
            &slots[2], &slots[1], &slots[0], 1, 0);
    } else {
        slots[1] = &header;
        slots[2] = &vehicle->network_delta_sequence;
        result = message_delta_encode_message(buffer, bit_budget, 0, message_type,
            &slots[1], &slots[2], 0, 1, 0);
    }

    vehicle->collision_update_pending = 0;
    if (result > 0) {
        vehicle->network_update_sequence++;
        if (vehicle->network_update_sequence >= 0xff) {
            vehicle->network_update_sequence = 0;
        }
    }
    unit->network_update_forced = 0;
    return result;
}

}

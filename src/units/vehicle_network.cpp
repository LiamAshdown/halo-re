#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/math/api.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"

extern "C" {
extern data_array *object_data;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern uint8_t message_delta_decode_compound_field_forced(void **context, void *destination, int32_t changed_offset, int32_t force);
extern uint8_t message_delta_decode_compound_field(void **context, void *destination);
extern uint8_t message_delta_decode_compound_field_staged(void **context);
extern double sqrt(double x);
extern network_id_table *object_network_id_table;
extern uint8_t *machine_table;
extern uint8_t network_object_index_cache[];
extern int32_t hash_table_get(hash_table *table, int32_t key);
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key);
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
}

namespace halo::units {

namespace vehicle_apply_network_update_local {

typedef struct vehicle_network_baseline {
    uint8_t object_flag_5;
    uint8_t pad_01[3];
    real_point3d position;
    real_vector3d velocity;
    real_vector3d angular_velocity;
    real_vector3d forward;
    real_vector3d up;
} vehicle_network_baseline;

}

/**
 * Engine function vehicle_apply_network_update.
 *
 * @address 0x5726e0
 */
void VehicleView::apply_network_update(void **message, uint8_t *connection)
{
    using namespace vehicle_apply_network_update_local;
    datum_index vehicle_index = datum_handle;
    uint8_t *vehicle = (uint8_t *)object_try_and_get(vehicle_index, 2);
    uint8_t *record;
    uint8_t *guard;
    vehicle_network_baseline baseline;
    real_vector3d side;
    uint8_t accepted;
    real dx;
    real dy;
    real dz;
    int32_t latency_base;
    int32_t latency;
    int32_t *timing;

    if (vehicle == 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    record = (uint8_t *)message[0x11];
    guard = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(vehicle_index)].data;
    if (test_flag(((struct object *)guard)->flags, objects::object_flag::took_network_update) && **(int32_t **)message == 1) {
        int32_t incoming = record[5];
        int32_t current = ((struct vehicle_object *)vehicle)->vehicle.network_update_sequence;

        if (record[4] != ((struct vehicle_object *)vehicle)->vehicle.network_epoch || (incoming <= current && incoming - current + 0xff >= 0x1e)) {
            message_delta_decode_compound_field_staged(message);
            return;
        }
    }
    if (**(int32_t **)message == 1) {
        memcpy(&baseline, vehicle + 0x528, sizeof(baseline));
        accepted = message_delta_decode_compound_field_forced(message, &baseline, (int32_t)(vehicle + 0x528), 0);
    } else {
        accepted = message_delta_decode_compound_field(message, &baseline);
    }
    if (!accepted) {
        return;
    }
    ((struct vehicle_object *)vehicle)->vehicle.network_update_sequence = record[5];
    set_flag(((unit_object *)vehicle)->base.flags, objects::object_flag::took_network_update);
    if (record[6] != 0) {
        ((struct vehicle_object *)vehicle)->vehicle.network_epoch = record[4];
        memcpy(vehicle + 0x528, &baseline, sizeof(baseline));
    }
    halo::math::vector3d_cross_product(side, baseline.up, baseline.forward);
    halo::math::vector3d_cross_product(baseline.up, baseline.forward, side);
    halo::math::vector3d_normalize_with_length(baseline.forward);
    halo::math::vector3d_normalize_with_length(baseline.up);
    memcpy(vehicle + 0x1c, &baseline.position, 12);
    memcpy(vehicle + 0x48, &baseline.velocity, 12);
    memcpy(vehicle + 0x2c, &baseline.forward, 12);
    memcpy(vehicle + 0x38, &baseline.up, 12);
    ((struct object *)vehicle)->network_position_valid = 1;
    ((struct object *)vehicle)->network_velocity_valid = 1;
    ((struct object *)vehicle)->unknown_028[0] = 1;
    if (baseline.object_flag_5 == 0) {
        clear_flag(((unit_object *)vehicle)->base.flags, objects::object_flag::at_rest);
    }
    memcpy(vehicle + 0x68, &baseline.velocity, 12);
    memcpy(vehicle + 0x8c, &baseline.angular_velocity, 12);
    memcpy(vehicle + 0x74, &baseline.forward, 12);
    memcpy(vehicle + 0x80, &baseline.up, 12);
    ::halo::units::unit_propagate_position_delta_to_children(&baseline.position, vehicle_index);
    dx = baseline.position.x - ((unit_object *)vehicle)->base.position.x;
    dy = baseline.position.y - ((unit_object *)vehicle)->base.position.y;
    dz = baseline.position.z - ((unit_object *)vehicle)->base.position.z;
    if ((real)sqrt(dx * dx + dy * dy + dz * dz) > 10.0f || test_flag(((unit_object *)vehicle)->base.flags, objects::object_flag::at_rest) ||
        baseline.up.j * ((real *)&((struct object *)vehicle)->up)[1] + baseline.up.k * ((real *)&((struct object *)vehicle)->up)[2] +
                baseline.up.i * ((real *)&((struct object *)vehicle)->up)[0] < 0.70710677f) {
        memcpy(vehicle + 0x1c, &baseline.position, 12);
        memcpy(vehicle + 0x48, &baseline.velocity, 12);
        memcpy(vehicle + 0x74, &baseline.forward, 12);
        memcpy(vehicle + 0x80, &baseline.up, 12);
        ::halo::units::unit_propagate_position_delta_to_children(&baseline.position, vehicle_index);
    }
    timing = *(int32_t **)(connection + 0xad8);
    latency_base = timing != 0 ? timing[0] : (int32_t)record;
    latency = timing != 0 ? timing[1] : (int32_t)record;
    if (latency > 10) {
        ((struct object *)vehicle)->network_timestamp_valid = 1;
        *(int32_t *)&((unit_object *)vehicle)->base.network_timestamp = *(int32_t *)(record + 8) - latency_base;
    } else {
        ((struct object *)vehicle)->network_timestamp_valid = 0;
    }
    ((struct object *)vehicle)->network_position_valid = 1;
    ((struct object *)vehicle)->network_velocity_valid = 1;
    ((struct unit_object *)vehicle)->unit.unknown_475 = 1;
    memcpy(vehicle + 0x56c, &baseline, sizeof(baseline));
}

namespace vehicle_encode_network_create_local {

typedef struct vehicle_network_create_record {
    datum_index definition;
    int32_t network_key;
    int16_t owner_team;
    int16_t pad_0a;
    int32_t machine_key;
    int32_t creator_key;
    int32_t seat_keys[4];
    uint8_t vectors[5][12];
    uint8_t network_epoch;
    uint8_t pad_61[3];
} vehicle_network_create_record;

}

/**
 * Engine function vehicle_encode_network_create.
 *
 * @address 0x571f20
 */
int32_t VehicleView::encode_network_create(int32_t buffer, int32_t bit_budget)
{
    using namespace vehicle_encode_network_create_local;
    datum_index vehicle_index = datum_handle;
    uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(vehicle_index)].data;
    hash_table *keys = &object_network_id_table->id_to_index;
    vehicle_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;
    int32_t i;

    if (vehicle_index != k_datum_index_none) {
        key = hash_table_get(keys, (int32_t)vehicle_index);
    }
    if (*(int32_t *)&((unit_object *)vehicle)->base.creator_object != -1) {
        creator = hash_table_get(keys, *(int32_t *)&((unit_object *)vehicle)->base.creator_object);
        if (creator == -1) {
            creator = 0;
        }
    }
    if (*(int32_t *)&((unit_object *)vehicle)->base.owner_linkage != -1) {
        machine = hash_table_get((hash_table *)(machine_table + 0xc), *(int32_t *)&((unit_object *)vehicle)->base.owner_linkage);
        if (machine == -1) {
            machine = 0;
        }
    }
    if (key == -1) {
        key = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)vehicle_index);
    }
    record.definition = *(datum_index *)vehicle;
    record.network_key = key;
    record.owner_team = ((unit_object *)vehicle)->base.owner_team;
    record.creator_key = creator;
    record.machine_key = machine;
    for (i = 0; i < 4; i++) {
        int32_t seat = ((int32_t *)&((struct unit_object *)vehicle)->unit.weapons)[i];

        record.seat_keys[i] = seat == -1 ? 0 : hash_table_get(keys, seat);
    }
    record.network_epoch = ((struct vehicle_object *)vehicle)->vehicle.network_epoch;
    memcpy(record.vectors[0], vehicle + 0x52c, 12);
    memcpy(record.vectors[1], vehicle + 0x550, 12);
    memcpy(record.vectors[2], vehicle + 0x55c, 12);
    memcpy(record.vectors[3], vehicle + 0x538, 12);
    memcpy(record.vectors[4], vehicle + 0x544, 12);
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1c, 0, &item, 0, 1, 0);
}

namespace vehicle_network_baseline_take_local {

static void copy3(uint8_t *obj, int32_t to, int32_t from)
{
    ((uint32_t *)(obj + to))[0] = ((uint32_t *)(obj + from))[0];
    ((uint32_t *)(obj + to))[1] = ((uint32_t *)(obj + from))[1];
    ((uint32_t *)(obj + to))[2] = ((uint32_t *)(obj + from))[2];
}

}

/**
 * Engine function vehicle_network_baseline_take.
 *
 * @address 0x572410
 */
void VehicleView::network_baseline_take()
{
    using namespace vehicle_network_baseline_take_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)object_try_and_get(object_index, 2);

    if (obj == 0) {
        return;
    }
    ((struct vehicle_object *)obj)->vehicle.network_epoch++;
    ((struct vehicle_object *)obj)->vehicle.unknown_525 = 1;
    ((struct vehicle_object *)obj)->vehicle.network_delta_sequence = 1;
    copy3(obj, 0x52c, 0x5c);
    copy3(obj, 0x538, 0x68);
    copy3(obj, 0x544, 0x8c);
    copy3(obj, 0x550, 0x74);
    ((struct vehicle_object *)obj)->vehicle.network_update_sequence = 0;
    copy3(obj, 0x55c, 0x80);
}

}

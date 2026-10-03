#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/units/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "win32.h"
#include "halo/math/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"

extern "C" {
extern int64_t __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high);
extern int32_t __alldiv(int64_t a, int32_t b_low, int32_t b_high);
}
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &machine_table = halo::link::ref<network_id_table *>(halo::game::vars().machine_table);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);

namespace halo::units {

namespace unit_network_create_update_apply_local {

typedef struct biped_network_create_message {
    datum_index definition;
    int32_t network_key;
    int16_t owner_team;
    int16_t pad_0a;
    int32_t machine_key;
    int32_t creator_key;
    real_point3d position;
    real_vector3d forward;
    real_vector3d up;
    real_vector3d vector_38;
    uint8_t block_44[0x30];
    uint8_t flag_80000;
    uint8_t pad_75[3];
    uint32_t scalar_344;
    uint8_t update_sequence;
    uint8_t grenade_counts[2];
    uint8_t pad_7f;
    uint32_t body_vitality;
    real shield_vitality;
    uint8_t shield_stunned;
    uint8_t pad_89[3];
} biped_network_create_message;

}

/**
 * Engine function unit_network_create_update_apply.
 *
 * @address 0x55b110
 */
void halo::units::unit_network_create_update_apply(void *incoming_record)
{
    using namespace unit_network_create_update_apply_local;
    biped_network_create_message message;
    real_vector3d side;
    uint8_t placement[0x88];
    int32_t creator = -1;
    int32_t machine = -1;
    datum_index biped_index;
    uint8_t *biped;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)incoming_record);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)incoming_record, &message) != 1) {
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
    memcpy(placement + 0x28, &message.vector_38, 12);
    memcpy(placement + 0x34, &message.forward, 12);
    memcpy(placement + 0x40, &message.up, 12);
    memcpy(placement + 0x58, message.block_44, 0x30);
    biped_index = halo::objects::object_new_with_datum_role_control((object_placement_data *)placement, 1);
    if (biped_index == k_datum_index_none) {
        return;
    }
    halo::networking::network_index_cache_insert_if_free(network_object_index_cache, message.network_key, (int32_t)biped_index);
    biped = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(biped_index)].data;
    *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality = message.body_vitality;
    ((biped_object *)biped)->biped.network_shield_vitality = message.shield_vitality;
    biped[0x538] = message.shield_stunned;
    memcpy(biped + 0x52c, message.grenade_counts, 2);
    ((unit_object *)biped)->base.shield_vitality = ((biped_object *)biped)->biped.network_shield_vitality * 3.0f;
    biped[0x527] = message.update_sequence;
    *(uint32_t *)&((unit_object *)biped)->base.body_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality;
    ((unit_object *)biped)->unit.saved_control.zoom_level = (int16_t)((unit_object *)biped)->unit.desired_zoom_level;
    biped[0x526] = 1;
    biped[0x528] = 0;
    ((struct unit_object *)biped)->unit.network_update_applied = 1;
    ((unit_object *)biped)->base.shield_stun_ticks = biped[0x538] == 1;
    memcpy(biped + 0x4ac, biped + 0x494, 12);
    *(int16_t *)(biped + 0x31e) = ((biped_object *)biped)->biped.network_grenade_counts;
    if (message.flag_80000 != 0) {
        set_flag(((unit_object *)biped)->unit.flags, units::unit_flag::unknown_80000);
    } else {
        clear_flag(((unit_object *)biped)->unit.flags, units::unit_flag::unknown_80000);
    }
    *(uint32_t *)(biped + 0x344) = message.scalar_344;
}

namespace unit_submit_periodic_network_update_local {

typedef struct biped_network_update_header {
    int32_t network_key;
    uint8_t update_sequence;
    uint8_t delta_sequence;
    uint8_t is_delta;
    uint8_t shield_update_pending;
    int32_t timestamp_milliseconds;
} biped_network_update_header;

typedef struct biped_network_update_baseline {
    int16_t grenade_counts;
    int16_t pad_02;
    float body_vitality;
    float shield_vitality;
    uint8_t shield_stunned;
} biped_network_update_baseline;

}

/**
 * Engine function unit_submit_periodic_network_update.
 *
 * @address 0x55b440
 */
int32_t UnitView::submit_periodic_network_update(void *buffer, int32_t bit_budget, int32_t update_type)
{
    using namespace unit_submit_periodic_network_update_local;
    datum_index object_index = datum_handle;
    object *obj = halo::objects::object_try_and_get(object_index, 1);
    unit_data *unit;
    biped_data *biped;
    biped_network_update_header header;
    biped_network_update_baseline baseline;
    void *slots[3];
    large_integer counter;
    int32_t message_type;
    int32_t key = 0;
    int32_t result;

    if (obj == 0) {
        return 0;
    }
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if (object_index != k_datum_index_none) {
        key = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)object_index);
        if (key == -1) {
            key = 0;
        }
    }
    header.network_key = key;
    header.update_sequence = biped->network_update_sequence;
    header.delta_sequence = biped->network_delta_sequence;
    header.is_delta = (uint8_t)(update_type == 0);
    header.shield_update_pending = obj->shield_update_pending;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    header.timestamp_milliseconds = __alldiv(__allmul(counter.parts.low_part, counter.parts.high_part, 1000, 0),
                                             (int32_t)halo::cseries::globals().performance_frequency,
                                             (int32_t)(halo::cseries::globals().performance_frequency >> 32));

    message_type = (int32_t)object_type_definitions[obj->type]->network_delta_message_type;
    obj->shield_update_pending = 0;

    if (update_type == 1) {
        if (header.shield_update_pending == 1) {
            baseline.shield_vitality = obj->shield_vitality * 0.33333334f;
        } else {
            baseline.shield_vitality = biped->network_shield_vitality;
        }
        baseline.body_vitality = obj->body_vitality;
        baseline.shield_stunned = (uint8_t)(obj->shield_stun_ticks > 0);
        baseline.grenade_counts = *(int16_t *)&unit->grenade_counts[0];

        slots[0] = &biped->network_grenade_counts;
        slots[1] = &baseline;
        slots[2] = &header;
        result = halo::networking::message_delta_encode_message((int32_t)buffer, bit_budget, 1, message_type,
            (int32_t)(&slots[2]), &slots[1], (int32_t)(&slots[0]), 1, 0);
    } else {
        slots[1] = &header;
        slots[2] = &biped->network_grenade_counts;
        result = halo::networking::message_delta_encode_message((int32_t)buffer, bit_budget, 0, message_type,
            (int32_t)(&slots[1]), &slots[2], 0, 1, 0);
    }

    if (result > 0) {
        biped->network_delta_sequence++;
        if (biped->network_delta_sequence >= 0xff) {
            biped->network_delta_sequence = 0;
        }
    }
    unit->network_update_forced = 0;
    return result;
}

}

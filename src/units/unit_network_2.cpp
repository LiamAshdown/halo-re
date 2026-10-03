#include <string.h>
#include "halo/units/unit.hpp"

extern "C" {
extern data_array *object_data;
extern network_id_table *object_network_id_table;
extern uint8_t *machine_table;
extern uint8_t network_object_index_cache[];
extern int32_t hash_table_get(hash_table *table, int32_t key);
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key);
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
}

namespace halo::units {

namespace unit_build_network_update_local {

typedef struct biped_network_create_record {
    datum_index definition;
    int32_t network_key;
    int16_t owner_team;
    int16_t pad_0a;
    int32_t machine_key;
    int32_t creator_key;
    uint8_t position[12];
    uint8_t forward[12];
    uint8_t up[12];
    uint8_t velocity[12];
    uint8_t block_188[0x30];
    uint8_t flag_80000;
    uint8_t pad_75[3];
    uint32_t scalar_344;
    uint8_t update_sequence;
    uint8_t grenade_counts[2];
    uint8_t pad_7f;
    uint32_t body_vitality;
    uint32_t shield_vitality;
    uint8_t shield_stunned;
    uint8_t pad_89[3];
    uint32_t zero_8c;
} biped_network_create_record;

}

/**
 * Engine function unit_build_network_update.
 *
 * @address 0x55aed0
 */
int32_t UnitView::build_network_update(int32_t buffer, int32_t bit_budget)
{
    using namespace unit_build_network_update_local;
    uint32_t object_index = datum_handle;
    uint8_t *biped = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    hash_table *keys = &object_network_id_table->id_to_index;
    biped_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;

    if (object_index != 0xffffffff) {
        key = hash_table_get(keys, (int32_t)object_index);
    }
    if (*(int32_t *)&((unit_object *)biped)->base.creator_object != -1) {
        creator = hash_table_get(keys, *(int32_t *)&((unit_object *)biped)->base.creator_object);
        if (creator == -1) {
            creator = 0;
        }
    }
    if (*(int32_t *)&((unit_object *)biped)->base.owner_linkage != -1) {
        machine = hash_table_get((hash_table *)(machine_table + 0xc), *(int32_t *)&((unit_object *)biped)->base.owner_linkage);
        if (machine == -1) {
            machine = 0;
        }
    }
    if (key == -1) {
        key = network_index_cache_find_or_allocate_slot(network_object_index_cache, (int32_t)object_index);
    }
    record.definition = *(datum_index *)biped;
    record.network_key = key;
    record.owner_team = ((unit_object *)biped)->base.owner_team;
    record.machine_key = machine;
    record.creator_key = creator;
    memcpy(record.forward, biped + 0x74, 12);
    memcpy(record.up, biped + 0x80, 12);
    memcpy(record.position, biped + 0x5c, 12);
    memcpy(record.velocity, biped + 0x68, 12);
    memcpy(record.block_188, biped + 0x188, 0x30);
    record.flag_80000 = (uint8_t)((((unit_object *)biped)->unit.flags >> 0x13) & 1);
    record.scalar_344 = *(uint32_t *)(biped + 0x344);
    record.update_sequence = biped[0x527];
    record.body_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_body_vitality;
    record.shield_vitality = *(uint32_t *)&((biped_object *)biped)->biped.network_shield_vitality;
    record.shield_stunned = biped[0x538];
    memcpy(record.grenade_counts, biped + 0x52c, 2);
    record.zero_8c = 0;
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1d, 0, &item, 0, 1, 0);
}

}

// vehicle_encode_network_create  (reached only through a .data code pointer; no C existed)
// address 0x571f20, size 495 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x571f20..0x57210e: the vehicle object type  +0x64 hook (0x69b61c; the biped one
//   is 0x55aed0): the creation message (type 0x1c) -- definition tag, the network key (a new index-cache slot
//   0x6870d8 when unknown), the owner team, the machine and creator keys (0 when unknown), four seat keys (vehicle
//   +0x2f8; 0 for none, -1 unknown), five vectors (+0x52c, +0x550, +0x55c, +0x538, +0x544) and the +0x526 byte --
//   encoded with message_delta_encode_message into (buffer, bit budget).
// blam-cc: stack -> vehicle_index, buffer, bit_budget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;                    // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t *machine_table;                     // 0x00687558, hash_table at +0x0c
extern uint8_t network_object_index_cache[];       // 0x006870d8
extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, ESI table, ECX key
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key); // 0x4e9c20, EAX container
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX

typedef struct vehicle_network_create_record {
    datum_index definition;        // 0x00
    int32_t network_key;           // 0x04
    int16_t owner_team;            // 0x08
    int16_t pad_0a;
    int32_t machine_key;           // 0x0c
    int32_t creator_key;           // 0x10
    int32_t seat_keys[4];          // 0x14
    uint8_t vectors[5][12];        // 0x24
    uint8_t unknown_526;           // 0x60
    uint8_t pad_61[3];
} vehicle_network_create_record;   // size 0x64

int32_t vehicle_encode_network_create(datum_index vehicle_index, int32_t buffer, int32_t bit_budget)
{
    uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    hash_table *keys = &object_network_id_table->id_to_index;
    vehicle_network_create_record record;
    void *item = &record;
    int32_t key = 0;
    int32_t creator = 0;
    int32_t machine = 0;
    int32_t i;

    if (vehicle_index != (datum_index)0xffffffff) {
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
        int32_t seat = ((int32_t *)(vehicle + 0x2f8))[i];

        record.seat_keys[i] = seat == -1 ? 0 : hash_table_get(keys, seat);
    }
    record.unknown_526 = vehicle[0x526];
    memcpy(record.vectors[0], vehicle + 0x52c, 12);
    memcpy(record.vectors[1], vehicle + 0x550, 12);
    memcpy(record.vectors[2], vehicle + 0x55c, 12);
    memcpy(record.vectors[3], vehicle + 0x538, 12);
    memcpy(record.vectors[4], vehicle + 0x544, 12);
    return message_delta_encode_message(buffer, bit_budget, 0, 0x1c, 0, &item, 0, 1, 0);
}

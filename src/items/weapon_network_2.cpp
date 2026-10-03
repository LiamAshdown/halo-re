#include "halo/items/items.hpp"

extern "C" {
extern network_id_table *object_network_id_table;
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t hash_table_get(hash_table *table, uint32_t key);
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern data_array *object_data;
}

namespace halo::items {

/**
 * Broadcasts a weapon-ammo-pickup network event for one magazine.
 *
 * @address 0x4c2510
 */
void weapon_ref::notify_ammo_pickup(int16_t magazine_index, int16_t rounds)
{
    datum_index item_index = datum;
    weapon_ammo_pickup_message message;
    void *items[2];

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds = rounds;

    items[0] = &message;
    items[1] = 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_weapon_ammo_pickup, 0, items, 0, 1, 0), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

/**
 * Broadcasts a reload-begin network event for one weapon magazine.
 *
 * @address 0x4c3470
 */
void weapon_ref::notify_reload_begin(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_ammo_message message;
    void *items[2];

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds_unloaded = wd->magazines[magazine_index].rounds_unloaded;
    message.rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

    items[0] = &message;
    items[1] = 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_weapon_reload_begin, 0, items, 0, 1, 0), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

/**
 * Broadcasts a reload-cancelled/host-completed network event for one weapon magazine.
 *
 * @address 0x4c4a00
 */
void weapon_ref::notify_reload_cancel(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_ammo_message message;
    void *items[2];

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds_unloaded = wd->magazines[magazine_index].rounds_unloaded;
    message.rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

    items[0] = &message;
    items[1] = 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_weapon_reload_cancel, 0, items, 0, 1, 0), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

/**
 * Broadcasts a reload-step-finished network event for one weapon magazine.
 *
 * @address 0x4c37b0
 */
void weapon_ref::notify_reload_step(int16_t magazine_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_ammo_message message;
    void *items[2];

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    message.object_hash = 0;
    if (item_index != (datum_index)0xffffffff) {
        message.object_hash = hash_table_get(&object_network_id_table->id_to_index, item_index);
        if (message.object_hash == -1) {
            message.object_hash = 0;
        }
    }
    message.magazine_index = magazine_index;
    message.rounds_unloaded = wd->magazines[magazine_index].rounds_unloaded;
    message.rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

    items[0] = &message;
    items[1] = 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, k_message_weapon_reload_end, 0, items, 0, 1, 0), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

}

extern "C" {

void weapon_notify_ammo_pickup(datum_index item_index, int16_t magazine_index, int16_t rounds)
{
    halo::items::weapon_ref(item_index).notify_ammo_pickup(magazine_index, rounds);
}

void weapon_notify_reload_begin(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).notify_reload_begin(magazine_index);
}

void weapon_notify_reload_cancel(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).notify_reload_cancel(magazine_index);
}

void weapon_notify_reload_step(datum_index item_index, int16_t magazine_index)
{
    halo::items::weapon_ref(item_index).notify_reload_step(magazine_index);
}

}

/**
 * Network message handlers and gameplay event notifications of the game engine.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

#include "halo/game/game1_notifications.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern uint8_t shared_hud_text_draw_state;
extern uint8_t *machine_table;
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6);
extern network_id_table *object_network_id_table;
}

namespace halo::game::engine1 {

/**
 * Sends an object value event to the machines that need it.
 *
 * Original register convention: EAX -> value_byte, ECX -> hash_key, EDI -> machine_index, stack -> subject.
 *
 * @address 0x4779d0
 */
void Notifications::notify_object_value_event(uint8_t value_byte, int32_t hash_key, int32_t machine_index, void *subject)
{
    struct { uint8_t value_byte; int32_t hash_result; void *subject; } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.value_byte = value_byte;
    fields.hash_result = 0;
    if (hash_key != -1) {
        fields.hash_result = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)hash_key);
        if (fields.hash_result == -1) {
            fields.hash_result = 0;
        }
    }
    fields.subject = subject;
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 7, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
            return;
        }
        network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
    }
}

/**
 * Notifies the machines of a player interaction (for example entering a seat).
 *
 * Original register convention: ECX -> primary_key, EDI -> edi_key, stack -> mode, interaction_type_and_seat,
 * secondary_key.
 *
 * @address 0x478ff0
 */
void Notifications::notify_player_interaction(uint32_t primary_key, uint32_t edi_key, uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key)
{
    struct {
        int32_t primary_hash;
        int32_t mode;
        int32_t edi_hash;
        int16_t interaction_type;
        int16_t low_secondary_key;
        int32_t secondary_hash;
    } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.primary_hash = 0;
    if (primary_key != 0xffffffff) {
        fields.primary_hash = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)primary_key);
        if (fields.primary_hash == -1) {
            fields.primary_hash = 0;
        }
    }
    fields.mode = mode;
    fields.edi_hash = 0;
    if (edi_key != 0xffffffff) {
        fields.edi_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)edi_key);
        if (fields.edi_hash == -1) {
            fields.edi_hash = 0;
        }
    }
    fields.interaction_type = (int16_t)interaction_type;
    fields.low_secondary_key = (int16_t)interaction_seat;
    fields.secondary_hash = 0;
    if (secondary_key != -1) {
        fields.secondary_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)secondary_key);
        if (fields.secondary_hash == -1) {
            fields.secondary_hash = 0;
        }
    }
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 10, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

}

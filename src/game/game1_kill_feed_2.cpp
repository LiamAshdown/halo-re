/**
 * Kill attribution and the kill feed messages that are broadcast and displayed for it.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"

#include "halo/game/game1_kill_feed.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern data_array *player_data;
extern uint8_t *network_server;
extern uint8_t network_message_scratch[0x7ff8];
extern uint8_t *machine_table;
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern uint32_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data,
    uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
}

namespace halo::game::engine1 {

/**
 * Dispatches a networked kill-event notification (event id 0x18) to machines whose recorded machine id matches
 * the local player.
 *
 * Original register convention: EAX -> player_index, ECX -> hash_key, stack -> message_type, subject.
 *
 * @address 0x4608d0
 */
void KillFeed::notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type, datum_index subject)
{
    int32_t fields[3];
    void *fields_ptr;
    int32_t encoded_size;

    fields[0] = 0;
    if (hash_key != -1) {
        fields[0] = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)hash_key);
        if (fields[0] == -1) {
            fields[0] = 0;
        }
    }
    fields[1] = message_type;
    fields[2] = (int32_t)subject;
    fields_ptr = fields;

    encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x18, 0, &fields_ptr, 0, 1, '\0');
    if (0 < encoded_size) {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
        uint8_t player_machine_field = *(uint8_t *)&p->machine_index;
        int16_t *machine = (int16_t *)(network_server + 0x3c4);
        int32_t i = 0;

        while ((int32_t)*machine != (int32_t)(int8_t)player_machine_field) {
            i = i + 1;
            machine = machine + 0x30;
            if (0xf < i) {
                return;
            }
        }

        {
            uint8_t *entry = network_server + 0x3b8 + i * 0x60;
            if (entry != 0) {
                uint8_t flags = (uint8_t)*(uint16_t *)(entry + 0xe);
                if (((flags >> 1) & 1) != 0 && ((flags >> 2) & 1) != 0) {
                    network_session_send_to_machine((int32_t)(int8_t)player_machine_field, network_server, 1, network_message_scratch,
                                                    (uint32_t)encoded_size, 1, 0, 0, 3);
                }
            }
        }
    }
}

}

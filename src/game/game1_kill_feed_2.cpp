/**
 * Kill attribution and the kill feed messages that are broadcast and displayed for it.
 */

#include "tags.h"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "objects.h"

#include "halo/game/game1_kill_feed.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &network_server = halo::link::ref<uint8_t *>(halo::networking::vars().network_server);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &machine_table = halo::link::ref<uint8_t *>(halo::game::vars().machine_table);

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

    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::kill_event), 0, &fields_ptr, 0, 1, '\0');
    if (0 < encoded_size) {
        player *p = halo::game::player_at(player_index);
        uint8_t player_machine_field = *(uint8_t *)&p->machine_index;
        network_server_globals *server = (network_server_globals *)network_server;
        int32_t i = 0;

        while ((int32_t)server->machines[i].machine_id != (int32_t)(int8_t)player_machine_field) {
            i = i + 1;
            if (0xf < i) {
                return;
            }
        }

        {
            uint8_t flags = server->machines[i].flags;
            if (((flags >> 1) & 1) != 0 && ((flags >> 2) & 1) != 0) {
                halo::networking::network_session_send_to_machine((int32_t)(int8_t)player_machine_field, server, 1, network_message_scratch,
                                                (uint32_t)encoded_size, 1, 0, 0, 3);
            }
        }
    }
}

}

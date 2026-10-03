/**
 * Network message handlers and gameplay event notifications of the game engine.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#include "halo/game/game1_notifications.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &network_server = halo::link::ref<uint8_t *>(halo::networking::vars().network_server);
static auto &network_client = halo::link::ref<uint8_t *>(halo::networking::vars().network_client);
static auto &update_client_queues = halo::link::ref<data_array *>(halo::game::vars().update_client_queues);
static auto &join_message_table = halo::link::ref<uint8_t []>(halo::game::vars().join_message_table);
extern "C" {
extern void game_set_local_player(datum_index player_handle,
    int16_t local_player_index);
}

namespace halo::game::engine1 {

/**
 * Applies a received player join message.
 *
 * Original register convention: EAX -> event, ECX -> out_message (a leading identifier-table slot byte, then
 * the.
 *
 * @address 0x4778c0
 */
void Notifications::apply_player_join_message(void **envelope)
{
    struct { uint8_t slot_index; uint8_t pad[3]; uint32_t join_key; uint32_t hash_value;
        uint32_t team; } message;
    player *p;

    if (*(int32_t *)*envelope != 0) {
        halo::networking::message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!halo::networking::message_delta_decode_compound_field(envelope, &message)) {
        return;
    }

    {
    uint8_t *table_base = (network_server != 0) ? (network_server + 8) :
        ((network_client != 0) ? (network_client + 0xb14) : 0);
    uint8_t *identifier_record = table_base + (uint32_t)message.slot_index * 0x20 + 0x1a2;

    p = (player *)halo::memory::datum_get((datum_index)message.join_key, player_data);
    if (p == 0) {
        if (halo::networking::network_channel_key_close((network_player_entry *)identifier_record, (datum_index)message.join_key) != 1) {
            return;
        }
        halo::networking::network_index_cache_insert_if_free(join_message_table, (int32_t)message.hash_value, (int32_t)message.join_key);
        p = (player *)halo::memory::datum_get((datum_index)message.join_key, player_data);
        halo::memory::datum_new_at_index_with_salt((datum_index)message.join_key, update_client_queues);
        halo::game::game_engine_player_profile_cache_add(message.join_key);
        if (p == 0) {
            return;
        }
    }

    p->team = (int32_t)message.team;
    p->team_index = (int8_t)message.team;
    if (p->local_player_index != -1) {
        halo::game::game_set_local_player((datum_index)message.join_key,
            (int16_t)*(int8_t *)(identifier_record + 0x1d));
    }
    p->kill_streak[0] = 0;
    p->kill_streak[1] = 0;
    p->interaction_type = 0;
    p->interaction_object = (datum_index)0xffffffff;

    halo::game::game_engine_player_new_life(message.join_key);
    }
}

}

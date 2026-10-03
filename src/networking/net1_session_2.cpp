#include "halo/networking/net1_session.hpp"
#include "halo/networking/net1_dispatch.hpp"
#include <string.h>
#include <stdint.h>
#include "units.h"
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"

static auto &profile_globals_block = halo::link::ref<uint32_t [0x7ff]>(halo::ui::vars().profile_globals_block);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &network_game_mode = halo::link::ref<int16_t>(halo::networking::vars().network_game_mode);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &network_client = halo::link::ref<network_client_globals *>(halo::networking::vars().network_client);

namespace halo::networking {

/**
 * already named)
 * address 0x4e1c60, size 614 bytes
 * name confidence: 0.7   rewrite confidence: 0.85 (REWRITTEN; was 0.35)
 * out/phase4/networking_functions.md: "Top-level decoder/dispatcher for incoming
 * 'network game' protocol messages, decoding each message with the network-game message group
 * and routing it by type byte." The bitstream-header check (`(*record & 3) == 0 && ((*record
 * >> 2) & 3) == 3`) matches network_game_message_decode_dispatch.c's identical check; `in_ECX`'s
 *
 * @address 0x4e1c60
 */
uint32_t GameRuntime::process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server)
{
    uint8_t *bytes = (uint8_t *)record;
    uint8_t type_byte;
    uint8_t machine_flags;
    const ServerMessageHandler *handler;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }
    type_byte = bytes[(int16_t)length - 1];
    machine_flags = *((uint8_t *)machine + 0xe);
    if (((machine_flags >> 1) & 1) == 0 && type_byte != 0x0e && !(((machine_flags >> 4) & 1) != 0 && type_byte == 1)) {
        return 1;
    }
    handler = ServerMessageRegistry::find(type_byte);
    if (handler == nullptr) {
        return 1;
    }
    return handler->handle(server, machine, bytes, length);
}

/**
 * Builds (or copies the cached) default server profile into one 0x1ffc-byte scratch buffer, then
 * starts a new hosted game using the name/password fields near its tail.
 *
 * @address 0x4e40f0
 */
uint8_t GameRuntime::start_new_server_from_profile(uint32_t param_1)
{
    uint32_t profile[0x7ff];

    if (halo::saved_games::globals().player_profile_slots_handle == -1) {
        halo::saved_games::player_profile_set_default_server_options((saved_player_profile *)profile);
    } else {
        memcpy(profile, profile_globals_block, sizeof(profile));
    }
    return halo::networking::network_game_start_new_server_with_name_and_password(param_1,
        (uint16_t *)((uint8_t *)profile + 867 * 4),
        (uint16_t *)((uint8_t *)profile + 867 * 4 + 288));
}

/**
 * REVIEW PASS 2026-09-20: AggregateFieldCodec::decode_compound_field takes the decode context in EAX and the destination in
 * ECX (0x4dbaa3 `lea ecx,[esp+0xc]`). The scratch it fills is what supplies player_index
 * (scratch[0], read back at 0x4dbab4) and new_value; both were modelled as elided outputs.
 * The EAX argument is this function own incoming EAX, which Ghidra dropped entirely -- the
 * parameter below is added to carry it.
 *
 * @address 0x4dbaa0
 */
void PlayerReports::ping_field_update_and_report(void *decode_context)
{
    char ok;
    uint8_t decode_scratch[0x58];
    uint8_t player_index;
    int32_t new_value;
    data_iterator iter;
    void *element;
    char team_index;
    uint8_t fields_byte0;
    uint8_t fields_byte1;
    uint8_t *fields_ptr;
    int32_t fields_pad;
    int32_t encoded_bits;
    uint8_t message_buffer[64];

    ok = halo::networking::message_delta_decode_compound_field((void **)decode_context, decode_scratch);
    player_index = decode_scratch[0];
    new_value = 0;
    if (ok == 1) {
        team_index = -1;
        if (player_index != 0xff && (int16_t)(uint16_t)player_index < halo::game::globals().player_data->maximum_count) {
            uint8_t *player = (uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * (int16_t)(uint16_t)player_index;
            if (*(int16_t *)player != 0) {
                ((struct player *)player)->ping = new_value;
            }
        }

        iter.data = halo::game::globals().player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = halo::memory::data_iterator_next(&iter);
        if (element != 0) {
            team_index = -1;
            do {
                if (((player *)element)->local_player_index != -1) {
                    team_index = ((player *)element)->team_index_desired;
                    if (team_index != -1 && network_game_mode == 2) {
                        int32_t base = *(int32_t *)((uint8_t *)network_server + 0x9c0);
                        int32_t now = halo::cseries::time_query_performance_counter_ms();
                        ((player *)element)->ping = now - base;
                        return;
                    }
                    break;
                }
                element = halo::memory::data_iterator_next(&iter);
            } while (element != 0);
        }

        fields_ptr = &fields_byte0;
        fields_pad = 0;
        fields_byte0 = (uint8_t)team_index;
        (void)fields_pad;
        encoded_bits = halo::networking::message_delta_encode_message((int32_t)message_buffer, 0x200, 0, 0x34, 0, (void **)&fields_ptr, 0, 1, 0);
        if (encoded_bits > 0) {
            fields_byte1 = 1;
            if ((network_client->channel->flags & 1) == 0) {
                halo::networking::network_channel_reliable_pool_store(network_client->channel, message_buffer, &fields_byte1, 0, 1, (uint32_t)encoded_bits);
            }
        }
    }
}

/**
 * the key list callback (key type, key buffer, user data):
 * server keys 1 3 4 10 19 5 0x33 11 and, in a game, 0x36 8 6 12 7 13 0x34 0x35; in a game, player keys 0x15 0x16
 * 0x18 0x19 and team keys 0x1c 0x1d.
 *
 * @address 0x577fb0
 */
void HostSession::qr2_key_list(int32_t key_type, void *keybuffer, void *user_data)
{
    int32_t i;

    (void)user_data;
    if (key_type == 0) {
        static const int32_t always[8] = { 1, 3, 4, 10, 19, 5, 0x33, 11 };
        static const int32_t in_game[8] = { 0x36, 8, 6, 12, 7, 13, 0x34, 0x35 };

        for (i = 0; i < 8; i++) {
            qr2_keybuffer_add(keybuffer, always[i]);
        }
        if (halo::game::globals().current_engine != 0) {
            for (i = 0; i < 8; i++) {
                qr2_keybuffer_add(keybuffer, in_game[i]);
            }
        }
    } else if (key_type == 1) {
        if (halo::game::globals().current_engine != 0) {
            qr2_keybuffer_add(keybuffer, 0x15);
            qr2_keybuffer_add(keybuffer, 0x16);
            qr2_keybuffer_add(keybuffer, 0x18);
            qr2_keybuffer_add(keybuffer, 0x19);
        }
    } else if (key_type == 2) {
        if (halo::game::globals().current_engine != 0) {
            qr2_keybuffer_add(keybuffer, 0x1c);
            qr2_keybuffer_add(keybuffer, 0x1d);
        }
    }
}

}

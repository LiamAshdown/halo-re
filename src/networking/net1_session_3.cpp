#include "halo/networking/net1_session.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include <string.h>
#include <wchar.h>
#include "units.h"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"

static auto &current_game_engine = halo::link::ref<void *>(halo::game::vars().current_game_engine);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &network_qr2_text = halo::link::ref<char [0x100]>(halo::networking::vars().network_qr2_text);
static auto &network_session_host_closing = halo::link::ref<uint8_t>(halo::networking::vars().network_session_host_closing);
static auto &game_engine_variant = halo::link::ref<uint8_t []>(halo::game::vars().game_engine_variant);
static auto &motion_sensor_override_value = halo::link::ref<uint8_t>(halo::ui::vars().motion_sensor_override_value);
static auto &game_engine_variant_score_limit = halo::link::ref<int32_t>(halo::networking::vars().game_engine_variant_score_limit);

namespace halo::networking {

/**
 * the qr2 player key callback (key, index, buffer, user
 * data); the earlier version modeled three arguments. The player at that active index (validated: index in range,
 * live, salt 0 or matching) has key 0x15 its name (at most 0x40 characters, ASCII) and 0x19 its team; the game
 * engine's +0xa0 hook answers other keys; anything unanswered is empty. (Name kept.)
 *
 * @address 0x577e40
 */
void HostSession::dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    uint32_t handle = halo::game::players_get_active_by_index(index);
    int16_t player_index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    ::player *player;

    (void)user_data;
    if (handle == halo::k_dword_none || player_index < 0 || player_index >= player_data->maximum_count) {
        qr2_buffer_add(buffer, "");
        return;
    }
    player = (::player *)((uint8_t *)player_data->data + player_index * player_data->size);
    if (player->identifier == 0 || (salt != 0 && player->identifier != salt)) {
        qr2_buffer_add(buffer, "");
        return;
    }
    if (key_id == 0x15) {
        uint8_t name[0x40];

        memset(name, 0, sizeof(name));
        qr2_buffer_add(buffer, (const char *)halo::text::string_convert_unicode_to_ascii(name, (uint16_t *)player->name, 0x40));
        return;
    }
    if (key_id == 0x19) {
        qr2_buffer_add_int(buffer, player->team);
        return;
    }
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa0);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    qr2_buffer_add(buffer, "");
}

/**
 * the server key callback (key, buffer, user data). The game
 * engine's +0x9c hook may answer first. Without a server the value is empty; keys 1 hostname ("HALO SERVER" when
 * unnamed), 3 game version, 5 map (the file name of the server's map path), 6 game type (CTF / Slayer / Oddball /
 * King / Race), 7 variant name, 8 players, 10 maximum players (at least 1), 11 mode ("exiting" while closing, else
 * "openplaying"), 12 team play, 13 score limit, 19 password, 0x33 the server flag bit 2, 0x34 the packed custom
 * options, 0x35 the packed game type options, 0x36 bit 7 of 0x006f1cc0; every other key is empty.
 *
 * @address 0x5779c0
 */
void HostSession::qr2_server_key(int32_t key_id, void *buffer, void *user_data)
{
    network_server_globals *server = network_server;
    game_variant *variant = (game_variant *)game_engine_variant;
    uint8_t *options = (uint8_t *)&variant->engine;
    int32_t game_type = variant->game_engine_index;

    (void)user_data;
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, void *) = *(uint8_t (**)(int32_t, void *))((uint8_t *)current_game_engine + 0x9c);

        if (hook != 0 && hook(key_id, buffer) != 0) {
            return;
        }
    }
    if (server == 0) {
        qr2_buffer_add(buffer, "");
        return;
    }
    switch (key_id) {
    case 1:
        if (wcslen((const wchar_t *)&server->session) == 0) {
            qr2_buffer_add(buffer, "HALO SERVER");
        } else {
            qr2_buffer_add(buffer, (const char *)halo::text::string_convert_unicode_to_ascii((uint8_t *)network_qr2_text, (uint16_t *)&server->session, 0x100));
        }
        return;
    case 3:
        halo::networking::autopatch_current_version_string_get(network_qr2_text);
        qr2_buffer_add(buffer, network_qr2_text);
        return;
    case 5: {
        char name[0x100];
        const char *base = server->session.server_name;
        char *dot;

        // the file name part without its extension (_splitpath's fname)
        for (const char *c = base; *c != '\0'; c++) {
            if (*c == '\\' || *c == '/' || *c == ':') base = c + 1;
        }
        strncpy(name, base, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        dot = strrchr(name, '.');
        if (dot != nullptr) *dot = '\0';
        qr2_buffer_add(buffer, name);
        return;
    }
    case 6: {
        static const char *const names[5] = { "CTF", "Slayer", "Oddball", "King", "Race" };

        qr2_buffer_add(buffer, game_type >= 1 && game_type <= 5 ? names[game_type - 1] : "");
        return;
    }
    case 7:
        qr2_buffer_add(buffer, (const char *)halo::text::string_convert_unicode_to_ascii((uint8_t *)network_qr2_text,
            variant->name, 0x100));
        return;
    case 8:
        qr2_buffer_add_int(buffer, current_game_engine != 0 ? halo::game::players_active_count() : 0);
        return;
    case 10: {
        int8_t maximum = (int8_t)server->session.maximum_players;

        qr2_buffer_add_int(buffer, maximum > 1 ? maximum : 1);
        return;
    }
    case 11:
        qr2_buffer_add(buffer, network_session_host_closing != 0 ? "exiting" : "openplaying");
        return;
    case 12:
        qr2_buffer_add_int(buffer, halo::game::game_engine_get_teams_enabled() != 0);
        return;
    case 13:
        qr2_buffer_add_int(buffer, game_engine_variant_score_limit);
        return;
    case 19:
        qr2_buffer_add_int(buffer, halo::networking::network_server_password_is_set(server) != 0);
        return;
    case 0x33:
        qr2_buffer_add_int(buffer, (server->flags >> 2) & 1);
        return;
    case 0x34:
        qr2_buffer_add(buffer, halo::networking::server_browser_custom_options_pack((server_browser_custom_options *)&variant->teams));
        return;
    case 0x35:
        switch (game_type) {
        case 1: qr2_buffer_add_int(buffer, (int32_t)halo::networking::server_browser_gametype1_flags_pack((server_browser_gametype1_options *)options)); return;
        case 2: qr2_buffer_add_int(buffer, (int32_t)halo::networking::server_browser_gametype2_flags_pack(options)); return;
        case 3: qr2_buffer_add_int(buffer, (int32_t)halo::networking::server_browser_gametype3_flags_pack((server_browser_gametype3_options *)options)); return;
        case 4: qr2_buffer_add_int(buffer, (options[0] != 0 ? 8 : 0) | 4); return;
        case 5: qr2_buffer_add_int(buffer, (int32_t)halo::networking::server_browser_gametype5_flags_pack((int32_t *)options)); return;
        }
        break;
    case 0x36:
        qr2_buffer_add_int(buffer, (motion_sensor_override_value & 0x80) != 0);
        return;
    }
    qr2_buffer_add(buffer, "");
}

}

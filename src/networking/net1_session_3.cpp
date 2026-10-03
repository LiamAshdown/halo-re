#include "halo/networking/net1_session.hpp"
#include "halo/text/api.hpp"
#include <string.h>
#include <wchar.h>
#include "units.h"
#include "halo/game/api.hpp"

extern "C" {
extern void *current_game_engine;
extern void qr2_buffer_add(void *buffer, const char *value);
extern void qr2_buffer_add_int(void *buffer, int32_t value);
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id);
typedef struct data_array data_array;
extern uint8_t *player_data;
extern uint8_t *network_server;
extern char network_qr2_text[0x100];
extern uint8_t network_session_host_closing;
extern uint8_t game_engine_variant[];
extern uint8_t motion_sensor_override_value;
extern int32_t game_engine_variant_score_limit;
extern void autopatch_current_version_string_get(char *out);
extern char *server_browser_custom_options_pack(void *options);
extern uint32_t server_browser_gametype1_flags_pack(void *options);
extern uint32_t server_browser_gametype2_flags_pack(uint8_t *flags);
extern uint32_t server_browser_gametype3_flags_pack(void *options);
extern uint32_t server_browser_gametype5_flags_pack(int32_t *values);
extern int32_t network_server_password_is_set(void *server);
}

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
    uint8_t *player;

    (void)user_data;
    if (handle == 0xffffffff || player_index < 0 || player_index >= *(int16_t *)(player_data + 0x20)) {
        qr2_buffer_add(buffer, "");
        return;
    }
    player = *(uint8_t **)(player_data + 0x34) + player_index * *(int16_t *)(player_data + 0x22);
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt)) {
        qr2_buffer_add(buffer, "");
        return;
    }
    if (key_id == 0x15) {
        uint8_t name[0x40];

        memset(name, 0, sizeof(name));
        qr2_buffer_add(buffer, (const char *)halo::text::string_convert_unicode_to_ascii(name, (uint16_t *)(player + 4), 0x40));
        return;
    }
    if (key_id == 0x19) {
        qr2_buffer_add_int(buffer, ((struct player *)player)->team);
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
    uint8_t *server = network_server;
    uint8_t *options = game_engine_variant + 0x7c;
    int32_t game_type = *(int32_t *)(game_engine_variant + 0x30);

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
        if (wcslen((const wchar_t *)(server + 8)) == 0) {
            qr2_buffer_add(buffer, "HALO SERVER");
        } else {
            qr2_buffer_add(buffer, (const char *)halo::text::string_convert_unicode_to_ascii((uint8_t *)network_qr2_text, (uint16_t *)(server + 8), 0x100));
        }
        return;
    case 3:
        autopatch_current_version_string_get(network_qr2_text);
        qr2_buffer_add(buffer, network_qr2_text);
        return;
    case 5: {
        char name[0x100];

        _splitpath((const char *)(server + 0x8c), 0, 0, name, 0);
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
            (uint16_t *)game_engine_variant, 0x100));
        return;
    case 8:
        qr2_buffer_add_int(buffer, current_game_engine != 0 ? halo::game::players_active_count() : 0);
        return;
    case 10: {
        int8_t maximum = (int8_t)server[0x1a5];

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
        qr2_buffer_add_int(buffer, network_server_password_is_set(server) != 0);
        return;
    case 0x33:
        qr2_buffer_add_int(buffer, (server[6] >> 2) & 1);
        return;
    case 0x34:
        qr2_buffer_add(buffer, server_browser_custom_options_pack(game_engine_variant + 0x34));
        return;
    case 0x35:
        switch (game_type) {
        case 1: qr2_buffer_add_int(buffer, (int32_t)server_browser_gametype1_flags_pack(options)); return;
        case 2: qr2_buffer_add_int(buffer, (int32_t)server_browser_gametype2_flags_pack(options)); return;
        case 3: qr2_buffer_add_int(buffer, (int32_t)server_browser_gametype3_flags_pack(options)); return;
        case 4: qr2_buffer_add_int(buffer, (options[0] != 0 ? 8 : 0) | 4); return;
        case 5: qr2_buffer_add_int(buffer, (int32_t)server_browser_gametype5_flags_pack((int32_t *)options)); return;
        }
        break;
    case 0x36:
        qr2_buffer_add_int(buffer, (motion_sensor_override_value & 0x80) != 0);
        return;
    }
    qr2_buffer_add(buffer, "");
}

}

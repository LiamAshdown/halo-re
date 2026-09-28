// network_session_host_qr2_server_key  (not a Ghidra function; no C existed -- the host's qr2 callback, which network_session_host_start passed as NULL)
// address 0x5779c0, size 1116 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5779c0..0x577e1b: the server key callback (key, buffer, user data). The game
//   engine's +0x9c hook may answer first. Without a server the value is empty; keys 1 hostname ("HALO SERVER" when
//   unnamed), 3 game version, 5 map (the file name of the server's map path), 6 game type (CTF / Slayer / Oddball /
//   King / Race), 7 variant name, 8 players, 10 maximum players (at least 1), 11 mode ("exiting" while closing, else
//   "openplaying"), 12 team play, 13 score limit, 19 password, 0x33 the server flag bit 2, 0x34 the packed custom
//   options, 0x35 the packed game type options, 0x36 bit 7 of 0x006f1cc0; every other key is empty.
// blam-cc: cdecl (a qr2 server key callback)

#include "tags.h"
#include <string.h>
#include <wchar.h>

extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *; +0x9c/+0xa0/+0xa4/+0xa8 the query hooks)
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add
extern uint8_t *network_server;            // 0x0071c2d4
extern char network_qr2_text[0x100];         // 0x00722a28
extern uint8_t network_session_host_closing; // 0x00722a1c
extern uint8_t game_engine_variant_bytes[];  // 0x006f1c88 (game_variant)
extern uint8_t motion_sensor_override_value; // 0x006f1cc0
extern int32_t game_engine_variant_score_limit; // 0x006f1ce0
extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950, blam-cc: ESI dest, EDI source
extern void autopatch_current_version_string_get(char *out); // 0x578190, blam-cc: EAX out
extern char *server_browser_custom_options_pack(void *options); // 0x576180, blam-cc: EAX options
extern uint32_t server_browser_gametype1_flags_pack(void *options); // 0x5767d0, blam-cc: ECX
extern uint32_t server_browser_gametype2_flags_pack(uint8_t *flags); // 0x576920, blam-cc: EAX
extern uint32_t server_browser_gametype3_flags_pack(void *options); // 0x5769c0, blam-cc: ECX
extern uint32_t server_browser_gametype5_flags_pack(int32_t *values); // 0x576960, blam-cc: EDX
extern int32_t players_active_count(void); // 0x45c6a0
extern uint8_t game_engine_get_teams_enabled(void); // 0x462bf0
extern int32_t network_server_password_is_set(void *server); // 0x4e08e0, blam-cc: EAX server
extern void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);

void network_session_host_qr2_server_key(int32_t key_id, void *buffer, void *user_data)
{
    uint8_t *server = network_server;
    uint8_t *options = game_engine_variant_bytes + 0x7c;   // 0x006f1d04
    int32_t game_type = *(int32_t *)(game_engine_variant_bytes + 0x30); // 0x006f1cb8

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
            qr2_buffer_add(buffer, (const char *)string_convert_unicode_to_ascii((uint8_t *)network_qr2_text, (uint16_t *)(server + 8), 0x100));
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
        qr2_buffer_add(buffer, (const char *)string_convert_unicode_to_ascii((uint8_t *)network_qr2_text,
            (uint16_t *)game_engine_variant_bytes, 0x100));
        return;
    case 8:
        qr2_buffer_add_int(buffer, current_game_engine != 0 ? players_active_count() : 0);
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
        qr2_buffer_add_int(buffer, game_engine_get_teams_enabled() != 0);
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
        qr2_buffer_add(buffer, server_browser_custom_options_pack(game_engine_variant_bytes + 0x34));
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

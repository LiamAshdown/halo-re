import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')


def hdr(name, addr, size, note, cc, what='WRITTEN', origin='not a Ghidra function; no C existed -- the host\'s qr2 callback, which network_session_host_start passed as NULL'):
    lines = textwrap.wrap('%s 2026-09-28 from objdump 0x%x..0x%x: %s' % (what, addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    return ('// %s  (%s)\n// address 0x%x, size %d bytes\n// name confidence: 0.6   rewrite confidence: 0.85\n%s// blam-cc: %s\n\n'
            '#include "tags.h"\n#include <string.h>\n#include <wchar.h>\n\n' % (name, origin, addr, size, wr, cc))


COMMON = '''extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *; +0x9c/+0xa0/+0xa4/+0xa8 the query hooks)
extern void FUN_00615590(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void FUN_00616640(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void FUN_00615560(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add
'''


def write(name, text):
    open('src/networking/%s.c' % name, 'w', encoding='utf-8').write(text)


write('network_session_host_qr2_server_key', hdr('network_session_host_qr2_server_key', 0x5779c0, 1116,
    'the server key callback (key, buffer, user data). The game engine\'s +0x9c hook may answer first. Without a server the value is empty; keys 1 hostname ("HALO SERVER" when unnamed), 3 game version, 5 map (the file name of the server\'s map path), 6 game type (CTF / Slayer / Oddball / King / Race), 7 variant name, 8 players, 10 maximum players (at least 1), 11 mode ("exiting" while closing, else "openplaying"), 12 team play, 13 score limit, 19 password, 0x33 the server flag bit 2, 0x34 the packed custom options, 0x35 the packed game type options, 0x36 bit 7 of 0x006f1cc0; every other key is empty.',
    'cdecl (a qr2 server key callback)') + COMMON + '''extern uint8_t *network_server;            // 0x0071c2d4
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
        FUN_00615590(buffer, "");
        return;
    }
    switch (key_id) {
    case 1:
        if (wcslen((const wchar_t *)(server + 8)) == 0) {
            FUN_00615590(buffer, "HALO SERVER");
        } else {
            FUN_00615590(buffer, (const char *)string_convert_unicode_to_ascii((uint8_t *)network_qr2_text, (uint16_t *)(server + 8), 0x100));
        }
        return;
    case 3:
        autopatch_current_version_string_get(network_qr2_text);
        FUN_00615590(buffer, network_qr2_text);
        return;
    case 5: {
        char name[0x100];

        _splitpath((const char *)(server + 0x8c), 0, 0, name, 0);
        FUN_00615590(buffer, name);
        return;
    }
    case 6: {
        static const char *const names[5] = { "CTF", "Slayer", "Oddball", "King", "Race" };

        FUN_00615590(buffer, game_type >= 1 && game_type <= 5 ? names[game_type - 1] : "");
        return;
    }
    case 7:
        FUN_00615590(buffer, (const char *)string_convert_unicode_to_ascii((uint8_t *)network_qr2_text,
            (uint16_t *)game_engine_variant_bytes, 0x100));
        return;
    case 8:
        FUN_00616640(buffer, current_game_engine != 0 ? players_active_count() : 0);
        return;
    case 10: {
        int8_t maximum = (int8_t)server[0x1a5];

        FUN_00616640(buffer, maximum > 1 ? maximum : 1);
        return;
    }
    case 11:
        FUN_00615590(buffer, network_session_host_closing != 0 ? "exiting" : "openplaying");
        return;
    case 12:
        FUN_00616640(buffer, game_engine_get_teams_enabled() != 0);
        return;
    case 13:
        FUN_00616640(buffer, game_engine_variant_score_limit);
        return;
    case 19:
        FUN_00616640(buffer, network_server_password_is_set(server) != 0);
        return;
    case 0x33:
        FUN_00616640(buffer, (server[6] >> 2) & 1);
        return;
    case 0x34:
        FUN_00615590(buffer, server_browser_custom_options_pack(game_engine_variant_bytes + 0x34));
        return;
    case 0x35:
        switch (game_type) {
        case 1: FUN_00616640(buffer, (int32_t)server_browser_gametype1_flags_pack(options)); return;
        case 2: FUN_00616640(buffer, (int32_t)server_browser_gametype2_flags_pack(options)); return;
        case 3: FUN_00616640(buffer, (int32_t)server_browser_gametype3_flags_pack(options)); return;
        case 4: FUN_00616640(buffer, (options[0] != 0 ? 8 : 0) | 4); return;
        case 5: FUN_00616640(buffer, (int32_t)server_browser_gametype5_flags_pack((int32_t *)options)); return;
        }
        break;
    case 0x36:
        FUN_00616640(buffer, (motion_sensor_override_value & 0x80) != 0);
        return;
    }
    FUN_00615590(buffer, "");
}
''')

# 0x577e40: rewrite over the old dispatch_message file
p = 'src/networking/network_session_host_dispatch_message.c'
s = open(p, encoding='utf-8').read()
i = s.find('#if 0')
tail = s[i:] if i >= 0 else ''
write('network_session_host_dispatch_message', hdr('network_session_host_dispatch_message', 0x577e40, 241,
    'the qr2 player key callback (key, index, buffer, user data); the earlier version modeled three arguments. The player at that active index (validated: index in range, live, salt 0 or matching) has key 0x15 its name (at most 0x40 characters, ASCII) and 0x19 its team; the game engine\'s +0xa0 hook answers other keys; anything unanswered is empty. (Name kept.)',
    'cdecl (a qr2 player key callback)', what='REWRITTEN', origin='Ghidra: FUN_00577e40; the qr2 player key callback') + COMMON + '''typedef struct data_array data_array;
extern uint8_t *player_data_raw; // 0x0087a480 (data_array *)
extern uint32_t players_get_active_by_index(int32_t index); // 0x45c6f0, blam-cc: EAX index
extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950

void network_session_host_dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    uint32_t handle = players_get_active_by_index(index);
    int16_t player_index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *player;

    (void)user_data;
    if (handle == 0xffffffff || player_index < 0 || player_index >= *(int16_t *)(player_data_raw + 0x20)) {
        FUN_00615590(buffer, "");
        return;
    }
    player = *(uint8_t **)(player_data_raw + 0x34) + player_index * *(int16_t *)(player_data_raw + 0x22);
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt)) {
        FUN_00615590(buffer, "");
        return;
    }
    if (key_id == 0x15) {
        uint8_t name[0x40];

        memset(name, 0, sizeof(name));
        FUN_00615590(buffer, (const char *)string_convert_unicode_to_ascii(name, (uint16_t *)(player + 4), 0x40));
        return;
    }
    if (key_id == 0x19) {
        FUN_00616640(buffer, *(int32_t *)(player + 0x20));
        return;
    }
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa0);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    FUN_00615590(buffer, "");
}
''' + ('\n' + tail if tail else ''))

write('network_session_host_qr2_team_key', hdr('network_session_host_qr2_team_key', 0x577f40, 110,
    'the team key callback (key, index, buffer, user data): the game engine\'s +0xa4 hook answers first; key 0x1c is the team name ("Red" for 0, "Blue" for 1 -- as the binary has it, index 1 is "Blue"); anything else is empty.',
    'cdecl (a qr2 team key callback)') + COMMON + '''
void network_session_host_qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    (void)user_data;
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa4);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    if (key_id == 0x1c) {
        FUN_00615590(buffer, index == 1 ? "Blue" : "Red");
        return;
    }
    FUN_00615590(buffer, "");
}
''')
write('network_session_host_qr2_key_list', hdr('network_session_host_qr2_key_list', 0x577fb0, 261,
    'the key list callback (key type, key buffer, user data): server keys 1 3 4 10 19 5 0x33 11 and, in a game, 0x36 8 6 12 7 13 0x34 0x35; in a game, player keys 0x15 0x16 0x18 0x19 and team keys 0x1c 0x1d.',
    'cdecl (a qr2 key list callback)') + COMMON + '''
void network_session_host_qr2_key_list(int32_t key_type, void *keybuffer, void *user_data)
{
    int32_t i;

    (void)user_data;
    if (key_type == 0) {
        static const int32_t always[8] = { 1, 3, 4, 10, 19, 5, 0x33, 11 };
        static const int32_t in_game[8] = { 0x36, 8, 6, 12, 7, 13, 0x34, 0x35 };

        for (i = 0; i < 8; i++) {
            FUN_00615560(keybuffer, always[i]);
        }
        if (current_game_engine != 0) {
            for (i = 0; i < 8; i++) {
                FUN_00615560(keybuffer, in_game[i]);
            }
        }
    } else if (key_type == 1) {
        if (current_game_engine != 0) {
            FUN_00615560(keybuffer, 0x15);
            FUN_00615560(keybuffer, 0x16);
            FUN_00615560(keybuffer, 0x18);
            FUN_00615560(keybuffer, 0x19);
        }
    } else if (key_type == 2) {
        if (current_game_engine != 0) {
            FUN_00615560(keybuffer, 0x1c);
            FUN_00615560(keybuffer, 0x1d);
        }
    }
}
''')
write('network_session_host_qr2_count', hdr('network_session_host_qr2_count', 0x5780c0, 88,
    'the player / team count callback (key type, user data): 0 outside a game; the game engine\'s +0xa8 hook when it has one; otherwise players -> the active player count, teams -> 2 with teams (else 0), anything else 0.',
    'cdecl (a qr2 count callback)') + COMMON + '''extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t players_active_count(void); // 0x45c6a0

int32_t network_session_host_qr2_count(int32_t key_type, void *user_data)
{
    int32_t (*hook)(int32_t);

    (void)user_data;
    if (current_game_engine == 0) {
        return 0;
    }
    hook = *(int32_t (**)(int32_t))((uint8_t *)current_game_engine + 0xa8);
    if (hook != 0) {
        return hook(key_type);
    }
    if (key_type == 1) {
        return players_active_count();
    }
    if (key_type == 2 && game_engine_teams_enabled_flag != 0) {
        return 2;
    }
    return 0;
}
''')
write('network_session_host_qr2_add_error', hdr('network_session_host_qr2_add_error', 0x578100, 24,
    'the qr2 add-error callback (error, message, user data): "qr2_adderror_callback - %s" to the console in its standard color.',
    'cdecl (a qr2 add-error callback)') + '''typedef struct ColorARGB ColorARGB;
extern void *console_color_00685218; // 0x00685218
extern void console_printf_verbose(ColorARGB *color, char *format, ...); // 0x496a80, blam-cc: EAX color

void network_session_host_qr2_add_error(int32_t error, char *message, void *user_data)
{
    (void)error;
    (void)user_data;
    console_printf_verbose((ColorARGB *)console_color_00685218, "qr2_adderror_callback - %s", message);
}
''')

# network_session_host_start: from the binary, 0x577850..0x5778ee
p = 'src/networking/network_session_host_start.c'
s = open(p, encoding='utf-8').read()
i = s.find('#if 0')
tail = s[i:] if i >= 0 else ''
write('network_session_host_start', hdr('network_session_host_start', 0x577850, 159,
    'disposes the old session, opens the channels and starts query/report on the game socket\'s SOCKET (0x6175f0) with the port, game name and secret key strings, the public flag byte, natneg on, the six host callbacks (the earlier version passed NULL for five of them and the player key callback in the wrong slot) and the argument as user data; registers the natneg callback, and initializes the CD key server with game id 0x319 on the same record. Returns qr2_init_socketA\'s result.',
    'cdecl', what='REWRITTEN', origin='Ghidra: FUN_00577850') + '''extern int32_t network_game_socket;                    // 0x006f14c4 (GT2Socket; its first dword is the SOCKET)
extern void *network_session_host_object;               // 0x00722a20
extern int32_t network_session_start_game_type;          // 0x007227b8 (passed as the query port)
extern char network_session_start_host_name[];           // 0x00722798 (the qr2 game name)
extern char network_session_start_map_name[];            // 0x007227a0 (the qr2 secret key)
extern uint8_t network_session_host_flags_byte;          // 0x0069fe00
extern int32_t network_console_connection_id;            // 0x0069fdfc (the CD key game id)
extern void network_session_host_dispose(void);          // 0x5778f0
extern void network_channels_open(void);                 // 0x441300
extern int32_t FUN_00616340(void **qrec_out, uint32_t socket, int32_t port, const char *gamename, const char *secret_key,
    int32_t ispublic, int32_t natnegotiate, void *server_key, void *player_key, void *team_key, void *key_list, void *count,
    void *adderror, void *userdata); // 0x616340 qr2_init_socketA
extern void FUN_00615530(void *qrec, void *callback); // 0x615530 qr2_register_natneg_callback
extern void FUN_0061b6d0(void *qrec, int32_t game_id, int32_t use_network); // 0x61b6d0 gcd_init_qr2
extern void network_session_host_natneg_callback(int32_t cookie); // 0x578160
extern void network_session_host_qr2_server_key(int32_t key_id, void *buffer, void *user_data); // 0x5779c0
extern void network_session_host_dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data); // 0x577e40
extern void network_session_host_qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data); // 0x577f40
extern void network_session_host_qr2_key_list(int32_t key_type, void *keybuffer, void *user_data); // 0x577fb0
extern int32_t network_session_host_qr2_count(int32_t key_type, void *user_data); // 0x5780c0
extern void network_session_host_qr2_add_error(int32_t error, char *message, void *user_data); // 0x578100

int32_t network_session_host_start(void *user_data)
{
    int32_t result;

    network_session_host_dispose();
    network_channels_open();
    result = FUN_00616340(&network_session_host_object, *(uint32_t *)network_game_socket, network_session_start_game_type,
        network_session_start_host_name, network_session_start_map_name, network_session_host_flags_byte, 1,
        (void *)network_session_host_qr2_server_key, (void *)network_session_host_dispatch_message,
        (void *)network_session_host_qr2_team_key, (void *)network_session_host_qr2_key_list,
        (void *)network_session_host_qr2_count, (void *)network_session_host_qr2_add_error, user_data);
    FUN_00615530(network_session_host_object, (void *)network_session_host_natneg_callback);
    network_console_connection_id = 0x319;
    FUN_0061b6d0(network_session_host_object, 0x319, network_session_host_flags_byte);
    return result;
}
''' + ('\n' + tail if tail else ''))
print('ok')

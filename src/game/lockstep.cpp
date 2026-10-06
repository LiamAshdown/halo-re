/**
 * @file src/game/lockstep.cpp
 * Lockstep co-op: every machine runs the whole campaign simulation and the machines only
 * exchange each player's per-tick action. A tick runs once every player's action for it has arrived, so the
 * simulations stay identical; every 30 ticks the machines compare a state hash and print LSP-DESYNC if they differ.
 *
 * Players are created in slot order on every machine (slot 0 the host, then the joiners); each machine marks its own
 * slot as the local player. Local actions are stamped k_input_delay ticks ahead, which hides that much latency.
 *
 * Hosting and joining: while a single-player campaign level runs with co-op switched on (the CO-OP row of Choose
 * Difficulty, src/interface/coop_option.cpp), the game is open for co-op. It answers the server
 * browser's search (GameSpy qr2 on k_query_port) as a "Co-op" row with the level and one of two players; picking the
 * row (join_from_browser) sends the host HELLO. The host answers WELCOME with the level and difficulty, and both start
 * that level from its beginning (a joiner cannot yet take over a level in progress).
 * For testing in one browser: -coop host starts b30 by itself, -coop join broadcasts HELLO to whichever host answers,
 * and -coop list is a second game (own profile, see web/shell.html) that joins through the server browser.
 *
 * Transport (web build): UDP on k_port over the page server's virtual LAN. Messages start with u32 magic, u8 type,
 * u8 slot, u16 epoch; the epoch counts level starts and reverts, so actions from before one are ignored after it.
 *   WELCOME u8 player count, i16 difficulty, the level path (NUL terminated)
 *   INPUT   i32 ack (the last contiguous tick received from the receiver), i32 first tick, u8 count, count actions
 *   HASH    i32 tick, u32 hash
 */

#include "halo/game/api.hpp"
#include "halo/game/lockstep.hpp"
#include "halo/game/records.hpp"
#include "halo/main/api.hpp"
#include "halo/math/globals.hpp"
#include "halo/shell/api.hpp"
#include "halo/text/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include "saved_games.h"
#include "../gamespy/gamespy_calls.hpp"

#include <stdio.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C" int halo_net_ioctl(int s, long command, unsigned long *argument);
#endif

namespace halo::game::lockstep {

namespace {

constexpr uint16_t k_port = 2420;
constexpr uint16_t k_query_port = 2302;  // the port the server browser searches (game_socket_port)
constexpr uint32_t k_magic = 0x4f434c48;  // "HLCO"
constexpr int32_t k_input_delay = 3;
constexpr int32_t k_ring = 256;
constexpr int32_t k_max_players = 2;  // ponytail: host + one joiner; more joiners need a slot table and relaying
constexpr int32_t k_hash_interval = 30;
constexpr int32_t k_hash_ring = 64;
constexpr int32_t k_actions_per_packet = 64;
constexpr char k_test_level[] = "levels\\b30\\b30";
constexpr char k_coop_gametype[] = "Co-op";

enum : uint8_t { k_hello = 1, k_welcome = 2, k_input = 3, k_hash = 4 };
enum class role { none, host, joiner };

struct session {
    role kind = role::none;
    bool roles_read = false;
    bool test_host = false;           // -coop host: start the test level by itself
    bool coop_allowed = false;
    uint32_t join_address = 0;        // network order; INADDR_BROADCAST for -coop join
    bool connected = false;
    bool level_queued = false;
    int qr_socket = -1;
    void *qr = nullptr;
    int32_t local_slot = 0;
    int32_t player_count = 1;
    int socket = -1;
    uint32_t peer_address = 0;  // network order
    uint16_t peer_port = 0;     // network order
    uint16_t epoch = 0;
    int32_t frames = 0;

    player_action actions[k_ring][k_max_players];
    int32_t action_tick[k_ring][k_max_players];
    player_action latest_local;
    int32_t next_local_tick = 0;
    int32_t peer_contiguous = -1;  // every peer action up to this tick has arrived
    int32_t peer_acked = -1;       // the peer has every local action up to this tick

    int32_t my_hash_tick[k_hash_ring];
    uint32_t my_hash[k_hash_ring];
    int32_t their_hash_tick[k_hash_ring];
    uint32_t their_hash[k_hash_ring];
    int32_t desyncs = 0;
    int16_t difficulty = 0;
    char level[0x100] = {};
};

session g;
auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
auto &pending_difficulty = halo::link::ref<int16_t>(halo::ui::vars().pending_difficulty);
random_seed g_tick_effect_seed;
random_seed g_frame_effect_seed;

int32_t game_tick()
{
    return halo::game::globals().game_time->game_time;
}

bool in_level()
{
    return g.connected && halo::main::globals().main_globals.main_menu_scenario_loaded == 0;
}

int32_t peer_slot()
{
    return g.local_slot == 0 ? 1 : 0;
}

void read_role()
{
    const char *value = nullptr;

    if (g.roles_read) {
        return;
    }
    g.roles_read = true;
    if (halo::shell::command_line_check_flag("-coop", &value) && value != nullptr) {
        g.test_host = strcmp(value, "host") == 0;
        if (strcmp(value, "join") == 0) {
            g.kind = role::joiner;
            g.join_address = 0xffffffffu;
        }
    }
#if !defined(__EMSCRIPTEN__)
    g.kind = role::none;
#endif
}

/** A single-player campaign level is running, which other players may join. */
bool hostable()
{
    const main_globals &main = halo::main::globals().main_globals;

    return (g.coop_allowed || g.test_host) && g.kind != role::joiner && main.main_menu_scenario_loaded == 0 &&
        main.game_connection == _game_connection_local &&
        halo::game::globals().game_time->initialized != 0;
}

void queue_level(const char *level, int16_t difficulty)
{
    if (g.level_queued) {
        return;
    }
    g.level_queued = true;
    printf("lockstep: slot %d of %d, starting %s\n", g.local_slot, g.player_count, level);
    pending_difficulty = difficulty;
    halo::main::main_queue_map_change(level);
    halo::main::globals().main_globals.restore_checkpoint_on_load = 0;
}

void reset_actions(int32_t start_tick)
{
    player_action idle;

    memset(&idle, 0, sizeof(idle));
    idle.weapon_index = -1;
    idle.grenade_index = -1;
    idle.zoom_level = -1;
    for (int32_t i = 0; i < k_ring; i++) {
        for (int32_t s = 0; s < k_max_players; s++) {
            g.action_tick[i][s] = -1;
        }
    }
    // the first ticks precede anyone's input: every machine fills them the same way
    for (int32_t t = start_tick; t < start_tick + k_input_delay; t++) {
        for (int32_t s = 0; s < g.player_count; s++) {
            g.actions[t % k_ring][s] = idle;
            g.action_tick[t % k_ring][s] = t;
        }
    }
    g.latest_local = idle;
    g.next_local_tick = start_tick + k_input_delay;
    g.peer_contiguous = start_tick + k_input_delay - 1;
    g.peer_acked = start_tick + k_input_delay - 1;
    for (int32_t i = 0; i < k_hash_ring; i++) {
        g.my_hash_tick[i] = -1;
        g.their_hash_tick[i] = -1;
    }
}

#if defined(__EMSCRIPTEN__)

void put(uint8_t *&at, const void *value, size_t size)
{
    memcpy(at, value, size);
    at += size;
}

uint8_t *header(uint8_t *buffer, uint8_t type, uint8_t slot)
{
    uint8_t *at = buffer;

    put(at, &k_magic, 4);
    put(at, &type, 1);
    put(at, &slot, 1);
    put(at, &g.epoch, 2);
    return at;
}

void send_to(uint32_t address, uint16_t port, const uint8_t *data, size_t size)
{
    sockaddr_in to;

    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = address;
    to.sin_port = port;
    sendto(g.socket, data, size, 0, reinterpret_cast<sockaddr *>(&to), sizeof(to));
}

bool open_socket()
{
    sockaddr_in address;
    unsigned long nonblocking = 1;

    if (g.socket >= 0) {
        return true;
    }
    g.socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g.socket < 0) {
        return false;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(k_port);
    if (bind(g.socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        printf("lockstep: port %u is taken\n", k_port);
        close(g.socket);
        g.socket = -1;
        g.kind = role::none;
        return false;
    }
    halo_net_ioctl(g.socket, static_cast<long>(FIONBIO), &nonblocking);
    return true;
}

// The server browser's view of a co-op host: qr2 keys (hostname 1, gamever 3, mapname 5, gametype 6, numplayers 8,
// maxplayers 10, gamemode 11, teamplay 12, password 19, dedicated 0x33, game_flags 0x35, game_classic 0x36). The
// yes/no keys must be explicit numbers: the browser reads an empty value as true.
constexpr int k_server_keys[] = {1, 3, 5, 6, 8, 10, 11, 12, 19, 0x33, 0x35, 0x36};

const char *host_name()
{
    static char name[0x20];

    halo::text::string_convert_unicode_to_ascii(reinterpret_cast<uint8_t *>(name),
        reinterpret_cast<uint16_t *>(profile_globals_block[0].profile.name), sizeof(name));
    return name[0] != 0 ? name : "Halo";
}

void qr_server_key(int key, void *buffer, void *)
{
    switch (key) {
    case 1:
        qr2_buffer_add(buffer, host_name());
        break;
    case 3:
        qr2_buffer_add(buffer, "01.00.10.0621");
        break;
    case 5:
        qr2_buffer_add(buffer, halo::main::globals().main_globals.scenario_path);
        break;
    case 6:
        qr2_buffer_add(buffer, k_coop_gametype);
        break;
    case 8:
        qr2_buffer_add_int(buffer, g.connected ? g.player_count : 1);
        break;
    case 10:
        qr2_buffer_add_int(buffer, k_max_players);
        break;
    case 11:
        qr2_buffer_add(buffer, "openplaying");
        break;
    default:
        qr2_buffer_add_int(buffer, 0);
        break;
    }
}

// the player list under the browser's rows: the host, by name (key 0x15)
void qr_player_key(int key, int, void *buffer, void *)
{
    qr2_buffer_add(buffer, key == 0x15 ? host_name() : "");
}

void qr_team_key(int, int, void *buffer, void *)
{
    qr2_buffer_add(buffer, "");
}

void qr_key_list(int key_type, void *keys, void *)
{
    if (key_type == 0) {
        for (int key : k_server_keys) {
            qr2_keybuffer_add(keys, key);
        }
    } else if (key_type == 1) {
        qr2_keybuffer_add(keys, 0x15);
    }
}

int qr_count(int key_type, void *)
{
    return key_type == 1 ? 1 : 0;
}

void qr_add_error(int, char *, void *)
{
}

void advertise()
{
    sockaddr_in address;
    unsigned long nonblocking = 1;

    if (g.qr != nullptr) {
        return;
    }
    if (g.qr_socket < 0) {
        g.qr_socket = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(k_query_port);
        if (g.qr_socket < 0 || bind(g.qr_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
            // ponytail: a page that played multiplayer may still hold the port; it then is not listed
            printf("lockstep: port %u is taken, this game is not listed for co-op\n", k_query_port);
            if (g.qr_socket >= 0) {
                close(g.qr_socket);
            }
            g.qr_socket = -2;
            return;
        }
        halo_net_ioctl(g.qr_socket, static_cast<long>(FIONBIO), &nonblocking);
    }
    if (g.qr_socket >= 0) {
        qr2_init_socketA(&g.qr, static_cast<uint32_t>(g.qr_socket), k_query_port, "halor", "e4Rd9J", 0, 0,
            reinterpret_cast<void *>(qr_server_key), reinterpret_cast<void *>(qr_player_key), reinterpret_cast<void *>(qr_team_key),
            reinterpret_cast<void *>(qr_key_list), reinterpret_cast<void *>(qr_count), reinterpret_cast<void *>(qr_add_error), nullptr);
        printf("lockstep: open for co-op as %s\n", host_name());
    }
}

void stop_advertising()
{
    if (g.qr != nullptr) {
        qr2_shutdown(g.qr);
        g.qr = nullptr;
    }
}

void answer_queries()
{
    char buffer[0x600];
    sockaddr_in from;
    socklen_t length;

    while (g.qr != nullptr) {
        length = sizeof(from);
        ssize_t size = recvfrom(g.qr_socket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&from), &length);

        if (size < 0) {
            return;
        }
        if (size >= 2 && static_cast<uint8_t>(buffer[0]) == 0xfe && static_cast<uint8_t>(buffer[1]) == 0xfd) {
            qr2_parse_queryA(g.qr, buffer, static_cast<int32_t>(size), &from);
        }
    }
}

void send_inputs()
{
    uint8_t buffer[16 + k_actions_per_packet * sizeof(player_action)];
    int32_t first = g.peer_acked + 1;
    int32_t count = g.next_local_tick - first;

    if (!g.connected || count <= 0) {
        return;
    }
    if (count > k_actions_per_packet) {
        count = k_actions_per_packet;
    }
    uint8_t *at = header(buffer, k_input, static_cast<uint8_t>(g.local_slot));
    uint8_t count_byte = static_cast<uint8_t>(count);

    put(at, &g.peer_contiguous, 4);
    put(at, &first, 4);
    put(at, &count_byte, 1);
    for (int32_t t = first; t < first + count; t++) {
        put(at, &g.actions[t % k_ring][g.local_slot], sizeof(player_action));
    }
    send_to(g.peer_address, g.peer_port, buffer, at - buffer);
}

void receive_inputs(const uint8_t *at, const uint8_t *end)
{
    int32_t ack;
    int32_t first;
    uint8_t count;
    int32_t slot = peer_slot();
    int32_t now = game_tick();

    if (end - at < 9) {
        return;
    }
    memcpy(&ack, at, 4);
    memcpy(&first, at + 4, 4);
    count = at[8];
    at += 9;
    if (end - at < static_cast<ptrdiff_t>(count * sizeof(player_action))) {
        return;
    }
    if (ack > g.peer_acked) {
        g.peer_acked = ack;
    }
    for (int32_t t = first; t < first + count; t++, at += sizeof(player_action)) {
        if (t < now || t >= now + k_ring) {
            continue;
        }
        memcpy(&g.actions[t % k_ring][slot], at, sizeof(player_action));
        g.action_tick[t % k_ring][slot] = t;
    }
    while (g.action_tick[(g.peer_contiguous + 1) % k_ring][slot] == g.peer_contiguous + 1) {
        g.peer_contiguous++;
    }
}

void compare_hashes(int32_t index)
{
    if (g.my_hash_tick[index] < 0 || g.my_hash_tick[index] != g.their_hash_tick[index]) {
        return;
    }
    if (g.my_hash[index] != g.their_hash[index]) {
        if (g.desyncs++ < 20) {
            printf("LSP-DESYNC %d mine=%08x theirs=%08x\n", g.my_hash_tick[index], g.my_hash[index], g.their_hash[index]);
        }
    } else if (g.my_hash_tick[index] % (k_hash_interval * 10) == 0) {
        printf("LSP-SYNC %d %08x\n", g.my_hash_tick[index], g.my_hash[index]);
    }
    g.my_hash_tick[index] = -1;
}

void pump()
{
    uint8_t buffer[0x2000];
    sockaddr_in from;
    socklen_t length;

    for (;;) {
        length = sizeof(from);
        ssize_t size = recvfrom(g.socket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&from), &length);
        uint32_t magic;
        uint16_t epoch;

        if (size < 8) {
            if (size < 0) {
                return;
            }
            continue;
        }
        memcpy(&magic, buffer, 4);
        memcpy(&epoch, buffer + 6, 2);
        if (magic != k_magic) {
            continue;
        }
        uint8_t type = buffer[4];
        uint8_t slot = buffer[5];

        if (type == k_hello && g.kind == role::host) {
            if (!g.connected) {
                g.connected = true;
                g.peer_address = from.sin_addr.s_addr;
                g.peer_port = from.sin_port;
                g.player_count = 2;
                g.local_slot = 0;
                g.difficulty = halo::main::globals().game_globals->difficulty;
                strncpy(g.level, halo::main::globals().main_globals.scenario_path, sizeof(g.level) - 1);
            }
            if (from.sin_addr.s_addr == g.peer_address) {
                uint8_t reply[16 + sizeof(g.level)];
                uint8_t *at = header(reply, k_welcome, 1);
                uint8_t players = static_cast<uint8_t>(g.player_count);

                put(at, &players, 1);
                put(at, &g.difficulty, 2);
                put(at, g.level, strlen(g.level) + 1);
                send_to(g.peer_address, g.peer_port, reply, at - reply);
                queue_level(g.level, g.difficulty);
            }
        } else if (type == k_welcome && g.kind == role::joiner && !g.connected && size >= 12 && buffer[size - 1] == 0) {
            g.connected = true;
            g.peer_address = from.sin_addr.s_addr;
            g.peer_port = from.sin_port;
            g.local_slot = slot;
            g.player_count = buffer[8];
            memcpy(&g.difficulty, buffer + 9, 2);
            strncpy(g.level, reinterpret_cast<char *>(buffer + 11), sizeof(g.level) - 1);
            queue_level(g.level, g.difficulty);
        } else if (g.connected && from.sin_addr.s_addr == g.peer_address && epoch == g.epoch && in_level()) {
            if (type == k_input) {
                receive_inputs(buffer + 8, buffer + size);
            } else if (type == k_hash && size >= 16) {
                int32_t tick;
                uint32_t hash;

                memcpy(&tick, buffer + 8, 4);
                memcpy(&hash, buffer + 12, 4);
                int32_t index = (tick / k_hash_interval) % k_hash_ring;

                g.their_hash_tick[index] = tick;
                g.their_hash[index] = hash;
                compare_hashes(index);
            }
        }
    }
}

#endif

}  // namespace

bool active()
{
    return in_level() || probe_active();
}

bool session_active()
{
    return in_level();
}

int32_t player_count()
{
    return g.connected ? g.player_count : 1;
}

bool coop_allowed()
{
    return g.coop_allowed;
}

void set_coop_allowed(bool allowed)
{
    g.coop_allowed = allowed;
}

bool is_coop_gametype(const char *gametype)
{
    return gametype != nullptr && strcmp(gametype, k_coop_gametype) == 0;
}

bool join_from_browser(void *server)
{
#if defined(__EMSCRIPTEN__)
    if (server == nullptr || !is_coop_gametype(SBServerGetStringValue(server, "gametype", "")) || g.connected) {
        return false;
    }
    const char *address = SBServerGetPublicAddress(static_cast<int32_t>(reinterpret_cast<uintptr_t>(server)));

    if (address == nullptr) {
        return false;
    }
    stop_advertising();
    g.kind = role::joiner;
    g.join_address = inet_addr(address);
    g.frames = 0;
    printf("lockstep: joining %s\n", address);
    return true;
#else
    (void)server;
    return false;
#endif
}

void frame_begin()
{
    static int32_t frames_since_start;

    read_role();
    if (g.test_host && ++frames_since_start == 60) {
        queue_level(k_test_level, pending_difficulty);
        g.level_queued = false;  // the co-op start afterwards queues its own
    }
#if defined(__EMSCRIPTEN__)
    if (g.kind == role::none && hostable()) {
        g.kind = role::host;
    } else if (g.kind == role::host && !g.connected && !hostable()) {
        stop_advertising();
        g.kind = role::none;
    }
    if (g.kind == role::none || !open_socket()) {
        return;
    }
    if (g.kind == role::host && !g.connected) {
        advertise();
    } else if (g.connected) {
        stop_advertising();
    }
    answer_queries();
    pump();
    g.frames++;
    if (g.kind == role::joiner && !g.connected && g.frames % 30 == 1) {
        uint8_t hello[8];

        header(hello, k_hello, 0xff);
        send_to(g.join_address, htons(k_port), hello, sizeof(hello));
    }
#endif
}

void on_new_map(uint32_t game_seed)
{
    g_tick_effect_seed = game_seed ^ 0x5eed5eedu;
    if (g.connected) {
        g.epoch++;
        reset_actions(0);
    }
}

void on_revert()
{
    if (g.connected) {
        g.epoch++;
        reset_actions(game_tick());
    }
}

bool create_players()
{
    if (!g.connected) {
        return false;
    }
    datum_index *local_players = halo::game::globals().local_player_globals->local_players;

    if (local_players[0] != k_datum_index_none) {
        halo::game::player_at(local_players[0])->local_player_index = -1;
        local_players[0] = k_datum_index_none;
    }
    for (int32_t slot = 0; slot < g.player_count; slot++) {
        bool local = slot == g.local_slot;
        datum_index player = halo::game::player_new_network(k_datum_index_none, 0, local ? 0 : -1, 0);

        if (local) {
            local_players[0] = player;
        }
    }
    return true;
}

void capture_local_action(const player_action &action)
{
    g.latest_local = action;
    g.latest_local.pad_1e = 0;
}

int32_t schedule_ticks(int32_t wanted)
{
    if (!in_level()) {
        return wanted;
    }
    int32_t now = game_tick();

    while (g.next_local_tick < now + wanted + k_input_delay && g.next_local_tick < now + k_ring - 1) {
        g.actions[g.next_local_tick % k_ring][g.local_slot] = g.latest_local;
        if (probe_active()) {
            probe_scripted_action(g.next_local_tick, &g.actions[g.next_local_tick % k_ring][g.local_slot]);
        }
        g.action_tick[g.next_local_tick % k_ring][g.local_slot] = g.next_local_tick;
        g.next_local_tick++;
    }
#if defined(__EMSCRIPTEN__)
    send_inputs();
#endif
    int32_t ready = 0;

    while (ready < wanted) {
        int32_t t = now + ready;
        bool all = true;

        for (int32_t s = 0; s < g.player_count; s++) {
            all = all && g.action_tick[t % k_ring][s] == t;
        }
        if (!all) {
            break;
        }
        ready++;
    }
    if (ready < wanted && g.frames % 60 == 0) {
        printf("lockstep: waiting at tick %d (local sent to %d, peer has %d, peer sent to %d)\n", now, g.next_local_tick - 1,
            g.peer_acked, g.peer_contiguous);
    }
    return ready;
}

void apply_actions(player_action *actions)
{
    if (in_level()) {
        int32_t t = game_tick();

        for (int32_t s = 0; s < g.player_count; s++) {
            actions[s] = g.actions[t % k_ring][s];
        }
        return;
    }
    probe_override_actions(actions);
}

void tick_begin()
{
    if (!active()) {
        return;
    }
    g_frame_effect_seed = halo::math::globals().effect_random_seed;
    halo::math::globals().effect_random_seed = g_tick_effect_seed;
}

void tick_end()
{
    if (!active()) {
        return;
    }
    g_tick_effect_seed = halo::math::globals().effect_random_seed;
    halo::math::globals().effect_random_seed = g_frame_effect_seed;
#if defined(__EMSCRIPTEN__)
    int32_t tick = game_tick();

    if (in_level() && tick % k_hash_interval == 0) {
        int32_t index = (tick / k_hash_interval) % k_hash_ring;
        uint8_t buffer[16];
        uint8_t *at = header(buffer, k_hash, static_cast<uint8_t>(g.local_slot));
        uint32_t hash = simulation_hash(nullptr);

        g.my_hash_tick[index] = tick;
        g.my_hash[index] = hash;
        put(at, &tick, 4);
        put(at, &hash, 4);
        send_to(g.peer_address, g.peer_port, buffer, at - buffer);
        compare_hashes(index);
    }
#endif
}

}  // namespace halo::game::lockstep

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
 * row (join_from_browser) sends the host HELLO. The host takes a snapshot of its game state between two frames and makes
 * both machines start from it: it applies the snapshot to itself (the after-load callbacks a revert runs), answers
 * WELCOME, and streams the snapshot in STATE chunks; the joiner loads the level, applies the snapshot the same way and
 * resumes at its tick. Both then add the joiner's player, dead, so the co-op respawn brings it in beside the host.
 * For testing in one browser: -coop host starts b30 by itself, -coop join broadcasts HELLO to whichever host answers,
 * and -coop list is a second game (own profile, see web/shell.html) that joins through the server browser.
 *
 * Transport (web build): UDP on k_port over the page server's virtual LAN. Messages start with u32 magic, u8 type,
 * u8 slot, u16 epoch; the epoch counts level starts and reverts, so actions from before one are ignored after it.
 *   HELLO   the joiner's profile name (NUL terminated)
 *   WELCOME u8 player count, i16 difficulty, the level path (NUL terminated), then the snapshot: i32 tick, u32 size,
 *           u32 game seed, u32 simulation effect seed, i32 camera script end tick, u16 epoch; then the host's name
 *   STATE   u16 chunk index, the chunk (k_chunk_size bytes, the last one shorter)
 *   STATE_ACK i32 chunks received in order
 *   INPUT   i32 ack (the last contiguous tick received from the receiver), i32 first tick, u8 count, count actions
 *   HASH    i32 tick, u32 hash
 */

#include "halo/game/api.hpp"
#include "halo/game/lockstep.hpp"
#include "halo/game/records.hpp"
#include "halo/interface/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/game1_local_control.hpp"
#include "halo/main/api.hpp"
#include "halo/math/globals.hpp"
#include "halo/math/random.hpp"
#include "halo/shell/api.hpp"
#include "halo/text/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/platform/time.hpp"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include "saved_games.h"
#include "cutscene.h"
#include "../gamespy/gamespy_calls.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
constexpr uint32_t k_peer_timeout_ms = 10000;          // nothing from the partner this long: carry on without them
constexpr uint32_t k_peer_loading_timeout_ms = 300000; // before their first action of a level (they may be downloading its map)
constexpr char k_test_level[] = "levels\\b30\\b30";
constexpr uint32_t k_skip_cinematic_flag = 0x80000000u;  // in player_action::control_flags; units use the low 16 bits
constexpr uint32_t k_held_control_flags_mask = 0x4d0;    // UpdateClient::queue_apply_tick: these act on the press only
constexpr uint32_t k_back_flag = 0x40000000u;            // Back held (player_control_input::melee), for hs player_action_test_back
constexpr uint32_t k_revert_flag = 0x20000000u;          // menu: revert to the last checkpoint
constexpr uint32_t k_restart_flag = 0x10000000u;         // menu: restart the level
constexpr uint32_t k_lockstep_flags = k_skip_cinematic_flag | k_back_flag | k_revert_flag | k_restart_flag;
constexpr char k_coop_gametype[] = "Co-op";

enum : uint8_t { k_hello = 1, k_welcome = 2, k_input = 3, k_hash = 4, k_state = 5, k_state_ack = 6 };
constexpr uint32_t k_chunk_size = 8000;     // under the page server's 8 KB datagram limit
constexpr int32_t k_chunks_per_frame = 6;   // ~3 MB/s at 60 frames: inside the page server's per-page rate
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

    // the snapshot a join starts from: the host sends it, the joiner receives it
    uint8_t *snapshot = nullptr;
    uint32_t snapshot_size = 0;
    int32_t snapshot_chunks_done = 0;  // host: chunks the joiner has in order; joiner: chunks received in order
    int32_t snapshot_tick = 0;
    uint32_t snapshot_game_seed = 0;
    uint32_t snapshot_effect_seed = 0;
    int32_t snapshot_camera_end_tick = 0;
    bool awaiting_snapshot = false;    // joiner: connected, but the game it joins has not been applied yet
    char peer_name[16] = {};
    int32_t snapshot_chunks_sent = 0;  // host: chunks sent so far (a window ahead of what the joiner has)
    uint32_t snapshot_progress_ms = 0; // host: when the joiner last acknowledged more
    bool skip_pressed = false;  // a local cinematic skip waiting for the next stamped action
    uint32_t menu_request = 0;  // k_revert_flag / k_restart_flag waiting for the next stamped action
    bool peer_lost = false;     // the partner left: their player stands idle and nothing is sent
    uint32_t last_heard_ms = 0;
    bool heard_this_level = false;
    bool stop_ticks = false;    // after_tick: the frame runs no more ticks
    bool in_tick = false;
    uint32_t held_control_flags[k_max_players] = {};  // per player, as UpdateClient::queue_apply_tick keeps them
    uint32_t pressed_since_stamp = 0;  // buttons down in any frame since the last stamped tick, so a short press counts
    float previous_yaw[k_max_players] = {};   // the last applied desired facing, for hs look tests
    float previous_pitch[k_max_players] = {};
};

session g;
auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
auto &pending_difficulty = halo::link::ref<int16_t>(halo::ui::vars().pending_difficulty);
auto &split_screen_quit_prompt_string = halo::link::ref<uint16_t>(halo::ui::vars().split_screen_quit_prompt_string);
random_seed g_tick_effect_seed;  // halo::math::simulation_effect_seed under lockstep
int32_t g_camera_script_end_tick;

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

    return (coop_allowed() || g.test_host) && g.kind != role::joiner && main.main_menu_scenario_loaded == 0 &&
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
    memset(g.previous_yaw, 0, sizeof(g.previous_yaw));
    memset(g.held_control_flags, 0, sizeof(g.held_control_flags));
    g.pressed_since_stamp = 0;
    memset(g.previous_pitch, 0, sizeof(g.previous_pitch));
    g.last_heard_ms = halo::platform::tick_milliseconds();
    g.heard_this_level = false;
    g.next_local_tick = start_tick + k_input_delay;
    g.peer_contiguous = start_tick + k_input_delay - 1;
    g.peer_acked = start_tick + k_input_delay - 1;
    for (int32_t i = 0; i < k_hash_ring; i++) {
        g.my_hash_tick[i] = -1;
        g.their_hash_tick[i] = -1;
    }
}

const char *host_name()
{
    static char name[0x20];

    halo::text::string_convert_unicode_to_ascii(reinterpret_cast<uint8_t *>(name),
        reinterpret_cast<uint16_t *>(profile_globals_block[0].profile.name), sizeof(name));
    return name[0] != 0 ? name : "Halo";
}

/** A line on this machine's HUD, as multiplayer shows players joining and leaving. */
void show_message(const char *format, ...)
{
    char text[0x80];
    uint16_t wide[0x80];
    va_list arguments;
    size_t i;

    va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    for (i = 0; text[i] != 0 && i < 0x7f; i++) {
        wide[i] = static_cast<uint8_t>(text[i]);
    }
    wide[i] = 0;
    halo::interface::hud_message_broadcast_to_local_players(wide);
}

int32_t snapshot_chunk_count()
{
    return static_cast<int32_t>((g.snapshot_size + k_chunk_size - 1) / k_chunk_size);
}

/** The joining player, added on both machines right after the snapshot: dead, so the co-op respawn places it. */
void add_joining_player(bool local)
{
    datum_index player = halo::game::player_new_network(k_datum_index_none, 0, local ? 0 : -1, 0);

    if (player != k_datum_index_none) {
        halo::game::player_at(player)->deaths = 1;
        if (local) {
            halo::game::globals().local_player_globals->local_players[0] = player;
        }
    }
}

/** What both machines do once the snapshot is the game state: the streams, the tick, and the joining player. */
void start_from_snapshot(bool joiner)
{
    halo::math::globals().random_seed_global = g.snapshot_game_seed;
    g_tick_effect_seed = g.snapshot_effect_seed;
    g_camera_script_end_tick = g.snapshot_camera_end_tick;
    if (joiner) {
        // the snapshot is the host's view: its player was the local one
        datum_index *local_players = halo::game::globals().local_player_globals->local_players;

        if (local_players[0] != k_datum_index_none) {
            halo::game::player_at(local_players[0])->local_player_index = -1;
            local_players[0] = k_datum_index_none;
        }
        halo::game::globals().player_control->local_players[0].unit = k_datum_index_none;
    }
    reset_actions(g.snapshot_tick);
    add_joining_player(joiner);
    printf("lockstep: playing together from tick %d\n", g.snapshot_tick);
    if (joiner) {
        show_message("Joined %s's game.", g.peer_name[0] != 0 ? g.peer_name : "the host");
    } else {
        show_message("%s joined the game.", g.peer_name[0] != 0 ? g.peer_name : "A player");
    }
}

/** The host only lets a player in when nothing the main loop acts on between frames is pending. */
bool join_allowed()
{
    const main_globals &main = halo::main::globals().main_globals;

    return main.lost_map == 0 && main.won_map == 0 && main.revert_map == 0 && main.revert_map_if_allowed == 0 &&
        main.reset_map == 0 && main.level_transition == 0 && main.save_map == 0 && main.save_map_write_pending == 0 &&
        main.respawn_coop_players == 0 && main.switch_structure_bsp_index == -1 &&
        halo::game::globals().game_time->initialized != 0;
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

    if (!g.connected) {
        return;
    }
    if (count < 0) {
        count = 0;  // nothing new: the packet still carries the ack and says this machine is there
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
        if (g.desyncs == 0) {
            show_message("Co-op games are out of sync.");
        }
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
                uint32_t size = 0;
                const uint8_t *live = halo::saved_games::game_state_snapshot_bytes(&size);

                if (!join_allowed() || (g.snapshot = static_cast<uint8_t *>(malloc(size))) == nullptr) {
                    continue;  // the joiner asks again shortly
                }
                g.connected = true;
                g.peer_address = from.sin_addr.s_addr;
                g.peer_port = from.sin_port;
                g.player_count = 2;
                g.local_slot = 0;
                memset(g.peer_name, 0, sizeof(g.peer_name));
                if (size > 8) {
                    memcpy(g.peer_name, buffer + 8, size - 8 < (ssize_t)sizeof(g.peer_name) - 1 ? size - 8 : sizeof(g.peer_name) - 1);
                }
                g.difficulty = halo::main::globals().game_globals->difficulty;
                strncpy(g.level, halo::main::globals().main_globals.scenario_path, sizeof(g.level) - 1);

                memcpy(g.snapshot, live, size);
                g.snapshot_size = size;
                g.snapshot_chunks_done = 0;
                g.snapshot_chunks_sent = 0;
                g.snapshot_progress_ms = halo::platform::tick_milliseconds();
                g.snapshot_tick = game_tick();
                g.snapshot_game_seed = halo::math::globals().random_seed_global;
                g.snapshot_effect_seed = g_tick_effect_seed;
                g.snapshot_camera_end_tick = g_camera_script_end_tick;
                g.epoch++;
                // the host starts from the snapshot too, so both machines have run the same after-load callbacks
                halo::saved_games::game_state_apply_snapshot(g.snapshot);
                start_from_snapshot(false);
            }
            if (from.sin_addr.s_addr == g.peer_address) {
                uint8_t reply[48 + sizeof(g.level)];
                uint8_t *at = header(reply, k_welcome, 1);
                uint8_t players = static_cast<uint8_t>(g.player_count);

                put(at, &players, 1);
                put(at, &g.difficulty, 2);
                put(at, g.level, strlen(g.level) + 1);
                put(at, &g.snapshot_tick, 4);
                put(at, &g.snapshot_size, 4);
                put(at, &g.snapshot_game_seed, 4);
                put(at, &g.snapshot_effect_seed, 4);
                put(at, &g.snapshot_camera_end_tick, 4);
                put(at, &g.epoch, 2);
                put(at, host_name(), strlen(host_name()) + 1);
                send_to(g.peer_address, g.peer_port, reply, at - reply);
            }
        } else if (type == k_welcome && g.kind == role::joiner && !g.connected && size >= 12) {
            const uint8_t *end = buffer + size;
            const uint8_t *level = buffer + 11;
            const uint8_t *nul = static_cast<const uint8_t *>(memchr(level, 0, end - level));

            if (nul == nullptr || end - (nul + 1) < 22) {
                continue;
            }
            const uint8_t *at = nul + 1;
            uint32_t snapshot_size;

            memcpy(&snapshot_size, at + 4, 4);
            if (snapshot_size == 0 || snapshot_size > (64u << 20) || (g.snapshot = static_cast<uint8_t *>(malloc(snapshot_size))) == nullptr) {
                continue;
            }
            g.connected = true;
            g.peer_address = from.sin_addr.s_addr;
            g.peer_port = from.sin_port;
            g.local_slot = slot;
            g.player_count = buffer[8];
            memcpy(&g.difficulty, buffer + 9, 2);
            strncpy(g.level, reinterpret_cast<const char *>(level), sizeof(g.level) - 1);
            memcpy(&g.snapshot_tick, at, 4);
            g.snapshot_size = snapshot_size;
            memcpy(&g.snapshot_game_seed, at + 8, 4);
            memcpy(&g.snapshot_effect_seed, at + 12, 4);
            memcpy(&g.snapshot_camera_end_tick, at + 16, 4);
            memcpy(&g.epoch, at + 20, 2);
            memset(g.peer_name, 0, sizeof(g.peer_name));
            if (end - (at + 22) > 0) {
                memcpy(g.peer_name, at + 22, end - (at + 22) < (ptrdiff_t)sizeof(g.peer_name) - 1 ? end - (at + 22) : sizeof(g.peer_name) - 1);
            }
            g.snapshot_chunks_done = 0;
            g.awaiting_snapshot = true;
            printf("lockstep: joining %s, receiving the game (%u KB)\n", g.level, snapshot_size / 1024);
            queue_level(g.level, g.difficulty);
        } else if (type == k_state && g.awaiting_snapshot && from.sin_addr.s_addr == g.peer_address && epoch == g.epoch &&
                   size >= 10) {
            uint16_t index;

            memcpy(&index, buffer + 8, 2);
            if (index == g.snapshot_chunks_done) {
                uint32_t offset = index * k_chunk_size;
                uint32_t length = static_cast<uint32_t>(size - 10);

                if (offset < g.snapshot_size && length <= g.snapshot_size - offset) {
                    memcpy(g.snapshot + offset, buffer + 10, length);
                    g.snapshot_chunks_done++;
                }
            }
        } else if (type == k_state_ack && g.kind == role::host && g.snapshot != nullptr && from.sin_addr.s_addr == g.peer_address &&
                   epoch == g.epoch && size >= 12) {
            int32_t done;

            memcpy(&done, buffer + 8, 4);
            if (done > g.snapshot_chunks_done) {
                g.snapshot_chunks_done = done;
                g.snapshot_progress_ms = halo::platform::tick_milliseconds();
            }
            g.last_heard_ms = halo::platform::tick_milliseconds();
        } else if (g.connected && from.sin_addr.s_addr == g.peer_address && epoch == g.epoch && in_level()) {
            g.last_heard_ms = halo::platform::tick_milliseconds();
            g.heard_this_level = g.heard_this_level || type == k_input;
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

namespace {

/** The CO-OP choice is kept between visits (Continue starts a game without Choose Difficulty). */
const char *coop_option_path()
{
    static char path[0x200];
    const char *home = getenv("HOME");

    if (home == nullptr) {
        return nullptr;
    }
    snprintf(path, sizeof(path), "%s/coop_option", home);
    return path;
}

}  // namespace

bool coop_allowed()
{
    static bool loaded;

    if (!loaded) {
        const char *path = coop_option_path();
        FILE *file = path != nullptr ? fopen(path, "rb") : nullptr;

        loaded = true;
        if (file != nullptr) {
            g.coop_allowed = fgetc(file) == '1';
            fclose(file);
        }
    }
    return g.coop_allowed;
}

void set_coop_allowed(bool allowed)
{
    const char *path = coop_option_path();
    FILE *file = path != nullptr ? fopen(path, "wb") : nullptr;

    coop_allowed();
    g.coop_allowed = allowed;
    if (file != nullptr) {
        fputc(allowed ? '1' : '0', file);
        fclose(file);
    }
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
    if (g.kind == role::host && g.snapshot != nullptr) {
        // a window of chunks ahead of what the joiner has, each sent once; resent from what it has when nothing moves
        // for half a second (a joiner still loading the level reads nothing, and must not find a pile of repeats)
        int32_t count = snapshot_chunk_count();
        uint32_t now_ms = halo::platform::tick_milliseconds();

        if (g.snapshot_chunks_sent < g.snapshot_chunks_done || now_ms - g.snapshot_progress_ms > 500) {
            g.snapshot_chunks_sent = g.snapshot_chunks_done;
            g.snapshot_progress_ms = now_ms;
        }
        for (int32_t i = g.snapshot_chunks_sent; i < count && i < g.snapshot_chunks_done + k_chunks_per_frame; i++, g.snapshot_chunks_sent++) {
            uint8_t packet[16 + k_chunk_size];
            uint8_t *at = header(packet, k_state, 0);
            uint16_t index = static_cast<uint16_t>(i);
            uint32_t offset = i * k_chunk_size;
            uint32_t length = g.snapshot_size - offset < k_chunk_size ? g.snapshot_size - offset : k_chunk_size;

            put(at, &index, 2);
            put(at, g.snapshot + offset, length);
            send_to(g.peer_address, g.peer_port, packet, at - packet);
        }
        if (g.snapshot_chunks_done >= count) {
            free(g.snapshot);
            g.snapshot = nullptr;
        }
    }
    if (g.awaiting_snapshot) {
        uint8_t ack[16];
        uint8_t *at = header(ack, k_state_ack, static_cast<uint8_t>(g.local_slot));

        put(at, &g.snapshot_chunks_done, 4);
        send_to(g.peer_address, g.peer_port, ack, at - ack);
        // the level is loaded (it was queued on WELCOME) and the whole game has arrived: take it over
        if (g.snapshot_chunks_done >= snapshot_chunk_count() && halo::main::globals().main_globals.main_menu_scenario_loaded == 0 &&
            halo::main::globals().main_globals.level_transition == 0 && halo::game::globals().game_time->initialized != 0) {
            halo::saved_games::game_state_apply_snapshot(g.snapshot);
            free(g.snapshot);
            g.snapshot = nullptr;
            g.awaiting_snapshot = false;
            start_from_snapshot(true);
        }
    }
    if (in_level() && !g.awaiting_snapshot) {
        send_inputs();
    }
    g.frames++;
    if (g.kind == role::joiner && !g.connected && g.frames % 30 == 1) {
        uint8_t hello[8 + 16];
        uint8_t *at = header(hello, k_hello, 0xff);
        const char *name = host_name();  // this profile's name, as the host will show it
        size_t length = strlen(name) < 15 ? strlen(name) : 15;

        put(at, name, length);
        *at++ = 0;
        send_to(g.join_address, htons(k_port), hello, at - hello);
    }
#endif
}

void on_new_map(uint32_t game_seed)
{
    g_tick_effect_seed = game_seed ^ 0x5eed5eedu;
    if (g.connected && !g.awaiting_snapshot) {  // a joiner's epoch is the host's, from WELCOME
        g.epoch++;
        reset_actions(0);
    }
}

void on_revert()
{
    if (g.connected) {
        // the checkpoint does not hold the random streams, and the machines may have run different numbers of ticks
        // before reverting: both restart from the restored game time
        uint32_t seed = static_cast<uint32_t>(game_tick()) * 0x9e3779b9u ^ halo::main::globals().game_globals->random_seed;

        halo::math::globals().random_seed_global = seed;
        g_tick_effect_seed = seed ^ 0x5eed5eedu;
        g.epoch++;
        reset_actions(game_tick());
    }
}

bool after_tick()
{
    if (!in_level()) {
        return false;
    }
    main_globals &main = halo::main::globals().main_globals;

    if (main.lost_map != 0 && halo::game::globals().game_time->paused == 0) {
        main.lost_map_frames = static_cast<int16_t>(main.lost_map_frames + 1);
        if (main.lost_map_frames > k_main_revert_delay_frames) {
            g.stop_ticks = true;  // the main loop reverts at the next frame; no tick runs past this one
        }
    }
    if (main.save_map != 0) {
        halo::main::main_save_map_private();
    }
    halo::main::main_checkpoint_write_service();
    if (main.switch_structure_bsp_index != -1) {
        halo::main::main_switch_structure_bsp_and_notify();
        halo::game::players_structure_bsp_switch_regroup();  // as split screen does: the partner comes along
    }
    if (main.respawn_coop_players != 0 && halo::game::globals().game_time->paused == 0 &&
        halo::cutscene::globals().cinematic_globals->in_progress == 0) {
        int16_t ticks = main.respawn_coop_frames;

        main.respawn_coop_frames = static_cast<int16_t>(ticks + 1);
        if (ticks > k_main_respawn_delay_frames && halo::game::game_engine_attach_players_to_new_bsp() != 0) {
            main.respawn_coop_players = 0;
            main.respawn_coop_frames = 0;
        }
    }
    bool stop = g.stop_ticks;

    g.stop_ticks = false;
    return stop;
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

void local_skip_pressed()
{
    g.skip_pressed = true;
}

void local_revert_requested()
{
    g.menu_request |= k_revert_flag;
}

void local_restart_requested()
{
    g.menu_request |= k_restart_flag;
}

bool in_tick()
{
    return g.in_tick;
}

void camera_script_started(float seconds)
{
    g_camera_script_end_tick = game_tick() + static_cast<int32_t>(seconds * 30.0f);
}

int32_t camera_script_ticks_remaining()
{
    int32_t remaining = g_camera_script_end_tick - game_tick();

    return remaining > 0 ? remaining : 0;
}

void capture_local_action(const player_action &action, bool back)
{
    g.latest_local = action;
    g.latest_local.pad_1e = 0;
    if (back) {
        g.latest_local.control_flags |= k_back_flag;
    }
    g.pressed_since_stamp |= g.latest_local.control_flags;
}

int32_t schedule_ticks(int32_t wanted)
{
    if (!in_level()) {
        return wanted;
    }
    if (g.awaiting_snapshot) {
        return 0;  // the level it joins is not this one's own start but the host's snapshot, still arriving
    }
    int32_t now = game_tick();

    while (g.next_local_tick < now + wanted + k_input_delay && g.next_local_tick < now + k_ring - 1) {
        g.actions[g.next_local_tick % k_ring][g.local_slot] = g.latest_local;
        g.actions[g.next_local_tick % k_ring][g.local_slot].control_flags |= g.pressed_since_stamp;
        g.pressed_since_stamp = 0;
        if (probe_active()) {
            probe_scripted_action(g.next_local_tick, &g.actions[g.next_local_tick % k_ring][g.local_slot]);
        }
        if (g.skip_pressed) {
            g.actions[g.next_local_tick % k_ring][g.local_slot].control_flags |= k_skip_cinematic_flag;
            g.skip_pressed = false;
        }
        g.actions[g.next_local_tick % k_ring][g.local_slot].control_flags |= g.menu_request;
        g.menu_request = 0;
        g.action_tick[g.next_local_tick % k_ring][g.local_slot] = g.next_local_tick;
        g.next_local_tick++;
    }
#if defined(__EMSCRIPTEN__)
    send_inputs();
#endif
    int32_t ready = 0;
    uint32_t now_ms = halo::platform::tick_milliseconds();

    if (!g.peer_lost && now_ms - g.last_heard_ms > (g.heard_this_level ? k_peer_timeout_ms : k_peer_loading_timeout_ms)) {
        g.peer_lost = true;
        printf("lockstep: nothing from the other player for %u s, carrying on without them\n", (now_ms - g.last_heard_ms) / 1000);
        show_message("%s left the game.", g.peer_name[0] != 0 ? g.peer_name : "The other player");
    }
    if (g.peer_lost) {
        player_action idle;

        memset(&idle, 0, sizeof(idle));
        idle.weapon_index = -1;
        idle.grenade_index = -1;
        idle.zoom_level = -1;
        for (int32_t t = now; t < now + wanted; t++) {
            if (g.action_tick[t % k_ring][peer_slot()] != t) {
                g.actions[t % k_ring][peer_slot()] = idle;
                g.action_tick[t % k_ring][peer_slot()] = t;
            }
        }
    }

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
        bool skip = false;
        uint32_t menu = 0;

        for (int32_t s = 0; s < g.player_count; s++) {
            player_control_input input;
            bool back;

            actions[s] = g.actions[t % k_ring][s];
            skip = skip || (actions[s].control_flags & k_skip_cinematic_flag) != 0;
            back = (actions[s].control_flags & k_back_flag) != 0;
            menu |= actions[s].control_flags & (k_revert_flag | k_restart_flag);
            actions[s].control_flags &= ~k_lockstep_flags;

            // the hs player_action_test flags, from every player's applied action rather than one machine's buttons:
            // digitize_control_input on the input this action came from (action is button 2, the 0x40 control bit)
            memset(&input, 0, sizeof(input));
            input.throttle_x = actions[s].throttle_x;
            input.throttle_y = actions[s].throttle_y;
            input.primary_trigger = actions[s].primary_trigger;
            input.yaw_delta = actions[s].desired_yaw - g.previous_yaw[s];
            if (input.yaw_delta > 3.1415927f) {  // the facing wraps at 2 pi
                input.yaw_delta -= 6.2831855f;
            } else if (input.yaw_delta < -3.1415927f) {
                input.yaw_delta += 6.2831855f;
            }
            input.pitch_delta = actions[s].desired_pitch - g.previous_pitch[s];
            input.action = (actions[s].control_flags & 0x40) != 0 ? 1 : 0;
            input.melee = back ? 1 : 0;
            input.control_flags = actions[s].control_flags;
            g.previous_yaw[s] = actions[s].desired_yaw;
            g.previous_pitch[s] = actions[s].desired_pitch;
            halo::game::engine1::LocalControl::digitize_control_input(&input);

            // what queue_apply_tick does to the buttons on the normal path: some act on the press only
            uint32_t flags = actions[s].control_flags;

            actions[s].control_flags = ~g.held_control_flags[s] & flags;
            g.held_control_flags[s] = flags & k_held_control_flags_mask;
        }
        // what digitize_control_input does for one machine's own button, here for anyone's, on the same tick everywhere
        if (menu != 0) {
            // what the menu's revert / restart buttons do (uis_events_1.cpp slots 11 and 12), for everyone
            main_globals &main = halo::main::globals().main_globals;

            halo::networking::globals().join_error_reason = 0;
            main.lost_map = 0;
            split_screen_quit_prompt_string = 0xffff;
            if (menu & k_restart_flag) {
                main.reset_map = 1;
            } else {
                main.revert_map = 1;
            }
            g.stop_ticks = true;
        }
        if (skip && halo::cutscene::globals().cinematic_globals->skip_in_progress != 0) {
            split_screen_quit_prompt_string = 0xffff;
            halo::networking::globals().join_error_reason = 0;
            halo::main::globals().main_globals.revert_map_if_allowed = 1;
            g.stop_ticks = true;
        }
        return;
    }
    probe_override_actions(actions);
}

void tick_begin()
{
    halo::math::set_simulation_effect_seed(active() ? &g_tick_effect_seed : nullptr);
    g.in_tick = true;
}

void tick_end()
{
    g.in_tick = false;
    if (!active()) {
        return;
    }
#if defined(__EMSCRIPTEN__)
    int32_t tick = game_tick();

    if (in_level() && !g.awaiting_snapshot && tick % k_hash_interval == 0) {
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

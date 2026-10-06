/**
 * @file src/game/lockstep.cpp
 * Lockstep co-op: every machine runs the whole campaign simulation and the machines only exchange each player's
 * per-tick action. A tick runs once every player's action for it has arrived, so the simulations stay identical; every
 * 30 ticks the machines compare a state hash and print LSP-DESYNC if they differ.
 *
 * Players are created in slot order on every machine (slot 0 the host, then the joiners, up to k_max_players); each
 * machine marks its own slot as the local player. Local actions are stamped k_input_delay ticks ahead, which hides
 * that much latency.
 *
 * Hosting and joining: while a single-player campaign level runs with co-op switched on (the CO-OP row of Choose
 * Difficulty, src/interface/coop_option.cpp), the game is open for co-op. It answers the server browser's search
 * (GameSpy qr2 on k_query_port) as a "Co-op" row with the level and its players; picking the row (join_from_browser)
 * sends the host HELLO. The host marks the join in its own action for a tick J nobody can have run yet; right before J
 * every machine applies its own game state as a snapshot (the after-load callbacks a revert runs, so all have run the
 * same ones) and adds the joiner's player, dead, so the co-op respawn brings it in beside a living player. The host
 * answers WELCOME and streams its snapshot from J in STATE chunks; the joiner loads the level, applies the snapshot the
 * same way and resumes at J. Everyone else waits for the joiner's first actions meanwhile.
 * For testing in one browser: -coop host starts b30 by itself, -coop join broadcasts HELLO to whichever host answers,
 * and -coop list is a second game (own profile, see web/shell.html) that joins through the server browser.
 *
 * Transport (web build): UDP on k_port over the page server's virtual LAN, the host in the middle: a joiner sends to
 * the host only, and the host relays each joiner's actions to the others. Only the host decides that a player has
 * left; it then sends idle actions for that slot itself. Messages start with u32 magic, u8 type, u8 slot, u16 epoch;
 * the epoch counts level starts and reverts, so actions from before one are ignored after it.
 *   HELLO   the joiner's profile name (NUL terminated)
 *   WELCOME u8 player count, i16 difficulty, the level path (NUL terminated), then the snapshot: i32 tick, u32 size,
 *           u32 game seed, u32 simulation effect seed, i32 camera script end tick, u16 epoch, then per existing slot
 *           u32 held control flags, f32 previous yaw, f32 previous pitch; then the host's name
 *   STATE   u16 chunk index, the chunk (k_chunk_size bytes, the last one shorter)
 *   STATE_ACK i32 chunks received in order
 *   INPUT   (the slot's actions) i32 what everyone has of the receiver's actions (host to joiner, else -1), u8 n,
 *           n x i32 the last tick the sender has every action of for each slot, i32 first tick, u8 count, actions
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
constexpr int32_t k_max_players = 4;
constexpr int32_t k_hash_interval = 30;
constexpr int32_t k_hash_ring = 64;
constexpr int32_t k_actions_per_packet = 64;
constexpr uint32_t k_peer_timeout_ms = 10000;          // nothing from a player this long: carry on without them
constexpr uint32_t k_peer_loading_timeout_ms = 300000; // before their first action of a level (they may be downloading its map)
constexpr char k_test_level[] = "levels\\b30\\b30";
constexpr uint32_t k_skip_cinematic_flag = 0x80000000u;  // in player_action::control_flags; units use the low 16 bits
constexpr uint32_t k_held_control_flags_mask = 0x4d0;    // UpdateClient::queue_apply_tick: these act on the press only
constexpr uint32_t k_back_flag = 0x40000000u;            // Back held (player_control_input::melee), for hs player_action_test_back
constexpr uint32_t k_revert_flag = 0x20000000u;          // menu: revert to the last checkpoint
constexpr uint32_t k_restart_flag = 0x10000000u;         // menu: restart the level
constexpr uint32_t k_join_flag = 0x08000000u;            // host only: a player joins right before this tick
constexpr uint32_t k_lockstep_flags = k_skip_cinematic_flag | k_back_flag | k_revert_flag | k_restart_flag | k_join_flag;
constexpr char k_coop_gametype[] = "Co-op";

enum : uint8_t { k_hello = 1, k_welcome = 2, k_input = 3, k_hash = 4, k_state = 5, k_state_ack = 6 };
constexpr uint32_t k_chunk_size = 8000;     // under the page server's 8 KB datagram limit
constexpr int32_t k_chunks_per_frame = 6;   // ~3 MB/s at 60 frames: inside the page server's per-page rate
enum class role { none, host, joiner };

/** One player slot of the game. */
struct member {
    bool present = false;
    bool lost = false;              // left: the host sends idle actions for it
    uint32_t address = 0;           // network order; the host knows every joiner's, a joiner only the host's (slot 0)
    uint16_t port = 0;
    char name[16] = {};
    uint32_t last_heard_ms = 0;
    bool heard_this_level = false;
    int32_t contiguous = -1;        // this machine has every action of this slot up to this tick
    int32_t their_view[k_max_players] = {};  // host: what this joiner has of each slot, from its INPUT packets
    int32_t hash_tick[k_hash_ring] = {};     // host: this joiner's hashes; joiner: the host's (slot 0)
    uint32_t hash[k_hash_ring] = {};
};

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
    uint16_t epoch = 0;
    int32_t frames = 0;
    member members[k_max_players];
    int32_t resend_from = -1;         // everyone has this machine's actions up to this tick

    player_action actions[k_ring][k_max_players];
    int32_t action_tick[k_ring][k_max_players];
    player_action latest_local;
    int32_t next_local_tick = 0;

    int32_t my_hash_tick[k_hash_ring];
    uint32_t my_hash[k_hash_ring];
    int32_t desyncs = 0;
    int16_t difficulty = 0;
    char level[0x100] = {};

    // a join: the host's pending one, then the snapshot the host sends and the joiner receives
    bool join_pending = false;        // host: a HELLO was accepted, the join tick not stamped yet
    int32_t join_tick = -1;           // host: the tick its action marks for the join
    int32_t rebased_tick = -1;        // the join tick this machine has already acted on
    uint8_t *snapshot = nullptr;
    uint32_t snapshot_size = 0;
    int32_t snapshot_slot = 0;        // the joining slot
    int32_t snapshot_chunks_done = 0; // host: chunks the joiner has in order; joiner: chunks received in order
    int32_t snapshot_chunks_sent = 0;
    uint32_t snapshot_progress_ms = 0;
    int32_t snapshot_tick = 0;
    uint32_t snapshot_game_seed = 0;
    uint32_t snapshot_effect_seed = 0;
    int32_t snapshot_camera_end_tick = 0;
    uint32_t snapshot_held[k_max_players] = {};
    float snapshot_yaw[k_max_players] = {};
    float snapshot_pitch[k_max_players] = {};
    bool awaiting_snapshot = false;   // joiner: connected, but the game it joins has not been applied yet

    bool skip_pressed = false;        // a local cinematic skip waiting for the next stamped action
    uint32_t menu_request = 0;        // k_revert_flag / k_restart_flag waiting for the next stamped action
    bool stop_ticks = false;          // after_tick: the frame runs no more ticks
    bool in_tick = false;
    uint32_t held_control_flags[k_max_players] = {};  // per player, as UpdateClient::queue_apply_tick keeps them
    uint32_t pressed_since_stamp = 0; // buttons down in any frame since the last stamped tick, so a short press counts
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

bool is_host()
{
    return g.kind == role::host;
}

player_action idle_action()
{
    player_action idle;

    memset(&idle, 0, sizeof(idle));
    idle.weapon_index = -1;
    idle.grenade_index = -1;
    idle.zoom_level = -1;
    return idle;
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

/** Fills `slot`'s actions with idle ones for the ticks before its first own action (every machine the same way). */
void prefill_idle(int32_t slot, int32_t start_tick)
{
    for (int32_t t = start_tick; t < start_tick + k_input_delay; t++) {
        g.actions[t % k_ring][slot] = idle_action();
        g.action_tick[t % k_ring][slot] = t;
    }
    g.members[slot].contiguous = start_tick + k_input_delay - 1;
}

/** A level start or revert: every slot's actions begin again at start_tick. */
void reset_actions(int32_t start_tick)
{
    int32_t first = start_tick + k_input_delay - 1;

    for (int32_t i = 0; i < k_ring; i++) {
        for (int32_t s = 0; s < k_max_players; s++) {
            g.action_tick[i][s] = -1;
        }
    }
    for (int32_t s = 0; s < g.player_count; s++) {
        member &m = g.members[s];

        prefill_idle(s, start_tick);
        m.last_heard_ms = halo::platform::tick_milliseconds();
        m.heard_this_level = false;
        for (int32_t v = 0; v < k_max_players; v++) {
            m.their_view[v] = first;
        }
        for (int32_t i = 0; i < k_hash_ring; i++) {
            m.hash_tick[i] = -1;
        }
    }
    g.latest_local = idle_action();
    memset(g.previous_yaw, 0, sizeof(g.previous_yaw));
    memset(g.previous_pitch, 0, sizeof(g.previous_pitch));
    memset(g.held_control_flags, 0, sizeof(g.held_control_flags));
    g.pressed_since_stamp = 0;
    g.next_local_tick = start_tick + k_input_delay;
    g.resend_from = first;
    for (int32_t i = 0; i < k_hash_ring; i++) {
        g.my_hash_tick[i] = -1;
    }
}

const char *host_name()
{
    static char name[0x20];

    halo::text::string_convert_unicode_to_ascii(reinterpret_cast<uint8_t *>(name),
        reinterpret_cast<uint16_t *>(profile_globals_block[0].profile.name), sizeof(name));
    return name[0] != 0 ? name : "Halo";
}

const char *member_name(int32_t slot, const char *fallback)
{
    return g.members[slot].name[0] != 0 ? g.members[slot].name : fallback;
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

/** The joining player, added on every machine right after the snapshot: dead, so the co-op respawn places it. */
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

/** The host only lets a player in when nothing the main loop acts on between frames is pending. */
bool join_allowed()
{
    const main_globals &main = halo::main::globals().main_globals;

    return main.lost_map == 0 && main.won_map == 0 && main.revert_map == 0 && main.revert_map_if_allowed == 0 &&
        main.reset_map == 0 && main.level_transition == 0 && main.save_map == 0 && main.save_map_write_pending == 0 &&
        main.respawn_coop_players == 0 && main.switch_structure_bsp_index == -1 &&
        halo::game::globals().game_time->initialized != 0 && !g.join_pending && g.snapshot == nullptr &&
        g.player_count < k_max_players;
}

/**
 * Right before the join tick, on every machine already playing: the game state becomes itself through the after-load
 * callbacks (as the joiner's will from the host's snapshot), and the joining slot gets its player and idle actions.
 */
void rebase_for_join(int32_t tick)
{
    uint32_t size = 0;
    const uint8_t *live = halo::saved_games::game_state_snapshot_bytes(&size);
    int32_t slot = g.player_count;

    g.rebased_tick = tick;
    if (is_host()) {
        g.snapshot = static_cast<uint8_t *>(malloc(size));
        if (g.snapshot != nullptr) {
            memcpy(g.snapshot, live, size);
            g.snapshot_size = size;
            g.snapshot_slot = slot;
            g.snapshot_chunks_done = 0;
            g.snapshot_chunks_sent = 0;
            g.snapshot_progress_ms = halo::platform::tick_milliseconds();
            g.snapshot_tick = tick;
            g.snapshot_game_seed = halo::math::globals().random_seed_global;
            g.snapshot_effect_seed = g_tick_effect_seed;
            g.snapshot_camera_end_tick = g_camera_script_end_tick;
            for (int32_t s = 0; s < slot; s++) {
                g.snapshot_held[s] = g.held_control_flags[s];
                g.snapshot_yaw[s] = g.previous_yaw[s];
                g.snapshot_pitch[s] = g.previous_pitch[s];
            }
        }
        g.join_pending = false;
    }
    halo::saved_games::game_state_apply_snapshot(live);

    member &joining = g.members[slot];

    g.player_count = slot + 1;
    joining.present = true;
    joining.lost = false;
    joining.last_heard_ms = halo::platform::tick_milliseconds();
    joining.heard_this_level = false;
    for (int32_t v = 0; v < k_max_players; v++) {
        joining.their_view[v] = tick - 1;  // it has nothing yet: everyone resends from the join tick
    }
    joining.their_view[slot] = tick + k_input_delay - 1;
    for (int32_t i = 0; i < k_hash_ring; i++) {
        joining.hash_tick[i] = -1;
    }
    prefill_idle(slot, tick);
    g.held_control_flags[slot] = 0;
    g.previous_yaw[slot] = 0.0f;
    g.previous_pitch[slot] = 0.0f;
    add_joining_player(false);
    printf("lockstep: player %d joins at tick %d\n", slot, tick);
    show_message("%s joined the game.", is_host() ? member_name(slot, "A player") : "A player");
}

/** The joiner, once the host's snapshot is the game state. */
void start_from_snapshot()
{
    int32_t slot = g.local_slot;

    halo::math::globals().random_seed_global = g.snapshot_game_seed;
    g_tick_effect_seed = g.snapshot_effect_seed;
    g_camera_script_end_tick = g.snapshot_camera_end_tick;
    {
        // the snapshot is the host's view: its player was the local one
        datum_index *local_players = halo::game::globals().local_player_globals->local_players;

        if (local_players[0] != k_datum_index_none) {
            halo::game::player_at(local_players[0])->local_player_index = -1;
            local_players[0] = k_datum_index_none;
        }
        halo::game::globals().player_control->local_players[0].unit = k_datum_index_none;
    }
    for (int32_t i = 0; i < k_ring; i++) {
        for (int32_t s = 0; s < k_max_players; s++) {
            g.action_tick[i][s] = -1;
        }
    }
    for (int32_t s = 0; s < g.player_count; s++) {
        member &m = g.members[s];

        m.present = true;
        m.contiguous = g.snapshot_tick - 1;  // the others' actions from the join tick on are resent to it
        m.last_heard_ms = halo::platform::tick_milliseconds();
        m.heard_this_level = false;
        for (int32_t i = 0; i < k_hash_ring; i++) {
            m.hash_tick[i] = -1;
        }
        g.held_control_flags[s] = s < slot ? g.snapshot_held[s] : 0;
        g.previous_yaw[s] = s < slot ? g.snapshot_yaw[s] : 0.0f;
        g.previous_pitch[s] = s < slot ? g.snapshot_pitch[s] : 0.0f;
    }
    prefill_idle(slot, g.snapshot_tick);
    g.latest_local = idle_action();
    g.pressed_since_stamp = 0;
    g.next_local_tick = g.snapshot_tick + k_input_delay;
    g.resend_from = g.snapshot_tick + k_input_delay - 1;
    g.rebased_tick = g.snapshot_tick;
    for (int32_t i = 0; i < k_hash_ring; i++) {
        g.my_hash_tick[i] = -1;
    }
    add_joining_player(true);
    printf("lockstep: playing together from tick %d as player %d of %d\n", g.snapshot_tick, slot, g.player_count);
    show_message("Joined %s's game.", member_name(0, "the host"));
}

/** Host: the last tick every other machine has of `slot`'s actions. */
int32_t everyone_has(int32_t slot)
{
    int32_t lowest = 0x7fffffff;

    for (int32_t m = 0; m < g.player_count; m++) {
        if (m == slot || !g.members[m].present || (m != 0 && g.members[m].lost)) {
            continue;
        }
        int32_t has = m == 0 ? g.members[slot].contiguous : g.members[m].their_view[slot];

        lowest = has < lowest ? has : lowest;
    }
    return lowest == 0x7fffffff ? g.next_local_tick - 1 : lowest;
}

void advance_contiguous(int32_t slot)
{
    member &m = g.members[slot];

    while (g.action_tick[(m.contiguous + 1) % k_ring][slot] == m.contiguous + 1) {
        m.contiguous++;
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

void send_to_member(int32_t slot, const uint8_t *data, size_t size)
{
    send_to(g.members[slot].address, g.members[slot].port, data, size);
}

/** Host: to every joiner that is still there, but `except`. */
void send_to_joiners(const uint8_t *data, size_t size, int32_t except)
{
    for (int32_t s = 1; s < g.player_count; s++) {
        if (s != except && g.members[s].present && !g.members[s].lost && g.members[s].address != 0) {
            send_to_member(s, data, size);
        }
    }
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

int32_t players_present()
{
    int32_t count = 0;

    for (int32_t s = 0; s < g.player_count; s++) {
        count += g.members[s].present && !g.members[s].lost ? 1 : 0;
    }
    return count > 0 ? count : 1;
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
        qr2_buffer_add_int(buffer, players_present());
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

/** The players still in the game, in slot order, for the player list under the browser's rows (key 0x15). */
int32_t present_slot(int32_t index)
{
    for (int32_t s = 0; s < g.player_count; s++) {
        if (s == 0 || (g.members[s].present && !g.members[s].lost)) {
            if (index-- == 0) {
                return s;
            }
        }
    }
    return 0;
}

void qr_player_key(int key, int index, void *buffer, void *)
{
    int32_t slot = present_slot(index);

    qr2_buffer_add(buffer, key != 0x15 ? "" : slot == 0 ? host_name() : member_name(slot, "Player"));
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
    return key_type == 1 ? players_present() : 0;
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

/** One INPUT packet of `slot`'s actions from first on; nothing new still carries the receipts and says it is there. */
size_t build_inputs(uint8_t *buffer, int32_t slot, int32_t first, int32_t told_everyone_has)
{
    int32_t count = g.members[slot].contiguous + 1 - first;
    uint8_t *at = header(buffer, k_input, static_cast<uint8_t>(slot));
    uint8_t n = static_cast<uint8_t>(g.player_count);

    if (slot == g.local_slot) {
        count = g.next_local_tick - first;
    }
    count = count < 0 ? 0 : count > k_actions_per_packet ? k_actions_per_packet : count;
    put(at, &told_everyone_has, 4);
    put(at, &n, 1);
    for (int32_t s = 0; s < g.player_count; s++) {
        int32_t has = s == g.local_slot ? g.next_local_tick - 1 : g.members[s].contiguous;

        put(at, &has, 4);
    }
    uint8_t count_byte = static_cast<uint8_t>(count);

    put(at, &first, 4);
    put(at, &count_byte, 1);
    for (int32_t t = first; t < first + count; t++) {
        put(at, &g.actions[t % k_ring][slot], sizeof(player_action));
    }
    return at - buffer;
}

void send_inputs()
{
    uint8_t buffer[64 + k_max_players * 4 + k_actions_per_packet * sizeof(player_action)];

    if (!g.connected || g.awaiting_snapshot) {
        return;
    }
    if (!is_host()) {
        send_to_member(0, buffer, build_inputs(buffer, g.local_slot, g.resend_from + 1, -1));
        return;
    }
    for (int32_t j = 1; j < g.player_count; j++) {
        if (!g.members[j].present || g.members[j].lost || g.members[j].address == 0) {
            continue;
        }
        // the host's own actions, with what everyone has of this joiner's
        send_to_member(j, buffer, build_inputs(buffer, 0, everyone_has(0) + 1, everyone_has(j)));
    }
    for (int32_t l = 1; l < g.player_count; l++) {
        if (g.members[l].present && g.members[l].lost) {
            // a player who left: the host speaks for it with idle actions
            send_to_joiners(buffer, build_inputs(buffer, l, everyone_has(l) + 1, -1), -1);
        }
    }
}

void receive_inputs(const uint8_t *packet, const uint8_t *end, int32_t slot, int32_t from_slot)
{
    const uint8_t *at = packet + 8;
    int32_t told;
    uint8_t n;
    int32_t first;
    uint8_t count;
    int32_t now = game_tick();

    if (end - at < 5) {
        return;
    }
    memcpy(&told, at, 4);
    n = at[4];
    at += 5;
    if (n > k_max_players || end - at < n * 4 + 5) {
        return;
    }
    const uint8_t *receipts = at;

    at += n * 4;
    memcpy(&first, at, 4);
    count = at[4];
    at += 5;
    if (end - at < static_cast<ptrdiff_t>(count * sizeof(player_action)) || slot >= g.player_count || slot == g.local_slot) {
        return;
    }
    for (int32_t t = first; t < first + count; t++, at += sizeof(player_action)) {
        if (t < now || t >= now + k_ring) {
            continue;
        }
        memcpy(&g.actions[t % k_ring][slot], at, sizeof(player_action));
        g.action_tick[t % k_ring][slot] = t;
    }
    advance_contiguous(slot);
    if (is_host() && from_slot == slot) {
        // a joiner's own actions: its receipts tell the host what it has, and the others get the actions relayed
        for (int32_t s = 0; s < n && s < k_max_players; s++) {
            int32_t has;

            memcpy(&has, receipts + s * 4, 4);
            if (has > g.members[slot].their_view[s]) {
                g.members[slot].their_view[s] = has;
            }
        }
        send_to_joiners(packet, end - packet, slot);
    } else if (!is_host() && slot == 0 && told >= 0) {
        g.resend_from = told;  // the host's word, lower too: after a join the newcomer needs the actions from its tick
    }
}

void compare_hash(int32_t index, int32_t slot)
{
    member &m = g.members[slot];

    if (g.my_hash_tick[index] < 0 || g.my_hash_tick[index] != m.hash_tick[index]) {
        return;
    }
    if (g.my_hash[index] != m.hash[index]) {
        if (g.desyncs == 0) {
            show_message("Co-op games are out of sync.");
        }
        if (g.desyncs++ < 20) {
            printf("LSP-DESYNC %d player %d mine=%08x theirs=%08x\n", g.my_hash_tick[index], slot, g.my_hash[index], m.hash[index]);
        }
    } else if (g.my_hash_tick[index] % (k_hash_interval * 10) == 0) {
        printf("LSP-SYNC %d player %d %08x\n", g.my_hash_tick[index], slot, g.my_hash[index]);
    }
    m.hash_tick[index] = -1;
}

void compare_hashes(int32_t index)
{
    for (int32_t s = 0; s < g.player_count; s++) {
        if (s != g.local_slot && g.members[s].present && !g.members[s].lost && (is_host() || s == 0)) {
            compare_hash(index, s);
        }
    }
}

void send_welcome(int32_t slot)
{
    uint8_t reply[96 + sizeof(g.level) + k_max_players * 12];
    uint8_t *at = header(reply, k_welcome, static_cast<uint8_t>(slot));
    uint8_t players = static_cast<uint8_t>(slot + 1);

    put(at, &players, 1);
    put(at, &g.difficulty, 2);
    put(at, g.level, strlen(g.level) + 1);
    put(at, &g.snapshot_tick, 4);
    put(at, &g.snapshot_size, 4);
    put(at, &g.snapshot_game_seed, 4);
    put(at, &g.snapshot_effect_seed, 4);
    put(at, &g.snapshot_camera_end_tick, 4);
    put(at, &g.epoch, 2);
    for (int32_t s = 0; s < slot; s++) {
        put(at, &g.snapshot_held[s], 4);
        put(at, &g.snapshot_yaw[s], 4);
        put(at, &g.snapshot_pitch[s], 4);
    }
    put(at, host_name(), strlen(host_name()) + 1);
    send_to_member(slot, reply, at - reply);
}

int32_t slot_of(uint32_t address, uint16_t port)
{
    for (int32_t s = 0; s < g.player_count; s++) {
        if (g.members[s].present && g.members[s].address == address && g.members[s].port == port) {
            return s;
        }
    }
    return -1;
}

void receive_hello(const uint8_t *buffer, ssize_t size, const sockaddr_in &from)
{
    int32_t known = slot_of(from.sin_addr.s_addr, from.sin_port);

    if (known > 0) {
        if (g.snapshot != nullptr && g.snapshot_slot == known) {
            send_welcome(known);  // the first WELCOME got lost
        }
        return;
    }
    if (!join_allowed()) {
        return;  // the joiner asks again shortly
    }
    int32_t slot = g.player_count;
    member &joining = g.members[slot];

    if (!g.connected) {
        // the first joiner: this game becomes a co-op session, starting from its own state
        g.connected = true;
        g.local_slot = 0;
        g.members[0] = member{};
        g.members[0].present = true;
        g.difficulty = halo::main::globals().game_globals->difficulty;
        strncpy(g.level, halo::main::globals().main_globals.scenario_path, sizeof(g.level) - 1);
        g.epoch++;
        reset_actions(game_tick());
    }
    joining = member{};
    joining.address = from.sin_addr.s_addr;
    joining.port = from.sin_port;
    if (size > 8) {
        memcpy(joining.name, buffer + 8, size - 8 < (ssize_t)sizeof(joining.name) - 1 ? size - 8 : sizeof(joining.name) - 1);
    }
    g.join_pending = true;
    g.join_tick = -1;
}

void receive_welcome(const uint8_t *buffer, ssize_t size, const sockaddr_in &from)
{
    const uint8_t *end = buffer + size;
    const uint8_t *level = buffer + 11;
    const uint8_t *nul = static_cast<const uint8_t *>(size > 11 ? memchr(level, 0, end - level) : nullptr);
    int32_t slot = buffer[5];
    int32_t players = buffer[8];

    if (nul == nullptr || slot < 1 || slot >= k_max_players || players != slot + 1 || end - (nul + 1) < 22 + slot * 12) {
        return;
    }
    const uint8_t *at = nul + 1;
    uint32_t snapshot_size;

    memcpy(&snapshot_size, at + 4, 4);
    if (snapshot_size == 0 || snapshot_size > (64u << 20) || (g.snapshot = static_cast<uint8_t *>(malloc(snapshot_size))) == nullptr) {
        return;
    }
    g.connected = true;
    g.local_slot = slot;
    g.player_count = players;
    for (int32_t s = 0; s < k_max_players; s++) {
        g.members[s] = member{};
    }
    g.members[0].address = from.sin_addr.s_addr;
    g.members[0].port = from.sin_port;
    memcpy(&g.difficulty, buffer + 9, 2);
    strncpy(g.level, reinterpret_cast<const char *>(level), sizeof(g.level) - 1);
    memcpy(&g.snapshot_tick, at, 4);
    g.snapshot_size = snapshot_size;
    memcpy(&g.snapshot_game_seed, at + 8, 4);
    memcpy(&g.snapshot_effect_seed, at + 12, 4);
    memcpy(&g.snapshot_camera_end_tick, at + 16, 4);
    memcpy(&g.epoch, at + 20, 2);
    at += 22;
    for (int32_t s = 0; s < slot; s++, at += 12) {
        memcpy(&g.snapshot_held[s], at, 4);
        memcpy(&g.snapshot_yaw[s], at + 4, 4);
        memcpy(&g.snapshot_pitch[s], at + 8, 4);
    }
    if (end - at > 0) {
        memcpy(g.members[0].name, at, end - at < (ptrdiff_t)sizeof(g.members[0].name) - 1 ? end - at : sizeof(g.members[0].name) - 1);
    }
    g.snapshot_chunks_done = 0;
    g.awaiting_snapshot = true;
    printf("lockstep: joining %s as player %d, receiving the game (%u KB)\n", g.level, slot, snapshot_size / 1024);
    queue_level(g.level, g.difficulty);
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
        // who it came from: on the host a joiner's slot, on a joiner always the host (slot 0)
        int32_t sender = is_host() ? slot_of(from.sin_addr.s_addr, from.sin_port)
                                   : (g.connected && from.sin_addr.s_addr == g.members[0].address ? 0 : -1);

        if (type == k_hello && is_host()) {
            receive_hello(buffer, size, from);
        } else if (type == k_welcome && g.kind == role::joiner && !g.connected) {
            receive_welcome(buffer, size, from);
        } else if (type == k_state && g.awaiting_snapshot && sender == 0 && epoch == g.epoch && size >= 10) {
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
        } else if (type == k_state_ack && is_host() && g.snapshot != nullptr && sender == g.snapshot_slot && epoch == g.epoch &&
                   size >= 12) {
            int32_t done;

            memcpy(&done, buffer + 8, 4);
            if (done > g.snapshot_chunks_done) {
                g.snapshot_chunks_done = done;
                g.snapshot_progress_ms = halo::platform::tick_milliseconds();
            }
            g.members[sender].last_heard_ms = halo::platform::tick_milliseconds();
        } else if (sender >= 0 && epoch == g.epoch && in_level() && !g.awaiting_snapshot) {
            g.members[sender].last_heard_ms = halo::platform::tick_milliseconds();
            g.members[sender].heard_this_level = g.members[sender].heard_this_level || type == k_input;
            if (type == k_input) {
                receive_inputs(buffer, buffer + size, slot, sender);
            } else if (type == k_hash && size >= 16 && sender != g.local_slot) {
                int32_t tick;
                uint32_t hash;

                memcpy(&tick, buffer + 8, 4);
                memcpy(&hash, buffer + 12, 4);
                int32_t index = (tick / k_hash_interval) % k_hash_ring;

                g.members[sender].hash_tick[index] = tick;
                g.members[sender].hash[index] = hash;
                compare_hashes(index);
            }
        }
    }
}

/** Host: the snapshot's next chunks for the joiner; resent from what it has when nothing moves for half a second. */
void send_snapshot_chunks()
{
    int32_t count = snapshot_chunk_count();
    uint32_t now_ms = halo::platform::tick_milliseconds();

    // a joiner still loading the level reads nothing, and must not find a pile of repeats
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
        send_to_member(g.snapshot_slot, packet, at - packet);
    }
    if (g.snapshot_chunks_done >= count || g.members[g.snapshot_slot].lost) {
        free(g.snapshot);
        g.snapshot = nullptr;
    }
}

#endif

/** Players gone silent: the host speaks for a joiner that left, a joiner whose host left carries on alone. */
void check_for_lost_players(int32_t now, int32_t wanted)
{
    uint32_t now_ms = halo::platform::tick_milliseconds();

    for (int32_t s = 0; s < g.player_count; s++) {
        member &m = g.members[s];

        if (s == g.local_slot || !m.present || m.lost || (!is_host() && s != 0)) {
            continue;
        }
        if (now_ms - m.last_heard_ms > (m.heard_this_level ? k_peer_timeout_ms : k_peer_loading_timeout_ms)) {
            m.lost = true;
            printf("lockstep: nothing from player %d for %u s, carrying on without them\n", s, (now_ms - m.last_heard_ms) / 1000);
            show_message("%s left the game.", member_name(s, s == 0 ? "The host" : "A player"));
        }
    }
    for (int32_t s = 0; s < g.player_count; s++) {
        // the host fills a lost joiner's slot (and sends it on); a joiner without its host fills every other slot
        bool fill = s != g.local_slot && g.members[s].present &&
            (is_host() ? g.members[s].lost : g.members[0].lost);

        if (!fill) {
            continue;
        }
        int32_t horizon = is_host() ? g.next_local_tick : now + wanted;

        for (int32_t t = g.members[s].contiguous + 1; t < horizon; t++) {
            if (t >= now && t < now + k_ring && g.action_tick[t % k_ring][s] != t) {
                g.actions[t % k_ring][s] = idle_action();
                g.action_tick[t % k_ring][s] = t;
            }
        }
        advance_contiguous(s);
    }
}

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
        g.level_queued = false;  // a later co-op start queues its own
    }
#if defined(__EMSCRIPTEN__)
    if (g.kind == role::none && hostable()) {
        g.kind = role::host;
    } else if (is_host() && !g.connected && !hostable()) {
        stop_advertising();
        g.kind = role::none;
    }
    if (g.kind == role::none || !open_socket()) {
        return;
    }
    if (is_host() && hostable() && g.player_count < k_max_players) {
        advertise();
    } else {
        stop_advertising();
    }
    answer_queries();
    pump();
    if (is_host() && g.snapshot != nullptr) {
        send_snapshot_chunks();
    }
    if (g.awaiting_snapshot) {
        uint8_t ack[16];
        uint8_t *at = header(ack, k_state_ack, static_cast<uint8_t>(g.local_slot));

        put(at, &g.snapshot_chunks_done, 4);
        send_to_member(0, ack, at - ack);
        // the level is loaded (it was queued on WELCOME) and the whole game has arrived: take it over
        if (g.snapshot_chunks_done >= snapshot_chunk_count() && halo::main::globals().main_globals.main_menu_scenario_loaded == 0 &&
            halo::main::globals().main_globals.level_transition == 0 && halo::game::globals().game_time->initialized != 0) {
            halo::saved_games::game_state_apply_snapshot(g.snapshot);
            free(g.snapshot);
            g.snapshot = nullptr;
            g.awaiting_snapshot = false;
            start_from_snapshot();
        }
    }
    if (in_level()) {
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
        // before reverting: all restart from the restored game time
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
        halo::game::players_structure_bsp_switch_regroup();  // as split screen does: the others come along
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

    // a join marked for this tick happens before it runs, on every machine already playing
    if (g.rebased_tick != now && g.action_tick[now % k_ring][0] == now && (g.actions[now % k_ring][0].control_flags & k_join_flag) != 0) {
        rebase_for_join(now);
#if defined(__EMSCRIPTEN__)
        if (is_host() && g.snapshot != nullptr) {
            send_welcome(g.snapshot_slot);
        }
#endif
    }
    while (g.next_local_tick < now + wanted + k_input_delay && g.next_local_tick < now + k_ring - 1) {
        player_action &stamped = g.actions[g.next_local_tick % k_ring][g.local_slot];

        stamped = g.latest_local;
        stamped.control_flags |= g.pressed_since_stamp;
        g.pressed_since_stamp = 0;
        if (probe_active()) {
            probe_scripted_action(g.next_local_tick, &stamped);
        }
        if (g.skip_pressed) {
            stamped.control_flags |= k_skip_cinematic_flag;
            g.skip_pressed = false;
        }
        stamped.control_flags |= g.menu_request;
        g.menu_request = 0;
        if (is_host() && g.join_pending && g.join_tick < 0) {
            stamped.control_flags |= k_join_flag;  // nobody can have run this tick: the join lands there everywhere
            g.join_tick = g.next_local_tick;
        }
        g.action_tick[g.next_local_tick % k_ring][g.local_slot] = g.next_local_tick;
        g.next_local_tick++;
    }
    check_for_lost_players(now, wanted);
#if defined(__EMSCRIPTEN__)
    send_inputs();
#endif
    int32_t ready = 0;

    while (ready < wanted) {
        int32_t t = now + ready;
        bool all = true;

        // ticks stop short of a later join: it is applied between frames, right before its tick
        if (t != now && g.action_tick[t % k_ring][0] == t && (g.actions[t % k_ring][0].control_flags & k_join_flag) != 0) {
            break;
        }
        for (int32_t s = 0; s < g.player_count; s++) {
            all = all && (!g.members[s].present || g.action_tick[t % k_ring][s] == t);
        }
        if (!all) {
            break;
        }
        ready++;
    }
    if (ready < wanted && g.frames % 60 == 0) {
        printf("lockstep: waiting at tick %d (players %d, sent to %d, everyone has to %d)\n", now, g.player_count,
            g.next_local_tick - 1, is_host() ? everyone_has(0) : g.resend_from);
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
        // what digitize_control_input does for one machine's own button, here for anyone's, on the same tick everywhere
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
        if (is_host()) {
            send_to_joiners(buffer, at - buffer, -1);
        } else {
            send_to_member(0, buffer, at - buffer);
        }
        compare_hashes(index);
    }
#endif
}

}  // namespace halo::game::lockstep

#include "halo/networking/net1_runtime.hpp"
#include "halo/text/api.hpp"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <wchar.h>
#include "units.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"

extern "C" {
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count];
extern int32_t network_connection_stats_lookup_or_add(int32_t connection_id, uint16_t connection_key);
extern uint8_t network_statistics_logging_enabled;
extern uint8_t network_connection_log_needs_open;
extern void *network_connection_stats_log_file;
extern int32_t network_connection_log_last_row_ms;
extern int32_t network_connection_log_start_ms;
extern int32_t network_connection_stats_count;
extern network_summary_statistics network_summary_stats;
extern network_client_globals *network_client;
extern network_server_globals *network_server;
extern char network_summary_log_mode_string[];
extern char *network_log_path_resolve(char *requested_path);
extern void network_bandwidth_graph_accumulate_sent(int32_t enabled);
extern void network_bandwidth_graph_accumulate_received(int32_t enabled);
extern void *gt2GetConnectionData(void *gamespy_connection);
extern uint32_t gamespy_array_length(int32_t object);
extern uint16_t gt2GetRemotePort(int32_t object);
extern uint8_t network_disabled_flag;
extern int16_t network_join_error_code;
extern int32_t network_join_error_reason;
extern uint8_t split_screen_quit_prompt_string[4];
extern data_packet_group network_game_messages_group;
extern int16_t network_initialize(void);
extern uint8_t network_hostname_ready;
extern uint8_t network_winsock_initialized;
extern uint32_t network_local_address;
extern uint32_t network_resolved_local_address;
extern int32_t network_initialized_at_ms;
extern int network_local_hostent_get(void **out_hostent);
extern uint32_t __stdcall autopatch_proxy_initialize(void *parameter);
extern char network_local_hostname_buffer[0x100];
extern void network_hostname_thread_proc(char *hostname_buffer);
extern uint8_t network_log_path_buffer[0x104];
extern char network_log_path_format[];
extern int32_t security_check_write_access(void);
extern uint8_t virtual_keyboard_character_is_legal(uint8_t ch, void *character);
extern uint8_t ui_wide_string_has_non_whitespace(void);
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length);
extern uint16_t network_challenge_packet_block[];
extern uint8_t network_random_seeded;
extern int32_t __ftol(int32_t value);
extern int32_t network_query_socket;
extern int32_t network_game_socket;
extern void *network_summary_log_file;
extern void gt2CloseSocket(int32_t socket);
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
extern int32_t network_high_res_clock_ms;
extern uint8_t network_update_unknown_869bf;
extern void network_connection_stats_log_tick(void);
extern void gt2Think(int32_t socket);
extern void gamespy_think_all(void);
extern data_array *player_data;
extern void *machine_table;
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void network_event_feed_flush(void);
extern void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value);
extern int32_t hash_table_get(hash_table *table, uint32_t key);
extern uint8_t network_summary_log_needs_open;
}

/**
 * Calls halo::cache::tag_lookup with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static void * tag_lookup_unresolved(const char *tag_path)
{
    using call_t = void * (*)(const char *tag_path);
    return reinterpret_cast<call_t>(&halo::cache::tag_lookup)(tag_path);
}

/**
 * Calls halo::memory::data_packet_group_encode_packet with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static int32_t data_packet_group_encode_packet_unresolved(uint8_t *buffer, data_packet_group *group, void *payload, int32_t *capacity, int32_t message_type, int32_t flag)
{
    using call_t = int32_t (*)(uint8_t *buffer, data_packet_group *group, void *payload, int32_t *capacity, int32_t message_type, int32_t flag);
    return reinterpret_cast<call_t>(&halo::memory::data_packet_group_encode_packet)(buffer, group, payload, capacity, message_type, flag);
}

namespace halo::networking {

/**
 * network_connection_stats_lookup_or_add
 *
 * @address 0x440d20
 */
void ConnectionStats::end(int32_t connection_id, uint16_t connection_key)
{
    int32_t index;
    int32_t now;

    if (2 < halo::cseries::globals().debug_log_level &&
        (index = network_connection_stats_lookup_or_add(connection_id, connection_key), index != -1) &&
        network_connection_stats[index].active != 0) {
        now = halo::cseries::time_query_performance_counter_ms();
        network_connection_stats[index].connected_duration_ms +=
            now - network_connection_stats[index].active_since_ms;
        network_connection_stats[index].active_since_ms = 0;
        network_connection_stats[index].active = 0;
    }
}

/**
 * out/phase4/networking_types_notes.md "network_connection_statistics (0x44)": the
 * header names interval_packets_sent/interval_bytes_sent/interval_reliable_bytes_sent/
 * interval_resend_bytes_sent directly from this function's own log header and %d group, and
 * the player-count accumulation ("network_client + 0xb14 or network_server + 8", reading
 * +0x1a0) is the cross-check that pinned network_game_session::player_count.
 *
 * @address 0x440d80
 */
void ConnectionStats::log_tick()
{
    char path_buf[0x208];
    char date_buf[0x104];
    time_t now_time;
    struct tm *tm_now;
    char *base_path;
    int32_t now;
    int32_t i;
    network_game_session *session;
    uint8_t control_char;

    if (2 < halo::cseries::globals().debug_log_level && network_statistics_logging_enabled == 1) {
        now = halo::cseries::time_query_performance_counter_ms();
        if (network_connection_log_needs_open == 1) {
            network_connection_log_needs_open = 0;
            network_connection_log_last_row_ms = now;
            network_connection_log_start_ms = now;

            time(&now_time);
            tm_now = localtime(&now_time);
            strftime(date_buf, 0x103, "%Y-%m-%d %H_%M_%S", tm_now);

            base_path = network_log_path_resolve((char *)"Gamespy Metrics");
            strcpy(path_buf, base_path);
            halo::cseries::directory_create_recursive(path_buf);

            strcat(path_buf, "\\gamespy ");
            strcat(path_buf, date_buf);
            strcat(path_buf, ".xls");

            network_connection_stats_log_file = fopen(path_buf, network_summary_log_mode_string);
            fprintf((FILE *)network_connection_stats_log_file,
                    "\tEach connection has five columns (see headers below). One empty column "
                    "separates each connection. Note: resend traffic is considered unreliable.\n");
            fprintf((FILE *)network_connection_stats_log_file,
                    "Time\tPackets Sent\tTotal Sent\tReliable Sent\tUnreliable Sent\tResends Sent\n");
        }
        if (100 < (uint32_t)(now - network_connection_log_last_row_ms)) {
            network_connection_log_last_row_ms = now;
            fprintf((FILE *)network_connection_stats_log_file, "%d",
                    (uint32_t)(now - network_connection_log_start_ms) / 1000);
            if (0 < network_connection_stats_count) {
                fprintf((FILE *)network_connection_stats_log_file, "\t");
                for (i = 0; i < network_connection_stats_count; i++) {
                    control_char = (i != network_connection_stats_count - 1) ? 9 : 0;
                    fprintf((FILE *)network_connection_stats_log_file, "%d\t%d\t%d\t%d\t%d\t%c",
                            network_connection_stats[i].interval_packets_sent,
                            network_connection_stats[i].interval_bytes_sent,
                            network_connection_stats[i].interval_reliable_bytes_sent,
                            network_connection_stats[i].interval_bytes_sent -
                                network_connection_stats[i].interval_reliable_bytes_sent,
                            network_connection_stats[i].interval_resend_bytes_sent,
                            control_char);
                    network_connection_stats[i].interval_packets_sent = 0;
                    network_connection_stats[i].interval_bytes_sent = 0;
                    network_connection_stats[i].interval_reliable_bytes_sent = 0;
                    network_connection_stats[i].interval_resend_bytes_sent = 0;
                    network_connection_stats[i].interval_bytes_received = 0;

                    session = 0;
                    if (network_client != 0) {
                        session = &network_client->session;
                    } else if (network_server != 0) {
                        session = &network_server->session;
                    }
                    if (session != 0) {
                        network_summary_stats.player_count_samples += 1;
                        network_summary_stats.player_count_total += session->player_count;
                    }
                }
            }
            fprintf((FILE *)network_connection_stats_log_file, "\n");
        }
    }
}

/**
 * already named)
 * address 0x440a80, size 160 bytes
 * name confidence: 0.5   rewrite confidence: 0.65
 * out/phase4/networking_types_notes.md "network_connection_statistics (0x44) and
 * network_summary_statistics (0x1c)": the stride is pinned three ways in this exact function
 * (the dword, word and byte indexings of network_connection_stats all resolve to the same element).
 *
 * @address 0x440a80
 */
int32_t ConnectionStats::lookup_or_add(int32_t connection_id, uint16_t connection_key)
{
    int32_t count;
    int32_t i;

    count = network_connection_stats_count;
    i = 0;
    if (0 < network_connection_stats_count) {
        do {
            if (network_connection_stats[i].connection_id == connection_id &&
                network_connection_stats[i].connection_key == connection_key) {
                return i;
            }
            i = i + 1;
        } while (i < network_connection_stats_count);
    }
    if (network_connection_stats_count < 0xff) {
        network_connection_stats_count = network_connection_stats_count + 1;
    }
    network_connection_stats[count].connection_id = connection_id;
    network_connection_stats[count].connection_key = connection_key;
    network_connection_stats[count].connected_duration_ms = 0;
    network_connection_stats[count].bytes_sent = 0;
    network_connection_stats[count].bytes_received = 0;
    network_connection_stats[count].reliable_bytes_sent = 0;
    network_connection_stats[count].resend_bytes_sent = 0;
    network_connection_stats[count].interval_bytes_sent = 0;
    network_connection_stats[count].interval_bytes_received = 0;
    network_connection_stats[count].interval_reliable_bytes_sent = 0;
    network_connection_stats[count].interval_resend_bytes_sent = 0;
    network_connection_stats[count].packets_sent = 0;
    network_connection_stats[count].packets_received = 0;
    network_connection_stats[count].interval_packets_sent = 0;
    network_connection_stats[count].interval_packets_received = 0;
    return count;
}

/**
 * is_sent/is_reliable/is_resend as ordinary stack parameters (param_1/2/3)
 *
 * @address 0x440b20
 */
void ConnectionStats::record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent, uint8_t is_reliable, uint8_t is_resend)
{
    int32_t total_bytes;
    int32_t *stats_index_field;
    int32_t index;

    if (2 < halo::cseries::globals().debug_log_level) {
        total_bytes = payload_length + 0x1c;
        if (is_sent == 0) {
            network_bandwidth_graph_accumulate_received(1);
        } else {
            network_bandwidth_graph_accumulate_sent(1);
        }
        if (network_statistics_logging_enabled == 1 && gamespy_connection != 0 &&
            (gamespy_connection = gt2GetConnectionData(gamespy_connection), gamespy_connection != 0)) {
            stats_index_field = (int32_t *)((uint8_t *)gamespy_connection + 0x14);
            if (*stats_index_field == -1) {
                index = network_connection_stats_lookup_or_add(
                    (int32_t)gamespy_array_length((int32_t)(uintptr_t)gamespy_connection),
                    gt2GetRemotePort((int32_t)(uintptr_t)gamespy_connection));
                *stats_index_field = index;
                network_connection_stats[index].active = 1;
                network_connection_stats[*stats_index_field].active_since_ms = halo::cseries::time_query_performance_counter_ms();
            }
            index = *stats_index_field;
            if (is_sent == 1) {
                network_connection_stats[index].bytes_sent += total_bytes;
                network_connection_stats[index].interval_bytes_sent += total_bytes;
                network_connection_stats[index].packets_sent += 1;
                network_connection_stats[index].interval_packets_sent += 1;
                if (is_reliable == 1) {
                    network_connection_stats[index].reliable_bytes_sent += total_bytes;
                    network_connection_stats[index].interval_reliable_bytes_sent += total_bytes;
                }
                if (is_resend == 1) {
                    network_connection_stats[index].resend_bytes_sent += total_bytes;
                    network_connection_stats[index].interval_resend_bytes_sent += total_bytes;
                }
                network_summary_stats.packets_sent += 1;
                network_summary_stats.bytes_sent += total_bytes;
                return;
            }
            network_connection_stats[index].bytes_received += total_bytes;
            network_connection_stats[index].interval_bytes_received += total_bytes;
            network_connection_stats[index].packets_received += 1;
            network_connection_stats[index].interval_packets_received += 1;
            network_summary_stats.packets_received += 1;
            network_summary_stats.bytes_received += total_bytes;
        }
    }
}

/**
 * Fills a 20-byte scratch buffer with the literal placeholder text "message in a bot".
 *
 * @address 0x4e0790
 */
void NetworkRuntime::debug_fill_canary_buffer(uint32_t *buffer)
{
    buffer[0] = 0;
    buffer[0] = 0x7373656d;
    buffer[1] = 0x20656761;
    buffer[2] = 0x61206e69;
    buffer[3] = 0x746f6220;
}

/**
 * out/phase4/networking_functions.md summary ("initializes networking (winsock etc.)
 * and, on first run, registers the network game-message dispatch group used to route incoming
 * gameplay packets"); types/networking.h's closing note that 0x006994f8 is the
 * data_packet_group 0x4414c0 registers (39 types, max decoded size 0x600), reusing
 * src/memory/struct_definition_table_compute_sizes.c's already-established prototype.
 *
 * @address 0x4414c0
 */
void NetworkRuntime::dispatch_initialize()
{
    int16_t initialize_result;

    initialize_result = network_initialize();
    if (initialize_result != 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = 5;
        }
        *(uint16_t *)&split_screen_quit_prompt_string[0] = 0xffff;
        network_join_error_reason = 0;
        split_screen_quit_prompt_string[3] = 1;
    }
    if (network_disabled_flag == 0) {
        halo::memory::struct_definition_table_compute_sizes(&network_game_messages_group);
    }
}

/**
 * out/phase4/networking_functions.md summary ("thread entry point that retrieves the
 * local machine's hostname into a buffer, signals completion, and exits the thread"); Ghidra
 * already recovered the full __stdcall signature and both Win32 calls by name.
 *
 * @address 0x441510
 */
void NetworkRuntime::hostname_thread_proc(char *hostname_buffer)
{
    gethostname(hostname_buffer, 0x100);
    network_hostname_ready = 1;

    ExitThread(0);
}

/**
 * out/phase4/networking_functions.md summary ("one-time networking subsystem
 * startup: calls WSAStartup, determines the local IP address, and launches the background
 * network processing thread"); shares network_disabled_flag (0x007196ec) and
 * network_local_address (0x006869b0, already documented in networking_types_notes.md as
 * "byte swapped before binding") with network_dispatch_initialize.c / network_channels_open.c.
 *
 * @address 0x4415c0
 */
int16_t NetworkRuntime::initialize()
{
    int16_t result;
    int32_t wsa_result;
    void *hostent;
    uint32_t thread_id;
    uint32_t raw_address;
    uint8_t wsa_data[400];

    result = 0;
    hostent = 0;
    if (network_disabled_flag != 0) {
        network_winsock_initialized = 0;
        return 0;
    }
    if (network_winsock_initialized == 0) {
        memset(wsa_data, 0, sizeof(wsa_data));
        wsa_result = WSAStartup(2, (LPWSADATA)wsa_data);
        if ((int16_t)wsa_result == 0) {
            if (network_local_address == 0) {
                if (network_local_hostent_get(&hostent) == 0) {
                    return -0x10;
                }
                raw_address = *(uint32_t *)**(uint32_t **)((uint8_t *)hostent + 0xc);
                network_resolved_local_address =
                    (raw_address & 0xff0000 | raw_address >> 0x10) >> 8 |
                    (raw_address << 0x10 | raw_address & 0xff00) << 8;
            } else {
                network_resolved_local_address = network_local_address;
            }
        }
        CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)autopatch_proxy_initialize, 0, 0, (LPDWORD)&thread_id);

        network_initialized_at_ms = halo::cseries::time_query_performance_counter_ms();
        network_winsock_initialized = 1;
        result = (int16_t)wsa_result;
    }
    return result;
}

/**
 * out/phase4/networking_functions.md summary ("resolves and returns the local
 * machine's hostent structure, retrieving the hostname on a watchdog-timed worker thread
 * first"); Ghidra already recovered the full signature. Shares network_hostname_ready
 * (0x006f14cc) with network_hostname_thread_proc.c and the hostname buffer with it too.
 *
 * @address 0x441540
 */
int NetworkRuntime::local_hostent_get(void **out_hostent)
{
    void *thread_handle;
    uint32_t wait_result;
    uint32_t thread_id;

    network_hostname_ready = 0;
    thread_handle = CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)network_hostname_thread_proc,
                                  network_local_hostname_buffer, 0, (LPDWORD)&thread_id);
    if (thread_handle != 0) {
        wait_result = WaitForSingleObject(thread_handle, 10000);
        if (wait_result == 0x102) {
            TerminateThread(thread_handle, 0);
        }
        CloseHandle(thread_handle);
        if (network_hostname_ready != 0) {
            *out_hostent = gethostbyname(network_local_hostname_buffer);
            return 1;
        }
    }
    return 0;
}

/**
 * Zeroes the shared path buffer, then formats requested_path into it with "%s" if the process
 * has write access to it; if the buffer is still empty afterward (either the access check
 * failed, or requested_path formatted to nothing), formats it in again unconditionally. Always
 * returns the shared buffer.
 *
 * @address 0x4e40a0
 */
char * NetworkRuntime::log_path_resolve(char *requested_path)
{
    network_log_path_buffer[0] = 0;
    if (halo::shell::security_check_write_access() != 0) {
        _snprintf((char *)network_log_path_buffer, 0x104, network_log_path_format, requested_path);
    }
    if (network_log_path_buffer[0] == 0) {
        _snprintf((char *)network_log_path_buffer, 0x104, network_log_path_format, requested_path);
    }
    return (char *)network_log_path_buffer;
}

/**
 * Checks that every character of `name` is renderable in the small UI font and, for mode 3
 * (UNSURE: player-name entry), that the name is non-empty and does not begin with a space or
 * byte 0xa0. For mode 1 (UNSURE: server-name entry), additionally requires UiStrings::wide_string_has_non_whitespace to pass.
 *
 * @address 0x4e4350
 */
uint8_t NetworkRuntime::name_string_is_valid_for_mode(char *name, void *character, int32_t mode)
{
    uint8_t ok = 1;
    int32_t len;
    int32_t i;

    datum_index small_ui_font_tag = halo::cache::tag_lookup(0x666f6e74, (char *)"ui\\small_ui");
    Font *small_ui_font = (Font *)halo::cache::globals().tag_instances[small_ui_font_tag & 0xffff].data;
    len = strlen(name);
    if (mode == 3) {
        ok = *name != 0;
        if (!ok) {
            return ok;
        }
    }
    for (i = 0; i < len; i = i + 1) {
        uint8_t ch = (uint8_t)name[i];
        if (ch < ' ' || ch == 0xff || halo::text::text_get_character_metrics(ch, small_ui_font) == 0 || virtual_keyboard_character_is_legal(ch, character) == 0) {
            ok = 0;
            break;
        }
        if (mode == 3) {
            if (i == 0) {
                if (*name == ' ') {
                    return 0;
                }
                if ((uint8_t)*name == 0xa0) {
                    return 0;
                }
            }
            ok = 1;
        }
    }
    if (mode != 1) {
        return ok;
    }
    return ok != 0 && ui_wide_string_has_non_whitespace() != 0;
}

/**
 * out/phase4/networking_functions.md: "Register-based helper that copies a
 * wide-character string (e.g. a password) into the object at unaff_ESI+8 and clears the field
 * immediately following it." Neither the object nor the exact fields at +8/+0x86 could be tied
 * to a specific header-declared struct (network_server_globals.password is 9 wide chars at
 * +0x9fc, not +8, and the 0x3f-char copy count does not match its size); kept as raw offsets on
 * a generic object pointer.
 *
 * @address 0x4df070
 */
void NetworkRuntime::password_field_set(uint8_t *object, wchar_t *source)
{
    wcsncpy((wchar_t *)(object + 8), source, 0x3f);
    *(uint16_t *)(object + 0x86) = 0;
}

/**
 * out/phase4/networking_functions.md: "Encodes the queued packet group into a freshly
 * allocated 0x600-byte network buffer, matching the chimera-identified 'prepare challenge
 * packet' code path."
 *
 * @address 0x4deaf0
 */
uint16_t * NetworkRuntime::prepare_challenge_packet(int32_t message_type, void *payload)
{
    uint8_t buffer[0x600];
    int32_t length;

    length = 0x600;
    if (data_packet_group_encode_packet_unresolved(buffer, &network_game_messages_group, payload, &length,
                                        message_type, 1) != 0) {
        return network_message_block_build(network_challenge_packet_block, (uint32_t *)buffer, 3,
                                           (uint32_t)length);
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md summary ("lazily seeds the C runtime random
 * number generator from the current time, then returns a random value offset by a
 * caller-supplied base held in ESI"); `python tools/pack.py 0x4403b0`.
 *
 * @address 0x4403b0
 */
int32_t NetworkRuntime::random_offset(int32_t base)
{
    int32_t value;

    if (network_random_seeded == 0) {
        srand((uint32_t)_time32(0));
        network_random_seeded = 1;
    }
    value = rand();
    value = __ftol(value);
    return value + base;
}

/**
 * out/phase4/networking_functions.md summary ("tears down the networking subsystem:
 * closes channels, flushes and closes both statistics log files with final summary lines, and
 * clears the initialized flag"); reuses every network_connection_statistics field name from
 * out/phase4/networking_types_notes.md, confirming connection_id/connection_key double as an
 * IPv4 address and port pair here (they are fed straight into the same address-formatting
 * call, gt2AddressToString, that network_channels_open.c uses for socket addresses).
 *
 * @address 0x4416e0
 */
int32_t NetworkRuntime::shutdown()
{
    int32_t i;
    int32_t total_sent;
    int32_t total_received;
    int32_t connection_duration_ms;
    int32_t now;
    uint32_t total_elapsed_sec;
    float total_sent_f;
    float connection_seconds;
    uint8_t address_buf[24];

    if (network_winsock_initialized == 0) {
        return 0xfffffffb;
    }
    if (network_query_socket != 0) {
        gt2CloseSocket(network_query_socket);
        network_query_socket = 0;
    }
    if (network_game_socket != 0) {
        gt2CloseSocket(network_game_socket);
        network_game_socket = 0;
    }
    if (network_summary_log_file != 0) {
        fclose((FILE *)network_summary_log_file);
    }
    if (network_connection_stats_log_file != 0) {
        fprintf((FILE *)network_connection_stats_log_file, "\n\n");
        total_sent = 0;
        total_received = 0;
        now = halo::cseries::time_query_performance_counter_ms();
        total_elapsed_sec = (uint32_t)(now - network_initialized_at_ms) / 1000;
        for (i = 0; i < network_connection_stats_count; i++) {
            total_sent = total_sent + network_connection_stats[i].bytes_sent;
            total_received = total_received + network_connection_stats[i].bytes_received;
            connection_duration_ms = network_connection_stats[i].connected_duration_ms;
            if (network_connection_stats[i].active == 1) {
                connection_duration_ms = connection_duration_ms +
                    (now - network_connection_stats[i].active_since_ms);
            }
            gt2AddressToString((uint32_t)network_connection_stats[i].connection_id,
                         (uint16_t)network_connection_stats[i].connection_key, address_buf);

            connection_seconds = (float)((uint32_t)connection_duration_ms / 1000);
            if ((int32_t)((uint32_t)connection_duration_ms / 1000) < 0) {
                connection_seconds = connection_seconds + 4.2949673e+09f;
            }
            fprintf((FILE *)network_connection_stats_log_file,
                    "Connection [%d]  Live for[%d] seconds  Was address[%s]  Total Sent[%d]  "
                    "Total Received[%d]  Bytes sent per second[%f]\n",
                    i, (uint32_t)connection_duration_ms / 1000, address_buf,
                    network_connection_stats[i].bytes_sent,
                    network_connection_stats[i].bytes_received,
                    (double)((float)network_connection_stats[i].bytes_sent / connection_seconds));
        }
        total_sent_f = (float)total_sent;
        if (total_sent < 0) {
            total_sent_f = total_sent_f + 4.2949673e+09f;
        }
        fprintf((FILE *)network_connection_stats_log_file,
                "total data sent[%d]  total received[%d] total time in seconds[%d]  "
                "bytes sent per second[%f]\n",
                total_sent, total_received, total_elapsed_sec,
                (double)(total_sent_f / (float)total_elapsed_sec));
        fprintf((FILE *)network_connection_stats_log_file, "Log file closed\n");
        fclose((FILE *)network_connection_stats_log_file);
        network_connection_stats_log_file = 0;
    }
    network_winsock_initialized = 0;
    return 0;
}

/**
 * out/phase4/networking_functions.md summary ("maps a small integer enum value to
 * a fixed output byte via a lookup switch, purpose unconfirmed"); `python tools/pack.py
 * 0x440610`. The returned bytes (0x2b '+', 0x37 '7', 0x38 '8', 0x39 '9', 0x2e '.', 0x31 '1')
 * are plain ASCII digits/punctuation, which in Blam's HUD font mapping typically select
 * icon glyphs (e.g. connection-quality bar icons); this is a guess and not confirmed by any
 * caller in this batch.
 *
 * @address 0x440610
 */
uint8_t NetworkRuntime::signal_quality_glyph(uint32_t code)
{
    switch (code) {
    case 3: return 0x37;
    case 4: return 0x38;
    case 5: return 0x39;
    case 6: return 0x2e;
    case 8: return 0x31;
    default: return 0x2b;
    }
}

/**
 * out/phase4/networking_functions.md summary ("per-frame networking service
 * routine: updates the high-resolution clock, drives the connection-statistics log, and pumps
 * both network channels"); reuses network_game_socket/network_query_socket from
 * networking_types_notes.md and performance_frequency from
 * src/math/random_seed_generate.c (same QueryPerformanceCounter/__allmul/__alldiv shape,
 * folded into plain int64_t arithmetic here for the same reason).
 *
 * @address 0x4418d0
 */
uint32_t NetworkRuntime::update_()
{
    large_integer counter;
    uint32_t result;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    network_high_res_clock_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (network_update_unknown_869bf == 1) {
        network_update_unknown_869bf = 0;
    }
    network_connection_stats_log_tick();
    if (network_game_socket != 0) {
        gt2Think(network_game_socket);
    }
    if (network_query_socket != 0) {
        gt2Think(network_query_socket);
    }
    gamespy_think_all();
    return 0;
}

/**
 * Resolves every queued event record's unit index (queue+8, stride 8) to its object_data slot's
 * player-count and player-table entry, keeping only records whose slot resolves to a live entry
 * matching (or wildcard-matching) the record's salt. For each surviving record, looks up its raw
 * key's network hash through the same hash table this batch's other builders use, replacing the
 * key in place. Encodes all surviving records as one batched message-0x26 update and sends it,
 * then clears the queue's count.
 *
 * @address 0x4e8040
 */
void EventFeed::flush(int32_t *queue)
{
    int32_t remaining;
    int32_t *key_slot;
    int32_t *payload_slot;
    void *survivors_key[16];
    void *survivors_payload[16];
    void *survivors_extra[16];
    int32_t survivor_count;
    int32_t i;
    int32_t raw_key;
    int32_t hash;
    hash_table *table;
    hash_node *node;
    char force_changed;
    void **type_offset_arg;
    int16_t entry_identifier;
    int16_t entry_salt;
    uint8_t *entry;

    for (i = 0; i < 16; i = i + 1) {
        survivors_key[i] = 0;
    }
    for (i = 0; i < 16; i = i + 1) {
        survivors_payload[i] = 0;
    }

    remaining = queue[1];
    survivor_count = 0;
    if (0 < remaining) {
        payload_slot = queue + 0x22;
        key_slot = queue;
        do {
            key_slot = key_slot + 2;
            raw_key = *key_slot;
            if (raw_key != -1 && (int16_t)raw_key >= 0 &&
                (int16_t)raw_key < player_data->maximum_count) {
                entry = (uint8_t *)player_data->data + (int16_t)raw_key * player_data->size;
                entry_identifier = *(int16_t *)entry;
                if (entry_identifier != 0) {
                    entry_salt = (int16_t)((uint32_t)raw_key >> 16);
                    if (entry_salt == 0 || entry_identifier == entry_salt) {
                        survivors_key[survivor_count] = key_slot;
                        survivors_payload[survivor_count] = payload_slot;
                        survivors_extra[survivor_count] = entry + 0x130;
                        survivor_count = survivor_count + 1;
                    }
                }
            }
            payload_slot = payload_slot + 0xc;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    remaining = 0;
    if (0 < survivor_count) {
        table = (hash_table *)((uint8_t *)machine_table + 0x0c);
        do {
            raw_key = *(int32_t *)survivors_key[remaining];
            hash = 0;
            if (raw_key != -1) {
                hash = -1;
                if (table->initialized == 1) {
                    int32_t bucket = (raw_key < 0 ? -raw_key : raw_key) % table->bucket_count;
                    for (node = table->buckets[bucket].first; node != 0; node = node->next) {
                        if (node->key == raw_key) {
                            hash = node->value;
                            break;
                        }
                    }
                }
                if (hash == -1) {
                    hash = 0;
                }
            }
            *(int32_t *)survivors_key[remaining] = hash;
            remaining = remaining + 1;
        } while (remaining < survivor_count);
    }

    force_changed = (char)*queue != 1;
    type_offset_arg = force_changed ? survivors_extra : 0;
    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, (uint32_t)force_changed, 0x26, (int32_t)survivors_key,
        survivors_payload, (int32_t)type_offset_arg, survivor_count, force_changed), network_server, 1, 0, (char)*queue, 0, 0, 2);
    queue[1] = 0;
}

/**
 * visible in the decompile, presumably an implicit EAX -> queue passthrough)
 * Appends one event record (a 2-dword key plus a 12-dword payload) to the 16-slot queue at
 * queue+0x04 (count) / queue+0x08 (keys, stride 8) / queue+0x88 (payloads, stride 0x30), and
 * flushes the queue once the 16th slot is filled.
 *
 * @address 0x4e7ff0
 */
void EventFeed::queue_append(uint8_t *queue, uint32_t *key, uint32_t *payload)
{
    int32_t count;
    uint32_t *slot_key;
    uint32_t *slot_payload;
    int32_t i;

    count = *(int32_t *)(queue + 4);
    slot_key = (uint32_t *)(queue + 8 + count * 8);
    slot_key[0] = key[0];
    slot_key[1] = key[1];
    slot_payload = (uint32_t *)(queue + 0x88 + count * 0x30);
    for (i = 0xc; i != 0; i = i - 1) {
        *slot_payload = *payload;
        payload = payload + 1;
        slot_payload = slot_payload + 1;
    }
    count = *(int32_t *)(queue + 4) + 1;
    *(int32_t *)(queue + 4) = count;
    if (count == 0x10) {
        network_event_feed_flush();
    }
}

/**
 * memory module; UNSURE: signature inferred, see file header
 * Looks up key in container's index cache; if present, returns its cached slot. Otherwise scans
 * forward from the cache's rotating cursor for a slot whose value is -1 (evicting/reusing it),
 * binds key to that slot in the hash table, and returns it. Returns -1 if the whole table was
 * scanned without finding a free slot.
 *
 * @address 0x4e9c20
 */
int32_t IndexCache::find_or_allocate_slot(uint8_t *container, int32_t key)
{
    network_index_cache *cache;
    hash_table *table;
    int32_t slot;
    int32_t abs_key;
    hash_node *node;
    int32_t start_cursor;
    int32_t cursor;
    int32_t free_slot;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;
    slot = -1;
    if (table->initialized == 1 && key != -1) {
        abs_key = key < 0 ? -key : key;
        for (node = table->buckets[abs_key % table->bucket_count].first; node != 0;
             node = node->next) {
            if (node->key == key) {
                slot = node->value;
                break;
            }
        }
    }
    if (slot != -1) {
        return slot;
    }

    start_cursor = cache->cursor;
    free_slot = -1;
    cursor = start_cursor;
    for (;;) {
        if (cache->slots[cursor] == -1) {
            free_slot = cursor;
        }
        cursor = cursor + 1;
        if (cache->capacity <= cursor) {
            cursor = 0;
        }
        cache->cursor = cursor;
        if (start_cursor == cursor) {
            break;
        }
        if (free_slot != -1) {
            hash_table_set_or_remove(table, key, free_slot);
            cache->slots[free_slot] = key;
            return free_slot;
        }
    }
    if (free_slot == -1) {
        return -1;
    }
    hash_table_set_or_remove(table, key, free_slot);
    cache->slots[free_slot] = key;
    return free_slot;
}

/**
 * Returns hash_table_get(table, key), or 0 if key is -1 (no entry).
 *
 * @address 0x4e9d20
 */
int32_t IndexCache::get(hash_table *table, int32_t key)
{
    if (key == -1) {
        return 0;
    }
    return hash_table_get(table, key);
}

/**
 * If container's cache slot is unoccupied (-1) and key is not already present in the hash
 * table, binds slot to key and returns true.
 *
 * @address 0x4e9cd0
 */
uint8_t IndexCache::insert_if_free(uint8_t *container, int32_t slot, int32_t key)
{
    network_index_cache *cache;
    hash_table *table;
    int32_t *slot_ptr;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;
    slot_ptr = &cache->slots[slot];
    if (*slot_ptr != -1) {
        return 0;
    }
    if (hash_table_get(table, key) == -1) {
        *slot_ptr = key;
        hash_table_set_or_remove(table, key, slot);
        return 1;
    }
    return 0;
}

/**
 * If key is bound in container's cache hash table, clears its slot to -1, removes the hash
 * mapping, and returns true; otherwise returns false.
 *
 * @address 0x4e9d40
 */
uint8_t IndexCache::remove(uint8_t *container, int32_t key)
{
    network_index_cache *cache;
    hash_table *table;
    int32_t abs_key;
    hash_node *node;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;
    if (table->initialized == 1 && key != -1) {
        abs_key = key < 0 ? -key : key;
        for (node = table->buckets[abs_key % table->bucket_count].first; node != 0;
             node = node->next) {
            if (node->key == key) {
                if (node->value == -1) {
                    return 0;
                }
                cache->slots[node->value] = -1;
                hash_table_set_or_remove(table, key, -1);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * length in param_1 (stack)
 *
 * @address 0x440350
 */
uint16_t * MessageBlocks::block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length)
{
    uint32_t *dest;
    uint32_t count;

    if (buffer == 0) {
        buffer = (uint16_t *)GlobalAlloc(0, (uint32_t)(int16_t)(length + 2));
        if (buffer == 0) {
            return 0;
        }
    }
    *buffer = (uint16_t)(int16_t)(((flags & 3) | (length + 2) * 4) << 2);
    if (source != 0) {
        dest = (uint32_t *)(buffer + 1);
        for (count = (length & 0xffff) >> 2; count != 0; count--) {
            *dest = *source;
            source++;
            dest++;
        }
        for (length = length & 3; length != 0; length--) {
            *(uint8_t *)dest = *(uint8_t *)source;
            source = (uint32_t *)((int)source + 1);
            dest = (uint32_t *)((int)dest + 1);
        }
    }
    return buffer;
}

/**
 * out/phase4/networking_functions.md: "Register-based helper (EDI) that validates a
 * size/capacity value obtained twice from bit_stream_view::read_bits_chunked against param_1 before returning the
 * buffer pointer, otherwise returns null." Reads a 16-bit chunked header into `buffer` itself,
 * checks its high 12 bits (a byte count) against the caller's capacity, then reads that many
 * more bits and confirms the bit count consumed matches exactly, before returning `buffer`.
 * The 16-bit header carries the total byte count in its high 12 bits; the payload (total*8 - 16
 * bits) is read into buffer + 2 bytes, after the header.
 *
 * @address 0x4de420
 */
uint16_t * MessageBlocks::read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream)
{
    int32_t consumed;
    uint16_t header;

    consumed = halo::memory::bit_stream_read_bits_chunked(0x10, (uint32_t *)buffer, stream);
    if (consumed == 0x10) {
        header = *buffer;
        if ((int32_t)(uint32_t)(header >> 4) <= capacity) {
            consumed = halo::memory::bit_stream_read_bits_chunked((header >> 4) * 8 - 0x10, (uint32_t *)(buffer + 1), stream);
            if (consumed == (header >> 4) * 8 - 0x10) {
                return buffer;
            }
        }
    }
    return 0;
}

/**
 * time(), localtime(), strftime() and fopen() come from <time.h>/<stdio.h> above; the retail
 * binary calls the 32-bit-time-specific CRT entry points (_time32, localtime, _fsopen) for
 * the same effect, per the _rand -> rand renaming precedent in src/math/random_seed_generate.c.
 *
 * @address 0x440670
 */
void StatsSummaryLog::open()
{
    char path_buf[0x208];
    char date_buf[0x104];
    time_t now;
    struct tm *tm_now;
    char *base_path;

    if (2 < halo::cseries::globals().debug_log_level && network_statistics_logging_enabled != 0) {
        if (network_summary_log_needs_open != 0) {
            time(&now);
            tm_now = localtime(&now);
            strftime(date_buf, 0x103, "%Y-%m-%d %H_%M_%S", tm_now);

            base_path = network_log_path_resolve((char *)"Gamespy Metrics");
            strcpy(path_buf, base_path);
            halo::cseries::directory_create_recursive(path_buf);

            strcat(path_buf, "\\Game Summary ");
            strcat(path_buf, date_buf);
            strcat(path_buf, ".xls");

            network_summary_log_file = fopen(path_buf, network_summary_log_mode_string);
            fprintf((FILE *)network_summary_log_file,
                    "Map\tLength (seconds)\tAvg # Players\tPackets Sent\tPackets Received\t"
                    "Packets Sent/sec\tPackets Received/sec\tBytes Sent\tBytes Received\t"
                    "Bytes Sent/sec\tBytes Received/sec\tBits Sent/sec/conn\t"
                    "Bits Received/sec/conn\tBytes Sent/packet\tBytes Received/packet\n");
            network_summary_log_needs_open = 0;
        }
        network_summary_stats.bytes_sent = 0;
        network_summary_stats.bytes_received = 0;
        network_summary_stats.packets_sent = 0;
        network_summary_stats.packets_received = 0;
        network_summary_stats.player_count_total = 0;
        network_summary_stats.player_count_samples = 0;
        network_summary_stats.start_ms = halo::cseries::time_query_performance_counter_ms();
    }
}

/**
 * out/phase4/networking_types_notes.md "bandwidth statistics" section; the fourteen
 * "%f\t"/"%d\t" writes match, in order, the fourteen columns of the "Game Summary" header
 * (network_stats_summary_log_open.c) that follow "Map" -- that column is written by this
 * function's caller, not here.
 *
 * @address 0x440820
 */
void StatsSummaryLog::write()
{
    int32_t now;
    float elapsed_ms;
    float elapsed_sec_inv;
    float avg_players;
    float avg_players_inv;
    float bytes_sent_rate;
    float bytes_received_rate;
    float packets_sent_rate;
    float packets_received_rate;
    float bytes_received_f;
    float bytes_sent_per_packet;
    float bytes_received_per_packet;

    if (2 < halo::cseries::globals().debug_log_level && network_statistics_logging_enabled != 0 &&
        network_summary_log_file != 0) {
        now = halo::cseries::time_query_performance_counter_ms();
        elapsed_ms = (float)(now - network_summary_stats.start_ms);
        if (now - network_summary_stats.start_ms < 0) {
            elapsed_ms = elapsed_ms + 4.2949673e+09f;
        }

        avg_players = (float)network_summary_stats.player_count_total /
                      (float)network_summary_stats.player_count_samples;
        elapsed_sec_inv = 1.0f / (elapsed_ms * 0.001f);
        packets_sent_rate = elapsed_sec_inv * (float)network_summary_stats.packets_sent;
        packets_received_rate = (float)network_summary_stats.packets_received * elapsed_sec_inv;
        bytes_sent_rate = (float)network_summary_stats.bytes_sent * elapsed_sec_inv;
        bytes_received_f = (float)network_summary_stats.bytes_received;
        bytes_received_rate = bytes_received_f * elapsed_sec_inv;
        bytes_sent_per_packet = (float)network_summary_stats.bytes_sent /
                                (float)network_summary_stats.packets_sent;
        bytes_received_per_packet = bytes_received_f / (float)network_summary_stats.packets_received;

        fprintf((FILE *)network_summary_log_file, "%f\t", (double)(elapsed_ms * 0.001f));
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)avg_players);
        fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.packets_sent);
        fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.packets_received);
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)packets_sent_rate);
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)packets_received_rate);
        fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.bytes_sent);
        fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.bytes_received);
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_sent_rate);
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_received_rate);

        avg_players_inv = 1.0f / avg_players;
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)(avg_players_inv * bytes_sent_rate * 8.0f));
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)(avg_players_inv * bytes_received_rate * 8.0f));
        fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_sent_per_packet);
        fprintf((FILE *)network_summary_log_file, "%f\n", (double)bytes_received_per_packet);
        fflush((FILE *)network_summary_log_file);
    }
}

}

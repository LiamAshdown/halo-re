/**
 * @file src/networking/net2_remote_console.cpp
 * RCON requests, console glue, update server and registry lookups.
 */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <stdint.h>
#include "halo/networking/net2_remote_console.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern int16_t network_join_error_code;
extern int32_t interface_loading_screen_progress;
extern int32_t join_ui_state;
extern void console_printf_verbose(const char *text);
extern void * rcon_out_channel_key;
extern uint8_t network_message_scratch[0x7ff8];
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern int16_t network_game_mode;
extern void * global_white_argb;
extern network_client_globals * network_client;
extern char registry_halo_version_buffer[0x40];
extern double sin(double x);
extern double cos(double x);
extern data_array * local_player_globals;
extern uint32_t update_client_staged[8];
extern uint32_t player_data;
extern void * message_delta_definition_table;
extern uint8_t update_server_pending_flush;
extern uint8_t update_server_history_index;
extern game_time_globals * game_time;
extern network_server_globals * network_server;
extern int32_t update_server_last_log_ms;
extern int32_t update_server_last_tick_ms;
extern void update_server_new(void);
extern void update_queues_dispose(void);
extern void update_server_dispose(void);
extern void update_client_stage_entry(void);
extern void ui_network_wait_timeout_check(void);
extern void ui_network_wait_timeout_start(void);
}

typedef struct rcon_request_record {
    char password[9];              
    char command[0x41];            
} rcon_request_record;

namespace halo::networking {

int8_t RemoteConsole::on_connect(const uint32_t *target_address, network_client_globals *client,
                            const uint32_t *session_info)
{
    network_connection_attempt_state *attempt;
    network_connection_endpoint *endpoint;
    large_integer counter;
    int32_t started_ms;
    network_channel *endpoint_probe;
    int16_t registration_result;
    int32_t i;

    halo::networking::network_address_to_string((s_network_address *)target_address);
    client->unknown_ec4 = 1;
    attempt = &client->connect_attempt;
    attempt->unknown_00 = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    started_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    attempt->elapsed_counter = 0;
    attempt->started_ms = started_ms;
    attempt->loading_started = 0;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    endpoint_probe = client->channel;
    if (endpoint_probe->endpoint != 0) {
        halo::networking::network_address_to_string(&(&client->connection)->address);
        registration_result = halo::networking::network_channel_attempt_connect(&(&client->connection)->address, (network_receive_queue *)client->channel, 0x96640, 1);
        if (registration_result != 0) {
            goto retry_limit_check;
        }
    }
    if (endpoint_probe->endpoint != 0) {
        client->state = 1;
        attempt->elapsed_counter = 0;
        console_printf_verbose("Connecting");

        endpoint = &client->connection;
        endpoint->address.ipv4 = 0;
        endpoint->address.ipv6_1 = 0;
        endpoint->address.ipv6_2 = 0;
        endpoint->address.ipv6_3 = 0;
        *(uint32_t *)&endpoint->address.size = 0;
        endpoint->unknown_14 = 0;
        endpoint->last_send_ms = 0;
        endpoint->message_count = 0;
        endpoint->retry_count = 0;
        endpoint->current_ping_ms = 0;
        endpoint->ready = 0;
        endpoint->unknown_23 = 0;
        endpoint->control_block = 0;

        endpoint->address.ipv4 = target_address[0];
        endpoint->address.ipv6_1 = target_address[1];
        endpoint->address.ipv6_2 = target_address[2];
        endpoint->address.ipv6_3 = target_address[3];
        *(uint32_t *)&endpoint->address.size = target_address[4];
        endpoint->unknown_14 = target_address[5];

        interface_loading_screen_progress = 0;
        join_ui_state = 5;
        if (endpoint->address.size == k_network_address_size_ipv4) {
            halo::networking::network_connection_endpoint_set(target_address, client);
        }
        return 1;
    }
retry_limit_check:
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
    return 0;
}

void RemoteConsole::rcon_out(char *text, int32_t unused_machine_id)
{
    char buf[81];
    void *fields[2];
    int32_t encoded_bits;

    strncpy(buf, text, 0x50);
    buf[0x50] = 0;
    fields[0] = buf;
    fields[1] = 0;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x37, 0, fields, 0, 1, 0);
    if (0 < encoded_bits) {
        halo::networking::network_session_send_to_machine(unused_machine_id, network_server, 1, network_message_scratch, encoded_bits, 1, 0, 0, 9);
    }
}

void RemoteConsole::bool_get_set(uint32_t argument_count, uint8_t *value, char **arguments, const char *name)
{
    char buffer[256];
    char *cursor;

    if (argument_count == 0) {
    report:
        chimera__console_out((ColorARGB *)0, (char *)"%s: %u", name, *value);
        return;
    }
    if (argument_count == 1) {
        char *text = arguments[0];
        if (text[0] != '\0') {
            strncpy(buffer, text, 0xff);
            buffer[0xff] = 0;
            halo::cseries::string_to_lowercase(buffer);
            cursor = buffer;
            halo::networking::string_trim_whitespace(&cursor);
            if (strncmp(buffer, "0", 2) == 0 || strncmp(buffer, "false", 6) == 0) {
                *value = 0;
                goto report;
            }
            if (strncmp(buffer, "1", 2) == 0 || strncmp(buffer, "true", 5) == 0) {
                *value = 1;
                goto report;
            }
        }
    }
    chimera__console_out((ColorARGB *)0, (char *)"Incorrect usage. Type help %s for more information.", name);
}

void RemoteConsole::rcon(int32_t argument_count, char **arguments)
{
    char *password;
    int32_t password_len;
    char command[68];
    int32_t budget;
    int32_t i;

    if (network_game_mode != 1) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon is a client-only function!");
        return;
    }
    if (argument_count < 2) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Incorrect usage. Type help rcon for more information.");
        return;
    }
    password = arguments[0];
    password_len = strlen(password);
    if (password_len == 0 || 8 < password_len) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon password must be between 1 and %d characters", 8);
        return;
    }

    command[0] = 0;
    budget = 0x40;
    for (i = 1; i < argument_count; i = i + 1) {
        char *word = arguments[i];
        int32_t word_len = strlen(word);

        budget = budget + (-3 - word_len);
        if (budget < 0) {
            chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon command can be no longer than %d characters", 0x40);
            return;
        }
        if (command[0] != 0) {
            strcat(command, " ");
        }
        if (1 < i) {
            strcat(command, "\"");
        }
        strcat(command, word);
        if (1 < i) {
            strcat(command, "\"");
        }
    }
    halo::networking::rcon_send_request(command, password);
}

void RemoteConsole::run_rcon_send_request(char *command, char *password)
{
    rcon_request_record record;
    void *items[2];
    int32_t encoded_bits;

    if (strlen(password) > 8) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"ERROR: Maximum rcon password length is %d characters", 8);
        return;
    }
    if (strlen(command) > 0x40) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"ERROR: Maximum rcon command length is %d characters", 0x40);
        return;
    }
    strcpy(record.password, password);
    strcpy(record.command, command);
    items[0] = &record;
    items[1] = 0;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x36, 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        network_channel *channel = network_client->channel;
        bit_stream *stream = (bit_stream *)((uint8_t *)channel + 0x10);
        int32_t free_bits = channel->outgoing.stream.last_bit -
            channel->outgoing.stream.byte_cursor * 8 - channel->outgoing.stream.bit_cursor + 1;

        if ((channel->flags & 1) == 0 &&
            (encoded_bits + 1 <= free_bits || halo::networking::network_channel_stream_flush((network_channel_stream *)((uint8_t *)channel + 0x10), (network_channel *)channel, 1) != 0)) {
            uint32_t item_flag = 1;

            channel->send_budget = channel->send_budget + encoded_bits + 1;
            halo::memory::bit_stream_write_bits_chunked(stream, &item_flag, 1);
            channel->outgoing.empty = 0;
            halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)network_message_scratch, encoded_bits);
            channel->outgoing.empty = 0;
        }
    }
}

uint32_t RemoteConsole::dist_id(void)
{
    uint32_t dist_id = 0;
    uint32_t size = 4;
    void *key;

    if (RegOpenKeyExA((HKEY)0x80000002, "Software\\Microsoft\\Microsoft Games\\Halo", 0, 0x20019,
                       (PHKEY)&key) == 0) {
        if (RegQueryValueExA((HKEY)key, "DistID", 0, 0, (uint8_t *)&dist_id, (LPDWORD)&size) != 0) {
            dist_id = 0;
        }
        RegCloseKey((HKEY)key);
    }
    return dist_id;
}

char * RemoteConsole::halo_version(void)
{
    void *key;
    int32_t i;
    int32_t status;
    uint32_t size;

    for (i = 0; i < 0x40; i++) {
        registry_halo_version_buffer[i] = 0;
    }

    size = 0x3f;
    status = RegOpenKeyExA((HKEY)0x80000002, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                            0x20019, (PHKEY)&key);
    if (status != 0) {
        registry_halo_version_buffer[0] = 0;
        return registry_halo_version_buffer;
    }
    status = RegQueryValueExA((HKEY)key, "Version", 0, 0, (uint8_t *)registry_halo_version_buffer, (LPDWORD)&size);
    if (status != 0) {
        registry_halo_version_buffer[0] = 0;
    }
    RegCloseKey((HKEY)key);
    return registry_halo_version_buffer;
}

char RemoteConsole::send_update(uint32_t *tick_count, char frame_time_overflow)
{
    char result;
    char flush_ok;
    int32_t now_ms;
    uint32_t reliable_seq;
    uint16_t player_id;
    uint32_t control[8];
    char history_byte;
    char logged_history_byte;
    uint32_t record[13];
    uint32_t checksum;
    float position[4];
    float direction[3];
    void *encoded;
    network_channel *channel;
    data_iterator iterator;
    void *it;
    int32_t max_bits;
    datum_index unit_index = k_datum_index_none;
    int32_t history_update_id = 0;

    result = 1;
    if (network_client == 0) {
        network_game_mode = 0;
        update_queues_dispose();
        update_server_new();
        update_server_dispose();
        ui_network_wait_timeout_check();
        return result;
    }
    if (network_client->state == 3) {
        now_ms = halo::cseries::time_query_performance_counter_ms();
        reliable_seq = network_client->last_update_id & 0x7fffffff;
        player_id = local_player_globals->maximum_count;
        memcpy(control, update_client_staged, sizeof(control));
        history_byte = 0;
        if ((int32_t)tick_count > 0 && frame_time_overflow == 0) {
            iterator.data = (data_array *)(uintptr_t)player_data;
            iterator.next_index = 0;
            iterator.index = k_datum_index_none;
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            it = halo::memory::data_iterator_next(&iterator);
            while (it != 0 && ((player *)it)->local_player_index == -1) {
                it = halo::memory::data_iterator_next(&iterator);
            }
            if (it != 0) {
                unit_index = ((player *)it)->unit;
            }
        }
        if (network_game_mode == 1) {
            flush_ok = halo::networking::player_update_history_add(unit_index, (player_update_history *)network_client->update_history, (int32_t)(uintptr_t)tick_count,
                *(player_action *)control, &history_update_id);
            if (flush_ok != 1) {
                update_client_stage_entry();
                ui_network_wait_timeout_start();
                goto after_send;
            }
        } else {
            logged_history_byte = update_server_history_index;
            update_server_history_index = (update_server_history_index + 1) & 0x3f;
            (void)logged_history_byte;
        }
        direction[0] = (float)cos(position[2]);
        direction[1] = (float)(cos(position[1]) * direction[0]);
        direction[2] = (float)(sin(position[1]) * direction[0]);
        history_byte = update_server_history_index;

        if (network_game_mode == 1) {
            encoded = (void *)(uintptr_t)halo::networking::message_delta_encode_single_value(0xd, &history_byte, record,
                (uint8_t *)network_client + 0xf14, (int32_t)network_message_scratch, 0x7ff8, 1);
            if (update_server_pending_flush == 1) {
                halo::cseries::time_query_performance_counter_ms();
            }
            channel = network_client->channel;
            update_server_pending_flush = 0;
            result = 1;
            if ((channel->flags & 1) == 0) {
                max_bits = (channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8) -
                           channel->outgoing.stream.bit_cursor + 1;
                if (max_bits < (int32_t)(uintptr_t)encoded + 1) {
                    flush_ok = halo::networking::network_channel_stream_flush(&channel->outgoing, channel, 1);
                    if (flush_ok == 0) {
                        goto after_channel_check;
                    }
                }
                channel->send_budget = channel->send_budget + (int32_t)(uintptr_t)encoded + 1;
                {
                    uint32_t item_flag = 1;

                    halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
                    channel->outgoing.empty = 0;
                    halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)network_message_scratch,
                                                  (int32_t)(uintptr_t)encoded);
                    channel->outgoing.empty = 0;
                }
                flush_ok = halo::networking::network_channel_stream_flush(&channel->outgoing, channel, 1);
            } else {
                flush_ok = 1;
            }
        after_channel_check:
            result = flush_ok;
            if (flush_ok != 0) {
                halo::networking::player_update_history_log_write(1, 0, "[%d]: Sent update [%d], [%d] ticks.\n",
                    game_time->game_time, (int32_t)history_byte, (int32_t)(uintptr_t)tick_count);

            }
        } else {
            network_machine *machine = halo::networking::network_machine_find_by_id(network_server, player_id);
            halo::networking::network_game_client_apply_position_update((uint8_t *)machine, control, tick_count, network_client);
        }
        memcpy(record, control, sizeof(record) < sizeof(control) ? sizeof(record) : sizeof(control));
        if (history_byte == 0) {
            goto after_send;
        }
    after_send:
        if (update_server_pending_flush == 0) {
            update_server_last_log_ms = halo::cseries::time_query_performance_counter_ms();
            update_server_pending_flush = 1;
        }
        update_server_last_tick_ms = now_ms;
    }
    ui_network_wait_timeout_check();
    return result;
}

}  // namespace halo::networking

namespace halo::networking {
int8_t chimera__on_connect(const uint32_t *target_address, network_client_globals *client,
                            const uint32_t *session_info)
{
    return halo::networking::RemoteConsole::on_connect(target_address, client, session_info);
}

void chimera__rcon_out(char *text, int32_t unused_machine_id)
{
    halo::networking::RemoteConsole::rcon_out(text, unused_machine_id);
}

void console_command_bool_get_set(uint32_t argument_count, uint8_t *value, char **arguments, const char *name)
{
    halo::networking::RemoteConsole::bool_get_set(argument_count, value, arguments, name);
}

void rcon(int32_t argument_count, char **arguments)
{
    halo::networking::RemoteConsole::rcon(argument_count, arguments);
}

void rcon_send_request(char *command, char *password)
{
    halo::networking::RemoteConsole::run_rcon_send_request(command, password);
}

uint32_t registry_get_dist_id(void)
{
    return halo::networking::RemoteConsole::dist_id();
}

char * registry_get_halo_version(void)
{
    return halo::networking::RemoteConsole::halo_version();
}

char update_server_send_update(uint32_t *tick_count, char frame_time_overflow)
{
    return halo::networking::RemoteConsole::send_update(tick_count, frame_time_overflow);
}

}

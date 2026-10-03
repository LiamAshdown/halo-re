/**
 * @file src/networking/net2_remote_console.cpp
 * RCON requests, console glue, update server and registry lookups.
 */
#include "win32.h"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
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
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern int16_t network_join_error_code;
extern int32_t interface_loading_screen_progress;
extern int32_t join_ui_state;
extern void * rcon_out_channel_key;
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern int16_t network_game_mode;
extern void * global_white_argb;
extern network_client_globals * network_client;
extern char registry_halo_version_buffer[0x40];
extern double sin(double x);
extern double cos(double x);
extern player_globals * local_player_globals;
extern uint32_t update_client_staged[8];
extern data_array * player_data;
extern void * message_delta_definition_table;
extern uint8_t update_server_pending_flush;
extern uint8_t update_server_history_index;
extern network_server_globals * network_server;
extern int32_t update_server_last_log_ms;
extern int32_t update_server_last_tick_ms;
extern void update_server_new(void);
extern void update_queues_dispose(void);
extern void update_server_dispose(void);
extern void ui_network_wait_timeout_check(void);
}

typedef struct rcon_request_record {
    char password[9];              
    char command[0x41];            
} rcon_request_record;

namespace halo::networking {

/**
 * Appends one message of `bit_count` bits from `bits` to the channel's outgoing stream, preceded by a set item
 * flag bit and counted against the channel's send budget. Flushes the stream first when it has no room; returns
 * 0 when that flush fails and nothing was staged.
 */
static bool stage_channel_item(network_channel *channel, const uint8_t *bits, int32_t bit_count)
{
    bit_stream *stream = &channel->outgoing.stream;
    int32_t free_bits = stream->last_bit - stream->byte_cursor * 8 - stream->bit_cursor + 1;
    uint32_t item_flag = 1;

    if (bit_count + 1 > free_bits && halo::networking::network_channel_stream_flush(&channel->outgoing, channel, 1) == 0) {
        return false;
    }
    channel->send_budget = channel->send_budget + bit_count + 1;
    halo::memory::bit_stream_write_bits_chunked(stream, &item_flag, 1);
    channel->outgoing.empty = 0;
    halo::memory::bit_stream_write_bits_chunked(stream, (const uint32_t *)bits, bit_count);
    channel->outgoing.empty = 0;
    return true;
}

/**
 * Starts a connection from `client` to `target_address`: stamps the attempt (start time, the 9-dword session
 * info), opens the transport connection through the channel's endpoint and, when that succeeds, moves the client
 * to the connecting state, records the target as the connection endpoint and shows the loading screen. Returns
 * 1 on success; on failure it returns 0 and sets join error 7 unless an error is already pending.
 *
 * @address 0x4d8ed0
 */
int8_t RemoteConsole::on_connect(const uint32_t *target_address, network_client_globals *client,
                            const uint32_t *session_info)
{
    network_connection_attempt_state *attempt = &client->connect_attempt;
    network_receive_queue *endpoint = client->channel->endpoint;
    large_integer counter;
    int8_t connected = 0;
    int32_t i;

    halo::networking::network_address_to_string((s_network_address *)target_address);
    client->unknown_ec4 = 1;
    attempt->unknown_00 = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    attempt->started_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    attempt->elapsed_counter = 0;
    attempt->loading_started = 0;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    if (endpoint != 0) {
        halo::networking::network_address_to_string((s_network_address *)target_address);
        connected = halo::networking::network_channel_attempt_connect((s_network_address *)target_address, endpoint, 0x96640, 1) == 0;
    }
    if (!connected) {
        if (network_join_error_code == -1) {
            network_join_error_code = 7;
        }
        return 0;
    }

    client->state = k_network_client_state_connecting;
    attempt->elapsed_counter = 0;
    halo::interface::console_printf_verbose((ColorARGB *)0, (char *)"Connecting");
    memset(&client->connection, 0, 10 * sizeof(uint32_t));
    memcpy(&client->connection.address, target_address, 6 * sizeof(uint32_t));
    interface_loading_screen_progress = 0;
    join_ui_state = 5;
    if (client->connection.address.size == k_network_address_size_ipv4) {
        halo::networking::network_connection_endpoint_set(target_address, client);
    }
    return connected;
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
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::rcon_output), 0, fields, 0, 1, 0);
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
        halo::interface::chimera__console_out((ColorARGB *)0, (char *)"%s: %u", name, *value);
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
    halo::interface::chimera__console_out((ColorARGB *)0, (char *)"Incorrect usage. Type help %s for more information.", name);
}

void RemoteConsole::rcon(int32_t argument_count, char **arguments)
{
    char *password;
    int32_t password_len;
    char command[68];
    int32_t budget;
    int32_t i;

    if (network_game_mode != 1) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon is a client-only function!");
        return;
    }
    if (argument_count < 2) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Incorrect usage. Type help rcon for more information.");
        return;
    }
    password = arguments[0];
    password_len = strlen(password);
    if (password_len == 0 || 8 < password_len) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon password must be between 1 and %d characters", 8);
        return;
    }

    command[0] = 0;
    budget = 0x40;
    for (i = 1; i < argument_count; i = i + 1) {
        char *word = arguments[i];
        int32_t word_len = strlen(word);

        budget = budget + (-3 - word_len);
        if (budget < 0) {
            halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"rcon command can be no longer than %d characters", 0x40);
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
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"ERROR: Maximum rcon password length is %d characters", 8);
        return;
    }
    if (strlen(command) > 0x40) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"ERROR: Maximum rcon command length is %d characters", 0x40);
        return;
    }
    strcpy(record.password, password);
    strcpy(record.command, command);
    items[0] = &record;
    items[1] = 0;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::rcon_request), 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        network_channel *channel = network_client->channel;

        if ((channel->flags & k_network_channel_listening) == 0) {
            stage_channel_item(channel, network_message_scratch, encoded_bits);
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

/**
 * Per-frame client side of the update stream. Without a client session it tears the update queues down
 * and resets them. While the client is playing it takes the staged control record, finds the local
 * player's unit and, if one exists, records the control in the prediction history and builds the
 * message 0x0d update record (ticks, update id, control and aim direction). A client sends the record
 * to the host's channel and logs it; a host feeds it straight into client_apply_position_update for its own
 * machine. The record becomes the baseline of the next delta. Returns 0 when the channel could not be
 * flushed, which the caller treats as a lost connection.
 *
 * @address 0x4ddfb0
 */
char RemoteConsole::send_update(int32_t tick_count, char frame_time_overflow)
{
    char result = 1;
    int32_t now_ms;
    uint8_t sent_update = 0;

    if (network_client == 0) {
        network_game_mode = 0;
        halo::game::update_queues_dispose();
        halo::game::update_server_new();
        halo::game::update_server_dispose();
        halo::interface::ui_network_wait_timeout_check();
        return result;
    }
    if (network_client->state != k_network_client_state_playing) {
        halo::interface::ui_network_wait_timeout_check();
        return result;
    }

    now_ms = halo::cseries::time_query_performance_counter_ms();
    if (tick_count > 0 && frame_time_overflow == 0) {
        struct {
            uint32_t update_id;
            int16_t pad_04;
            int16_t local_player_count;
            player_action action;
        } position_packet;
        client_update_record record;
        player_action control;
        data_iterator iterator;
        player *local_player;
        uint8_t history_byte;
        int32_t history_update_id = 0;

        position_packet.update_id = network_client->last_update_id & 0x7fffffff;
        position_packet.pad_04 = 0;
        position_packet.local_player_count = local_player_globals->local_player_count;
        memcpy(&position_packet.action, update_client_staged, sizeof(position_packet.action));
        control = position_packet.action;

        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        local_player = (player *)halo::memory::data_iterator_next(&iterator);
        while (local_player != 0 && local_player->local_player_index == -1) {
            local_player = (player *)halo::memory::data_iterator_next(&iterator);
        }

        if (local_player != 0 && local_player->unit != k_datum_index_none) {
            if (network_game_mode == 1) {
                char added = halo::networking::player_update_history_add(local_player->unit,
                    (player_update_history *)network_client->update_history, tick_count, control, &history_update_id);

                history_byte = (uint8_t)history_update_id;
                if (added != 1) {
                    player_action fallback = {};

                    fallback.weapon_index = network_client->last_update_sent.weapon_index;
                    fallback.grenade_index = network_client->last_update_sent.grenade_index;
                    fallback.zoom_level = network_client->last_update_sent.zoom_level;
                    halo::game::update_client_stage_entry((uint32_t *)&fallback);
                    halo::interface::ui_network_wait_timeout_start();
                    goto note_pending_flush;
                }
            } else {
                history_byte = update_server_history_index;
                update_server_history_index = (update_server_history_index + 1) & 0x3f;
            }

            {
                double cos_pitch = cos((double)control.desired_pitch);

                record.tick_count = (uint8_t)tick_count;
                record.update_id = position_packet.update_id;
                record.control_flags = control.control_flags;
                record.yaw = control.desired_yaw;
                record.pitch = control.desired_pitch;
                record.aim_direction[0] = (float)(cos((double)control.desired_yaw) * cos_pitch);
                record.aim_direction[1] = (float)(sin((double)control.desired_yaw) * cos_pitch);
                record.aim_direction[2] = (float)sin((double)control.desired_pitch);
                record.throttle_x = control.throttle_x;
                record.throttle_y = control.throttle_y;
                record.primary_trigger = control.primary_trigger;
                record.weapon_index = control.weapon_index;
                record.grenade_index = control.grenade_index;
                record.zoom_level = control.zoom_level;
            }

            if (network_game_mode == 1) {
                int32_t encoded_bits = halo::networking::message_delta_encode_single_value(0xd, &history_byte, &record,
                    &network_client->last_update_sent, (int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1);
                network_channel *channel = network_client->channel;

                sent_update = 1;
                update_server_pending_flush = 0;
                if ((channel->flags & k_network_channel_listening) != 0) {
                    result = 1;
                } else {
                    result = stage_channel_item(channel, network_message_scratch, encoded_bits) &&
                        halo::networking::network_channel_stream_flush(&channel->outgoing, channel, 1);
                }
                if (result != 0) {
                    halo::networking::player_update_history_log_write(8, 0, "[%d]: Sent update [%d], [%d] ticks.\n",
                        halo::game::globals().game_time->game_time, (int32_t)history_byte, tick_count);
                }
            } else {
                halo::networking::network_game_client_apply_position_update(
                    halo::networking::network_machine_find_by_id(network_server, (int8_t)local_player->machine_index),
                    (const client_position_packet *)&position_packet, tick_count, history_byte);
            }
            network_client->last_update_sent = record;
        }
    }

note_pending_flush:
    if (sent_update == 0) {
        if (update_server_pending_flush == 0) {
            update_server_last_log_ms = halo::cseries::time_query_performance_counter_ms();
            update_server_pending_flush = 1;
        }
    }
    update_server_last_tick_ms = now_ms;
    halo::interface::ui_network_wait_timeout_check();
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

char update_server_send_update(int32_t tick_count, char frame_time_overflow)
{
    return halo::networking::RemoteConsole::send_update(tick_count, frame_time_overflow);
}

}

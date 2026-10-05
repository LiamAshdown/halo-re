/**
 * @file src/networking/net2_message_delta_parameters.cpp
 * Parameters-protocol registration, config file and update packets.
 */
#include "crt.h"
#include "halo/core/cstring.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include "halo/networking/net2_message_delta_parameters.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/networking/vars.hpp"
#include <stdio.h>
#include "halo/game/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/platform/memory.hpp"

static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &message_delta_parameters_enabled = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_enabled);
void message_delta_table_guard(bool open);  // net2_field_codec.cpp
static auto &message_delta_unknown_table_0069a304 = halo::link::ref<uint8_t [28][0x18]>(halo::networking::vars().message_delta_unknown_table_0069a304);
static auto &message_delta_config_text_buffer = halo::link::ref<char []>(halo::hs::vars().message_delta_config_text_buffer);
static auto &message_delta_config_write_mode_string = halo::link::ref<char []>(halo::networking::vars().message_delta_config_write_mode_string);
static auto &message_delta_parameter_count = halo::link::ref<int32_t>(halo::networking::vars().message_delta_parameter_count);
static auto &message_delta_parameters = halo::link::ref<message_delta_parameter []>(halo::networking::vars().message_delta_parameters);
static auto &message_delta_config_value_delimiters = halo::link::ref<char []>(halo::networking::vars().message_delta_config_value_delimiters);
static auto &message_delta_parameters_protocol_sequence = halo::link::ref<int32_t>(halo::networking::vars().message_delta_parameters_protocol_sequence);
static auto &message_delta_config_mode_string = halo::link::ref<char []>(halo::networking::vars().message_delta_config_mode_string);
static auto &message_delta_parameters_sending = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_sending);
static auto &message_delta_parameters_protocol_broadcast_target = halo::link::ref<uint8_t []>(halo::networking::vars().message_delta_parameters_protocol_broadcast_target);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);


namespace halo::networking {

void ParametersProtocol::dump_to_config_file(void)
{
    int32_t i;
    void *file;

    halo::networking::message_delta_definitions_teardown_field_bindings();
    message_delta_table_guard(true);
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 0;
    }
    message_delta_table_guard(false);
    if (message_delta_parameters_enabled == 1) {
        file = fopen("parameters.cfg", message_delta_config_write_mode_string);
        if (file != 0) {
            fprintf((FILE *)file, message_delta_config_text_buffer);
            fclose((FILE *)file);
        }
        halo::networking::message_delta_parameters_protocol_free_registered();
    }
}

uint8_t ParametersProtocol::find_registered(char *name, void **out_value)
{
    int32_t (*const strcmp)(const char *a, const char *b) = reinterpret_cast<int32_t (*)(const char *a, const char *b)>(&::strcmp);
    int32_t i;

    for (i = 0; i < message_delta_parameter_count; i++) {
        if (strcmp(name, message_delta_parameters[i].name) == 0) {
            if (out_value != 0) {
                *out_value = message_delta_parameters[i].value;
            }
            return 1;
        }
    }
    return 0;
}

void ParametersProtocol::format_received_values(int32_t *values)
{
    void * (*const memcpy)(void *dest, const void *src, int32_t count) = reinterpret_cast<void * (*)(void *dest, const void *src, int32_t count)>(&::memcpy);
    char *dest;
    int32_t i;
    int32_t name_len;
    int32_t written;

    dest = message_delta_config_text_buffer;
    for (i = 0; i < message_delta_parameter_count; i++) {
        char *name = message_delta_parameters[i].name;
        for (name_len = 0; name[name_len] != 0; name_len++) {
        }
        memcpy(dest, name, name_len);
        dest[name_len] = ' ';
        dest = dest + name_len + 1;
        if (message_delta_parameters[i].type == 1) {
            written = sprintf(dest, "%d\n");

        } else {
            written = sprintf(dest, "%f\n", (double)*(float *)&values[i]);
        }
        dest = dest + written;
    }
}

void ParametersProtocol::format_registered_values(void)
{
    int32_t offset;
    int32_t i;
    int32_t written;

    offset = 0;
    for (i = 0; i < message_delta_parameter_count; i++) {
        if (message_delta_parameters[i].type == 1) {
            written = sprintf(message_delta_config_text_buffer + offset, "%s %d\n",
                               message_delta_parameters[i].name, *(int32_t *)message_delta_parameters[i].value);
        } else {
            written = sprintf(message_delta_config_text_buffer + offset, "%s %f\n",
                               message_delta_parameters[i].name, (double)*(float *)message_delta_parameters[i].value);
        }
        offset = offset + written;
    }
    message_delta_config_text_buffer[offset] = 0;
}

void ParametersProtocol::free_registered(void)
{
    int32_t i;

    if (message_delta_parameters_enabled == 1) {
        for (i = 0; i < message_delta_parameter_count; i++) {
            halo::platform::heap_free(message_delta_parameters[i].name);
            message_delta_parameters[i].name = 0;
        }
        message_delta_parameter_count = 0;
    }
}

void ParametersProtocol::pack_values(int32_t *out_values)
{
    int32_t i;

    for (i = 0; i < message_delta_parameter_count; i++) {
        out_values[i] = *(int32_t *)message_delta_parameters[i].value;
    }
}

int32_t ParametersProtocol::parse_value_from_config(char *name, char *format, void *out_value)
{
    char *found;
    char *value_start;
    int32_t scanned;

    found = strstr(message_delta_config_text_buffer, name);
    if (found != 0) {
        value_start = strstr(found, message_delta_config_value_delimiters);
        scanned = sscanf(value_start + 1, format, out_value);
        if (0 < scanned) {
            return 1;
        }
    }
    return 0;
}

void ParametersProtocol::receive_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];

    if (message_delta_parameters_enabled == 1) {
        if (state->incremental == 0) {
            struct {
                uint8_t sequence;
                uint8_t pad[3];
                int32_t values[64];
            } destination;

            if (halo::networking::message_delta_decode_compound_field(context, &destination) == 1) {
                halo::networking::message_delta_parameters_protocol_format_received_values(destination.values);
                message_delta_parameters_protocol_sequence = destination.sequence;
            }
        } else {
            halo::networking::message_delta_decode_compound_field_staged(context);
        }
    }
}

void ParametersProtocol::run_register(char *scope, char *name, int32_t type, void *value)
{
    char *buffer;

    if (message_delta_parameters_enabled == 1) {
        if (scope == 0) {
            buffer = (char *)halo::platform::heap_allocate(0, strlen(name) + 1);
            strcpy(buffer, strdup(name));
        } else {
            int32_t scope_len = strlen(scope);
            int32_t name_len = strlen(name);
            buffer = (char *)halo::platform::heap_allocate(0, scope_len + name_len + 3);
            memcpy(buffer, scope, scope_len + 1);
            buffer[scope_len] = ':';
            buffer[scope_len + 1] = ':';
            buffer[scope_len + 2] = '\0';
            memcpy(buffer + scope_len + 2, name, name_len);
        }
        if (halo::networking::message_delta_parameters_protocol_find_registered(buffer, 0) == 0) {
            message_delta_parameters[message_delta_parameter_count].name = buffer;
            message_delta_parameters[message_delta_parameter_count].type = type;
            message_delta_parameters[message_delta_parameter_count].value = value;
            message_delta_parameter_count = message_delta_parameter_count + 1;
        }
        if (type == 1) {
            halo::networking::message_delta_parameters_protocol_parse_value_from_config(buffer, halo::mutable_literal("%d"), value);
        } else {
            halo::networking::message_delta_parameters_protocol_parse_value_from_config(buffer, halo::mutable_literal("%f"), value);
        }
    }
}

void ParametersProtocol::reload_from_config_file(void)
{
    void *file;
    int32_t length;

    if (message_delta_parameters_enabled == 1) {
        file = fopen("parameters.cfg", message_delta_config_mode_string);
        if (file != 0) {
            fseek((FILE *)file, 0, 2);
            length = ftell((FILE *)file);
            fseek((FILE *)file, 0, 0);
            fread(message_delta_config_text_buffer, 1, length, (FILE *)file);
            message_delta_config_text_buffer[length] = 0;
            fclose((FILE *)file);
        }
    }
}

void ParametersProtocol::send_update(void)
{
    uint32_t next_sequence;
    int32_t encoded_bits;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_sending = 1;
        next_sequence = (message_delta_parameters_protocol_sequence + 1) & 0x80000003;
        if ((int32_t)next_sequence < 0) {
            next_sequence = (next_sequence - 1 | 0xfffffffc) + 1;
        }
        halo::networking::message_delta_parameters_protocol_format_registered_values();
        {
            uint8_t update_record[260];
            uint8_t *record_pointer;
            int32_t record_pointer_pad;

            update_record[0] = (uint8_t)next_sequence;
            halo::networking::message_delta_parameters_protocol_pack_values((int32_t *)(update_record + 4));
            record_pointer = update_record;
            record_pointer_pad = 0;
            encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::parameters_update), 0, (void **)&record_pointer, 0, 1, '\0');
            if (0 < encoded_bits) {
                if (halo::networking::network_session_broadcast_to_all(network_server, encoded_bits, 1, network_message_scratch, 1, 0, 1, 3) != '\0') {
                    message_delta_parameters_protocol_sequence = next_sequence;
                }
            }
        }
        message_delta_parameters_sending = 0;
    }
}

}  // namespace halo::networking

namespace halo::networking {
void message_delta_parameters_protocol_dump_to_config_file(void)
{
    halo::networking::ParametersProtocol::dump_to_config_file();
}

uint8_t message_delta_parameters_protocol_find_registered(char *name, void **out_value)
{
    return halo::networking::ParametersProtocol::find_registered(name, out_value);
}

void message_delta_parameters_protocol_format_received_values(int32_t *values)
{
    halo::networking::ParametersProtocol::format_received_values(values);
}

void message_delta_parameters_protocol_format_registered_values(void)
{
    halo::networking::ParametersProtocol::format_registered_values();
}

void message_delta_parameters_protocol_free_registered(void)
{
    halo::networking::ParametersProtocol::free_registered();
}

void message_delta_parameters_protocol_pack_values(int32_t *out_values)
{
    halo::networking::ParametersProtocol::pack_values(out_values);
}

int32_t message_delta_parameters_protocol_parse_value_from_config(char *name, char *format, void *out_value)
{
    return halo::networking::ParametersProtocol::parse_value_from_config(name, format, out_value);
}

void message_delta_parameters_protocol_receive_update(void **context)
{
    halo::networking::ParametersProtocol::receive_update(context);
}

void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value)
{
    halo::networking::ParametersProtocol::run_register(scope, name, type, value);
}

void message_delta_parameters_protocol_reload_from_config_file(void)
{
    halo::networking::ParametersProtocol::reload_from_config_file();
}

void message_delta_parameters_protocol_send_update(void)
{
    halo::networking::ParametersProtocol::send_update();
}

}

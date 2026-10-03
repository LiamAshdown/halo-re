/**
 * @file src/networking/net2_message_delta_parameters.cpp
 * Parameters-protocol registration, config file and update packets.
 */
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "win32.h"
#include <string.h>
#include "halo/networking/net2_message_delta_parameters.hpp"

extern "C" {
extern uint8_t message_delta_parameters_enabled;
extern uint8_t message_delta_unknown_table_0069a304[28][0x18];
extern char message_delta_config_text_buffer[];
extern char message_delta_config_write_mode_string[];
extern void message_delta_definitions_teardown_field_bindings(void);
extern int32_t message_delta_parameter_count;
extern message_delta_parameter message_delta_parameters[];
extern int32_t sprintf(char *buffer, const char *format, ...);
extern char message_delta_config_value_delimiters[];
extern int32_t sscanf(const char *buffer, const char *format, ...);
extern int32_t message_delta_parameters_protocol_sequence;
extern uint8_t message_delta_decode_compound_field(void **context, void *destination);
extern void message_delta_decode_compound_field_staged(void **context);
extern char message_delta_config_mode_string[];
extern uint8_t message_delta_parameters_sending;
extern uint8_t message_delta_parameters_protocol_broadcast_target[];
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_all(int32_t a1, void *a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
void message_delta_parameters_protocol_dump_to_config_file(void);
uint8_t message_delta_parameters_protocol_find_registered(char *name, void **out_value);
void message_delta_parameters_protocol_format_received_values(int32_t *values);
void message_delta_parameters_protocol_format_registered_values(void);
void message_delta_parameters_protocol_free_registered(void);
void message_delta_parameters_protocol_pack_values(int32_t *out_values);
int32_t message_delta_parameters_protocol_parse_value_from_config(char *name, char *format, void *out_value);
void message_delta_parameters_protocol_receive_update(void **context);
void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value);
void message_delta_parameters_protocol_reload_from_config_file(void);
void message_delta_parameters_protocol_send_update(void);
}


namespace halo::networking {

void ParametersProtocol::dump_to_config_file(void)
{
    int32_t i;
    void *file;

    message_delta_definitions_teardown_field_bindings();
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 0;
    }
    if (message_delta_parameters_enabled == 1) {
        file = fopen("parameters.cfg", message_delta_config_write_mode_string);
        if (file != 0) {
            fprintf((FILE *)file, message_delta_config_text_buffer);
            fclose((FILE *)file);
        }
        message_delta_parameters_protocol_free_registered();
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
            GlobalFree(message_delta_parameters[i].name);
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

            if (message_delta_decode_compound_field(context, &destination) == 1) {
                message_delta_parameters_protocol_format_received_values(destination.values);
                message_delta_parameters_protocol_sequence = destination.sequence;
            }
        } else {
            message_delta_decode_compound_field_staged(context);
        }
    }
}

void ParametersProtocol::run_register(char *scope, char *name, int32_t type, void *value)
{
    char (*const message_delta_parameters_protocol_find_registered)(char *name, void **out_value) = reinterpret_cast<char (*)(char *name, void **out_value)>(&::message_delta_parameters_protocol_find_registered);
    int32_t (*const message_delta_parameters_protocol_parse_value_from_config)(char *format, void *out_value) = reinterpret_cast<int32_t (*)(char *format, void *out_value)>(&::message_delta_parameters_protocol_parse_value_from_config);
    char *buffer;

    if (message_delta_parameters_enabled == 1) {
        if (scope == 0) {
            buffer = (char *)GlobalAlloc(0, strlen(name) + 1);
            strcpy(buffer, strdup(name));
        } else {
            int32_t scope_len = strlen(scope);
            int32_t name_len = strlen(name);
            buffer = (char *)GlobalAlloc(0, scope_len + name_len + 3);
            memcpy(buffer, scope, scope_len + 1);
            buffer[scope_len] = ':';
            buffer[scope_len + 1] = ':';
            buffer[scope_len + 2] = '\0';
            memcpy(buffer + scope_len + 2, name, name_len);
        }
        if (message_delta_parameters_protocol_find_registered(buffer, 0) == 0) {
            message_delta_parameters[message_delta_parameter_count].name = buffer;
            message_delta_parameters[message_delta_parameter_count].type = type;
            message_delta_parameters[message_delta_parameter_count].value = value;
            message_delta_parameter_count = message_delta_parameter_count + 1;
        }
        if (type == 1) {
            message_delta_parameters_protocol_parse_value_from_config((char *)"%d", value);
        } else {
            message_delta_parameters_protocol_parse_value_from_config((char *)"%f", value);
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
    void (*const message_delta_parameters_protocol_pack_values)(void) = reinterpret_cast<void (*)(void)>(&::message_delta_parameters_protocol_pack_values);
    uint32_t next_sequence;
    int32_t encoded_bits;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_sending = 1;
        next_sequence = (message_delta_parameters_protocol_sequence + 1) & 0x80000003;
        if ((int32_t)next_sequence < 0) {
            next_sequence = (next_sequence - 1 | 0xfffffffc) + 1;
        }
        message_delta_parameters_protocol_format_registered_values();
        message_delta_parameters_protocol_pack_values();
        {
            uint8_t local_104[260];
            uint8_t *local_10c;
            int32_t local_108;

            local_104[0] = (uint8_t)next_sequence;
            local_10c = local_104;
            local_108 = 0;
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x22, 0, (void **)&local_10c, 0, 1, '\0');
            if (0 < encoded_bits) {
                if (network_session_broadcast_to_all(1, message_delta_parameters_protocol_broadcast_target, 1, 0, 1, 3) != '\0') {
                    message_delta_parameters_protocol_sequence = next_sequence;
                }
            }
        }
        message_delta_parameters_sending = 0;
    }
}

}  // namespace halo::networking

extern "C" {
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

#pragma once

#include "halo/shell/runtime.hpp"

namespace halo::shell {

/**
 * View over the hardware requirements (config.txt) parser object.
 */
class HwreqParser {
public:
    explicit HwreqParser(const hwreq_parser *value) : self(const_cast<hwreq_parser *>(value)) {}

    static bool is_delimiter(char c)
    {
        return c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t';
    }

    void advance_line_raw() const
    {
        char *line;

        do {
            line = (char *)self->cursor;
            self->cursor = (uint32_t)(line + 1);
            if (*line == '\r') {
                break;
            }
        } while ((char *)self->cursor < (char *)self->end);
        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
            self->cursor = (uint32_t)(line + 2);
        }
    }

    uint32_t match_keyword(const char *keyword);
    int32_t parse_hex_digit();
    uint32_t parse_hex_id();
    int32_t parse_hex_id_byteswap();
    int32_t parse_number();
    char *parse_quoted_string();
    void skip_line();
    void skip_whitespace();
    void report_error(const char *message);
    hwreq_parser *construct();
    static hwreq_parser *create();
    void destruct();
    void scalar_deleting_destruct();
    char *error_message_text();
    uint32_t flag_count();
    char *flag_name(uint32_t index);
    char *flag_value(uint32_t index);
    hwreq_property_set *flags_set();
    char *graphics_device_name_text();
    char *graphics_vendor_name_text();
    uint32_t requirement_count();
    char *requirement_name(uint32_t index);
    char *requirement_value(uint32_t index);
    char *sound_device_name_text();
    char *sound_vendor_name_text();
    uint8_t has_error();
    hwreq_property_set *find_property_set(const char *name);
    uint8_t find_requirements_section();
    uint8_t parse(const char *path, const shell_sound_device *sound_device, const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed);
    void rewind();
    uint8_t parse_block(hwreq_property_set *target);
    const char *parse_flag_assignment(hwreq_property_set *property_set);
    uint8_t parse_propertyset_directive();
    uint8_t parse_vendor_block();
    uint8_t parse_audiovendor_block();
    uint8_t scan_for_applytoall();
    uint8_t scan_for_applytoall_or_vendor();
    const char *evaluate_condition(int32_t kind, uint32_t value);
    const char *resolve_field();

    hwreq_parser *self;
};

}

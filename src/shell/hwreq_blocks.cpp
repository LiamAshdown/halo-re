#include "halo/shell/hwreq.hpp"
#include "halo/shell/layout.hpp"

namespace halo::shell {

/**
 * Scans forward from the parser's cursor for a line beginning "Requirements" followed by a
 * delimiter; once found, skips that line and parses the remainder of the file as a block into
 * this->requirements. Returns true (with nothing parsed) if the end of the file is reached first in
 * either scan.
 *
 * @address 0x57ae50
 */
uint8_t HwreqParser::find_requirements_section()
{
    char c;

    for (;;) {
        if (_strnicmp(text_cursor(), "Requirements", 12) == 0) {
            c = (text_cursor())[12];
            if (is_delimiter(c)) {
                break;
            }
        }
        advance_line_raw();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (!(text_cursor() < text_end())) {
            break;
        }
    }

    if (text_end() <= text_cursor()) {
        return 1;
    }

    advance_line_raw();
    self->line_number = self->line_number + 1;
    self->line_start = self->cursor;

    return parse_block((hwreq_property_set *)self->requirements) != 0;
}

/**
 * Parses one nested block of the hardware-requirements script line by line, starting at the
 * parser's current cursor: "if <condition>" / "endif" toggle which lines are active; "break" ends
 * the block early; "MaxOverallGraphicDetail = N" merges a graphic-detail-level property set into
 * this->flags when N undercuts any existing "OverallGraphicDetail" override; "propertyset =
 * \"name\"" merges an already-registered property set into target; every other active line is
 * parsed as a "flag = value" assignment into target. A line beginning "vendor", "audiovendor" or
 * "Requirements" ends the block immediately (cursor left there, returns true) regardless of
 * unclosed ifs; reaching the end of the file or a "break" keyword ends it too, but only succeeds if
 * every "if" was matched by an "endif" first.
 *
 * @address 0x57af10
 */
uint8_t HwreqParser::parse_block(hwreq_property_set *target)
{
    int32_t if_state;
    int32_t if_stack[17];
    int32_t if_depth;
    uint8_t is_first_line;
    char *cursor;
    char c;
    char *name;
    const char *resolve_result;
    int32_t parsed_number;
    char number_text[40];
    char override_text[16];
    msvc_std_string key;
    hwreq_map_node *node;
    hwreq_property_set *source_set;
    const char *flag_result;
    int i;

    if_state = 0;
    if_depth = 0;
    is_first_line = 1;

    for (;;) {
        if (is_first_line) {
            is_first_line = 0;
        } else {
            advance_line_raw();
            self->line_start = self->cursor;
            self->line_number = self->line_number + 1;
        }

        if (_strnicmp(text_cursor(), "vendor", 6) == 0) {
            c = (text_cursor())[6];
            if (is_delimiter(c)) {
                return 1;
            }
        }
        if (_strnicmp(text_cursor(), "audiovendor", 11) == 0) {
            c = (text_cursor())[11];
            if (is_delimiter(c)) {
                return 1;
            }
        }
        if (_strnicmp(text_cursor(), "Requirements", 12) == 0) {
            c = (text_cursor())[12];
            if (is_delimiter(c)) {
                return 1;
            }
        }

        cursor = text_cursor();
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        self->cursor = (uint32_t)cursor;

        c = *cursor;
        if (c == '\r' || *(uint16_t *)cursor == char_pair('/', '/') || (c >= '0' && c <= '9') ||
            (_strnicmp(cursor, "unknown", 7) == 0 &&
             (c = cursor[7], is_delimiter(c)))) {
        } else if (match_keyword("break")) {
            break;
        } else if (match_keyword("MaxOverallGraphicDetail")) {
            self->cursor = self->cursor + 23;
            cursor = text_cursor();
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }
            self->cursor = (uint32_t)cursor;
            if (*cursor != '=') {
                report_error("Expecting '=', didn't get it");
                return 0;
            }
            do {
                cursor++;
                self->cursor = (uint32_t)cursor;
            } while (*cursor == ' ' || *cursor == '\t');

            parsed_number = parse_number();
            if (parsed_number == -1) {
                report_error("MaxOverallGraphicDetail did not specify a number!");
                return 0;
            }

            override_text[0] = '0';
            override_text[1] = 0;
            for (i = 2; i < 14; i++) {
                override_text[i] = 0;
            }
            sprintf(number_text, "%d", parsed_number);

            if (PropertySet((hwreq_property_set *)self->flags).find_value("OverallGraphicDetail", override_text, 0x10) != 0 &&
                (uint32_t)parsed_number < (uint32_t)atol(override_text)) {
                key.capacity = k_msvc_string_inline_capacity;
                key.size = 0;
                key.buffer.inline_buffer[0] = 0;
                StdString(&key).construct_cstr(number_text);

                node = StdMap(&self->graphic_detail_sets).find(&key);
                if (node == (hwreq_map_node *)self->graphic_detail_sets.head) {
                    report_error("Unrecognized graphic detail");
                    StdString(&key).destroy();
                    return 0;
                }
                source_set = (hwreq_property_set *)node->value;
                PropertySet(source_set).apply_to((hwreq_property_set *)self->flags);
                StdString(&key).destroy();
            }
        } else if (match_keyword("if")) {
            self->cursor = self->cursor + 2;
            resolve_result = resolve_field();

            if_stack[if_depth + 1] = if_state;
            if_depth = if_depth + 1;
            if (if_depth == 0x10) {
                report_error("IF's nested too deep");
                return 0;
            }

            if (resolve_result == (const char *)1) {
                if (if_state != 2) {
                    if_state = 1;
                }
            } else {
                if (resolve_result != 0) {
                    report_error(resolve_result);
                    return 0;
                }
                if_state = 2;
            }
        } else if (match_keyword("endif")) {
            if (if_depth == 0) {
                report_error("Unexpected ENDIF");
                return 0;
            }
            if_state = if_stack[if_depth];
            if_depth = if_depth - 1;
        } else if (if_state == 0 || if_state == 1) {
            if (match_keyword("propertyset")) {
                self->cursor = self->cursor + 11;
                skip_whitespace();
                if (*text_cursor() != '=') {
                    report_error("Missing =");
                    return 0;
                }
                self->cursor = self->cursor + 1;

                name = parse_quoted_string();
                if (name == 0) {
                    return 0;
                }

                key.capacity = k_msvc_string_inline_capacity;
                key.size = 0;
                key.buffer.inline_buffer[0] = 0;
                StdString(&key).construct_cstr(name);

                node = StdMap(&self->property_sets).find(&key);
                if (node == (hwreq_map_node *)self->property_sets.head) {
                    report_error("Unrecognized property set");
                    StdString(&key).destroy();
                    return 0;
                }
                source_set = (hwreq_property_set *)node->value;
                PropertySet(source_set).apply_to(target);
                StdString(&key).destroy();
            } else {
                flag_result = parse_flag_assignment(target);
                if (flag_result != 0) {
                    report_error(flag_result);
                    return 0;
                }
            }
        }

        if (!(text_cursor() < text_end())) {
            break;
        }
    }

    if (if_state == 0 && if_depth == 0) {
        return 1;
    }
    report_error("Bad IF/ENDIF");
    return 0;
}

/**
 * Parses one 'flag = value' assignment line from the requirements script, storing it in the flag
 * table and, for the special OverallGraphicDetail flag, also recording its value for the caller.
 * Returns NULL on success, or a static error message string.
 *
 * @address 0x578cf0
 */
const char *HwreqParser::parse_flag_assignment(hwreq_property_set *property_set)
{
    char name[256];
    char value[256];
    char *name_out = name;
    char *value_out;
    char *src;
    char c;
    char *p2;

    c = *text_cursor();
    while (c != ' ') {
        c = *text_cursor();
        if (c == '=' || c == '\r') break;
        self->cursor++;
        if (c > '@' && c < '[') c += ' ';
        *name_out = c;
        name_out++;
        if (name_out == name + 254) {
            return "Flag too long";
        }
        c = *text_cursor();
    }
    *name_out = '\0';

    while (*text_cursor() == ' ' || *text_cursor() == '\t') {
        self->cursor++;
    }

    src = text_cursor();
    value_out = value;

    if (*src != '\r') {
        if (*src != '=') {
            return "flag = xxx expected";
        }
        do {
            do {
                src++;
                self->cursor = (uint32_t)src;
                c = *src;
            } while (c == ' ');
        } while (c == '\t');

        if (c == '"') {
            c = '"';
            for (;;) {
                src++;
                *value_out = c;
                p2 = value_out + 1;
                self->cursor = (uint32_t)src;
                if (p2 == value + 0xfe) {
                    return "Flag too long";
                }
                c = *src;
                if (c == '"') {
                    p2[0] = '"';
                    value_out += 2;
                    if (value_out != value + 0xfe) {
                        break;
                    }
                    return "Flag too long";
                }
                value_out = p2;
                if (c == '\r') {
                    return "Missing Quote";
                }
            }
        } else {
            while (c != ' ' && (c = *src, c != '\r')) {
                src++;
                self->cursor = (uint32_t)src;
                if (c > '@' && c < '[') c += ' ';
                *value_out = c;
                value_out++;
                if (value_out == value + 0xfe) {
                    return "Flag too long";
                }
                c = *text_cursor();
            }
        }
    }

    *value_out = '\0';

    PropertySet(property_set).upsert(name, value);

    if (_stricmp("OverallGraphicDetail", name) == 0) {
        msvc_std_string value_string;

        value_string.size = 0;
        value_string.buffer.inline_buffer[0] = 0;
        value_string.capacity = k_msvc_string_inline_capacity;
        StdString(&value_string).assign_n(value, (uint32_t)(value_out - value));
        *StdMap(&self->graphic_detail_sets).index_property_set(&value_string) = property_set;
        if (value_string.capacity >= k_string_inline_capacity + 1) {
            free((void *)value_string.buffer.heap_buffer);
        }
    }

    return 0;
}

/**
 * Scans forward from the parser's cursor for "propertyset" directives, parsing each one's "=
 * \"name\" { ... }" form into a freshly allocated property set that is registered in
 * this->property_sets under that name, and keeps scanning past it; stops (leaving the cursor
 * unconsumed, returning true) at the first line starting with "vendor" or "applytoall", or once the
 * end of the file is reached. Returns false on a malformed directive or a failed nested block
 * parse.
 *
 * @address 0x57a3e0
 */
uint8_t HwreqParser::parse_propertyset_directive()
{
    char *cursor;
    char *line;
    char c;
    msvc_std_string name;
    char *quoted;
    uint32_t length;
    hwreq_property_set *set;
    uint8_t result;
    hwreq_property_set **slot;

    for (;;) {
        bool directive_consumed = false;

        if (_strnicmp(text_cursor(), "propertyset", 11) == 0) {
            cursor = text_cursor() + 11;
            c = *cursor;
            if (is_delimiter(c)) {
                while (*cursor == ' ' || *cursor == '\t') {
                    cursor++;
                }
                self->cursor = (uint32_t)cursor;
                if (*cursor != '=') {
                    report_error("Missing =");
                    return 0;
                }
                self->cursor = self->cursor + 1;

                quoted = parse_quoted_string();
                if (quoted == 0) {
                    return 0;
                }

                length = 0;
                while (quoted[length] != 0) {
                    length++;
                }
                name.capacity = k_msvc_string_inline_capacity;
                name.size = 0;
                name.buffer.inline_buffer[0] = 0;
                StdString(&name).assign_n(quoted, length);

                advance_line_raw();
                self->line_start = self->cursor;
                self->line_number = self->line_number + 1;

                set = (hwreq_property_set *)malloc(k_hwreq_property_set_size);
                if (set != 0) {
                    set->flags.first = 0;
                    set->flags.last = 0;
                    set->flags.end = 0;
                    set->owner = (uint32_t)self;
                }

                result = parse_block(set);
                if (result == 0) {
                    if (set != 0) {
                        PropertySet(set).destroy_flags();
                        free(set);
                    }
                    if (name.capacity > k_msvc_string_inline_capacity) {
                        free((void *)name.buffer.heap_buffer);
                    }
                    return 0;
                }

                slot = StdMap(&self->property_sets).index_property_set(&name);
                *slot = set;

                if (name.capacity > k_msvc_string_inline_capacity) {
                    free((void *)name.buffer.heap_buffer);
                }
                name.capacity = k_msvc_string_inline_capacity;
                name.size = 0;
                name.buffer.inline_buffer[0] = 0;

                directive_consumed = true;
            }
        } else if (_strnicmp(text_cursor(), "vendor", 6) == 0) {
            c = (text_cursor())[6];
            if (is_delimiter(c)) {
                break;
            }
        }

        if (!directive_consumed && _strnicmp(text_cursor(), "applytoall", 10) == 0) {
            c = (text_cursor())[10];
            if (is_delimiter(c)) {
                break;
            }
        }

        advance_line_raw();
        line = text_cursor();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (text_end() <= line) {
            return 1;
        }
    }

    return 1;
}

namespace {

/**
 * Parses the body of a matched "vendor" or "audiovendor" directive: records the vendor name, parses
 * lines nested under it directly as flags until the first line that starts a device id, then finds
 * the device entry matching device_id (or "unknown") and parses its block into the parser's flags.
 * A line starting with the vendor keyword (or, for audio, "vendor") ends the scan without a match.
 */
uint8_t parse_matched_vendor(HwreqParser &parser, const char *keyword, uint32_t keyword_length, bool also_stop_at_vendor,
                             int32_t device_id_actual, msvc_std_string *vendor_name, msvc_std_string *device_name)
{
    hwreq_parser *self = parser.self;
    char *cursor;
    char c;
    char *name;
    uint32_t length;
    int32_t device_id;
    uint8_t ok;

    name = parser.parse_quoted_string();
    if (name == 0) {
        return 0;
    }
    length = 0;
    while (name[length] != 0) {
        length++;
    }
    StdString(vendor_name).assign_n(name, length);

    for (;;) {
        parser.advance_line_raw();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;

        if (!(_strnicmp(parser.text_cursor(), keyword, keyword_length) == 0 &&
              (c = parser.text_cursor()[keyword_length], HwreqParser::is_delimiter(c)))) {
            cursor = parser.text_cursor();
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }
            self->cursor = (uint32_t)cursor;
            c = *cursor;
            if (c >= '0' && c <= '9') {
                break;
            }
            if (c != '\r' && *(uint16_t *)cursor != char_pair('/', '/')) {
                ok = parser.parse_block((hwreq_property_set *)self->flags);
                if (ok == 0) {
                    return 0;
                }
            }
        }
        if (!(parser.text_cursor() < parser.text_end())) {
            break;
        }
    }

    for (;;) {
        bool matched = false;

        if (_strnicmp(parser.text_cursor(), keyword, keyword_length) == 0) {
            c = parser.text_cursor()[keyword_length];
            if (HwreqParser::is_delimiter(c)) {
                return 1;
            }
        }
        if (also_stop_at_vendor && _strnicmp(parser.text_cursor(), "vendor", 6) == 0) {
            c = parser.text_cursor()[6];
            if (HwreqParser::is_delimiter(c)) {
                return 1;
            }
        }
        if (_strnicmp(parser.text_cursor(), "unknown", 7) == 0) {
            c = parser.text_cursor()[7];
            if (HwreqParser::is_delimiter(c)) {
                device_id = device_id_actual;
                self->cursor = self->cursor + 7;
                parser.skip_whitespace();
                matched = true;
            }
        }
        if (!matched) {
            cursor = parser.text_cursor();
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }
            self->cursor = (uint32_t)cursor;
            if (*cursor >= '0' && *cursor <= '9') {
                device_id = parser.parse_number();
                matched = device_id == device_id_actual;
            }
        }

        if (!matched) {
            parser.advance_line_raw();
            self->line_start = self->cursor;
            self->line_number = self->line_number + 1;
            if (parser.text_end() <= parser.text_cursor()) {
                return 1;
            }
            continue;
        }

        if (device_id == -1) {
            return 1;
        }
        if (*parser.text_cursor() == '=') {
            self->cursor = self->cursor + 1;
            name = parser.parse_quoted_string();
            if (name != 0) {
                StdString(device_name).assign_cstr(name);
                parser.skip_line();
                ok = parser.parse_block((hwreq_property_set *)self->flags);
                return ok != 0;
            }
        } else {
            parser.report_error("xxx = Device Name expected");
        }
        return 0;
    }
}

/**
 * Scans lines for a "<keyword> = id" directive whose id matches vendor_id_actual (or the keyword
 * "unknown"), then parses the matched directive. A non-matching directive line is skipped for its
 * single line only; "applytoall" or the end of the file returns true.
 */
uint8_t parse_vendor_directive(HwreqParser &parser, const char *keyword, uint32_t keyword_length, bool also_stop_at_vendor,
                               int32_t vendor_id_actual, int32_t device_id_actual, msvc_std_string *vendor_name,
                               msvc_std_string *device_name)
{
    hwreq_parser *self = parser.self;
    char *cursor;
    char c;
    int32_t vendor_id;

    for (;;) {
        if (_strnicmp(parser.text_cursor(), keyword, keyword_length) == 0) {
            c = parser.text_cursor()[keyword_length];
            if (HwreqParser::is_delimiter(c)) {
                cursor = parser.text_cursor() + keyword_length;
                while (*cursor == ' ' || *cursor == '\t') {
                    cursor++;
                }
                self->cursor = (uint32_t)cursor;

                if (*cursor == '=') {
                    bool matches = true;

                    cursor++;
                    self->cursor = (uint32_t)cursor;

                    if (_strnicmp(cursor, "unknown", 7) == 0 && (c = cursor[7], HwreqParser::is_delimiter(c))) {
                        vendor_id = vendor_id_actual;
                    } else {
                        vendor_id = parser.parse_number();
                        matches = vendor_id == vendor_id_actual;
                    }
                    if (matches) {
                        if (vendor_id == -1) {
                            return 1;
                        }
                        return parse_matched_vendor(parser, keyword, keyword_length, also_stop_at_vendor, device_id_actual,
                                                    vendor_name, device_name);
                    }
                }
            }
        } else if (_strnicmp(parser.text_cursor(), "applytoall", 10) == 0) {
            c = parser.text_cursor()[10];
            if (HwreqParser::is_delimiter(c)) {
                return 1;
            }
        }

        parser.advance_line_raw();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (parser.text_end() <= parser.text_cursor()) {
            return 1;
        }
    }
}

}  // namespace

/**
 * Scans forward from the parser's cursor for a "vendor[=id] = \"name\" { ... }" directive whose id
 * matches the detected graphics vendor (or the literal keyword "unknown", which always matches),
 * records the vendor name, and then parses the directive's nested lines: a "device[=id] = \"name\""
 * or "unknown = \"name\"" line whose id matches the detected device records the device name and
 * parses its own trailing { ... } block into this->flags; any other nested line is parsed directly
 * as a flags block. A non-matching "vendor" line at the top level is skipped for its single line
 * only (so a later vendor clause in the same section can still be tried); reaching "applytoall", or
 * the end of the file, without ever entering a matching vendor block returns true with the cursor
 * left at that point.
 *
 * @address 0x57a680
 */
uint8_t HwreqParser::parse_vendor_block()
{
    return parse_vendor_directive(*this, "vendor", 6, false, (int32_t)self->adapter.vendor_id,
                                  (int32_t)self->adapter.device_id, &self->graphics_vendor_name,
                                  &self->graphics_device_name);
}

/**
 * Scans forward from the parser's cursor for an "audiovendor[=id] = \"name\" { ... }" directive
 * whose id matches the detected sound device vendor (or the literal keyword "unknown", which always
 * matches), records the vendor name, and then parses the directive's nested lines: a "device[=id] =
 * \"name\"" or "unknown = \"name\"" line whose id matches the detected device records the device
 * name and parses its own trailing { ... } block into this->flags; any other nested line is parsed
 * directly as a flags block. A non-matching "audiovendor" line at the top level is skipped for its
 * single line only; reaching "applytoall", or the end of the file, without ever entering a matching
 * audiovendor block returns true with the cursor left there.
 *
 * @address 0x57aa40
 */
uint8_t HwreqParser::parse_audiovendor_block()
{
    return parse_vendor_directive(*this, "audiovendor", 11, true, (int32_t)self->sound_device.vendor_id,
                                  (int32_t)self->sound_device.device_id, &self->sound_vendor_name,
                                  &self->sound_device_name);
}

/**
 * Scans forward from the parser's current cursor to the end of the file, parsing every "applytoall
 * { ... }" block it finds into this->flags. Returns false only if a matched applytoall block fails
 * to parse; returns true once the cursor reaches the end of the file.
 *
 * @address 0x57a320
 */
uint8_t HwreqParser::scan_for_applytoall()
{
    char *line;
    char c;
    uint8_t result;

    for (;;) {
        if (_strnicmp(text_cursor(), "applytoall", 10) == 0) {
            c = (text_cursor())[10];
            if (is_delimiter(c)) {
                advance_line_raw();
                self->line_start = self->cursor;
                self->line_number = self->line_number + 1;

                result = parse_block((hwreq_property_set *)self->flags);
                if (result == 0) {
                    return result;
                }
            }
        }

        advance_line_raw();
        line = text_cursor();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (text_end() <= line) {
            return 1;
        }
    }
}

/**
 * Scans forward from the parser's current cursor, parsing every "applytoall { ... }" block it finds
 * into this->flags, until either a line beginning with "vendor" (followed by a delimiter) is
 * reached -- left unconsumed, for the caller to handle -- or the end of the file is reached.
 * Returns false only if a matched applytoall block fails to parse.
 *
 * @address 0x57a220
 */
uint8_t HwreqParser::scan_for_applytoall_or_vendor()
{
    char *line;
    char c;
    uint8_t result;

    for (;;) {
        if (_strnicmp(text_cursor(), "applytoall", 10) == 0) {
            c = (text_cursor())[10];
            if (is_delimiter(c)) {
                advance_line_raw();
                self->line_start = self->cursor;
                self->line_number = self->line_number + 1;

                result = parse_block((hwreq_property_set *)self->flags);
                if (result == 0) {
                    return result;
                }
            }
        } else if (_strnicmp(text_cursor(), "vendor", 6) == 0) {
            c = (text_cursor())[6];
            if (is_delimiter(c)) {
                break;
            }
        }

        advance_line_raw();
        line = text_cursor();
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if (text_end() <= line) {
            return 1;
        }
    }

    return 1;
}

}

#define _NO_CRT_STDIO_INLINE

#include "halo/shell/hwreq.hpp"

extern "C" {
extern char hwreq_quoted_string[k_hwreq_quoted_string_length];
}

namespace halo::shell {

/**
 * Tests whether the parser's cursor is currently positioned at the given keyword, followed by a
 * valid delimiter character, without consuming the token.
 *
 * @address 0x578fa0
 */
uint32_t HwreqParser::match_keyword(const char *keyword)
{
    uint32_t length;
    const char *p;
    char delimiter;

    p = keyword;
    while (*p != '\0') p++;
    length = (uint32_t)(p - keyword);

    if (_strnicmp((char *)self->cursor, keyword, length) == 0) {
        delimiter = ((char *)self->cursor)[length];
        if (delimiter == '>' || delimiter == '<' || delimiter == '!' || delimiter == '=' ||
            delimiter == ' ' || delimiter == '\r' || delimiter == '\t') {
            return 1;
        }
    }
    return 0;
}

/**
 * Parses a single hexadecimal digit at the cursor, advancing past it and returning its value, or -1
 * if the character is not a hex digit.
 *
 * @address 0x578ad0
 */
int32_t HwreqParser::parse_hex_digit()
{
    char *cursor = (char *)self->cursor;
    char c = *cursor;

    if (c > '/' && c < ':') {
        self->cursor = (uint32_t)(cursor + 1);
        return c - 0x30;
    }
    if (c > '`' && c < 'g') {
        self->cursor = (uint32_t)(cursor + 1);
        return c - 0x57;
    }
    if (c > '@' && c < 'G') {
        self->cursor = (uint32_t)(cursor + 1);
        return c - 0x37;
    }
    return -1;
}

/**
 * Parses a 4-hex-digit token (e.g. a vendor or device ID) into a 16-bit value, returning -1 on any
 * invalid digit.
 *
 * @address 0x578ef0
 */
uint32_t HwreqParser::parse_hex_id()
{
    char c;
    int32_t d0, d1, d2, d3;

    c = *(char *)self->cursor;
    if (c >= '0' && c <= '9') {
        d0 = c - 0x30;
    } else if (c >= 'a' && c <= 'f') {
        d0 = c - 0x57;
    } else if (c >= 'A' && c <= 'F') {
        d0 = c - 0x37;
    } else {
        return 0xffffffff;
    }
    self->cursor++;
    if (d0 == -1) {
        return 0xffffffff;
    }

    d1 = parse_hex_digit();
    if (d1 == -1) return 0xffffffff;
    d2 = parse_hex_digit();
    if (d2 == -1) return 0xffffffff;
    d3 = parse_hex_digit();
    if (d3 == -1) return 0xffffffff;

    return (uint32_t)(d0 << 0xc | d1 << 8 | d2 << 4 | d3);
}

/**
 * Parses a four digit hex token and returns it byte swapped, used for the trailing GUID groups.
 * Returns -1 on a bad digit.
 *
 * @address 0x578f80
 */
int32_t HwreqParser::parse_hex_id_byteswap()
{
    uint32_t value = parse_hex_id();
    if (value == 0xffffffff) {
        return -1;
    }
    return (int32_t)((value & 0xff) * 0x100 + (value >> 8 & 0xff));
}

/**
 * Parses a decimal or 0x prefixed hexadecimal integer at the cursor and skips trailing blanks.
 * Reports 'Number expected' or 'Number too large' and returns -1 on failure.
 *
 * @address 0x578b20
 */
int32_t HwreqParser::parse_number()
{
    char *cursor;
    char c;
    int32_t digit;
    int32_t next_digit;
    int32_t value;
    uint32_t digit_count;

    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
        self->cursor++;
    }

    cursor = (char *)self->cursor;

    if (*(uint16_t *)cursor == 0x7830) {
        self->cursor = (uint32_t)(cursor + 2);
        digit = parse_hex_digit();
        if (digit != -1) {
            value = 0;
            digit_count = 0;
            for (;;) {
                if (digit_count > 7) {
                    report_error("Number too large");
                    return -1;
                }
                c = *(char *)self->cursor;
                value = value * 0x10 + digit;
                digit_count++;
                next_digit = -1;
                if (c >= '0' && c <= '9') {
                    next_digit = c - 0x30;
                    self->cursor++;
                } else if (c > '`' && c < 'g') {
                    next_digit = c - 0x57;
                    self->cursor++;
                } else if (c > '@' && c < 'G') {
                    next_digit = c - 0x37;
                    self->cursor++;
                }
                if (next_digit == -1) {
                    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
                        self->cursor++;
                    }
                    return value;
                }
                digit = next_digit;
            }
        }
    } else if (*cursor > '/' && *cursor < ':' && (digit = parse_hex_digit(), digit != -1)) {
        c = *(char *)self->cursor;
        while (c > '/' && c < ':') {
            next_digit = parse_hex_digit();
            if (next_digit == -1) break;
            digit = next_digit + digit * 10;
            c = *(char *)self->cursor;
        }
        skip_whitespace();
        return digit;
    }

    report_error("Number expected");
    return -1;
}

/**
 * Parses a double-quoted string literal token into a shared static buffer, reporting parser errors
 * for a missing quote or an overlong string.
 *
 * @address 0x578c60
 */
char *HwreqParser::parse_quoted_string()
{
    char *cursor;
    char c;
    char *out;
    char *buffer_end;

    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
        self->cursor++;
    }

    c = *(char *)self->cursor;
    cursor = (char *)self->cursor + 1;
    self->cursor = (uint32_t)cursor;
    if (c != '"') {
        report_error("Expecting ");
        return 0;
    }

    c = *cursor;
    out = hwreq_quoted_string;
    buffer_end = hwreq_quoted_string + k_hwreq_quoted_string_length - 1;

    for (;;) {
        if (c == '"') {
            *out = '\0';
            cursor = (char *)self->cursor + 1;
            self->cursor = (uint32_t)cursor;
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
                self->cursor = (uint32_t)cursor;
            }
            return hwreq_quoted_string;
        }
        *out = c;
        out++;
        cursor = (char *)self->cursor + 1;
        self->cursor = (uint32_t)cursor;
        if (out > buffer_end) break;
        c = *cursor;
    }

    report_error("String too long");
    return 0;
}

/**
 * Tokenizer helper that advances the parser's cursor to the start of the next line, updating the
 * cached line-start pointer and line-number counter.
 *
 * @address 0x5789d0
 */
void HwreqParser::skip_line()
{
    char *p;
    char *cursor = (char *)self->cursor;
    char *end = (char *)self->end;

    do {
        p = cursor;
        cursor = p + 1;
        if (*p == '\r') break;
    } while (cursor < end);

    if (cursor < end && *cursor == '\n') {
        cursor = p + 2;
    }

    self->cursor = (uint32_t)cursor;
    self->line_start = (uint32_t)cursor;
    self->line_number = self->line_number + 1;
}

/**
 * Tokenizer helper that skips spaces and tabs at the parser's current cursor position.
 *
 * @address 0x578a00
 */
void HwreqParser::skip_whitespace()
{
    char *cursor = (char *)self->cursor;
    while (*cursor == ' ' || *cursor == '\t') {
        cursor++;
    }
    self->cursor = (uint32_t)cursor;
}

/**
 * Records a parser error message together with the current line number and a snippet of the
 * offending line, for later reporting; ignored if an error was already latched on this line.
 *
 * @address 0x578a20
 */
void HwreqParser::report_error(const char *message)
{
    char context[k_hwreq_error_context_length + 4];
    char formatted[0x100];
    const char *line;
    int32_t n;
    int32_t length;

    if (self->error_reported != 0) {
        return;
    }

    line = (const char *)self->line_start;
    n = 0;
    if (line[0] != '\r') {
        do {
            if (n == k_hwreq_error_context_length) {
                break;
            }
            context[n] = line[n];
            n++;
        } while (line[n] != '\r');
        if (n == k_hwreq_error_context_length) {
            context[n] = '.';
            context[n + 1] = '.';
            context[n + 2] = '.';
            n += 3;
        }
    }
    context[n] = '\0';

    _snprintf(formatted, 0x100, "%s on line %d - '%s'", message, self->line_number, context);

    length = 0;
    while (formatted[length] != '\0') length++;
    StdString(&self->error_message).assign_n(formatted, (uint32_t)length);
    self->error_reported = 1;
}

}

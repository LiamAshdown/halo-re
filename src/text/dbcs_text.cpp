/**
 * @file src/text/dbcs_text.cpp
 * Character-boundary logic for the narrow text encodings (single byte, DBCS code pages and |x markup escapes).
 * The original author notes and decompiles are in docs/original/text/.
 */

#include "halo/text/text.hpp"
#include "halo/text/api.hpp"


namespace halo::text {

uint8_t dbcs_text::char_is_double_byte(uint8_t *string)
{
    uint8_t lead, trail;
    uint8_t result;
    int trail_below_threshold;
    int trail_at_boundary;

    lead = string[0];
    if (lead == 0) {
        return 0;
    }
    trail = string[1];
    result = 0;

    if (lead == 0x7c && trail != 0 && strchr((const char *)globals().text_markup_codes, trail) != (const char *)0) {
        goto mark_double_byte;
    }

    switch (globals().text_encoding_state) {
    case _text_encoding_shift_jis:
        if (lead < 0x81 || 0x9f < lead) {
            if (lead < 0xe0) return 0;
            if (lead == 0xff) return 0;
        }
        if (trail < 0x40) return 0;
        if (0xfc < trail) return 0;
        if (trail == 0x7f) return 0;
        return 1;

    case _text_encoding_euc:
        if (lead < 0xa1) return 0;
        if (lead == 0xff) return 0;
        trail_below_threshold = trail < 0xa1;
        break;

    case _text_encoding_big5:
        if (lead < 0x81) return 0;
        if (lead == 0xff) return 0;
        if (0x3f < trail && trail < 0x7f) goto mark_double_byte;
        trail_below_threshold = trail < 0xa1;
        break;

    case _text_encoding_korean_uhc:
        if (lead < 0x81) return 0;
        if (lead == 0xff) return 0;
        if (0x40 < trail && trail < 0x5b) goto mark_double_byte;
        if (0x60 < trail) {
            trail_below_threshold = trail < 0x7a;
            trail_at_boundary = trail == 0x7a;
            goto check_trail_boundary;
        }
        goto trail_below_0x81;

    case _text_encoding_korean_johab:
        if ((lead < 0x84 || 0xd3 < lead) && (lead < 0xd8 || 0xde < lead)) {
            if (lead < 0xe0) return 0;
            if (0xf9 < lead) return 0;
        }
        if (0x40 < trail) {
            trail_below_threshold = trail < 0x7e;
            trail_at_boundary = trail == 0x7e;
check_trail_boundary:
            if (trail_below_threshold || trail_at_boundary) goto mark_double_byte;
        }
trail_below_0x81:
        trail_below_threshold = trail < 0x81;
        break;

    default:
        return result;
    }

    if (!trail_below_threshold && trail != 0xff) {
mark_double_byte:
        result = 1;
    }
    return result;
}

uint16_t dbcs_text::get_next_character(uint8_t *string, int16_t *cursor)
{
    uint8_t *here;

    here = string + *cursor;
    if (dbcs_text::char_is_double_byte(here)) {
        *cursor += 2;
        return (uint16_t)((here[0] << 8) | here[1]);
    }
    *cursor += 1;
    return (uint16_t)here[0];
}

uint8_t dbcs_text::find_character(int16_t target_character, uint8_t *string)
{
    int16_t position;

    position = 0;
    for (;;) {
        uint8_t *here;
        uint16_t character;

        here = string + position;
        if (dbcs_text::char_is_double_byte(here)) {
            character = (uint16_t)((here[0] << 8) | here[1]);
            position += 2;
        } else {
            character = (uint16_t)here[0];
            position += 1;
        }
        if (character == 0) {
            return 0;
        }
        if (character == (uint16_t)target_character) {
            return 1;
        }
    }
}

uint16_t dbcs_text::find_character_boundary(uint8_t *string, int16_t *length_inout)
{
    int16_t position;
    int16_t boundary;
    uint16_t character;

    position = 0;
    do {
        boundary = position;
        if (dbcs_text::char_is_double_byte(string + boundary)) {
            position = boundary + 2;
            character = (uint16_t)((string[boundary] << 8) | string[boundary + 1]);
        } else {
            character = (uint16_t)string[boundary];
            position = boundary + 1;
        }
    } while (position < *length_inout);

    *length_inout = boundary;
    return character;
}

void dbcs_text::clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout)
{
    int16_t position;

    position = 0;
    if (0 < *length_inout) {
        do {
            if (dbcs_text::char_is_double_byte(string + position)) {
                position += 2;
            } else {
                position += 1;
            }
        } while (position < *length_inout);
    }
    *length_inout = position;
}

}  // namespace halo::text

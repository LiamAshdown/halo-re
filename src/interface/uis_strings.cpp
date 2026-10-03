/**
 * Wide-string replacement helpers used to expand placeholders in UI text.
 */

#include "crt.h"
#include "halo/text/api.hpp"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

#include "halo/interface/uis_strings.hpp"
#include "halo/memory/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

#define WCTYPE_SPACE 0x0008

extern "C" {
extern long lrintf(float x);
}
static auto &empty_string = halo::link::ref<uint16_t []>(halo::game::vars().empty_string);
static auto &ui_player_number_text = halo::link::ref<uint16_t [2]>(halo::ui::vars().ui_player_number_text);
static auto &ui_product_id_text = halo::link::ref<uint16_t []>(halo::ui::vars().ui_product_id_text);
static auto &ui_format_narrow_string = halo::link::ref<uint16_t []>(halo::ui::vars().ui_format_narrow_string);
static auto &ui_version_text = halo::link::ref<uint16_t []>(halo::ui::vars().ui_version_text);
static auto &ui_version_string = halo::link::ref<char []>(halo::ui::vars().ui_version_string);
static auto &ui_replace_function_table = halo::link::ref<void *[4]>(halo::ui::vars().ui_replace_function_table);
static auto &ui_invalid_replacement_text = halo::link::ref<uint16_t []>(halo::ui::vars().ui_invalid_replacement_text);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);

namespace halo::ui {

/**
 * Original UI routine; see docs/original/interface/ui_real_to_int_truncate.c.txt for the recovery notes.
 *
 * @address 0x4ab590
 */
int32_t UiStrings::real_to_int_truncate(float value)
{
    int32_t rounded = (int32_t)lrintf(value);
    float remainder = value - (float)rounded;
    uint32_t value_bits;
    uint32_t remainder_bits;

    memcpy(&value_bits, &value, sizeof(value_bits));
    memcpy(&remainder_bits, &remainder, sizeof(remainder_bits));
    if ((int32_t)value_bits >= 0) {
        if (remainder_bits > 0x80000000u) {
            rounded--;
        }
    } else if ((int32_t)remainder_bits > 0) {
        rounded++;
    }
    return rounded;
}

/**
 * Original UI routine; see docs/original/interface/ui_replace_empty.c.txt for the recovery notes.
 *
 * @address 0x4a8750
 */
void * UiStrings::replace_empty(widget_instance *widget)
{
    return empty_string;
}

/**
 * Original UI routine; see docs/original/interface/ui_replace_player_number.c.txt for the recovery notes.
 *
 * @address 0x4a8840
 */
void * UiStrings::replace_player_number(widget_instance *widget)
{
    uint16_t digit;

    switch (widget->controller_index) {
    case -1: case 0: digit = '1'; break;
    case 1: digit = '2'; break;
    case 2: digit = '3'; break;
    case 3: digit = '4'; break;
    default: digit = '?'; break;
    }
    ui_player_number_text[0] = digit;
    ui_player_number_text[1] = 0;
    return ui_player_number_text;
}

/**
 * Original UI routine; see docs/original/interface/ui_replace_product_id.c.txt for the recovery notes.
 *
 * @address 0x4a8810
 */
void * UiStrings::replace_product_id(widget_instance *widget)
{
    if (ui_product_id_text[0] == 0) {
        halo::text::string_format_wide_va(ui_product_id_text, ui_format_narrow_string, halo::interface::registry_get_product_id());
    }
    return ui_product_id_text;
}

/**
 * Original UI routine; see docs/original/interface/ui_replace_version.c.txt for the recovery notes.
 *
 * @address 0x4a8760
 */
void * UiStrings::replace_version(widget_instance *widget)
{
    if (ui_version_text[0] == 0) {
        halo::text::string_format_wide_va(ui_version_text, ui_format_narrow_string, ui_version_string);
    }
    return ui_version_text;
}

/**
 * Original UI routine; see docs/original/interface/ui_search_replace_function_call.c.txt for the recovery notes.
 *
 * Register convention: index -> AX, widget -> ECX
 *
 * @address 0x4a8730
 */
const uint16_t * UiStrings::search_replace_function_call(int16_t index, widget_instance *widget)
{
    if (index >= 0 && (uint16_t)index < 4) {
        return (const uint16_t *)((ui_search_replace_function)ui_replace_function_table[index])(widget);
    }
    return ui_invalid_replacement_text;
}

/**
 * Replaces every occurrence of `search` inside `*buffer` with `replacement`, growing `*buffer` through the
 * widget heap if needed. Returns the number of replacements, or -1 if growth failed.
 *
 * @address 0x49be10
 */
int32_t UiStrings::string_replace_all(wchar_t *search, uint16_t *replacement, wchar_t **buffer)
{
    int32_t search_length;
    uint32_t replacement_length;
    int32_t total_length;
    int32_t count = 0;
    wchar_t *original;
    wchar_t *base;
    wchar_t *match;

    if (buffer == (wchar_t **)0 || (original = *buffer) == (wchar_t *)0) {
        return 0;
    }

    search_length = (int32_t)wcslen((const wchar_t *)((const uint16_t *)search));
    replacement_length = wcslen((const wchar_t *)replacement);
    total_length = (int32_t)wcslen((const wchar_t *)((const uint16_t *)original)) + 1;

    if (search_length < (int32_t)replacement_length) {
        match = wcsstr(original, search);
        if (match == (wchar_t *)0) {
            return 0;
        }
        do {
            count = count + 1;
            match = wcsstr(match + search_length, search);
        } while (match != (wchar_t *)0);

        base = (wchar_t *)halo::memory::heap_reallocate(original,
                                          ((replacement_length - search_length) * count + total_length) * 2,
                                          widget_memory_pool);
        if (base == (wchar_t *)0) {
            return -1;
        }
        match = wcsstr(base, search);
        if (match != (wchar_t *)0) {
            do {
                uint16_t *src;
                wchar_t *dst;
                uint32_t words;
                uint32_t tail_bytes;

                tail_bytes = (uint32_t)((total_length - (int32_t)((match - base))) - search_length) * 2;
                memmove(match + replacement_length, match + search_length, tail_bytes);

                src = replacement;
                dst = match;
                for (words = replacement_length >> 1; words != 0; words = words - 1) {
                    *(uint32_t *)dst = *(uint32_t *)src;
                    src += 2;
                    dst += 2;
                }
                if ((replacement_length & 1) != 0) {
                    *dst = (wchar_t)*src;
                }

                total_length = total_length + (replacement_length - search_length);
                match = wcsstr(base, search);
            } while (match != (wchar_t *)0);
        }
        *buffer = base;
    } else {
        match = wcsstr(original, search);
        if (match == (wchar_t *)0) {
            return 0;
        }
        do {
            uint16_t *src = replacement;
            wchar_t *dst = match;
            uint32_t words;

            count = count + 1;
            for (words = replacement_length >> 1; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src += 2;
                dst += 2;
            }
            if ((replacement_length & 1) != 0) {
                *dst = (wchar_t)*src;
            }
            if (search_length > (int32_t)replacement_length) {
                uint32_t tail_bytes = (uint32_t)((total_length - (int32_t)((match - original))) -
                                                  (int32_t)replacement_length) * 2;
                memmove(match + replacement_length, match + search_length, tail_bytes);
                total_length = total_length - (search_length - replacement_length);
            }
            match = wcsstr(original, search);
        } while (match != (wchar_t *)0);
        return count;
    }
    return count;
}

/**
 * Returns true as soon as a non-whitespace wide character is found, or false if the string is all whitespace (or
 * empty).
 *
 * Register convention: EAX -> text
 *
 * @address 0x4a8b10
 */
uint8_t UiStrings::wide_string_has_non_whitespace(const uint16_t *text)
{
    uint16_t ch = *text;
    while (ch != 0) {
        if (!iswctype(ch, WCTYPE_SPACE)) {
            return 1;
        }
        text = text + 1;
        ch = *text;
    }
    return 0;
}

}

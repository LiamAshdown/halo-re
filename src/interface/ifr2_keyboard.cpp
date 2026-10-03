#include "halo/interface/ifr2_keyboard.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "crt.h"
#include <string.h>
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/text/text.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

#ifdef interface
#undef interface
#endif

static auto &virtual_keyboard = halo::link::ref<virtual_keyboard_globals>(halo::ui::vars().virtual_keyboard);
static auto &virtual_keyboard_blacklist_charset = halo::link::ref<uint8_t *>(halo::ui::vars().virtual_keyboard_blacklist_charset);
static auto &controls_input_capture_flags = halo::link::ref<uint8_t>(halo::ui::vars().controls_input_capture_flags);
static auto &keyboard_device = halo::link::ref<void **>(halo::ui::vars().keyboard_device);
static auto &key_frames = halo::link::ref<uint8_t [0x6d]>(halo::ui::vars().key_frames);
static auto &key_release_pending = halo::link::ref<uint8_t [0x6d]>(halo::ui::vars().key_release_pending);
static auto &hud_text_draw_font_tag_id = halo::link::ref<int32_t>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);
extern "C" {
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data, int16_t *clip_rect, uint32_t vertex_color);
}
static auto &key_event_read_index = halo::link::ref<int16_t>(halo::ui::vars().key_event_read_index);
static auto &key_event_count = halo::link::ref<int16_t>(halo::ui::vars().key_event_count);
static auto &key_events = halo::link::ref<ui_key_event []>(halo::ui::vars().key_events);
static auto &fortune_easter_egg_text = halo::link::ref<uint16_t []>(halo::ui::vars().fortune_easter_egg_text);
static auto &missing_string_text = halo::link::ref<uint16_t []>(halo::ui::vars().missing_string_text);

#define WCTYPE_SPACE 0x0008

namespace halo::interface {

/**
 * memset of the whole destination (maximum_length bytes, zero-extended) and caret to its start.
 */
void VirtualKeyboard::vk_clear_text(void)
{
    memset(virtual_keyboard.destination, 0, (uint16_t)virtual_keyboard.maximum_length);
    virtual_keyboard.destination_end = virtual_keyboard.destination;
}

/**
 * Trims trailing whitespace. Returns 0 when nothing but whitespace was left.
 */
uint8_t VirtualKeyboard::vk_trim_trailing_whitespace(void)
{
    int32_t i = wcslen((const wchar_t *)virtual_keyboard.destination) - 1;
    while (i >= 0) {
        if (iswctype(virtual_keyboard.destination[i], WCTYPE_SPACE) == 0) {
            return 1;
        }
        virtual_keyboard.destination[i] = 0;
        i--;
    }
    return 0;
}

void VirtualKeyboard::virtual_keyboard_set_text_state(int16_t column)
{
    hud_text_draw_font_tag_id = virtual_keyboard.large_ui_tag;
    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_r = 0.9f;
    hud_text_draw_color_g = 0.9f;
    hud_text_draw_color_b = 0.9f;
    hud_text_draw_color_or_flags = halo::k_word_none;
    halo::text::globals().hud_text_draw_column = column;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;
}

/**
 * Deletes the wide character immediately before the caret in the virtual keyboard's edit buffer (shifting the
 * remaining tail left by 2 bytes and shrinking destination_end), then always plays the UI sound effect.
 *
 * @address 0x4a96f0
 */
void VirtualKeyboard::backspace()
{
    uint8_t *destination = (uint8_t *)virtual_keyboard.destination;
    uint8_t *destination_end = (uint8_t *)virtual_keyboard.destination_end;

    if (destination < destination_end) {
        int32_t size = (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                        (int32_t)destination_end + (int32_t)destination;
        if (size >= 0) {
            memmove(destination_end - 2, destination_end, size);
            *(int16_t *)(destination + ((virtual_keyboard.maximum_length & ~1) - 2)) = 0;
            virtual_keyboard.destination_end = (uint16_t *)(destination_end - 2);
        }
    }
    halo::interface::widget_play_sound_effect(1);
}

/**
 * blam-cc: validation_mode -> EAX, character -> CL Filters a typed character against the current
 * virtual-keyboard field's allowed character set: field kind 5 is digits only; field kind 4 allows '.', '-'
 * and ':' plus alphanumerics (hostname/IP-like); every other kind (including 3) falls through to a general
 * blacklist membership test.
 *
 * @address 0x4a8b80
 */
uint8_t VirtualKeyboard::character_is_legal(int32_t validation_mode, uint8_t character)
{
    if (validation_mode != 3) {
        if (validation_mode != 4) {
            if (validation_mode != 5) {
                const char *blocked = strchr((const char *)virtual_keyboard_blacklist_charset, character);
                return (uint8_t)(1 - (blocked != 0));
            }
            return (uint8_t)isdigit(character);
        }
        if (character != '.' && character != '-' && character != ':') {
            if (!isalnum(character)) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Commits the virtual keyboard's edited text back to the caller's buffer, clears the keyboard's state and
 * plays the UI sound effect, then resets the DirectInput keyboard device the same way virtual_keyboard_open
 * does.
 *
 * @address 0x4a9250
 */
uint8_t VirtualKeyboard::close()
{
    virtual_keyboard.active = 0;
    if (virtual_keyboard.destination != 0) {
        int32_t wide_chars = (uint16_t)virtual_keyboard.maximum_length >> 1;
        wcsncpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text, wide_chars);
        *(int16_t *)((uint8_t *)virtual_keyboard.destination +
                      ((virtual_keyboard.maximum_length & ~1) - 2)) = 0;
    }
    virtual_keyboard.destination = 0;
    virtual_keyboard.text[0] = 0;
    virtual_keyboard.committed = 0;
    halo::interface::widget_play_sound_effect(3);

    controls_input_capture_flags &= 0xfb;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = *(void ***)keyboard_device;
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
    return 1;
}

/**
 * @address 0x4a9300
 */
void VirtualKeyboard::draw_text(Rectangle2D *bounds)
{
    const uint8_t *font_data = halo::interface::tag_data<const uint8_t>(virtual_keyboard.small_ui_tag);

    hud_text_draw_font_tag_id = virtual_keyboard.small_ui_tag;
    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_b = 0.9f;
    hud_text_draw_color_r = 0.9f;
    hud_text_draw_color_g = 0.9f;
    hud_text_draw_color_or_flags = halo::k_word_none;
    halo::text::globals().hud_text_draw_column = 2;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;

    if (virtual_keyboard.opened == 1) {
        BitmapData *white = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(virtual_keyboard.white_bitmap, 0, 0);

        if (white != 0) {
            Rectangle2D cursor;
            Rectangle2D highlight;

            halo::text::text_context::measure_string_extents(bounds, &cursor, &highlight, virtual_keyboard.destination);
            highlight.left -= 2;
            highlight.right += 2;
            halo::interface::ui_draw_screen_quad((int16_t *)bounds, (int16_t *)&highlight, (int32_t)white, 0, 0x7f7f7f7f);
        }
    }

    halo::rasterizer::chimera__draw_16_bit_text(bounds, (int32_t *)bounds, 0, 0, (const int16_t *)virtual_keyboard.destination);

    if (virtual_keyboard.opened == 0 && virtual_keyboard.white_bitmap != (datum_index)-1 &&
        ((halo::cseries::time_query_performance_counter_ms() / 1000) & 1) != 0) {
        int16_t height = (int16_t)(*(const int16_t *)(font_data + 6) + *(const int16_t *)(font_data + 4));
        int16_t advance_before_caret = 0;
        int16_t total_advance = 0;
        const uint16_t *cursor = virtual_keyboard.destination;
        BitmapData *white = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(virtual_keyboard.white_bitmap, 0, 0);

        if (white != 0) {
            Rectangle2D caret;
            uint16_t character = *cursor;

            while (character != 0) {
                const int16_t *metrics = reinterpret_cast<const int16_t *>(halo::text::text_context::get_character_metrics(character, reinterpret_cast<Font *>(const_cast<uint8_t *>(font_data))));

                if (metrics == 0) {
                    break;
                }
                if (cursor < virtual_keyboard.destination_end) {
                    advance_before_caret += metrics[1];
                }
                character = cursor[1];
                total_advance += metrics[1];
                cursor++;
            }
            caret.top = 0x78;
            caret.left = (int16_t)((bounds->left + bounds->right) / 2 - (total_advance >> 1) + advance_before_caret);
            caret.bottom = (int16_t)(height + 0x78);
            caret.right = (int16_t)(caret.left + 1);
            halo::interface::ui_draw_screen_quad(0, (int16_t *)&caret, (int32_t)white, 0, halo::k_dword_none);
        }
    }
}

/**
 * Resets the virtual-keyboard globals to inactive and looks up the ui\english strings tag and the shared white
 * bitmap tag; returns whether the strings tag was found.
 *
 * @address 0x4a88f0
 */
int32_t VirtualKeyboard::initialize()
{
    datum_index strings_tag;

    virtual_keyboard.active = 0;
    virtual_keyboard.unknown_01 = 0;
    virtual_keyboard.unknown_02 = 0;
    virtual_keyboard.unknown_03 = 0;

    strings_tag = halo::interface::lookup_tag(halo::fourcc('v', 'c', 'k', 'y'), "ui\\english");
    if (strings_tag != halo::k_dword_none) {
        virtual_keyboard.strings_tag_data = halo::interface::tag_data<void>(strings_tag);
        virtual_keyboard.caret = 0;
        virtual_keyboard.unknown_0a = 0;
        virtual_keyboard.maximum_length = 0;
        virtual_keyboard.selection_start = -1;
        virtual_keyboard.selection_end = -1;
        virtual_keyboard.unknown_12 = 0;
        virtual_keyboard.destination = 0;
        virtual_keyboard.destination_end = 0;
        virtual_keyboard.open_time = 0;
    }

    virtual_keyboard.white_bitmap = halo::interface::lookup_tag(halo::fourcc('b', 'i', 't', 'm'), "ui\\shell\\bitmaps\\white");
    return virtual_keyboard.strings_tag_data != 0;
}

/**
 * @address 0x4a8be0
 */
void VirtualKeyboard::process_input()
{
    for (;;) {
        uint8_t mode_flags = controls_input_capture_flags;
        ui_key_event event;

        if (mode_flags == 1 || (mode_flags & 8) != 0 || (mode_flags & 4) == 0 ||
            key_event_read_index >= key_event_count) {
            return;
        }
        event = key_events[key_event_read_index];
        key_event_read_index++;

        switch (event.key_code) {
        case 0x00:
            halo::interface::virtual_keyboard_close();
            continue;

        case 0x1d:
            if (virtual_keyboard.opened != 1) {
                halo::interface::virtual_keyboard_backspace();
                continue;
            }
            vk_clear_text();
            break;

        case 0x38:
        case 0x66: {
            uint8_t name_ok;

            if (wcscmp((const wchar_t *)virtual_keyboard.text, (const wchar_t *)virtual_keyboard.destination) == 0) {
                goto commit_ok;
            }
            switch (virtual_keyboard.validation_mode) {
            case 1:
                if (!halo::interface::ui_wide_string_has_non_whitespace(virtual_keyboard.destination) ||
                    !vk_trim_trailing_whitespace()) {
                    goto invalid;
                }
                if (halo::saved_games::saved_game_name_is_available(virtual_keyboard.destination)) {
                    goto commit_ok;
                }
                name_ok = halo::interface::saved_item_name_matches((wchar_t *)virtual_keyboard.destination);
                break;
            case 2:
                if (!halo::interface::ui_wide_string_has_non_whitespace(virtual_keyboard.destination) ||
                    !vk_trim_trailing_whitespace()) {
                    goto invalid;
                }
                if (halo::interface::saved_item_name_matches((wchar_t *)virtual_keyboard.destination)) {
                    goto commit_ok;
                }
                if (!halo::saved_games::saved_game_name_is_available(virtual_keyboard.destination)) {
                    goto name_taken;
                }
                name_ok = halo::interface::ui_variant_name_is_available(virtual_keyboard.destination);
                break;
            case 3:
                if (virtual_keyboard.destination[0] != 0) {
                    goto commit_ok;
                }
                wcslen((const wchar_t *)virtual_keyboard.text);
                wcscpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text);
                halo::interface::virtual_keyboard_close();
                goto finish;
            default:
                goto finish;
            }
            if (name_ok) {
                goto commit_ok;
            }
name_taken:
            halo::interface::display_error(0x1b, -1, 1, 0);
            halo::interface::virtual_keyboard_close();
            goto finish;
invalid:
            halo::interface::display_error(0x1d, -1, 1, 0);
            halo::interface::virtual_keyboard_close();
            goto finish;
commit_ok:
            virtual_keyboard.committed = 1;
finish:
            halo::interface::widget_play_sound_effect(3);
            controls_input_capture_flags &= 0xfb;
            virtual_keyboard.active = 0;
            if (keyboard_device != 0) {
                int32_t minus_one = -1;
                void **vtable = *(void ***)keyboard_device;
                ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
                memset(key_release_pending, 0, sizeof(key_release_pending));
                memset(key_frames, 0, sizeof(key_frames));
            }
            continue;
        }

        case 0x4f:
            if (virtual_keyboard.destination_end > virtual_keyboard.destination) {
                virtual_keyboard.destination_end--;
            }
            break;

        case 0x50:
            if (*virtual_keyboard.destination_end != 0) {
                virtual_keyboard.destination_end++;
            }
            break;

        case 0x52:
            virtual_keyboard.destination_end = virtual_keyboard.destination;
            break;

        case 0x55:
            virtual_keyboard.destination_end =
                virtual_keyboard.destination + wcslen((const wchar_t *)virtual_keyboard.destination);
            break;

        case 0x54:
            if (virtual_keyboard.opened == 1) {
                vk_clear_text();
                break;
            }
            if (*virtual_keyboard.destination_end != 0) {
                int32_t bytes = (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                                (int32_t)((uint8_t *)virtual_keyboard.destination_end -
                                          (uint8_t *)virtual_keyboard.destination) - 1;
                if (bytes > 0) {
                    memmove(virtual_keyboard.destination_end, virtual_keyboard.destination_end + 1, bytes);
                    virtual_keyboard.destination[((uint16_t)virtual_keyboard.maximum_length >> 1) - 1] = 0;
                    halo::interface::widget_play_sound_effect(1);
                }
            }
            continue;

        default: {
            uint8_t ch = event.character;
            uint8_t *font;
            int32_t *character_map;
            int16_t *glyph;

            if (ch < 0x20 || ch == 0xff) {
                continue;
            }
            font = halo::interface::tag_data<uint8_t>(virtual_keyboard.small_ui_tag);
            character_map = *(int32_t **)(font + 0x34) + (ch >> 8) * 3;
            if (character_map[0] <= 0) {
                goto rejected;
            }
            glyph = (character_map[0] == 0x100) ? (int16_t *)(uintptr_t)character_map[1] + ch : nullptr;
            if (*glyph == -1 || *(int32_t *)(font + 0x80) + *glyph * 0x14 == 0 ||
                !halo::interface::virtual_keyboard_character_is_legal(virtual_keyboard.validation_mode, ch)) {
                goto rejected;
            }
            if (virtual_keyboard.validation_mode == 3 &&
                virtual_keyboard.destination_end == virtual_keyboard.destination &&
                (ch == 0x20 || (uint32_t)ch == 0xffffffa0u)) {
                goto rejected;
            }
            if (virtual_keyboard.opened == 1) {
                vk_clear_text();
                virtual_keyboard.opened = 0;
            }
            if ((int32_t)(uint16_t)virtual_keyboard.maximum_length -
                    (wcslen((const wchar_t *)virtual_keyboard.destination) * 2 + 2) < 2) {
                goto rejected;
            }
            memmove(virtual_keyboard.destination_end + 1, virtual_keyboard.destination_end,
                    (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                        (int32_t)((uint8_t *)virtual_keyboard.destination_end -
                                  (uint8_t *)virtual_keyboard.destination) - 2);
            *virtual_keyboard.destination_end = ch;
            virtual_keyboard.destination_end++;

            if (wcscmp((const wchar_t *)virtual_keyboard.destination, (const wchar_t *)fortune_easter_egg_text) != 0) {
                halo::interface::widget_play_sound_effect(1);
                continue;
            }
            {
                uint32_t pick = halo::cseries::time_query_performance_counter_ms() % 10;
                if (pick > 9) {
                    pick = 9;
                }
                virtual_keyboard.field_kind = (int16_t)(pick + 0xb);
            }
            if (virtual_keyboard.text[0] != 0) {
                wcslen((const wchar_t *)virtual_keyboard.text);
                wcscpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text);
                virtual_keyboard.destination_end =
                    virtual_keyboard.destination + wcslen((const wchar_t *)virtual_keyboard.destination);
            } else {
                vk_clear_text();
            }
            halo::interface::widget_play_sound_effect(1);
            continue;
rejected:
            halo::interface::widget_play_sound_effect(4);
            continue;
        }
        }

        virtual_keyboard.opened = 0;
        halo::interface::widget_play_sound_effect(1);
    }
}

/**
 * @address 0x4a9510
 */
void VirtualKeyboard::render()
{
    const uint8_t *strings = (const uint8_t *)virtual_keyboard.strings_tag_data;
    datum_index background = *(const datum_index *)(strings + 0x1c);
    datum_index string_list;
    const uint16_t *prompt = missing_string_text;
    Rectangle2D rect;

    if (background != (datum_index)-1) {
        BitmapData *bitmap = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(background, 0, 0);

        rect.top = 0;
        rect.left = 0;
        rect.bottom = halo::interface::k_base_screen_height;
        rect.right = halo::interface::k_base_screen_width;
        halo::interface::ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, (int32_t)bitmap, 0, halo::k_dword_none);
    }

    virtual_keyboard_set_text_state(0);
    string_list = *(const datum_index *)((const uint8_t *)virtual_keyboard.strings_tag_data + 0x2c);
    if (string_list != (datum_index)-1) {
        const uint16_t *title = halo::text::text_string_list_get_string(string_list, virtual_keyboard.field_kind);

        rect.top = 0x4e;
        rect.left = 0x72;
        rect.bottom = 0x6e;
        rect.right = halo::interface::k_base_screen_width;
        halo::rasterizer::chimera__draw_16_bit_text(&rect, (int32_t *)&rect, 0, 0, (const int16_t *)title);
    }

    string_list = *(const datum_index *)((const uint8_t *)virtual_keyboard.strings_tag_data + 0x2c);
    if (string_list != (datum_index)-1) {
        UnicodeStringList *list = halo::interface::tag_data<UnicodeStringList>(string_list);

        if (list->strings.count > 0xe) {
            UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer + 0xe;

            if ((int32_t)entry->string.size > 0) {
                uint16_t *text = (uint16_t *)entry->string.pointer;

                text[(entry->string.size >> 1) - 1] = 0;
                prompt = text;
            }
        }
    }
    virtual_keyboard_set_text_state(1);
    rect.top = 0x19e;
    rect.left = 0;
    rect.bottom = 0x1c2;
    rect.right = 0x276;
    halo::rasterizer::chimera__draw_16_bit_text(&rect, (int32_t *)&rect, 0, 0, (const int16_t *)prompt);

    rect.top = 0x76;
    rect.left = 0x78;
    rect.bottom = 0x8f;
    rect.right = 0x208;
    halo::interface::virtual_keyboard_draw_text(&rect);
}

} // namespace halo::interface

namespace halo::interface {

void virtual_keyboard_backspace(void)
{
    halo::interface::VirtualKeyboard::backspace();
}

uint8_t virtual_keyboard_character_is_legal(int32_t validation_mode, uint8_t character)
{
    return halo::interface::VirtualKeyboard::character_is_legal(validation_mode, character);
}

uint8_t virtual_keyboard_close(void)
{
    return halo::interface::VirtualKeyboard::close();
}

void virtual_keyboard_draw_text(Rectangle2D *bounds)
{
    halo::interface::VirtualKeyboard::draw_text(bounds);
}

int32_t virtual_keyboard_initialize(void)
{
    return halo::interface::VirtualKeyboard::initialize();
}

void virtual_keyboard_process_input(void)
{
    halo::interface::VirtualKeyboard::process_input();
}

void virtual_keyboard_render(void)
{
    halo::interface::VirtualKeyboard::render();
}

}

#include "halo/interface/ifr2_main.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "crt.h"
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern datum_index ui_cursor_bitmap;
extern int32_t ui_cursor_x;
extern int32_t ui_cursor_y;
extern first_person_weapon_interface *first_person_weapon_interfaces;
extern int32_t interface_loading_screen_address_b;
extern uint32_t interface_loading_screen_address_a;
extern progress_screen_state join_ui_state;
extern int32_t interface_loading_screen_progress;
extern uint16_t progress_screen_text[0x20];
extern uint16_t progress_screen_subtext[0x20];
extern datum_index interface_loading_screen_request_id;
extern uint8_t ui_cursor_changed;
extern widget_instance *ui_root_widget[1];
extern int32_t ui_time_milliseconds;
extern widget_history_node *ui_widget_history[3];
extern void sound_looping_stop(datum_index sound_tag);
extern int32_t main_menu_music_datum;
extern map_list_entry *map_list;
extern int32_t map_list_count;
extern heap *widget_memory_pool;
extern uint16_t missing_string_text[];
extern data_array *terminal_messages;
extern uint8_t terminal_initialized;
extern terminal_console *console_active;
extern datum_index console_message_head;
extern datum_index console_message_tail;
extern int32_t console_caret_blink_time;
extern int32_t console_rcon_handle;
}

namespace halo::interface {

/**
 * Draws the mouse cursor bitmap (a 32x32 quad) over the interface at ui_cursor_x/y; if the cursor bitmap tag
 * or its bitmap data has not loaded yet, falls back to a 16x16 solid translucent red rectangle. objdump
 * 0x497380..0x497403: the rect is a Rectangle2D {y, x, y + size, x + size}; the first rewrite stored x and y +
 * size in swapped slots.
 *
 * @address 0x497380
 */
void InterfaceMain::draw_cursor()
{
    Rectangle2D rect;
    int32_t bitmap_data;

    rect.top = (int16_t)ui_cursor_y;
    rect.left = (int16_t)ui_cursor_x;

    if (ui_cursor_bitmap != (datum_index)-1) {
        bitmap_data = reinterpret_cast<int32_t>(halo::bitmaps::bitmap_group_sequence_get_bitmap_data(ui_cursor_bitmap, 0, 0));
        if (bitmap_data != 0) {
            rect.bottom = (int16_t)(ui_cursor_y + 0x20);
            rect.right = (int16_t)(ui_cursor_x + 0x20);
            halo::interface::ui_draw_screen_quad(nullptr, (int16_t *)&rect, bitmap_data, nullptr, halo::k_dword_none);
            return;
        }
    }
    rect.bottom = (int16_t)(ui_cursor_y + 0x10);
    rect.right = (int16_t)(ui_cursor_x + 0x10);
    halo::interface::ui_draw_filled_rectangle(halo::interface::k_missing_bitmap_color, &rect);
}

/**
 * Initializes the developer console and HUD state blocks, then reserves one first_person_weapon_interface
 * (0x1ea0 bytes) from the game-state bump allocator, folding its size into the running game-state checksum.
 *
 * @address 0x494340
 */
void InterfaceMain::globals_allocate()
{
    int32_t block;
    int32_t size = sizeof(first_person_weapon_interface);

    halo::interface::terminal_initialize();
    halo::interface::hud_state_allocate();

    block = halo::saved_games::globals().game_state_cursor + (int32_t)halo::saved_games::globals().game_state_base;
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + sizeof(first_person_weapon_interface);
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    first_person_weapon_interfaces = (first_person_weapon_interface *)block;
}

/**
 * Resets every progress-screen global to its inactive state.
 *
 * @address 0x4978d0
 */
void InterfaceMain::loading_screen_reset()
{
    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = (progress_screen_state)0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;
    progress_screen_subtext[0] = 0;
    interface_loading_screen_request_id = (datum_index)-1;
}

/**
 * blam-cc: EAX -> text Sets the progress screen's message text: clears it when text is null, otherwise
 * forwards to string_convert_ascii_to_unicode (presumably a formatted copy into progress_screen_text).
 *
 * @address 0x4978a0
 */
void InterfaceMain::loading_screen_set_text(const char *text)
{
    if (text == nullptr) {
        progress_screen_text[0] = 0;
    } else {
        halo::text::string_convert_ascii_to_unicode(progress_screen_text, 0x40, text);
    }
}

/**
 * blam-cc: EAX -> new_cursor_x, ECX -> new_cursor_y Stores a new cursor position into ui_cursor_x/ui_cursor_y,
 * clamping each axis into the engine's 640x480 UI coordinate space (or to 0 if negative), and raises
 * ui_cursor_changed whenever either axis actually moved.
 *
 * @address 0x497250
 */
void InterfaceMain::update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y)
{
    if (ui_cursor_x != new_cursor_x) {
        ui_cursor_changed = 1;
    } else {
        ui_cursor_changed = 0;
        if (ui_cursor_y != new_cursor_y) {
            ui_cursor_changed = 1;
        }
    }

    if (new_cursor_x < 0) {
        ui_cursor_x = 0;
    } else {
        ui_cursor_x = halo::interface::k_base_screen_width;
        if (new_cursor_x < halo::interface::k_base_screen_width + 1) {
            ui_cursor_x = new_cursor_x;
        }
    }

    if (new_cursor_y < 0) {
        ui_cursor_y = 0;
        return;
    }
    ui_cursor_y = halo::interface::k_base_screen_height;
    if (new_cursor_y < halo::interface::k_base_screen_height + 1) {
        ui_cursor_y = new_cursor_y;
    }
}

/**
 * Called when the main menu widget is (re)shown: detaches the title music if it is still marked pending (it
 * never actually started playing, e.g.
 *
 * @address 0x498ab0
 */
void InterfaceMain::on_shown(int32_t fade_milliseconds)
{
    if (halo::main::globals().menu_music_pending == 1) {
        datum_index sound_tag = halo::interface::lookup_tag(halo::fourcc('l', 's', 'n', 'd'), "sound\\music\\title1\\title1");

        if (sound_tag != (datum_index)-1) {
            halo::sound::sound_looping_stop(sound_tag);
        }
        halo::main::globals().menu_music_pending = 0;
    }
    if (ui_root_widget[0] != (widget_instance *)0 && ui_root_widget[0]->is_error_dialog == 0) {
        ui_root_widget[0]->milliseconds_auto_close_fade = fade_milliseconds;
        ui_root_widget[0]->milliseconds_to_auto_close =
            (ui_time_milliseconds - ui_root_widget[0]->creation_time) + 100;
        if (ui_widget_history[0] != (widget_history_node *)0) {
            halo::interface::widget_pool_list_free_all(&ui_widget_history[0]);
        }
    }
}

/**
 * Starts the main menu's title theme music if it is not already playing.
 *
 * @address 0x4993e0
 */
void InterfaceMain::play_title_music()
{
    datum_index sound_tag;

    if (halo::main::globals().menu_music_pending == 0 && main_menu_music_datum == 0) {
        sound_tag = halo::interface::lookup_tag(halo::fourcc('l', 's', 'n', 'd'), "sound\\music\\title1\\title1");
        if (sound_tag != (datum_index)-1) {
            halo::sound::sound_looping_start(sound_tag, -1, 1.0f);
            halo::main::globals().menu_music_pending = 1;
        }
    }
}

/**
 * blam-cc: EAX -> map_path Lowercases a local copy of `map_path` and linear-scans map_list for an entry whose
 * path matches it exactly, returning that entry's index, or -1 if none match (or the list is empty).
 *
 * @address 0x494ff0
 */
int32_t MapList::find_known_map_index(char *map_path)
{
    char local_path[halo::interface::k_map_path_chars];
    char *cursor;
    int32_t index;

    strncpy(local_path, map_path, halo::interface::k_map_path_chars);
    for (cursor = local_path; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((uint8_t)*cursor);
    }

    if (map_list_count < 1) {
        return -1;
    }

    for (index = 0; index < map_list_count; index++) {
        if (strcmp(map_list[index].path, local_path) == 0) {
            return index;
        }
    }
    return -1;
}

/**
 * blam-cc: EAX -> map_path, ESI -> destination_capacity, stack -> destination Resolves the friendly display
 * name for `map_path` into `destination` (capacity `destination_capacity` wide characters): if it matches one
 * of the built-in multiplayer maps (index 0..18 into the "ui\shell\main_menu\mp_map_list" unicode_string_list
 * tag), copies that localized string;
 *
 * @address 0x494f50
 */
void MapList::get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity)
{
    datum_index map_list_tag;
    int32_t index;
    wchar_t *source;
    char *filename;

    map_list_tag = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\shell\\main_menu\\mp_map_list");
    index = halo::interface::map_list_find_known_map_index(map_path);
    if (-1 < index && index < 0x13 && index != -1) {

        source = (wchar_t *)halo::text::text_string_list_get_string(map_list_tag, (int16_t)map_list[index].map_id);
        wcsncpy(destination, source, destination_capacity - 1);
        destination[destination_capacity - 1] = L'\0';
        return;
    }

    filename = strrchr(map_path, '\\');
    if (filename != nullptr) {
        halo::text::string_convert_ascii_to_unicode((uint16_t *)destination, destination_capacity * 2, filename + 1);
        return;
    }
    halo::text::string_convert_ascii_to_unicode((uint16_t *)destination, destination_capacity * 2, map_path);
}

/**
 * blam-cc: EBX -> widget, stack -> name_source Allocates a 0x80 byte (64 wide char) text buffer for `widget`
 * and formats it as "<suffix> <name_source>", where suffix is the 8th entry of the common button captions
 * string list (its own last character stripped) if that list has at least 8 entries and the entry is
 * non-empty, else a compiled-in default suffix string.
 *
 * @address 0x49c710
 */
void InterfaceMain::set_profile_name(widget_instance *widget, const uint16_t *name_source)
{
    datum_index tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\shell\\strings\\common_button_captions");
    uint16_t *suffix = missing_string_text;
    void *buffer = halo::memory::heap_reallocate(widget->text, 0x80, widget_memory_pool);

    widget->text = buffer;
    if (buffer != nullptr) {
        if (tag_id != (datum_index)-1) {
            UnicodeStringList *list = halo::interface::tag_data<UnicodeStringList>(tag_id);

            if (list->strings.count > 7) {
                UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                uint32_t size = strings[7].string.size;

                if ((int32_t)size > 0) {
                    suffix = (uint16_t *)strings[7].string.pointer;

                    *(uint16_t *)((uint8_t *)suffix + ((size & ~1u) - 2)) = 0;
                }
            }
        }
        halo::text::string_format_wide_va_bounded(0x3f, (uint16_t *)((wchar_t *)buffer), (const uint16_t *)L"%s %s", suffix, name_source);
        ((uint16_t *)widget->text)[0x3f] = 0;
    }
}

/**
 * @address 0x496df0
 */
void InterfaceMain::string_replace_all_in_place(char *buffer, char *search, char *replacement)
{
    uint32_t search_length;
    uint32_t replacement_length;
    char *buffer_end;
    char *cursor;

    search_length = strlen(search);
    replacement_length = strlen(replacement);
    buffer_end = buffer + strlen(buffer) + 1;

    cursor = buffer;
    if (buffer != nullptr) {
        while ((cursor = strstr(cursor, search)) != nullptr) {
            uint32_t tail_size = (uint32_t)(buffer_end - cursor) - 1;

            memmove(cursor, replacement, replacement_length);
            memmove(cursor + replacement_length, cursor + search_length, tail_size);
        }
    }
}

/**
 * Allocates the terminal-output console_message data array (capacity 0x20) and resets the developer
 * console/terminal globals to their empty state.
 *
 * @address 0x4963d0
 */
void InterfaceMain::initialize_terminal()
{
    halo::main::globals().terminal_messages = halo::memory::data_new(sizeof(console_message), (char *)"terminal output", 0x20);
    halo::main::globals().terminal_initialized = 1;
    halo::main::globals().terminal_messages->valid = 1;
    halo::memory::data_delete_all(halo::main::globals().terminal_messages);
    halo::main::globals().console_active = (terminal_console *)0;
    halo::main::globals().console_message_head = (datum_index)halo::k_dword_none;
    halo::main::globals().console_message_tail = (datum_index)halo::k_dword_none;
    halo::main::globals().console_caret_blink_time = 0;
    halo::main::globals().console_rcon_handle = (int32_t)halo::k_dword_none;
}

} // namespace halo::interface

namespace halo::interface {

void interface_draw_cursor(void)
{
    halo::interface::InterfaceMain::draw_cursor();
}

void interface_globals_allocate(void)
{
    halo::interface::InterfaceMain::globals_allocate();
}

void interface_loading_screen_reset(void)
{
    halo::interface::InterfaceMain::loading_screen_reset();
}

void interface_loading_screen_set_text(const char *text)
{
    halo::interface::InterfaceMain::loading_screen_set_text(text);
}

void interface_update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y)
{
    halo::interface::InterfaceMain::update_for_resolution_change(new_cursor_x, new_cursor_y);
}

void main_menu_on_shown(int32_t fade_milliseconds)
{
    halo::interface::InterfaceMain::on_shown(fade_milliseconds);
}

void main_menu_play_title_music(void)
{
    halo::interface::InterfaceMain::play_title_music();
}

int32_t map_list_find_known_map_index(char *map_path)
{
    return halo::interface::MapList::find_known_map_index(map_path);
}

void map_list_get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity)
{
    halo::interface::MapList::get_friendly_level_name(destination, map_path, destination_capacity);
}

void set_profile_name(widget_instance *widget, const uint16_t *name_source)
{
    halo::interface::InterfaceMain::set_profile_name(widget, name_source);
}

void string_replace_all_in_place(char *buffer, char *search, char *replacement)
{
    halo::interface::InterfaceMain::string_replace_all_in_place(buffer, search, replacement);
}

void terminal_initialize(void)
{
    halo::interface::InterfaceMain::initialize_terminal();
}

}

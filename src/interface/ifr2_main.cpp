#include "halo/interface/ifr2_main.hpp"
#include "halo/core/ui_tag_paths.hpp"
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
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"
#include "halo/interface/game_state_block.hpp"
#include "halo/interface/layout_checks.hpp"

#ifdef interface
#undef interface
#endif

static auto &ui_cursor_bitmap = halo::link::ref<datum_index>(halo::ui::vars().ui_cursor_bitmap);
static auto &ui_cursor_x = halo::link::ref<int32_t>(halo::ui::vars().ui_cursor_x);
static auto &ui_cursor_y = halo::link::ref<int32_t>(halo::ui::vars().ui_cursor_y);
static auto &first_person_weapon_interfaces = halo::link::ref<first_person_weapon_interface *>(halo::ui::vars().first_person_weapon_interfaces);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &interface_loading_screen_address_a = halo::link::ref<uint32_t>(halo::main::vars().interface_loading_screen_address_a);
static auto &join_ui_state = halo::link::ref<progress_screen_state>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_progress = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_progress);
static auto &progress_screen_text = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_text);
static auto &progress_screen_subtext = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_subtext);
static auto &interface_loading_screen_request_id = halo::link::ref<datum_index>(halo::networking::vars().interface_loading_screen_request_id);
static auto &ui_cursor_changed = halo::link::ref<uint8_t>(halo::ui::vars().ui_cursor_changed);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &ui_time_milliseconds = halo::link::ref<int32_t>(halo::ui::vars().ui_time_milliseconds);
static auto &ui_widget_history = halo::link::ref<widget_history_node *[3]>(halo::ui::vars().ui_widget_history);
static auto &main_menu_music_datum = halo::link::ref<int32_t>(halo::ui::vars().main_menu_music_datum);
static auto &map_list = halo::link::ref<map_list_entry *>(halo::ui::vars().map_list);
static auto &map_list_count = halo::link::ref<int32_t>(halo::ui::vars().map_list_count);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &missing_string_text = halo::link::ref<uint16_t []>(halo::ui::vars().missing_string_text);
static auto &terminal_messages = halo::link::ref<data_array *>(halo::main::vars().terminal_messages);
static auto &terminal_initialized = halo::link::ref<uint8_t>(halo::main::vars().terminal_initialized);
static auto &console_active = halo::link::ref<terminal_console *>(halo::main::vars().console_active);
static auto &console_message_head = halo::link::ref<datum_index>(halo::main::vars().console_message_head);
static auto &console_message_tail = halo::link::ref<datum_index>(halo::main::vars().console_message_tail);
static auto &console_caret_blink_time = halo::link::ref<int32_t>(halo::ui::vars().console_caret_blink_time);
static auto &console_rcon_handle = halo::link::ref<int32_t>(halo::main::vars().console_rcon_handle);

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
    BitmapData *bitmap_data;

    rect.top = (int16_t)ui_cursor_y;
    rect.left = (int16_t)ui_cursor_x;

    if (ui_cursor_bitmap != k_datum_index_none) {
        bitmap_data = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(ui_cursor_bitmap, 0, 0);
        if (bitmap_data != 0) {
            rect.bottom = (int16_t)(ui_cursor_y + 0x20);
            rect.right = (int16_t)(ui_cursor_x + 0x20);
            halo::interface::ui_draw_screen_quad(nullptr, &rect, bitmap_data, nullptr, halo::k_dword_none);
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
    halo::interface::terminal_initialize();
    halo::interface::hud_state_allocate();

    first_person_weapon_interfaces = halo::interface::game_state_allocate_block<first_person_weapon_interface>();
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
    interface_loading_screen_request_id = k_datum_index_none;
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

        if (sound_tag != k_datum_index_none) {
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
        if (sound_tag != k_datum_index_none) {
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

    map_list_tag = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::mp_map_list);
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
    datum_index tag_id = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::common_button_captions);
    uint16_t *suffix = missing_string_text;
    void *buffer = halo::memory::heap_reallocate(widget->text, 0x80, widget_memory_pool);

    widget->text = buffer;
    if (buffer != nullptr) {
        if (tag_id != k_datum_index_none) {
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
        halo::text::string_format_wide_va_bounded(0x3f, (uint16_t *)((wchar_t *)buffer), halo::interface::wide(L"%s %s"), suffix, name_source);
        (halo::interface::widget_text(widget))[0x3f] = 0;
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
    halo::main::globals().terminal_messages = halo::memory::data_new(sizeof(console_message), "terminal output", 0x20);
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

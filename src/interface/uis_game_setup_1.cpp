/**
 * Level, map and variant selection lists and the campaign start and restart paths.
 */

#include "crt.h"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include <string.h>
#include "objects.h"
#include "units.h"

#include "halo/interface/uis_game_setup.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"

extern "C" {
extern char level_select_current_path_00719068[0x106];
extern level_select_entry level_select_entries[10];
extern int32_t cached_saved_game_something;
extern uint8_t level_select_flags_0071916a;
extern uint8_t level_select_flags_0071916b;
extern uint8_t level_select_flags_0071916c;
extern uint8_t profile_globals_block[0x60a4];
extern int16_t known_solo_level_index_00712f00;
extern growable_array ui_lists[3];
extern int32_t ui_list_current;
extern uint8_t ui_list_has_default;
extern campaign_level_entry known_campaign_levels_00692acc[10];
extern uint16_t missing_string_text[];
extern int16_t level_select_frame_00719168;
extern int32_t last_level_widget_selection_00692afc;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern uint8_t coop_profile_globals_block_00714ddc[k_saved_player_profile_size];
extern map_list_entry *map_list;
extern int32_t map_list_count;
extern uint8_t save_in_progress_00719010;
extern int32_t cached_profile_slot;
extern uint8_t network_wait_flag_00719739;
extern char last_profile_name[];
extern void saved_game_delete_files(void);
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name);
extern void saved_game_last_profile_clear(char *name);
}

namespace halo::ui {

/**
 * Builds the campaign level-selection list. If more than one known level exists, delegates entirely to the co-op
 * variant ui_build_level_select_list_coop. Otherwise rebuilds the 10-slot known-level table from the current
 * profile's per-level flags and adds each as a growable-array list entry (name looked up from the
 * map_list_oneline string list, falling back to a default), marking whichever slot matches the profile's
 * currently-selected level.
 *
 * @address 0x49c8f0
 */
uint32_t UiGameSetup::build_level_select_list(widget_instance *widget, void *param_2, void *param_3)
{
    datum_index string_list_tag;
    uint8_t profile_copy[0x2000];
    int16_t scan_type = 0;
    int16_t scan_level = 0;
    int32_t i;

    if (halo::game::globals().local_player_count > 1) {
        memset(level_select_current_path_00719068, 0, sizeof(level_select_current_path_00719068));
        halo::interface::ui_build_level_select_list_coop(widget, param_2, param_3);
        return 1;
    }

    string_list_tag = halo::interface::lookup_tag(halo::groups::unicode_string_list, "ui\\shell\\main_menu\\map_list_oneline");
    memset(level_select_entries, 0, sizeof(level_select_entries));

    if (halo::saved_games::globals().player_profile_slots_handle != cached_saved_game_something) {
        memset(level_select_current_path_00719068, 0, sizeof(level_select_current_path_00719068));
        level_select_flags_0071916b = halo::saved_games::game_state_read_checkpoint_summary(&level_select_flags_0071916c,
            &level_select_frame_00719168, level_select_current_path_00719068);
        cached_saved_game_something = halo::saved_games::globals().player_profile_slots_handle;
    }

    memcpy(profile_copy, profile_globals_block, sizeof(profile_copy) < sizeof(profile_globals_block)
                                                     ? sizeof(profile_copy)
                                                     : sizeof(profile_globals_block));
    halo::saved_games::player_profile_scan_campaign_progress(&scan_type, (saved_player_profile *)profile_copy, &scan_level);

    ui_lists[0].element_size = 0x10;
    ui_lists[1].element_size = 0x10;
    ui_lists[2].element_size = 0x10;
    ui_lists[0].count = 0;
    ui_lists[1].count = 0;
    ui_lists[2].count = 0;
    ui_lists[0].data = nullptr;
    ui_lists[1].data = nullptr;
    ui_lists[2].data = nullptr;
    ui_list_current = -1;
    ui_list_has_default = 0;

    if (known_solo_level_index_00712f00 < 0) {
        widget->selection_index = 0;
        i = 0;
    } else if (known_solo_level_index_00712f00 < 10) {
        widget->selection_index = (int16_t)known_solo_level_index_00712f00;
        i = 0;
    } else {
        widget->selection_index = 9;
        i = 0;
    }

    do {
        uint16_t *entry_name;
        uint8_t is_selected;
        int32_t element_index;

        level_select_entries[i].path = known_campaign_levels_00692acc[i].path;

        if (profile_copy[0x11e + i] != 0 || i == scan_level + 1 || i == 0) {
            uint32_t flags = (uint32_t)(uint8_t)profile_copy[0x11e + i];

            level_select_entries[i].flag_bit1 = (uint8_t)((flags >> 1) & 1);
            level_select_entries[i].valid = 1;
            level_select_entries[i].flag_bit2 = (uint8_t)((flags >> 2) & 1);
            level_select_entries[i].flag_bit3 = (uint8_t)((flags >> 3) & 1);
        }

        entry_name = missing_string_text;
        if (string_list_tag != (datum_index)-1) {
            UnicodeStringList *list = halo::interface::tag_data<UnicodeStringList>(string_list_tag);

            if (i >= 0 && i < (int32_t)list->strings.count) {
                UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                uint32_t size = strings[i].string.size;

                if ((int32_t)size > 0) {
                    entry_name = (uint16_t *)strings[i].string.pointer;
                    *(uint16_t *)((uint8_t *)entry_name + ((size & ~1u) - 2)) = 0;
                }
            }
        }

        is_selected = (i == widget->selection_index);
        element_index = halo::memory::growable_array_add_element(&ui_lists[1]);
        if (element_index != -1) {
            ui_list_item *item = (ui_list_item *)ui_lists[1].data + element_index;
            uint32_t name_length = wcslen((const wchar_t *)entry_name);

            item->data = nullptr;
            item->name = (uint16_t *)GlobalAlloc(0, name_length * 2 + 2);
            item->id = i;
            item->is_default = is_selected;
            if (is_selected) {
                ui_list_has_default = 1;
            }
            wcscpy((wchar_t *)item->name, (const wchar_t *)entry_name);
        }
        i = i + 1;
    } while (i < 10);

    *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
    widget->list_items = level_select_entries;
    widget->item_count = 10;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;

    if (level_select_flags_0071916b == 1) {
        level_select_frame_00719168 = 0;
        for (i = 0; i < 10; i++) {
            int32_t cmp = _stricmp(level_select_current_path_00719068, known_campaign_levels_00692acc[i].path);
            int16_t saved_frame = level_select_frame_00719168;

            if (cmp == 0) {
                level_select_flags_0071916a = (uint8_t)i;
                if (level_select_frame_00719168 < 0) {
                    level_select_frame_00719168 = 0;
                } else {
                    level_select_frame_00719168 = 3;
                    if (saved_frame < 4) {
                        level_select_frame_00719168 = saved_frame;
                    }
                }
                break;
            }
            level_select_frame_00719168 = saved_frame;
        }
        if (i == 10) {
            level_select_flags_0071916b = 0;
            return 1;
        }
    } else if (level_select_flags_0071916c == 1 && halo::saved_games::globals().player_profile_slots_handle != -1) {
        if (last_level_widget_selection_00692afc == -1) {
            if (quit_confirm_error_string_index == -1) {
                quit_confirm_error_string_index = 0x27;
                quit_confirm_error_unknown_ae = halo::k_word_none;
                quit_confirm_error_modal = 1;
                quit_confirm_error_is_error = 0;
            }
            last_level_widget_selection_00692afc = halo::saved_games::globals().player_profile_slots_handle;
            return 1;
        }
        last_level_widget_selection_00692afc = -1;
    }
    return 1;
}

/**
 * Populates the level-selection widget's list from BOTH players' progress buffers, marking a level's flag bits
 * set if either buffer's per-level byte has them set.
 *
 * @address 0x49cc80
 */
void UiGameSetup::build_level_select_list_coop(widget_instance *widget, void *param_2, void *param_3)
{
    uint8_t profile_copy_a[0x2000];
    uint8_t profile_copy_b[0x2000];
    int16_t level_a = 0, type_a = 0;
    int16_t level_b = 0, type_b = 0;
    int32_t i;

    (void)param_2;
    (void)param_3;

    memset(level_select_entries, 0, sizeof(level_select_entries));

    memcpy(profile_copy_a, profile_globals_block,
           sizeof(profile_copy_a) < sizeof(profile_globals_block) ? sizeof(profile_copy_a)
                                                                    : sizeof(profile_globals_block));
    halo::saved_games::player_profile_scan_campaign_progress(&type_a, (saved_player_profile *)profile_copy_a, &level_a);
    memcpy(profile_copy_b, coop_profile_globals_block_00714ddc,
           sizeof(profile_copy_b) < sizeof(coop_profile_globals_block_00714ddc)
               ? sizeof(profile_copy_b)
               : sizeof(coop_profile_globals_block_00714ddc));
    halo::saved_games::player_profile_scan_campaign_progress(&type_b, (saved_player_profile *)profile_copy_b, &level_b);

    for (i = 0; i < 10; i++) {
        uint8_t flag_a = profile_copy_a[0x11e + i];
        uint8_t flag_b = profile_copy_b[0x11e + i];

        level_select_entries[i].path = known_campaign_levels_00692acc[i].path;
        if (flag_a != 0 || i == level_a + 1 || flag_b != 0 || i == level_b + 1 || i == 0) {
            uint32_t flags = (uint32_t)(uint8_t)(flag_b | flag_a);

            level_select_entries[i].flag_bit1 = (uint8_t)((flags >> 1) & 1);
            level_select_entries[i].valid = 1;
            level_select_entries[i].flag_bit2 = (uint8_t)((flags >> 2) & 1);
            level_select_entries[i].flag_bit3 = (uint8_t)((flags >> 3) & 1);
        }
    }

    widget->list_items = level_select_entries;
    widget->item_count = 10;
    if (known_solo_level_index_00712f00 < 0) {
        widget->selection_index = 0;
    } else if (known_solo_level_index_00712f00 > 9) {
        widget->selection_index = 9;
    } else {
        widget->selection_index = (int16_t)known_solo_level_index_00712f00;
    }
}

/**
 * Rebuilds this widget's rows, refreshes the linked game-variant description widget, then walks to the row 11
 * slots down (a fixed layout row) and hides/dims it when the selected variant's flag byte (+0x94, bit 0) is set.
 *
 * @address 0x4a8560
 */
void UiGameSetup::game_variant_flag_list_widget_build(widget_instance *widget)
{
    int16_t combo_index;
    void *variant_data = 0;
    widget_instance *row;
    widget_instance *target;
    int32_t depth;

    halo::interface::ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)halo::interface::ui_list_default_item_format));

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text;
    if (combo_index > -1 && combo_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
        variant_data = entry->data;
    }
    halo::interface::multiplayer_settings_select_list_update_item(widget->extended_description, (const uint16_t *)variant_data);

    row = widget->first_child;
    depth = 0;
    {
        widget_instance *cur = row;
        row = 0;
        do {
            row = 0;
            if (cur == 0) break;
            cur = cur->next_sibling;
            depth = depth + 1;
            row = cur;
        } while (depth < 0x0b);
    }

    target = row->first_child->next_sibling;
    if ((*((const uint8_t *)variant_data + 0x94) & 1) != 0) {
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
}

/**
 * Accepts the selected entry of the multiplayer map list: if its cache file exists, queues the map change and
 * remembers the map in lastmpmp.txt; otherwise plays the error sound. Returns whether the cache file exists.
 *
 * @address 0x49d7c0
 */
uint8_t UiGameSetup::map_select_confirm_choice(widget_instance *widget)
{
    int32_t selection = *(int16_t *)&((struct widget_instance *)widget)->text;
    int32_t count = map_list_count;
    int32_t map_index = -1;
    char *path;
    char *file_name;
    cache_file_header header;
    uint8_t exists;
    int32_t i;

    if (selection >= 0 && selection < ui_lists[ui_list_current].count) {
        map_index = ((ui_list_item *)ui_lists[ui_list_current].data)[selection].id;
    }
    path = map_list[map_index].path;

    file_name = strrchr(path, '\\');
    file_name = (file_name != 0) ? file_name + 1 : path;
    exists = halo::cache::cache_file_exists(file_name, &header);
    if (!exists) {
        halo::interface::widget_play_sound_effect(4);
        return exists;
    }

    halo::main::main_queue_map_change_by_name_or_clear(path);
    for (i = 0; i < count; i++) {
        if (_stricmp(path, map_list[i].path) == 0) {
            halo::saved_games::saved_game_last_mp_map_clear(map_list[i].path);
            return exists;
        }
    }
    return exists;
}

/**
 * Original UI routine; see docs/original/interface/ui_restart_saved_game.c.txt for the recovery notes.
 *
 * @address 0x4a1110
 */
uint32_t UiGameSetup::restart_saved_game(void)
{
    if (save_in_progress_00719010 != 0) {
        return 0;
    }
    halo::saved_games::saved_game_delete_files(last_profile_name);
    halo::networking::globals().game_mode = 0;
    network_wait_flag_00719739 = 1;
    if (cached_profile_slot != halo::saved_games::globals().player_profile_slots_handle) {
        if (halo::saved_games::globals().player_profile_slots_handle != -1) {
            halo::saved_games::saved_game_get_directory_by_handle(halo::saved_games::globals().player_profile_slots_handle, last_profile_name);
        }
        cached_profile_slot = halo::saved_games::globals().player_profile_slots_handle;
    }
    if (last_profile_name[0] != '\0') {
        halo::saved_games::saved_game_last_profile_clear(last_profile_name);
    }
    return 1;
}

/**
 * Returns 1 when no saved game variant is called name.
 *
 * Register convention: name -> EDI
 *
 * @address 0x4a8b50
 */
uint8_t UiGameSetup::variant_name_is_available(const uint16_t *name)
{
    char narrow[0x24];

    halo::text::string_convert_unicode_to_ascii((uint8_t *)narrow, (uint16_t *)name, 0x20);
    return halo::game::game_engine_get_variant_by_name(narrow, (game_variant *)0) == 0;
}

}

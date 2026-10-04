#include "halo/interface/ifr2_players.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "crt.h"
#include <string.h>
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/text/text.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/flags.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/interface/wide_text.hpp"

#ifdef interface
#undef interface
#endif

static auto &first_person_weapon_interfaces = halo::link::ref<first_person_weapon_interface *>(halo::ui::vars().first_person_weapon_interfaces);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &global_white_argb = halo::link::ref<const ColorARGB *>(halo::networking::vars().global_white_argb);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &hud_text_draw_font_tag_id = halo::link::ref<int32_t>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);
static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &player_help_name_a10 = halo::link::ref<char []>(halo::ui::vars().player_help_name_a10);
static auto &player_help_name_a30 = halo::link::ref<char []>(halo::ui::vars().player_help_name_a30);
static auto &player_help_name_a50 = halo::link::ref<char []>(halo::ui::vars().player_help_name_a50);
static auto &player_help_name_b30 = halo::link::ref<char []>(halo::ui::vars().player_help_name_b30);
static auto &player_help_name_b40 = halo::link::ref<char []>(halo::ui::vars().player_help_name_b40);
static auto &player_help_name_c10 = halo::link::ref<char []>(halo::ui::vars().player_help_name_c10);
static auto &player_help_name_c20 = halo::link::ref<char []>(halo::ui::vars().player_help_name_c20);
static auto &player_help_name_c40 = halo::link::ref<char []>(halo::ui::vars().player_help_name_c40);
static auto &player_help_name_d20 = halo::link::ref<char []>(halo::ui::vars().player_help_name_d20);
static auto &player_help_name_d40 = halo::link::ref<char []>(halo::ui::vars().player_help_name_d40);
static auto &profile_carousel_slots = halo::link::ref<profile_carousel_slot [3]>(halo::ui::vars().profile_carousel_slots);
static auto &joystick_set_separator_0065f010 = halo::link::ref<char []>(halo::ui::vars().joystick_set_separator_0065f010);
static auto &empty_string = halo::link::ref<uint16_t []>(halo::game::vars().empty_string);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &directsound_initialized = halo::link::ref<uint8_t>(halo::ui::vars().directsound_initialized);
static auto &directsound_eax_available = halo::link::ref<uint8_t>(halo::ui::vars().directsound_eax_available);
static auto &sound_permutation_limit = halo::link::ref<uint16_t>(halo::ui::vars().sound_permutation_limit);
static auto &loading_thread_result = halo::link::ref<uint8_t>(halo::ui::vars().loading_thread_result);
static auto &playlist_profiles_need_defaults = halo::link::ref<uint8_t>(halo::ui::vars().playlist_profiles_need_defaults);
static auto &last_profile_name = halo::link::ref<char []>(halo::ui::vars().last_profile_name);
static auto &cached_profile_slot = halo::link::ref<int32_t>(halo::ui::vars().cached_profile_slot);
static auto &hud_text_unknown = halo::link::ref<uint16_t []>(halo::ui::vars().hud_text_unknown);
static auto &profile_globals_block = halo::link::ref<saved_player_profile_slot [k_maximum_local_player_profiles]>(halo::ui::vars().profile_globals_block);
static auto &port_overridden = halo::link::ref<uint8_t>(halo::ui::vars().port_overridden);
static auto &game_cport = halo::link::ref<uint32_t>(halo::ui::vars().game_cport);
static auto &network_session_start_game_type = halo::link::ref<uint32_t>(halo::networking::vars().network_session_start_game_type);
static auto &input_globals = halo::link::ref<player_control_settings []>(halo::main::vars().input_globals);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &saved_item_disk_copy = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().saved_item_disk_copy);
static auto &saved_item_working_copy = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().saved_item_working_copy);
static auto &virtual_keyboard = halo::link::ref<virtual_keyboard_globals>(halo::ui::vars().virtual_keyboard);
static auto &default_profile_data = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().default_profile_data);
static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);

namespace halo::interface {

namespace {

static const float k_table_80[10] = {
    80.0f, 100.0f, 120.0f, 140.0f, 160.0f, 180.0f, 200.0f, 220.0f, 240.0f, 260.0f
};

static const float k_table_40[10] = {
    40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f, 110.0f, 120.0f, 130.0f
};

static const float k_table_01[10] = {
    0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 3.0f, 4.0f
};

} // namespace

uint32_t PlayerProfiles::slider_index(uint8_t value)
{
    return (value < 10) ? value : 9;
}

/**
 * Resets the HUD runtime state and text language, then zeroes local player 0's entire
 * first_person_weapon_interface, re-seeding unit_index/unknown_1e98/unknown_1e9c to -1 and priming the shared
 * HUD text-draw color to the module's default (opaque white) with the globals interface_bitmaps' font_terminal
 * tag id.
 *
 * @address 0x494390
 */
void LocalPlayers::state_reset()
{
    first_person_weapon_interface *fp;
    GlobalsInterfaceBitmaps *interface_bitmaps;

    halo::interface::hud_state_reset();
    halo::text::text_context::language_initialize_from_string_list();

    fp = &first_person_weapon_interfaces[0];
    memset(fp, 0, sizeof(*fp));
    fp->unit_index = (datum_index)halo::k_dword_none;
    fp->frame_sound_index = -1;
    fp->frame_sound_state = -1;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_text_draw_font_tag_id = halo::interface::tag_handle(interface_bitmaps->font_terminal.tag_id);
    hud_text_draw_color_a = global_white_argb->alpha;
    hud_text_draw_color_r = global_white_argb->red;
    hud_text_draw_color_g = global_white_argb->green;
    hud_text_draw_color_b = global_white_argb->blue;
    hud_text_draw_color_or_flags = halo::k_word_none;
    halo::text::globals().hud_text_draw_column = 0;
    halo::text::globals().hud_text_draw_flags = 0;
}

/**
 * Finds which local player's first-person weapon interface currently has object_index attached as its weapon,
 * returning -1 if none matches.
 *
 * @address 0x4926f0
 */
int32_t LocalPlayers::index_for_object(datum_index object_index)
{
    int32_t i;

    for (i = 0; i < 1; i++) {
        if (first_person_weapon_interfaces[i].weapon_index == object_index &&
            first_person_weapon_interfaces[i].attached != 0) {
            break;
        }
    }
    if (i == 1) {
        return -1;
    }
    return i;
}

/**
 * Finds which local player currently controls unit_index, returning -1 if none does.
 *
 * @address 0x4940a0
 */
int32_t LocalPlayers::index_for_unit(datum_index unit_index)
{
    int32_t i;
    player *record;

    for (i = 0; i < 1; i++) {
        if (halo::game::globals().local_player_globals->local_players[i] == (datum_index)halo::k_dword_none) {
            continue;
        }
        record = halo::interface::player_record(halo::game::globals().local_player_globals->local_players[i]);
        if (record->unit == unit_index) {
            return i;
        }
    }
    return -1;
}

/**
 * Finds which local player currently has weapon_index equipped as its unit's active weapon, returning -1 if
 * none does.
 *
 * @address 0x494010
 */
int32_t LocalPlayers::index_for_weapon(datum_index weapon_index)
{
    int32_t i;
    player *record;
    unit_data *u;
    int16_t slot;

    for (i = 0; i < 1; i++) {
        if (halo::game::globals().local_player_globals->local_players[i] == (datum_index)halo::k_dword_none) {
            continue;
        }
        record = halo::interface::player_record(halo::game::globals().local_player_globals->local_players[i]);
        if (record->unit == (datum_index)halo::k_dword_none) {
            continue;
        }
        u = &halo::interface::object_record<unit_object>(record->unit)->unit;
        slot = u->current_weapon_index;
        if (slot != -1 && weapon_index == u->weapons[slot]) {
            return i;
        }
    }
    return -1;
}

/**
 * blam-cc: ECX -> player_index
 *
 * @address 0x4ab170
 */
datum_index LocalPlayers::get_vehicle(datum_index player_index)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)((uint32_t)player_index >> 16);
    player *p;
    object *unit;

    if (player_index == k_datum_index_none || index < 0 || index >= halo::game::globals().player_data->maximum_count) {
        return k_datum_index_none;
    }
    p = halo::interface::player_record(index);
    if (p->identifier == 0 || (salt != 0 && p->identifier != salt)) {
        return k_datum_index_none;
    }
    unit = halo::objects::object_try_and_get(p->unit, 3);
    if (unit == 0 || ((unit_object *)unit)->base.parent_object == k_datum_index_none ||
        ((unit_object *)unit)->unit.vehicle_seat_index == -1) {
        return k_datum_index_none;
    }
    return ((unit_object *)unit)->base.parent_object;
}

/**
 * Selects and opens a specific player-help screen tag based on matching the current profile's name against
 * hardcoded name lists, then stashes `value` into the opened widget's first text_box child (selection_index).
 *
 * @address 0x4991f0
 */
void LocalPlayers::help_screen_select_by_name(int16_t value)
{
    char name[256];
    char *p;
    const char *tag_path;
    widget_instance *dialog;

    if (halo::scenario::globals().scenario_index == k_datum_index_none) {
        return;
    }
    strncpy(name, halo::cache::globals().tag_instances[(int16_t)halo::scenario::globals().scenario_index].path, 0xff);

    for (p = name; *p != 0; p++) {
        *p = (char)tolower((uint8_t)*p);
    }

    if (strstr(name, player_help_name_a10) != 0) {
        tag_path = halo::tag_paths::player_help_screen_a10;
    } else if (strstr(name, player_help_name_a30) != 0) {
        tag_path = halo::tag_paths::player_help_screen_a30;
    } else if (strstr(name, player_help_name_a50) != 0) {
        tag_path = halo::tag_paths::player_help_screen_a50;
    } else if (strstr(name, player_help_name_b30) != 0) {
        tag_path = halo::tag_paths::player_help_screen_b30;
    } else if (strstr(name, player_help_name_b40) != 0) {
        tag_path = halo::tag_paths::player_help_screen_b40;
    } else if (strstr(name, player_help_name_c10) != 0) {
        tag_path = halo::tag_paths::player_help_screen_c10;
    } else if (strstr(name, player_help_name_c20) != 0) {
        tag_path = halo::tag_paths::player_help_screen_c20;
    } else if (strstr(name, player_help_name_c40) != 0) {
        tag_path = halo::tag_paths::player_help_screen_c40;
    } else if (strstr(name, player_help_name_d20) != 0) {
        tag_path = halo::tag_paths::player_help_screen_d20;
    } else if (strstr(name, player_help_name_d40) != 0) {
        tag_path = halo::tag_paths::player_help_screen_d40;
    } else {
        return;
    }

    dialog = halo::interface::chimera__load_ui_widget(tag_path, k_datum_index_none, (widget_instance *)0,
                                     (uint16_t)profile_slot_id[0], k_datum_index_none, k_datum_index_none, -1);
    if (dialog != (widget_instance *)0) {
        widget_instance *child;

        for (child = dialog->first_child; child != (widget_instance *)0 && child->widget_type != uiwidgettype_text_box;
             child = child->next_sibling) {
        }
        if (child != (widget_instance *)0) {
            child->selection_index = value;
        }
    }
}

/**
 * @address 0x4a6380
 */
void PlayerProfiles::one_wide_list_update(widget_instance *widget)
{
    for (;;) {
        widget_instance *name_row = widget->parent->first_child;
        widget_instance *description_row = name_row->next_sibling;
        int32_t *ids = (int32_t *)widget->list_items;
        int32_t profile_id = ids[widget->selection_index];

        halo::interface::ui_profile_carousel_slot_cache_populate(1, &profile_id);

        if (profile_id != -1) {
            int32_t slot;
            for (slot = 0; slot < 3 && profile_carousel_slots[slot].profile_id != profile_id; slot++) {
            }
            if (slot < 3) {
                const saved_player_profile *profile = (const saved_player_profile *)profile_carousel_slots[slot].profile;
                uint16_t flags = profile->flags;
                int16_t color = profile->player_color;
                uint16_t *name = halo::interface::widget_pool_resize_text(widget->list_render_data, 0x18);

                widget->list_render_data = name;
                if (name == 0) {
                    return;
                }
                if (flags & 1) {
                    datum_index names = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::default_player_profile_names);
                    const uint16_t *source = empty_string;
                    if (names != k_datum_index_none) {
                        source = halo::text::text_string_list_get_string(names, (int16_t)(flags >> 8));
                    }
                    wcsncpy((wchar_t *)name, (const wchar_t *)source, 0xb);
                } else {
                    wcsncpy((wchar_t *)name, (const wchar_t *)profile->name, 0xb);
                }
                name[0xb] = 0;

                name_row->background_bitmap_frame = (color < 0) ? 0 : (color > 0x11) ? 0x11 : color;

                description_row->text = halo::memory::heap_reallocate(description_row->text, halo::interface::k_description_text_bytes, widget_memory_pool);
                if (description_row->text == 0) {
                    return;
                }
                {
                    datum_index joysticks;
                    datum_index buttons;
                    if (halo::interface::has_bit(flags, halo::interface::profile_flag::builtin)) {
                        joysticks = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::joystick_set_defaults_descriptions);
                        buttons = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::button_set_long_descriptions);
                        if (joysticks == k_datum_index_none || buttons == k_datum_index_none) {
                            (halo::interface::widget_text(description_row))[0] = 0;
                            (halo::interface::widget_text(description_row))[0xff] = 0;
                            return;
                        }
                    } else {
                        joysticks = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::joystick_set_short_descriptions);
                        buttons = halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::button_set_short_descriptions);
                        if (joysticks == k_datum_index_none || buttons == k_datum_index_none) {
                            (halo::interface::widget_text(description_row))[0xff] = 0;
                            return;
                        }
                    }
                    {
                        uint16_t *joystick_text = halo::text::text_string_list_get_string(joysticks, profile->joystick_set);
                        uint16_t *button_text = halo::text::text_string_list_get_string(buttons, profile->button_set);
                        halo::text::string_format_wide_va_bounded(0xff, reinterpret_cast<uint16_t *>((wchar_t *)description_row->text), reinterpret_cast<const uint16_t *>(L"%s%hs%s"),
                                                      joystick_text, joystick_set_separator_0065f010, button_text);
                    }
                    (halo::interface::widget_text(description_row))[0xff] = 0;
                }
                return;
            }
        }

        if (widget->item_count == 0) {
            uint16_t *text = halo::interface::widget_pool_resize_text(widget->list_render_data, 4);
            widget->list_render_data = text;
            if (text != 0) {
                text[0] = 0;
            }
            name_row->background_bitmap_frame = 0;
            text = halo::interface::widget_pool_resize_text(description_row->text, 4);
            description_row->text = text;
            if (text != 0) {
                text[0] = 0;
            }
            return;
        }

        {
            int32_t count = widget->item_count;
            int32_t valid = 0;
            qsort(ids, (uint32_t)count, 4, (_CoreCrtNonSecureSearchSortCompareFunction)(halo::interface::ui_carousel_slot_compare_valid_first));
            while (valid < count && ids[valid] != -1) {
                valid++;
            }
            widget->item_count = (uint16_t)valid;
            if (widget->selection_index < 0) {
                widget->selection_index = 0;
            } else if (widget->selection_index > (int32_t)(uint16_t)valid - 1) {
                widget->selection_index = (int16_t)((uint16_t)valid - 1);
            }
        }
    }
}

/**
 * blam-cc: ESI -> settings Applies the profile's audio settings: three 0..10 volume sliders
 * (settings+0xb78..0xb7a, scaled to 0.0..1.0 and clamped) for the master/effects/music gains, an
 * environment/EAX id (settings+0xb7f), and an environment-enable call gated on hardware support plus
 * settings+0xb7c, carrying settings+0xb7b and settings+0xb7d along.
 *
 * @address 0x4957d0
 */
void PlayerProfiles::apply_audio_options(saved_player_profile *settings)
{
    float gain;
    int32_t environment_enabled;

    if (safe_mode != 0) {
        settings->master_volume = 10;
        settings->effects_volume = 10;
        settings->music_volume = 6;
        settings->hardware_acceleration = 0;
        settings->eax_enabled = 0;
        settings->sound_quality = 0;
        settings->unknown_b7e = 0;
        settings->sound_variety = 0;
    }

    gain = (float)settings->master_volume * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    halo::sound::sound_set_master_gain(gain);

    gain = (float)settings->effects_volume * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    halo::sound::sound_set_effects_gain(gain);

    gain = (float)settings->music_volume * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    halo::sound::sound_set_music_gain(gain);

    sound_permutation_limit = settings->sound_variety;
    if (directsound_initialized == 0 || directsound_eax_available == 0 ||
        settings->eax_enabled == 0) {
        environment_enabled = 0;
    } else {
        environment_enabled = 1;
    }
    halo::sound::sound_driver_set_quality(environment_enabled, settings->hardware_acceleration == 1, settings->sound_quality);
}

/**
 * Looks for an existing player profile at startup: tries a type-0 enumeration with flag 0 first and loads that
 * slot if it validates; otherwise (nothing found) tries flag 1 and, if that slot validates, loads its record
 * as the default (-1) profile. Does nothing if neither validates.
 *
 * @address 0x4952c0
 */
void PlayerProfiles::auto_select()
{
    uint16_t count;
    int32_t slot;
    uint8_t profile_data[k_saved_player_profile_size];

    count = 1;
    slot = -1;
    halo::saved_games::saved_game_enumerate_by_type(0, &slot, 0, &count);
    if (static_cast<int16_t>(count) > 0 && slot != -1) {
        if (halo::saved_games::player_profile_get(slot, (saved_player_profile *)profile_data) == 0) {
            return;
        }
        halo::interface::player_profile_load(0, profile_data, slot);
        return;
    }

    count = 1;
    halo::saved_games::saved_game_enumerate_by_type(0, &slot, 1, &count);
    if (static_cast<int16_t>(count) <= 0 || slot == -1) {
        return;
    }
    if (halo::saved_games::player_profile_get(slot, (saved_player_profile *)profile_data) == 0) {
        return;
    }
    halo::interface::player_profile_load(0, profile_data, -1);
}

/**
 * Checks storage availability, creates the on-disk default profiles the first time this is needed, refreshes
 * both saved-game enumeration lists, and resolves the cached profile slot from the last-used profile name if
 * it is not already known. Always returns 0.
 *
 * @address 0x49c680
 */
int32_t PlayerProfiles::check_storage_and_defaults()
{
    int32_t enumeration_scratch[1];

    loading_thread_result = (uint8_t)halo::saved_games::saved_game_check_storage_availability();
    if (loading_thread_result == 0) {
        if (playlist_profiles_need_defaults == 1) {
            halo::saved_games::playlist_profile_create_default_profiles_on_disk();
            playlist_profiles_need_defaults = 0;
        }
        {
            uint16_t count = 1;
            halo::saved_games::saved_game_enumerate_by_type(1, enumeration_scratch, 1, &count);
            halo::saved_games::saved_game_enumerate_by_type(0, enumeration_scratch, 1, &count);
        }
        if (last_profile_name[0] == '\0') {
            if (halo::saved_games::saved_game_last_profile_read((uint8_t *)last_profile_name) != 0) {
                cached_profile_slot = halo::saved_games::saved_game_find_by_name(last_profile_name, 0);
            }
        }
    }
    return 0;
}

/**
 * blam-cc: EAX -> widget, stack -> profile_record
 *
 * @address 0x4a6100
 */
void PlayerProfiles::details_widget_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *a = widget->first_child;
    widget_instance *b = a->next_sibling;
    widget_instance *d = b->next_sibling->first_child;
    widget_instance *e = d->next_sibling;
    widget_instance *f = e->next_sibling;
    widget_instance *g = f->next_sibling;
    widget_instance *h = g->next_sibling;
    widget_instance *i = h->next_sibling;
    widget_instance *j = i->next_sibling;
    int16_t sensitivity;

    if (profile_record == nullptr) {
        a->state = 0;
        b->background_bitmap_frame = 0x12;
        d->state = 1;
        e->state = 0;
        f->state = 0;
        g->state = 0;
        h->state = 0;
        i->state = 0;
        j->state = 0;
        return;
    }

    a->state = 1;
    d->state = 0;
    e->state = 1;
    f->state = 1;
    g->state = 1;
    h->state = 1;
    i->state = 1;
    j->state = 1;

    a->text = halo::memory::heap_reallocate(a->text, 0x18, widget_memory_pool);
    if (a->text != 0) {
        uint16_t flags = ((struct saved_player_profile *)profile_record)->flags;

        if ((flags & 1) != 0) {
            datum_index names_tag =
                halo::interface::lookup_tag(halo::groups::unicode_string_list, halo::tag_paths::default_player_profile_names);
            const uint16_t *source = names_tag != k_datum_index_none
                ? halo::text::text_string_list_get_string(names_tag, (int16_t)(flags >> 8))
                : hud_text_unknown;

            wcsncpy((wchar_t *)(halo::interface::widget_text(a)), (const wchar_t *)source, 0xb);
        } else {
            wcsncpy((wchar_t *)(halo::interface::widget_text(a)), (const wchar_t *)((const saved_player_profile *)profile_record)->name, 0xb);
        }
        (halo::interface::widget_text(a))[0xb] = 0;
    }

    sensitivity = ((struct saved_player_profile *)profile_record)->player_color;
    if (sensitivity < 0) {
        sensitivity = 0;
    } else if (sensitivity > 0x11) {
        sensitivity = 0x11;
    }
    b->background_bitmap_frame = sensitivity;

    if (halo::interface::has_bit(((struct saved_player_profile *)profile_record)->flags, halo::interface::profile_flag::builtin)) {
        f->state = 0;
        h->state = 0;
        return;
    }

    {
        int16_t type = 0;
        int16_t level = 0;
        int32_t next_level;

        halo::saved_games::player_profile_scan_campaign_progress(&type, (saved_player_profile *)((void *)profile_record), &level);
        next_level = level + 1;
        if (next_level > 9) {
            next_level = 9;
        }
        f->selection_index = (int16_t)next_level;
        h->selection_index = type;
        j->selection_index = ((struct saved_player_profile *)profile_record)->look_inverted == 1;
    }
}

/**
 * Returns 0 if `id` matches the (sole, retail-PC) profile slot's stored id, else -1. FIXED (objdump): every
 * ret sets only AX; the upper bits of EAX are left as they were
 *
 * @address 0x4954f0
 */
int16_t PlayerProfiles::find_index_by_id(int16_t id)
{
    if (profile_slot_id[0] == id) {
        return 0;
    }
    return -1;
}

/**
 * blam-cc: EDX -> id Finds the (sole, retail-PC) profile slot whose stored id matches `id` -- unless
 * network_client/network_server are not both clear, in which case the slot index defaults to `id` itself
 * -- and returns the flag byte at offset 0x132 of that slot's profile record, or 0 if no slot was found.
 *
 * @address 0x495a60
 */
uint8_t PlayerProfiles::get_flag_by_id(int16_t id)
{
    int32_t slot;

    slot = id;
    if (halo::networking::globals().client == nullptr && halo::networking::globals().server == nullptr) {
        slot = -1;
        if (profile_slot_id[0] == id) {
            slot = 0;
        }
    }
    if (slot != -1) {
        return profile_globals_block[slot].profile.auto_center_look;
    }
    return 0;
}

/**
 * blam-cc: AX -> player_index, EDX -> source_profile, stack -> profile_id Copies a loaded 0x1ffc byte profile
 * record into player_index's slot of the module's profile globals, records profile_id as that slot's profile
 * index, refreshes the settings cache and applies the video/audio settings, reopens the network channels if
 * the record's port pair differs from what is currently bound, and (for a real, non-default profile) updates
 * the cached profile slot bookkeeping and the last-profile marker.
 *
 * @address 0x495970
 */
void PlayerProfiles::load(int16_t player_index, void *source_profile, int32_t profile_id)
{
    saved_player_profile_slot *slot = &profile_globals_block[player_index];
    saved_player_profile *record = &slot->profile;

    slot->handle = profile_id;
    memcpy(record, source_profile, k_saved_player_profile_size);

    halo::interface::player_profile_refresh_settings_cache(player_index);
    halo::saved_games::control_profile_reestablish_device_slot_mappings(record);
    halo::interface::player_profile_apply_video_options(record);
    halo::interface::player_profile_apply_audio_options(record);

    if (halo::game::globals().current_engine == nullptr && port_overridden == 0 &&
        (halo::networking::globals().game_socket_port != record->server_port ||
         game_cport != record->client_port)) {
        halo::networking::network_channels_close();
        halo::networking::globals().game_socket_port = record->server_port;
        game_cport = record->client_port;
        halo::networking::network_channels_open();
        network_session_start_game_type = halo::networking::globals().game_socket_port;
    }

    if (profile_id != -1) {
        if (cached_profile_slot != halo::saved_games::globals().player_profile_slots_handle) {
            if (halo::saved_games::globals().player_profile_slots_handle != -1) {
                halo::saved_games::saved_game_get_directory_by_handle(halo::saved_games::globals().player_profile_slots_handle, last_profile_name);
            }
            cached_profile_slot = halo::saved_games::globals().player_profile_slots_handle;
        }
        if (last_profile_name[0] != '\0') {
            halo::saved_games::saved_game_last_profile_clear(last_profile_name);
        }
    }
}

/**
 * blam-cc: BX -> player_index Rebuilds player_index's row of input_globals from its raw profile record. The
 * destination row is profile_slot_id[player_index] if one is assigned, else player_index.
 *
 * @address 0x496060
 */
void PlayerProfiles::refresh_settings_cache(int16_t player_index)
{
    player_control_settings settings;
    saved_player_profile *profile = &profile_globals_block[player_index].profile;
    int32_t slider;
    int32_t dest_slot;
    int32_t i;

    memset(&settings, 0, sizeof(settings));

    slider = (int32_t)profile->look_sensitivity - 1;
    if (slider < 0) {
        slider = 0;
    } else if (slider > 9) {
        slider = 9;
    }
    settings.look_rate_80 = k_table_80[slider];
    settings.look_rate_40 = k_table_40[slider];

    memcpy(settings.keyboard, profile->keyboard_bindings, sizeof(settings.keyboard));
    memcpy(settings.mouse_button, profile->mouse_button_bindings, sizeof(settings.mouse_button) + sizeof(settings.mouse_axis));
    memcpy(settings.gamepad_button, profile->gamepad_button_bindings, sizeof(settings.gamepad_button));
    memcpy(settings.gamepad_action_button, profile->gamepad_action_buttons, sizeof(settings.gamepad_action_button));
    memcpy(settings.gamepad_axis, profile->gamepad_axis_bindings, sizeof(settings.gamepad_axis));
    memcpy(settings.gamepad_pov, profile->gamepad_pov_bindings, sizeof(settings.gamepad_pov));
    memcpy(&settings.forward_rate, &profile->forward_rate, 6 * sizeof(float));

    settings.mouse_look_x_sensitivity = k_table_01[slider_index(profile->mouse_look_x_sensitivity)];
    settings.mouse_look_y_sensitivity = k_table_01[slider_index(profile->mouse_look_y_sensitivity)];

    memcpy(&settings.gamepad_axis_scale_x, &profile->gamepad_axis_scale_x, 2 * sizeof(float));

    for (i = 0; i < 4; i++) {
        settings.gamepad_rate_80[i] = k_table_80[slider_index(profile->gamepad_rate_a[i])];
        settings.gamepad_rate_40[i] = k_table_40[slider_index(profile->gamepad_rate_b[i])];
    }

    settings.look_inverted = profile->look_inverted;
    settings.look_inverted_driving = profile->look_inverted_driving;

    dest_slot = profile_slot_id[player_index];
    if (profile_slot_id[player_index] == -1) {
        dest_slot = player_index;
    }
    input_globals[dest_slot] = settings;
}

/**
 * Saves the currently-selected saved profile (type 0) or game variant (type 1): a profile is written back to
 * its slot (or a "not saved" message for the default -1 profile) and reloaded as player 0's profile; a variant
 * is sanitized and persisted, first allocating a new slot if it was an in-memory-only (+0x40000000) variant
 * whose name differs from its disk copy. selected_saved_item is reset to -1 on every path.
 *
 * @address 0x495d40
 */
uint8_t PlayerProfiles::save()
{
    char name[halo::interface::k_text_buffer_chars];
    int32_t item;
    int32_t new_slot;
    uint8_t result;

    item = selected_saved_item;
    result = 0;

    if ((item & 0xf) == 0) {
        if (item == -1) {
            halo::main::console_out_printf(0, "profile not saved since it was a default profile");
        } else {
            halo::saved_games::player_profile_write_data(item, (saved_player_profile *)saved_item_working_copy);
        }
        halo::interface::player_profile_load(0, saved_item_working_copy, selected_saved_item);
        result = 1;
    } else if ((item & 0xf) == 1) {
        if ((item & halo::interface::k_saved_item_builtin_marker) == 0) {
            if (item != -1) {
                halo::game::game_variant_sanitize_options((game_variant *)saved_item_working_copy);
                halo::saved_games::game_variant_write_request_start(item, (game_variant *)saved_item_working_copy);
            }
            if (halo::saved_games::saved_game_get_directory_by_handle(selected_saved_item, name) != 0) {
                halo::saved_games::saved_game_last_mp_variant_clear(name);
            }
            result = 1;
        } else if (wcsncmp((wchar_t *)saved_item_working_copy, (wchar_t *)saved_item_disk_copy,
                           0x18) != 0) {
            ((game_variant *)saved_item_working_copy)->variant_flags &= ~1u;
            new_slot = halo::saved_games::saved_game_create_custom_variant(0, (uint16_t *)saved_item_working_copy);
            if (new_slot != -1) {
                halo::game::game_variant_sanitize_options((game_variant *)saved_item_working_copy);
                halo::saved_games::game_variant_write_request_start(new_slot, (game_variant *)saved_item_working_copy);
                selected_saved_item = new_slot;
                if (halo::saved_games::saved_game_get_directory_by_handle(new_slot, name) != 0) {
                    halo::saved_games::saved_game_last_mp_variant_clear(name);
                }
                result = 1;
            }
        }
    }

    selected_saved_item = -1;
    return result;
}

/**
 * @address 0x4a62c0
 */
void PlayerProfiles::select_list_widget_build_thunk(widget_instance *widget)
{
    halo::interface::player_profile_select_list_widget_build(widget);
}

/**
 * Returns true if a saved item is selected and its working copy differs from its on-disk copy (map: the full
 * 0x1ffc byte record; variant: just the 0x98 byte game_variant portion).
 *
 * @address 0x495ea0
 */
uint8_t SavedItem::has_unsaved_changes()
{
    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            return memcmp(saved_item_disk_copy, saved_item_working_copy,
                          sizeof(saved_item_disk_copy)) != 0;
        }
        if ((selected_saved_item & 0xf) == 1) {
            return memcmp(saved_item_disk_copy, saved_item_working_copy, sizeof(game_variant)) != 0;
        }
    }
    return 0;
}

/**
 * Returns 1 if the currently-selected saved map or variant's name differs from the name stored in its on-disk
 * copy, 0 if they match (or nothing is selected).
 *
 * @address 0x495c90
 */
int32_t SavedItem::name_changed()
{
    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            if (wcsncmp((wchar_t *)(saved_item_working_copy + 2),
                        (wchar_t *)(saved_item_disk_copy + 2), 0xc) != 0) {
                return 1;
            }
        } else if ((selected_saved_item & 0xf) == 1) {
            if (wcsncmp((wchar_t *)saved_item_working_copy, (wchar_t *)saved_item_disk_copy,
                        0x18) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Opens the virtual keyboard on the currently-selected saved item's name field: the map-name screen for a map,
 * or the variant-name screen for a variant, in which case a successful open also arms
 * saved_variant_name_valid. Returns the keyboard's own open result (map selection returns it directly; variant
 * selection always returns 0 unless the open succeeded, in which case it returns the keyboard's nonzero
 * result).
 *
 * @address 0x495cf0
 */
uint8_t SavedItem::name_edit_begin()
{
    uint8_t opened;

    if (selected_saved_item != -1) {
        if ((selected_saved_item & 0xf) == 0) {
            return halo::interface::virtual_keyboard_open((uint16_t *)(saved_item_working_copy + 2), 0x18, 10);
        }
        if ((selected_saved_item & 0xf) == 1) {
            opened = halo::interface::virtual_keyboard_open((uint16_t *)saved_item_working_copy, 0x30, 10);
            if (opened != 0) {
                virtual_keyboard.validation_mode = 2;
                return opened;
            }
            return 0;
        }
    }
    return 0;
}

/**
 * Returns 1 if a saved item is selected and `name` matches its on-disk map-name field, else 0.
 *
 * @address 0x495e70
 */
uint8_t SavedItem::name_matches(wchar_t *name)
{
    if (selected_saved_item != -1) {
        return wcscmp(name, (wchar_t *)(saved_item_disk_copy + 2)) == 0;
    }
    return 0;
}

/**
 * blam-cc: EBX -> item Selects saved item `item` (low nibble 0 = map, 1 = variant; -1 selects "nothing"/the
 * default) into the working copy: for a map, either resets to the compiled-in default template or, if `item`
 * validates, copies the whole 0x1ffc byte disk record over;
 *
 * @address 0x495be0
 */
void SavedItem::select(int32_t item)
{
    selected_saved_item = -1;

    if ((item & 0xf) == 0) {
        if (item == -1) {
            memcpy(saved_item_disk_copy, default_profile_data, sizeof(default_profile_data));
            return;
        }
        if (halo::saved_games::player_profile_get(item, (saved_player_profile *)saved_item_disk_copy) != 0) {
            memcpy(saved_item_working_copy, saved_item_disk_copy, sizeof(saved_item_working_copy));
            selected_saved_item = item;
        }
    } else if ((item & 0xf) == 1) {
        if (item == -1) {
            halo::game::game_engine_apply_current_custom_variant();
            return;
        }
        if (halo::saved_games::saved_game_get_variant(item, (game_variant *)saved_item_disk_copy) != 0) {
            memcpy(saved_item_working_copy, saved_item_disk_copy, sizeof(game_variant));
            ((game_variant *)saved_item_working_copy)->flags &= ~0x180u;
            selected_saved_item = item;
        }
    }
}

/**
 * If object_index's parent object (e.g. the vehicle or unit wielding it) is controlled by the current local
 * player, and that player's first-person weapon is attached, looks up object_index's first-person marker and
 * returns its raw node_transform position, forward (as extents) and up (as direction) vectors. Returns 0 on
 * any failed gate.
 *
 * @address 0x492c30
 */
uint8_t LocalPlayers::get_first_person_marker_transform(datum_index object_index, const char *marker_name, real_point3d *out_position, real_vector3d *out_extents, real_vector3d *out_direction)
{
    object_header *header;
    object *obj;
    object_header *parent_header;
    unit_data *parent_unit;
    datum_index controlling_player;
    player *p;
    int16_t local_player;
    first_person_weapon_interface *fp;
    object_marker marker;
    int16_t result;

    header = &((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask];
    obj = header->data;

    parent_header = &((object_header *)halo::objects::globals().object_data->data)[obj->parent_object & halo::k_slot_mask];
    parent_unit = &((unit_object *)parent_header->data)->unit;
    controlling_player = parent_unit->controlling_player;

    if (controlling_player == (datum_index)halo::k_dword_none) {
        return 0;
    }

    p = halo::interface::player_record(controlling_player);
    local_player = p->local_player_index;
    if (local_player == -1 || local_player != current_local_player_index) {
        return 0;
    }

    fp = &first_person_weapon_interfaces[local_player];
    if (fp->attached == 0) {
        return 0;
    }

    result = (int16_t)halo::interface::first_person_weapon_get_marker_data(object_index, marker_name, &marker, 1);
    if (result <= 0) {
        return 0;
    }

    *out_position = marker.node_transform.position;
    *out_extents = marker.node_transform.forward;
    *out_direction = marker.node_transform.up;
    return 1;
}

} // namespace halo::interface

namespace halo::interface {

void interface_local_player_state_reset(void)
{
    halo::interface::LocalPlayers::state_reset();
}

int32_t local_player_index_for_object(datum_index object_index)
{
    return halo::interface::LocalPlayers::index_for_object(object_index);
}

int32_t local_player_index_for_unit(datum_index unit_index)
{
    return halo::interface::LocalPlayers::index_for_unit(unit_index);
}

int32_t local_player_index_for_weapon(datum_index weapon_index)
{
    return halo::interface::LocalPlayers::index_for_weapon(weapon_index);
}

datum_index player_get_vehicle(datum_index player_index)
{
    return halo::interface::LocalPlayers::get_vehicle(player_index);
}

void player_help_screen_select_by_name(int16_t value)
{
    halo::interface::LocalPlayers::help_screen_select_by_name(value);
}

void player_profile_1wide_list_update(widget_instance *widget)
{
    halo::interface::PlayerProfiles::one_wide_list_update(widget);
}

void player_profile_apply_audio_options(saved_player_profile *settings)
{
    halo::interface::PlayerProfiles::apply_audio_options(settings);
}

void player_profile_auto_select(void)
{
    halo::interface::PlayerProfiles::auto_select();
}

int32_t player_profile_check_storage_and_defaults(void)
{
    return halo::interface::PlayerProfiles::check_storage_and_defaults();
}

void player_profile_details_widget_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    halo::interface::PlayerProfiles::details_widget_refresh(widget, profile_record);
}

int16_t player_profile_find_index_by_id(int16_t id)
{
    return halo::interface::PlayerProfiles::find_index_by_id(id);
}

uint8_t player_profile_get_flag_by_id(int16_t id)
{
    return halo::interface::PlayerProfiles::get_flag_by_id(id);
}

void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id)
{
    halo::interface::PlayerProfiles::load(player_index, source_profile, profile_id);
}

void player_profile_refresh_settings_cache(int16_t player_index)
{
    halo::interface::PlayerProfiles::refresh_settings_cache(player_index);
}

uint8_t player_profile_save(void)
{
    return halo::interface::PlayerProfiles::save();
}

void player_profile_select_list_widget_build_thunk(widget_instance *widget)
{
    halo::interface::PlayerProfiles::select_list_widget_build_thunk(widget);
}

uint8_t saved_item_has_unsaved_changes(void)
{
    return halo::interface::SavedItem::has_unsaved_changes();
}

int32_t saved_item_name_changed(void)
{
    return halo::interface::SavedItem::name_changed();
}

uint8_t saved_item_name_edit_begin(void)
{
    return halo::interface::SavedItem::name_edit_begin();
}

uint8_t saved_item_name_matches(wchar_t *name)
{
    return halo::interface::SavedItem::name_matches(name);
}

void saved_item_select(int32_t item)
{
    halo::interface::SavedItem::select(item);
}

uint8_t unit_get_first_person_marker_transform(datum_index object_index, const char *marker_name, real_point3d *out_position, real_vector3d *out_extents, real_vector3d *out_direction)
{
    return halo::interface::LocalPlayers::get_first_person_marker_transform(object_index, marker_name, out_position, out_extents, out_direction);
}

}

#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "saved_games.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Lookups from objects to local players and first person helpers.
 */
class LocalPlayers {
public:
    LocalPlayers() = delete;

    static void state_reset();
    static int32_t index_for_object(datum_index object_index);
    static int32_t index_for_unit(datum_index unit_index);
    static int32_t index_for_weapon(datum_index weapon_index);
    static datum_index get_vehicle(datum_index player_index);
    static void help_screen_select_by_name(int16_t value);
    static uint8_t get_first_person_marker_transform(datum_index object_index, const char *marker_name, real_point3d *out_position, real_vector3d *out_extents, real_vector3d *out_direction);
};

/**
 * Player profile storage, settings cache and option application.
 */
class PlayerProfiles {
public:
    PlayerProfiles() = delete;

    static void one_wide_list_update(widget_instance *widget);
    static void apply_audio_options(uint8_t *settings);
    static uint8_t apply_video_options(uint8_t *settings);
    static void auto_select();
    static int32_t check_storage_and_defaults();
    static void details_widget_refresh(widget_instance *widget, const uint8_t *profile_record);
    static int16_t find_index_by_id(int16_t id);
    static uint8_t get_flag_by_id(int16_t id);
    static void load(int16_t player_index, void *source_profile, int32_t profile_id);
    static void refresh_settings_cache(int16_t player_index);
    static uint8_t save();
    static void save_495fb0(uint8_t flag);
    static void select_list_widget_build(widget_instance *widget);
    static void select_list_widget_build_thunk(widget_instance *widget);
    static void subsystem_initialize();

private:
    static uint32_t slider_index(uint8_t value);
};

/**
 * Saved item name editing state.
 */
class SavedItem {
public:
    SavedItem() = delete;

    static uint8_t has_unsaved_changes();
    static int32_t name_changed();
    static uint8_t name_edit_begin();
    static uint8_t name_matches(wchar_t *name);
    static void select(int32_t item);
};

} // namespace halo::interface

#include "halo/interface/ifr2_players.hpp"
#include <string.h>

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t profile_globals_block[0x60a4];
extern int32_t saved_player_profile_slots_handle;
extern int16_t profile_slot_id[];
extern int32_t selected_saved_item;
extern uint8_t default_profile_data[0x1ffc];
extern uint8_t savegame_index_dirty;
extern char last_profile_name[];
extern int32_t cached_profile_slot;
extern int32_t safe_mode;
extern uint8_t profile_load_complete;
extern void player_profile_refresh_settings_cache(int16_t player_index);
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id);
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only, uint16_t *capacity_and_count);
extern void player_profile_initialize(void *profile_globals, int32_t unknown_0, int32_t unknown_1);
extern uint8_t player_profile_get(int32_t slot, void *out_profile);
extern uint8_t saved_game_last_profile_read(char *name_buffer);
extern int32_t saved_game_find_by_name(char *name, int32_t unknown);
extern void player_profile_set_default_video_options(void *profile_data, int32_t unknown);
extern void player_profile_set_default_audio_options(void *profile_data);
extern void player_profile_write_data(int32_t slot, void *profile_data);
extern void console_out_printf(uint8_t unknown, const char *format, ...);
}

namespace halo::interface {

/**
 * Initializes the player-profile subsystem at startup: clears the profile module's scratch block, resets the
 * current-profile globals, then picks a profile to load: the cached slot if player_profile_get accepts it,
 * else the first type-0 enumerated slot if the enumeration found one and it validates.
 *
 * @address 0x495370
 */
void PlayerProfiles::subsystem_initialize()
{
    int32_t enumerated_count;
    int32_t enumerated_slot;
    uint8_t profile_data[0x1ffc];
    int32_t slot_to_load;

    memset(profile_globals_block, 0, sizeof(profile_globals_block));
    player_profile_initialize(profile_globals_block, 0, 0);
    saved_player_profile_slots_handle = -1;
    profile_slot_id[0] = -1;
    player_profile_refresh_settings_cache(0);
    selected_saved_item = -1;

    enumerated_count = 1;
    enumerated_slot = -1;
    saved_game_enumerate_by_type(0, &enumerated_slot, 0, (uint16_t *)&enumerated_count);
    savegame_index_dirty = 1;

    if (last_profile_name[0] == '\0' && saved_game_last_profile_read(last_profile_name) != 0) {
        cached_profile_slot = saved_game_find_by_name(last_profile_name, 0);
    }

    slot_to_load = cached_profile_slot;
    if (slot_to_load == -1) {
        memcpy(profile_data, default_profile_data, sizeof(profile_data));
    } else if (player_profile_get(slot_to_load, profile_data) != 0) {
        goto have_slot;
    }

    if ((int16_t)enumerated_count <= 0 || enumerated_slot == -1 ||
        player_profile_get(enumerated_slot, profile_data) == 0) {
        profile_load_complete = 1;
        return;
    }
    slot_to_load = enumerated_slot;

have_slot:
    if (slot_to_load != -1) {
        if (safe_mode != 0) {
            player_profile_set_default_video_options(profile_data, 0);
            player_profile_set_default_audio_options(profile_data);
        }
        player_profile_load(0, profile_data, slot_to_load);
        if (safe_mode != 0) {
            if (enumerated_slot == -1) {
                console_out_printf(0, "profile not saved since it was a default profile");
                profile_load_complete = 1;
                return;
            }
            player_profile_write_data(enumerated_slot, profile_data);
        }
    }
    profile_load_complete = 1;
}

} // namespace halo::interface

extern "C" {

void player_profile_subsystem_initialize(void)
{
    halo::interface::PlayerProfiles::subsystem_initialize();
}

}

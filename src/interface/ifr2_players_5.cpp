#include "halo/interface/ifr2_players.hpp"
#include <string.h>
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/interface/api.hpp"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"

#ifdef interface
#undef interface
#endif

static auto &profile_globals_block = halo::link::ref<uint8_t [0x60a4]>(halo::ui::vars().profile_globals_block);
static auto &profile_slot_id = halo::link::ref<int16_t []>(halo::ui::vars().profile_slot_id);
static auto &selected_saved_item = halo::link::ref<int32_t>(halo::ui::vars().selected_saved_item);
static auto &default_profile_data = halo::link::ref<uint8_t [k_saved_player_profile_size]>(halo::ui::vars().default_profile_data);
static auto &savegame_index_dirty = halo::link::ref<uint8_t>(halo::saved_games::vars().savegame_index_dirty);
static auto &last_profile_name = halo::link::ref<char []>(halo::ui::vars().last_profile_name);
static auto &cached_profile_slot = halo::link::ref<int32_t>(halo::ui::vars().cached_profile_slot);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);

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
    uint8_t profile_data[k_saved_player_profile_size];
    int32_t slot_to_load;

    memset(profile_globals_block, 0, sizeof(profile_globals_block));
    halo::saved_games::player_profile_initialize((saved_player_profile *)profile_globals_block, 0, 0);
    halo::saved_games::globals().player_profile_slots_handle = -1;
    profile_slot_id[0] = -1;
    halo::interface::player_profile_refresh_settings_cache(0);
    selected_saved_item = -1;

    enumerated_count = 1;
    enumerated_slot = -1;
    halo::saved_games::saved_game_enumerate_by_type(0, &enumerated_slot, 0, (uint16_t *)&enumerated_count);
    savegame_index_dirty = 1;

    if (last_profile_name[0] == '\0' && halo::saved_games::saved_game_last_profile_read((uint8_t *)last_profile_name) != 0) {
        cached_profile_slot = halo::saved_games::saved_game_find_by_name(last_profile_name, 0);
    }

    slot_to_load = cached_profile_slot;
    if (slot_to_load == -1) {
        memcpy(profile_data, default_profile_data, sizeof(profile_data));
    } else if (halo::saved_games::player_profile_get(slot_to_load, (saved_player_profile *)profile_data) != 0) {
        goto have_slot;
    }

    if ((int16_t)enumerated_count <= 0 || enumerated_slot == -1 ||
        halo::saved_games::player_profile_get(enumerated_slot, (saved_player_profile *)profile_data) == 0) {
        halo::saved_games::globals().profile_load_complete = 1;
        return;
    }
    slot_to_load = enumerated_slot;

have_slot:
    if (slot_to_load != -1) {
        if (safe_mode != 0) {
            halo::saved_games::player_profile_set_default_video_options((saved_player_profile *)profile_data, 0);
            halo::saved_games::player_profile_set_default_audio_options((saved_player_profile *)profile_data);
        }
        halo::interface::player_profile_load(0, profile_data, slot_to_load);
        if (safe_mode != 0) {
            if (enumerated_slot == -1) {
                halo::main::console_out_printf(0, "profile not saved since it was a default profile");
                halo::saved_games::globals().profile_load_complete = 1;
                return;
            }
            halo::saved_games::player_profile_write_data(enumerated_slot, (saved_player_profile *)profile_data);
        }
    }
    halo::saved_games::globals().profile_load_complete = 1;
}

} // namespace halo::interface

namespace halo::interface {

void player_profile_subsystem_initialize(void)
{
    halo::interface::PlayerProfiles::subsystem_initialize();
}

}

#include "halo/interface/ifr2_players.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/interface/engine_state.hpp"
#include "halo/cache/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t profile_globals_block[];
extern const uint16_t empty_string[];
extern const uint16_t missing_string_text[];
}

namespace halo::interface {

/**
 * blam-cc: AL -> flag Stashes `flag`, and, if a real profile is active: truncates the last entry of the
 * "ui\shell\strings\temp_strings" unicode_string_list tag if it has more than one string, broadcasts a HUD
 * message to local players, then either writes the profile to disk (for a real profile) or prints a "not
 * saved" message (for the default profile) before refreshing the settings cache either way.
 *
 * @address 0x495fb0
 */
void PlayerProfiles::save_495fb0(uint8_t flag)
{
    datum_index string_list_tag;
    int32_t *string_list_data;
    char *block;
    uint32_t length;
    const uint16_t *text;

    state::profile_slot_flag = flag;
    if (halo::saved_games::globals().player_profile_slots_handle != -1) {
        string_list_tag = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\shell\\strings\\temp_strings");

        text = empty_string;
        if (string_list_tag != (datum_index)halo::k_dword_none) {
            string_list_data = (int32_t *)halo::cache::globals().tag_instances[(uint16_t)string_list_tag].data;
            text = missing_string_text;
            if (string_list_data[0] > 1) {
                block = (char *)string_list_data[1];
                length = *(uint32_t *)(block + 0x14);
                if ((int32_t)length > 0) {
                    text = *(const uint16_t **)(block + 0x20);
                    *(int16_t *)(*(int32_t *)(block + 0x20) - 2 + (length & 0xfffffffe)) = 0;
                }
            }
        }
        halo::interface::hud_message_broadcast_to_local_players(text);
        if (halo::saved_games::globals().player_profile_slots_handle == -1) {
            halo::main::console_out_printf(0, "profile not saved since it was a default profile");
            halo::interface::player_profile_refresh_settings_cache(0);
            return;
        }
        halo::saved_games::player_profile_write_data(halo::saved_games::globals().player_profile_slots_handle, (saved_player_profile *)profile_globals_block);
    }
    halo::interface::player_profile_refresh_settings_cache(0);
}

} // namespace halo::interface

namespace halo::interface {

void player_profile_save_495fb0(uint8_t flag)
{
    halo::interface::PlayerProfiles::save_495fb0(flag);
}

}

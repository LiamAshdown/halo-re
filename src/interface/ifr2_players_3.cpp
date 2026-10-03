#include "halo/interface/ifr2_players.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t unknown_00712f07;
extern int32_t saved_player_profile_slots_handle;
extern tag_instance *tag_instances;
extern uint8_t profile_globals_block[];
extern datum_index tag_lookup(tag_group group, char *path);
extern void hud_message_broadcast_to_local_players(const uint16_t *text);
extern const uint16_t empty_string[];
extern const uint16_t missing_string_text[];
extern void player_profile_refresh_settings_cache(int16_t player_index);
extern void console_out_printf(uint8_t unknown, const char *format, ...);
extern void player_profile_write_data(int32_t slot, void *profile_data);
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

    unknown_00712f07 = flag;
    if (saved_player_profile_slots_handle != -1) {
        string_list_tag = tag_lookup(0x75737472, (char *)"ui\\shell\\strings\\temp_strings");

        text = empty_string;
        if (string_list_tag != (datum_index)0xffffffff) {
            string_list_data = (int32_t *)tag_instances[(uint16_t)string_list_tag].data;
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
        hud_message_broadcast_to_local_players(text);
        if (saved_player_profile_slots_handle == -1) {
            console_out_printf(0, "profile not saved since it was a default profile");
            player_profile_refresh_settings_cache(0);
            return;
        }
        player_profile_write_data(saved_player_profile_slots_handle, profile_globals_block);
    }
    player_profile_refresh_settings_cache(0);
}

} // namespace halo::interface

extern "C" {

void player_profile_save_495fb0(uint8_t flag)
{
    halo::interface::PlayerProfiles::save_495fb0(flag);
}

}

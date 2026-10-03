#include "halo/game/gamerest_cachemaps.hpp"
#include "interface.h"
#include "main.h"
#include "halo/cache/api.hpp"
#include <string.h>
#include "halo/saved_games/api.hpp"

extern "C" {
extern char *rasterizer_shader_file_name;
extern game_main_globals *main_game_globals;
extern main_globals main_globals_data;
extern int16_t local_player_count;
extern int32_t saved_player_profile_slots_handle;
extern int32_t cached_profile_slot;
extern char last_profile_name[];
extern void main_queue_cache_file_open(void);
extern void shell_display_fatal_error_dialog(uint32_t a, uint32_t b, uint32_t c);
}

namespace halo::game {

/**
 * Resolves `path`'s map cache slot and switches or queues loading of it if it is not already the
 * active cache file; only actually applies the state changes when `apply_state` is set.
 *
 * @address 0x45aea0
 */
void CacheFileMaps::switch_map_by_path(char *path, uint8_t apply_state)
{
    game_main_globals *globals;
    int16_t slot;

    strrchr(path, 0x5c);
    slot = halo::cache::cache_file_find_slot_by_name(path);
    if (slot == -1) {
        if (halo::cache::globals().map_download_in_progress == 0) {
        open_by_name:
            if (halo::cache::cache_file_open_by_name(path, apply_state) == 0) {
                if (apply_state == 0) {
                    return;
                }
                rasterizer_shader_file_name = path;
                shell_display_fatal_error_dialog(0x89, 0x7e, 1);
            }
        } else {
            if (halo::cache::cache_file_download_matches(path) == 0) {
                if (apply_state == 0) {
                    halo::cache::cache_file_download_stop();
                    main_queue_cache_file_open();
                } else {
                    halo::cache::cache_file_download_finish();
                }
            }
            if (halo::cache::globals().map_download_in_progress == 0) {
                goto open_by_name;
            }
        }
        globals = main_game_globals;
        if (apply_state == 0) {
            return;
        }
        main_game_globals->map_loading_in_progress = 0;
        *(uint32_t *)&globals->map_load_progress = 0x3f800000;
    }

    if (apply_state != 0) {
        main_globals_data.pending_cache_file_name[0] = 0;
        main_globals_data.cache_file_open_pending = 0;
        if (halo::cache::globals().map_download_in_progress != 0) {
            halo::cache::cache_file_download_finish();
        }
        if (local_player_count == 1) {
            if (cached_profile_slot != saved_player_profile_slots_handle) {
                if (saved_player_profile_slots_handle != -1) {
                    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, last_profile_name);
                }
                cached_profile_slot = saved_player_profile_slots_handle;
            }
            if (last_profile_name[0] != 0) {
                halo::saved_games::saved_game_last_profile_clear(last_profile_name);
            }
        }
    }
}

}  // namespace halo::game

extern "C" {

/**
 * C entry point for halo::game::CacheFileMaps::switch_map_by_path; forwards to the C++ implementation.
 * register convention: map path in EAX (in_EAX); a caller-supplied "apply the switch now" flag
 * in BL (unaff_BL).
 * // blam-cc: EAX -> path, EBX -> apply_state
 * blam-cc: EAX -> name, stack -> report_fatal_error (matches src/cache/cache_file_open_by_name.c)
 * blam-cc: EAX -> path, EBX -> apply_state
 *
 * @address 0x45aea0
 */
void cache_file_switch_map_by_path(char *path, uint8_t apply_state)
{
    halo::game::CacheFileMaps::switch_map_by_path(path, apply_state);
}

}

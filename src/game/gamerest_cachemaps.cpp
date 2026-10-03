#include "halo/game/gamerest_cachemaps.hpp"

extern "C" {
extern uint8_t map_download_in_progress;
extern char *rasterizer_shader_file_name;
extern game_main_globals *main_game_globals;
extern uint32_t unknown_00719979;
extern uint32_t unknown_00719774;
extern int16_t local_player_count;
extern int32_t saved_player_profile_slots_handle;
extern int32_t cached_profile_slot;
extern uint8_t last_profile_name;
extern char *strrchr(const char *s, int32_t c);
extern int16_t cache_file_find_slot_by_name(char *path);
extern uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error);
extern uint8_t cache_file_download_matches(char *path);
extern void cache_file_download_stop(void);
extern void cache_file_download_finish(void);
extern void main_queue_cache_file_open(void);
extern void saved_game_get_directory_by_handle(void);
extern void saved_game_last_profile_clear(void);
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
    slot = cache_file_find_slot_by_name(path);
    if (slot == -1) {
        if (map_download_in_progress == 0) {
        open_by_name:
            if (cache_file_open_by_name(path, apply_state) == 0) {
                if (apply_state == 0) {
                    return;
                }
                rasterizer_shader_file_name = path;
                shell_display_fatal_error_dialog(0x89, 0x7e, 1);
            }
        } else {
            if (cache_file_download_matches(path) == 0) {
                if (apply_state == 0) {
                    cache_file_download_stop();
                    main_queue_cache_file_open();
                } else {
                    cache_file_download_finish();
                }
            }
            if (map_download_in_progress == 0) {
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
        unknown_00719979 = 0;
        unknown_00719774 = 0;
        if (map_download_in_progress != 0) {
            cache_file_download_finish();
        }
        if (local_player_count == 1) {
            if (cached_profile_slot != saved_player_profile_slots_handle) {
                if (saved_player_profile_slots_handle != -1) {
                    saved_game_get_directory_by_handle();
                }
                cached_profile_slot = saved_player_profile_slots_handle;
            }
            if (last_profile_name != 0) {
                saved_game_last_profile_clear();
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

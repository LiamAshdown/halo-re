#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "rasterizer.h"
#include "interface.h"
#include "halo/hs/script_globals.hpp"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"
#include "halo/saved_games/layout.hpp"
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/hs/api.hpp"

static void copy_profile_block(saved_player_profile *destination, const saved_player_profile *source, size_t first_offset, size_t end_offset)
{
    memcpy((uint8_t *)destination + first_offset, (const uint8_t *)source + first_offset, end_offset - first_offset);
}

extern "C" {
extern network_thread_record *variant_write_thread;
extern variant_write_request variant_write_request_state;
extern int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter,
    network_thread_record **out_handle);
extern network_mutex_record *saved_game_files_mutex;
extern uint32_t player_color_table[k_player_color_count];
extern network_thread_record *player_profile_thread;
extern saved_player_profile default_profile_data;
extern uint32_t safe_mode;
extern uint32_t cpu_speed;
extern uint32_t physical_memory;
extern int32_t saved_player_profile_slots_handle;
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles];
extern void *memset(void *dest, int32_t value, uint32_t count);
extern void *memcpy(void *dest, const void *src, uint32_t count);
extern char unknown_00719779[];
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id);
extern char savegames_directory[0x100];
extern uint8_t savegame_index_read_slot(int32_t slot_index, saved_game_index_entry *out_entry);
extern uint8_t savegame_index_write_slot(int32_t slot_index, saved_game_index_entry *entry);
extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size);
extern uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path);
extern uint16_t empty_string[];
extern uint32_t rasterizer_device_version;
extern uint32_t video_memory;
extern uint8_t width640;
extern uint8_t rasterizer_decal_zbias_active(void);
extern uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out);
extern void display_mode_get_current(rasterizer_display_mode *out);
extern char default_player_profiles_directory[0x100];
extern int16_t default_game_variant_count;
extern uint8_t savegame_index_dirty;
extern char default_playlists_directory[0x100];
extern uint16_t missing_string_text[];
extern game_variant_defaults_proc default_game_variant_procs[k_default_game_variant_count];
}

/**
 * Fabricates a default profile into profile and names it from the display name of the saved-game
 * handle. No disk access.
 */
static void player_profile_build_default(saved_player_profile *profile, int32_t handle)
{
    uint16_t *name;

    halo::saved_games::player_profile_initialize(profile, 0, 0);
    name = halo::saved_games::saved_game_get_display_name(handle);
    wcsncpy((wchar_t *)profile->name, (const wchar_t *)name, k_player_profile_name_length - 1);
}

/**
 * Waits on saved_game_files_mutex, then opens request->handle's saved-game file, crcs and writes
 * request->variant over its body, closes it, renames the slot to match the (possibly just
 * written) variant's name if the close succeeded, and deletes the slot if the seek or write
 * failed. Always releases the mutex before returning. Return value is always 0.
 *
 * @address 0x0053c150
 */
uint32_t halo::saved_games::VariantWriteRequest::thread_proc()
{
    variant_write_request *request = self;
    uint32_t wait_result;
    file_reference_record ref;
    game_variant_file file;
    uint8_t opened;
    uint8_t seeked;
    uint8_t written;
    uint8_t closed;
    uint8_t write_failed;

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result != win32::k_wait_object_0 && wait_result != win32::k_wait_abandoned) {
        return 0;
    }
    write_failed = 0;
    opened = halo::saved_games::saved_game_open_file_by_handle(request->handle, &ref);
    if (opened != 0) {
        file.variant = request->variant;
        file.checksum = k_crc32_seed;
        halo::memory::crc32_update(&file.checksum, &file.variant, sizeof(file.variant));
        seeked = halo::saved_games::file_reference_seek(0, &ref);
        if (seeked == 0) {
            write_failed = 1;
        } else {
            written = halo::saved_games::file_reference_write(&ref, &file, sizeof(file));
            if (written == 0) {
                write_failed = 1;
            }
        }
        closed = halo::saved_games::file_reference_close(&ref);
        if (closed != 0) {
            halo::saved_games::player_profile_rename(request->handle, request->variant.name);
        }
        if (write_failed != 0) {
            halo::saved_games::saved_game_delete_by_handle(request->handle);
        }
    }
    ReleaseMutex(saved_game_files_mutex->handle);
    return 0;
}

/**
 * index >= 0: fabricates a fresh default profile named from index's display name, into
 * *out_buffer, always succeeding (no disk access). index == -1 (or any negative): joins any
 * pending profile-verification thread, then, holding saved_game_files_mutex, reads and
 * crc/version-validates the real default profile file (handle -1), falling back to a
 * fabricated default profile on any read or validation failure. Returns success.
 *
 * @address 0x0053a770
 */
uint8_t halo::saved_games::PlayerProfile::get(int32_t index)
{
    saved_player_profile *out_buffer = self;
    uint8_t result;
    file_reference_record ref;
    saved_player_profile_file file;
    uint32_t running_crc;
    uint32_t wait_result;

    result = 0;

    if (player_profile_thread != 0) {
        uint32_t exit_code;
        do {
            while (GetExitCodeThread(player_profile_thread->handle, (LPDWORD)&exit_code) == 0) {
            }
        } while (exit_code == win32::k_still_active );
        CloseHandle(player_profile_thread->handle);
        player_profile_thread->handle = 0;
        player_profile_thread->in_use = 0;
        player_profile_thread = 0;
    }

    if (0 <= index) {
        player_profile_build_default(out_buffer, index);
        return 1;
    }

    wait_result = WaitForSingleObject(saved_game_files_mutex->handle, 5000);
    if (wait_result == win32::k_wait_object_0 || wait_result == win32::k_wait_abandoned ) {
        if (halo::saved_games::saved_game_open_file_by_handle(index, &ref) != 0) {
            if (halo::saved_games::file_reference_read(&ref, &file, sizeof(file)) != 0) {
                running_crc = k_crc32_seed;
                halo::memory::crc32_update(&running_crc, (uint8_t *)&file.profile, k_saved_player_profile_size);
                if (running_crc == file.checksum && file.profile.version == k_saved_player_profile_version) {
                    *out_buffer = file.profile;
                } else {
                    player_profile_build_default(out_buffer, index);
                }
                result = 1;
            }
            halo::saved_games::file_reference_close(&ref);
        }
        ReleaseMutex(saved_game_files_mutex->handle);
    }
    return result;
}

/**
 * Returns the cached default profile data for handle -1 and delegates to player_profile_get for
 * real profile handles. The result is written to out_buffer.
 *
 * @address 0x00539bc0
 */
uint8_t halo::saved_games::PlayerProfile::get_or_cached_default(int32_t index)
{
    saved_player_profile *out_buffer = self;
    if (index == -1) {
        *out_buffer = default_profile_data;
        return 0;
    }
    return halo::saved_games::player_profile_get(index, out_buffer);
}

/**
 * Builds a fresh default profile in place: zeroes it, sets identity/version/flags from
 * local_player_index, resets every control binding to unbound plus a set of default bindings,
 * applies default video options, sets audio/network/misc defaults, and -- when merge_existing
 * is set -- copies the controls/video/audio/extra-settings/gamepad blocks from an existing
 * profile (the current local slot if one is active, otherwise the first enumerated real profile
 * saved game, falling back to leaving the freshly-built defaults alone if neither is available)
 * on top of the freshly-built defaults. Finally, if a map is loaded, fills any empty gamepad
 * slots with default device profiles.
 *
 * @address 0x0053a1c0
 */
void halo::saved_games::PlayerProfile::initialize(int32_t local_player_index, uint8_t merge_existing)
{
    saved_player_profile *profile = self;
    saved_player_profile existing;
    uint16_t capacity;
    int32_t handles[1];
    int32_t handle;

    memset(profile, 0, sizeof(*profile));
    profile->version = k_saved_player_profile_version;
    profile->player_color = -1;
    profile->unknown_130 = 0;
    profile->look_sensitivity = 3;
    profile->look_inverted = 0;
    profile->look_inverted_driving = 0;
    profile->flags |= ((uint16_t)(uint8_t)local_player_index << 8) | 1;
    profile->unknown_133 = 0;

    {
        int32_t gp, i, j;
        for (i = 0; i < k_control_keyboard_key_count; i = i + 1) {
            profile->keyboard_bindings[i] = k_control_binding_unbound;
        }
        for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
            profile->mouse_button_bindings[i] = k_control_binding_unbound;
        }
        for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
            profile->mouse_axis_bindings[i][0] = k_control_binding_unbound;
            profile->mouse_axis_bindings[i][1] = k_control_binding_unbound;
        }
        for (gp = 0; gp < k_control_gamepad_count; gp = gp + 1) {
            for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
                profile->gamepad_button_bindings[gp][i] = k_control_binding_unbound;
            }
            profile->gamepad_action_buttons[gp][0] = -1;
            profile->gamepad_action_buttons[gp][1] = -1;
            for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
                profile->gamepad_axis_bindings[gp][i][0] = k_control_binding_unbound;
                profile->gamepad_axis_bindings[gp][i][1] = k_control_binding_unbound;
            }
            for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
                for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
                    profile->gamepad_pov_bindings[gp][i][j] = k_control_binding_unbound;
                }
            }
        }
    }

    halo::saved_games::control_profile_reset_digital_bindings(profile);
    halo::saved_games::control_profile_reset_analog_bindings(profile);

    profile->forward_rate = 1.0f;
    profile->strafe_rate = 1.0f;
    profile->look_x_rate = 0.188495576f;
    profile->look_y_rate = 0.188495576f;
    profile->mouse_forward_scale = 128.0f;
    profile->mouse_strafe_scale = 128.0f;
    profile->mouse_look_x_sensitivity = 3;
    profile->mouse_look_y_sensitivity = 3;
    profile->look_inverted = 0;

    halo::saved_games::player_profile_set_default_video_options(profile, (uint8_t)merge_existing);

    if (safe_mode == 0 && k_fast_machine_cpu_speed_mhz < cpu_speed && k_fast_machine_physical_memory_mb < physical_memory) {
        profile->sound_quality = 1;
        profile->sound_variety = 2;
    } else {
        profile->sound_quality = 0;
        profile->sound_variety = 1;
    }
    profile->master_volume = 10;
    profile->server_browser_sort_ascending = 1;
    profile->effects_volume = 10;
    profile->music_volume = 6;
    profile->hardware_acceleration = 0;
    profile->server_browser_sort_column = 3;
    profile->eax_enabled = 0;
    profile->server_browser_allow_password = 1;
    profile->server_browser_dedicated_only = 0;
    profile->server_browser_classic_only = 0;
    profile->server_browser_allow_unknown_map = 0;
    profile->server_browser_allow_empty = 1;
    profile->server_browser_allow_full = 1;
    profile->server_browser_game_type = 0;
    profile->server_browser_team_play = 0;
    profile->server_browser_ping_limit = 0;

    profile->server_name[0] = 'H'; profile->server_name[1] = 'a';
    profile->server_name[2] = 'l'; profile->server_name[3] = 'o'; profile->server_name[4] = 0;
    profile->server_password[0] = 0;
    profile->gamepad_axis_scale_x = 0.75f;
    profile->gamepad_axis_scale_y = 0.75f;
    profile->unknown_ebe = 0;
    profile->server_maximum_players_index = 3;
    profile->join_server_address[0] = 0;
    profile->connection_type = 1;
    profile->server_port = k_default_server_port;
    profile->client_port = k_default_client_port;

    profile->gamepad_rate_a[0] = 3; profile->gamepad_rate_b[0] = 3;
    profile->gamepad_rate_a[1] = 3; profile->gamepad_rate_b[1] = 3;
    profile->gamepad_rate_a[2] = 3; profile->gamepad_rate_b[2] = 3;
    profile->gamepad_rate_a[3] = 3; profile->gamepad_rate_b[3] = 3;

    profile->last_campaign_level = 0;
    if (local_player_index == 0) {
        profile->button_set = 0;
        profile->joystick_set = 0;
    } else if (local_player_index == 1) {
        profile->look_inverted = 1;
        profile->button_set = 0;
        profile->joystick_set = 0;
    }

    if (merge_existing != 0) {
        uint8_t have_existing = 0;

        if (saved_player_profile_slots_handle == -1) {
            capacity = 1;
            handles[0] = -1;
            halo::saved_games::saved_game_enumerate_by_type(_saved_game_type_player_profile, handles, 0, &capacity);
            handle = handles[0];
            if (capacity >= 1 && handle != -1 && halo::saved_games::player_profile_get(handle, &existing) != 0) {
                have_existing = 1;
            }
        } else {
            existing = profile_globals_block[0].profile;
            have_existing = 1;
        }

        if (have_existing) {
            copy_profile_block(profile, &existing, offsetof(saved_player_profile, button_set), offsetof(saved_player_profile, screen_width));
            copy_profile_block(profile, &existing, offsetof(saved_player_profile, master_volume), offsetof(saved_player_profile, server_browser_sort_column));
            copy_profile_block(profile, &existing, offsetof(saved_player_profile, screen_width), offsetof(saved_player_profile, master_volume));
            copy_profile_block(profile, &existing, offsetof(saved_player_profile, server_browser_sort_column), offsetof(saved_player_profile, unknown_d8b));
            copy_profile_block(profile, &existing, offsetof(saved_player_profile, gamepads), offsetof(saved_player_profile, unknown_1988));
        }
    }

    if (halo::cache::globals().cache_file_index != -1) {
        halo::saved_games::control_profile_fill_default_gamepad_slots(profile);
    }
}

/**
 * Phase 4 review (objdump 0x539bf0..0x539c13): profile is the one stack argument
 * (mov ecx,[esp+4] / push ecx / push eax at 0x539c05), forwarded with the EAX handle.
 *
 * @address 0x00539bf0
 */
void halo::saved_games::PlayerProfile::save_539bf0(int32_t handle)
{
    saved_player_profile *profile = self;
    if (handle == -1) {
        halo::main::console_out_printf(0, "profile not saved since it was a default profile");
        return;
    }
    halo::saved_games::player_profile_write_data(handle, profile);
}

/**
 * Scans the ten per-level campaign progress bytes of the profile and reports the last active
 * entry: its type through out_type and its level index through out_level.
 *
 * @address 0x00539e00
 */
void halo::saved_games::PlayerProfile::scan_campaign_progress(int16_t *out_type, int16_t *out_level)
{
    saved_player_profile *profile = self;
    int32_t i;
    uint8_t progress;

    *out_level = -1;
    *out_type = 1;

    for (i = 0; i < k_campaign_level_count; i = i + 1) {
        progress = profile->campaign_progress[i];
        if (progress != 0) {
            if ((progress & 8) != 0) {
                *out_level = (int16_t)i;
                *out_type = 3;
            } else if ((progress & 4) != 0) {
                *out_level = (int16_t)i;
                *out_type = 2;
            } else if ((progress & 2) != 0) {
                *out_level = (int16_t)i;
                *out_type = 1;
            } else if ((progress & 1) != 0) {
                *out_level = (int16_t)i;
                *out_type = 0;
            }
        }
    }
}

/**
 * Writes the default audio options (master, effects and music volume and the related bytes)
 * into the profile, depending on the machine class.
 *
 * @address 0x0053b240
 */
uint8_t halo::saved_games::PlayerProfile::set_default_audio_options()
{
    saved_player_profile *profile = self;
    if (safe_mode == 0 && k_fast_machine_cpu_speed_mhz < cpu_speed && k_fast_machine_physical_memory_mb < physical_memory) {
        profile->sound_quality = 1;
        profile->sound_variety = 2;
    } else {
        profile->sound_quality = 0;
        profile->sound_variety = 1;
    }
    profile->unknown_b7e = 0;
    profile->eax_enabled = 0;
    profile->hardware_acceleration = 0;
    profile->music_volume = 6;
    profile->effects_volume = 10;
    profile->master_volume = 10;
    return 1;
}

/**
 * Writes the default server name, password and game options into the profile, as used when
 * a server is started without an existing profile.
 *
 * @address 0x0053a150
 */
void halo::saved_games::PlayerProfile::set_default_server_options()
{
    saved_player_profile *profile = self;
    wcslen(L"Halo");
    wcscpy((wchar_t *)profile->server_name, L"Halo");
    wcslen((const wchar_t *)empty_string);
    wcscpy((wchar_t *)profile->server_password, L"");
    profile->unknown_ebe = 0;
    profile->server_maximum_players_index = 3;
    profile->join_server_address[0] = 0;
    profile->connection_type = 1;
    profile->server_port = k_default_server_port;
    profile->client_port = k_default_client_port;
}

/**
 * Fills a profile's default video settings. On a low-end machine (any of four capability
 * thresholds unmet), hardcodes 640x480x60 with minimal capability flags. On a capable machine:
 * honors a "-vidmode" command-line override if present, else queries the current display mode
 * (if allow_display_query) or falls back to 800x600x60.
 *
 * @address 0x0053b000
 */
uint8_t halo::saved_games::PlayerProfile::set_default_video_options(uint8_t allow_display_query)
{
    saved_player_profile *profile = self;
    uint8_t gamma;
    int32_t override_width;
    int32_t override_height;
    long override_refresh;
    rasterizer_display_mode mode;

    gamma = (uint8_t)halo::rasterizer::globals().gamma_exponent;
    profile->unknown_a6e = 2;
    profile->unknown_a75 = 2;
    profile->gamma = (int8_t)gamma;
    if (gamma == 0) {
        gamma = 1;
    } else if ((int8_t)gamma == -1) {
        gamma = (uint8_t)-2;
    }
    profile->gamma = (int8_t)gamma;

    if (safe_mode != 0 || rasterizer_device_version < k_pixel_shader_version_1_1 ||
        cpu_speed < k_fast_machine_cpu_speed_mhz + 1 || physical_memory < k_fast_machine_physical_memory_mb + 1 || video_memory < k_fast_machine_video_memory_bytes + 1) {
        profile->specular = 0;
        profile->shadows = 0;
        profile->decals = 0;
        profile->particles = safe_mode == 0;
        profile->texture_quality = 1;
        profile->screen_width = k_default_low_screen_width;
        profile->screen_height = k_default_low_screen_height;
        profile->refresh_rate = k_default_refresh_rate;
        profile->frame_rate_mode = 2;
        return 1;
    }

    profile->specular = halo::shell::globals().disable_specular == 0;
    profile->shadows = 1;
    profile->decals = halo::rasterizer::rasterizer_decal_zbias_active() != 0;
    profile->texture_quality = 2;
    profile->particles = 2;
    profile->frame_rate_mode = 2;

    if (width640 != 0) {
        profile->screen_width = k_default_low_screen_width;
        profile->screen_height = k_default_low_screen_height;
        profile->refresh_rate = k_default_refresh_rate;
        return 1;
    }

    override_width = -1;
    override_height = -1;
    override_refresh = -1;
    if (halo::rasterizer::rasterizer_parse_vidmode_commandline(&override_width, &override_height, &override_refresh) == 0) {
        if (allow_display_query == 0) {
            profile->screen_width = 800;
            profile->screen_height = 600;
            profile->refresh_rate = k_default_refresh_rate;
            return 1;
        }
        halo::rasterizer::display_mode_get_current(&mode);
        profile->screen_height = (int16_t)mode.height;
        profile->screen_width = (int16_t)mode.width;
        profile->refresh_rate = (int16_t)mode.refresh_rate;
        profile->frame_rate_mode = 0;
        if (mode.vsync != 0) {
            profile->frame_rate_mode = (halo::hs::fields::framerate_throttle != 0) + 1;
            return 1;
        }
    } else {
        profile->refresh_rate = k_default_refresh_rate;
        if (override_width != -1 && override_height != -1) {
            profile->screen_width = (int16_t)override_width;
            profile->screen_height = (int16_t)override_height;
        }
        if (override_refresh != -1) {
            profile->refresh_rate = (int16_t)override_refresh;
            return 1;
        }
    }
    return 1;
}

/**
 * Checksums `profile` and writes it to the save slot named by `handle`. On a successful close,
 * re-syncs the slot's name/index via player_profile_rename(profile->name) (keeps the on-disk
 * slot name in step with a profile whose name field changed since it was created). Rolls the
 * slot back (deletes it) if the seek or write failed.
 *
 * @address 0x0053a950
 */
void halo::saved_games::PlayerProfile::write_data(int32_t handle)
{
    saved_player_profile *profile = self;
    file_reference_record ref;
    saved_player_profile_file file;
    uint8_t write_failed;

    write_failed = 0;
    if (halo::saved_games::saved_game_open_file_by_handle(handle, &ref) == 0) {
        return;
    }

    file.profile = *profile;
    file.checksum = k_crc32_seed;
    halo::memory::crc32_update(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

    if (halo::saved_games::file_reference_seek(0, &ref) == 0 || halo::saved_games::file_reference_write(&ref, &file, sizeof(file)) == 0) {
        write_failed = 1;
    }
    if (halo::saved_games::file_reference_close(&ref) != 0) {
        halo::saved_games::player_profile_rename(handle, profile->name);
    }
    if (write_failed != 0) {
        halo::saved_games::saved_game_delete_by_handle(handle);
    }
}

namespace halo::saved_games::variant {

/**
 * Waits for and clears any in-flight game-variant writer thread, fills
 * variant_write_request_state.handle/variant from the arguments, then starts
 * game_variant_write_thread_proc as a new thread over it.
 *
 * @address 0x0053c0b0
 */
void write_request_start(int32_t handle, game_variant *variant)
{
    uint32_t exit_code;

    if (variant_write_thread != 0) {
        do {
            do {
            } while (GetExitCodeThread(variant_write_thread->handle, (LPDWORD)&exit_code) == 0);
        } while (exit_code == win32::k_still_active);
        CloseHandle(variant_write_thread->handle);
        variant_write_thread->handle = 0;
        variant_write_thread->in_use = 0;
        variant_write_thread = 0;
    }
    variant_write_request_state.handle = handle;
    variant_write_request_state.variant = *variant;
    network_thread_create(0, (void *)halo::saved_games::game_variant_write_thread_proc, &variant_write_request_state,
        &variant_write_thread);
}

}  // namespace halo::saved_games::variant

namespace halo::saved_games::player_color {

/**
 * Looks up a player color index (clamped to 0..k_player_color_count-1) in player_color_table
 * and converts the packed 0x00RRGGBB entry to a normalized (0..1) float RGB triple.
 * EAX (out_rgb) is never overwritten (mov ecx,eax at 0x539c77), so callers such as
 * src/game/game_engine_get_player_color.c use it as the return value.
 *
 * @address 0x00539c20
 */
float *get_rgb(float *out_rgb, int32_t color_index)
{
    uint32_t packed;

    if (color_index > k_player_color_count - 2) {
        color_index = k_player_color_count - 1;
    }
    if (color_index < 0) {
        color_index = 0;
    }
    packed = player_color_table[color_index];

    out_rgb[0] = (float)((packed >> 16) & 0xff) * k_color_byte_scale;
    out_rgb[1] = (float)((packed >> 8) & 0xff) * k_color_byte_scale;
    out_rgb[2] = (float)(packed & 0xff) * k_color_byte_scale;
    return out_rgb;
}

}  // namespace halo::saved_games::player_color

namespace halo::saved_games::player_profile {

/**
 * Copies blam.sav, savegame.bin (and, alongside it, a same-stem "*.sav" file -- see the header
 * note above) and every checkpoints\*.sav and checkpoints\*.bin file from source_dir into
 * dest_dir. Stops at the first failed copy; a failed savegame.sav copy skips both checkpoint
 * phases and a failed checkpoints\*.sav copy skips the *.bin phase. Returns the blam.sav /
 * savegame.bin copy result (0 if either failed), never the later ones.
 *
 * @address 0x0053cb70
 */
uint32_t copy_files(const char *source_dir, char *dest_dir)
{
    char dest_path[0x100];
    char source_path[0x100];
    char search_path[0x100];
    uint8_t result;
    uint8_t copy_ok;
    char *dot;
    void *find_handle;
    win32_find_dataa find_data;

    _snprintf(dest_path, k_path_maximum_length, "%s%s", dest_dir, k_player_profile_file_name);
    _snprintf(source_path, k_path_maximum_length, "%s%s", source_dir, k_player_profile_file_name);
    dest_path[0xff] = '\0';
    source_path[0xff] = '\0';
    result = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (result == 0) {
        return 0;
    }

    _snprintf(dest_path, k_path_maximum_length, "%s%s", dest_dir, k_game_state_file_name);
    _snprintf(source_path, k_path_maximum_length, "%s%s", source_dir, k_game_state_file_name);
    dest_path[0xff] = '\0';
    source_path[0xff] = '\0';
    result = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (result == 0) {
        return 0;
    }

    dot = strrchr(dest_path, '.');
    if (dot != 0) {
        *dot = '\0';
    }
    strcat(dest_path, ".sav");
    dot = strrchr(source_path, '.');
    if (dot != 0) {
        *dot = '\0';
    }
    strcat(source_path, ".sav");
    copy_ok = (uint8_t)CopyFileA(source_path, dest_path, 0);
    if (copy_ok == 0) {
        return result;
    }

    _snprintf(search_path, k_path_maximum_length, "%scheckpoints\\*.sav", source_dir);
    find_handle = FindFirstFileA(search_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)-1) {
        do {
            _snprintf(dest_path, k_path_maximum_length, "%scheckpoints\\%s", dest_dir, find_data.cFileName);
            _snprintf(source_path, k_path_maximum_length, "%scheckpoints\\%s", source_dir, find_data.cFileName);
            copy_ok = (uint8_t)CopyFileA(source_path, dest_path, 0);
            if (copy_ok == 0) {
                break;
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0);
        FindClose(find_handle);
    }
    if (copy_ok == 0) {
        return result;
    }

    _snprintf(search_path, k_path_maximum_length, "%scheckpoints\\*.bin", source_dir);
    find_handle = FindFirstFileA(search_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)-1) {
        do {
            _snprintf(dest_path, k_path_maximum_length, "%scheckpoints\\%s", dest_dir, find_data.cFileName);
            _snprintf(source_path, k_path_maximum_length, "%scheckpoints\\%s", source_dir, find_data.cFileName);
            if ((uint8_t)CopyFileA(source_path, dest_path, 0) == 0) {
                break;
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0);
        FindClose(find_handle);
    }
    return result;
}

/**
 * Selects the given local player slot like player_profile_select_local_slot, but first flags the
 * current scenario as visited in that profile's campaign progress and saves it.
 *
 * @address 0x00539d50
 */
void mark_level_visited_and_select(int16_t local_player_index)
{
    int16_t current_level;
    int16_t difficulty;
    int32_t handle;
    saved_player_profile profile;

    current_level = halo::main::campaign_level_find_index_for_path(unknown_00719779);
    difficulty = halo::main::globals().game_globals->difficulty;

    if (local_player_index < 0 || 1 <= local_player_index) {
        return;
    }
    handle = profile_globals_block[local_player_index].handle;
    if (handle == -1) {
        return;
    }

    profile = profile_globals_block[local_player_index].profile;
    profile.campaign_progress[current_level] |= (uint8_t)(1 << (difficulty & 0x1f));
    halo::saved_games::player_profile_write_data(handle, &profile);
    player_profile_load(local_player_index, &profile, handle);
}

/**
 * Renames the saved-game (profile or playlist) identified by handle to new_name: creates a
 * fresh XCreateSaveGame slot under that name, copies the old files into it (profile: blam.sav,
 * savegame.bin and checkpoints via player_profile_copy_files; playlist: blam.lst via CopyFileA),
 * deletes the old slot, and rewrites the index entry's path and display name in place. Returns
 * 1 on success or if there was nothing to do (not found, no new name given, or new_name already
 * matches), 0 on failure, or (in one code path) the raw playlist CopyFileA result if it is
 * neither 0 nor 1.
 *
 * @address 0x0053ce80
 */
uint8_t rename(int32_t handle, uint16_t *new_name)
{
    uint32_t slot_index;
    saved_game_index_entry entry;
    uint8_t found;
    char directory[0x100];
    int32_t create_result;
    uint8_t result;

    slot_index = saved_game_handle_slot(handle);
    found = savegame_index_read_slot((int32_t)slot_index, &entry);
    if (found == 0) {
        return 1;
    }
    if (new_name == 0) {
        return 1;
    }
    if (*new_name == 0) {
        return 1;
    }
    if (wcscmp((const wchar_t *)entry.display_name, (const wchar_t *)new_name) == 0) {
        return 1;
    }

    memset(directory, 0, sizeof(directory));
    create_result = XCreateSaveGame(new_name, savegames_directory, 1, directory, k_saved_game_path_length);
    if (create_result != 0) {
        return 1;
    }

    if (entry.type == 0) {
        char dest_path[0x100];
        char old_directory[0x100];
        char *trunc;

        _snprintf(dest_path, k_path_maximum_length, "%s%s", directory, "blam.sav");
        strncpy(old_directory, entry.path, k_path_maximum_length);
        trunc = strstr(old_directory, "blam.sav");
        if (trunc != 0) {
            *trunc = '\0';
            result = (uint8_t)halo::saved_games::player_profile_copy_files(old_directory, directory);
        } else {
            goto finalize;
        }
        if (result == 1) {
        finalize:
            XDeleteSaveGame(entry.display_name, savegames_directory);
            strncpy(entry.path, dest_path, k_path_maximum_length);
            wcsncpy((wchar_t *)entry.display_name, (const wchar_t *)new_name, k_saved_game_display_name_length - 1);
            entry.path[0xff] = 0;
            entry.display_name[0x7f] = 0;
            savegame_index_write_slot((int32_t)slot_index, &entry);
            return 1;
        }
        if (result != 0) {
            return result;
        }
    } else {
        if (entry.type != 1) {
            goto rollback;
        }
        {
            char dest_path[0x100];

            _snprintf(dest_path, k_path_maximum_length, "%s%s", directory, "blam.lst");
            result = (uint8_t)CopyFileA(entry.path, dest_path, 1);
            if (result == 1) {
                XDeleteSaveGame(entry.display_name, savegames_directory);
                strncpy(entry.path, dest_path, k_path_maximum_length);
                wcsncpy((wchar_t *)entry.display_name, (const wchar_t *)new_name, k_saved_game_display_name_length - 1);
                entry.path[0xff] = 0;
                entry.display_name[0x7f] = 0;
                savegame_index_write_slot((int32_t)slot_index, &entry);
                return 1;
            }
            if (result != 0) {
                return result;
            }
        }
    }

rollback:
    XDeleteSaveGame(new_name, savegames_directory);
    return 0;
}

/**
 * Switches the active player profile to the given local slot, reloading it from disk when its
 * cached copy is stale.
 *
 * @address 0x00539cb0
 */
void select_local_slot(int16_t local_player_index)
{
    int16_t current_level;
    int32_t handle;
    saved_player_profile profile;

    current_level = halo::main::campaign_level_find_index_for_path(unknown_00719779);
    if (current_level == -1 || local_player_index < 0 || 1 <= local_player_index) {
        return;
    }

    handle = profile_globals_block[local_player_index].handle;
    if (handle == -1) {
        return;
    }

    profile = profile_globals_block[local_player_index].profile;
    if (profile.last_campaign_level != current_level) {
        profile.last_campaign_level = current_level;
        halo::saved_games::player_profile_write_data(handle, &profile);
    }
    player_profile_load(local_player_index, &profile, handle);
}

/**
 * Waits for the background profile verification thread to finish, then clears the cached default
 * profile buffer it produced.
 *
 * @address 0x00539a40
 */
void verify_thread_wait_and_clear(void)
{
    uint32_t exit_code;
    uint32_t *clear;
    int32_t i;

    if (player_profile_thread != 0) {
        do {
            while (GetExitCodeThread(player_profile_thread->handle, (LPDWORD)&exit_code) == 0) {
            }
        } while (exit_code == win32::k_still_active );
        CloseHandle(player_profile_thread->handle);
        player_profile_thread->handle = 0;
        player_profile_thread->in_use = 0;
    }

    clear = (uint32_t *)&default_profile_data;
    for (i = k_default_profile_clear_dwords; i != 0; i = i - 1) {
        *clear = 0;
        clear = clear + 1;
    }
}

/**
 * Generates the two default profile files (00.sav and 01.sav) in the default profiles
 * directory. Each is a freshly initialized profile with its crc.
 *
 * @address 0x0053a610
 */
void write_default_files(void)
{
    int32_t local_player_index;
    saved_player_profile_file file;
    char name[256];
    file_reference_record ref;
    char *end;

    for (local_player_index = 0; local_player_index < 2; local_player_index = local_player_index + 1) {
        halo::saved_games::player_profile_initialize(&file.profile, local_player_index, 0);
        _snprintf(name, k_path_maximum_length, "%s\\%02d.sav", default_player_profiles_directory, local_player_index);

        {
            uint8_t *zero = (uint8_t *)&ref;
            int32_t i;
            for (i = sizeof(ref); i != 0; i = i - 1) {
                *zero = 0;
                zero = zero + 1;
            }
        }
        ref.signature = k_file_reference_signature;
        ref.location = _file_location_absolute;

        if ((ref.flags & 1) != 0) {
            halo::saved_games::path_remove_last_component(ref.path);
        }

        if (name[0] != 0) {
            end = ref.path;
            while (*end != 0) {
                end = end + 1;
            }
            if (end != ref.path) {
                *end = '\\';
                end = end + 1;
                *end = 0;
                end = end + 1;
            }
            strncpy(end, name, k_path_maximum_length - (uint32_t)(end - (ref.path + 1)));
        }
        ref.flags |= 1;

        file.checksum = k_crc32_seed;
        halo::memory::crc32_update(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

        if (halo::saved_games::file_reference_create(&ref) != 0 && halo::saved_games::file_reference_open(&ref, 2) != 0 &&
            halo::saved_games::file_reference_seek(0, &ref) != 0) {
            halo::saved_games::file_reference_write(&ref, &file, sizeof(file));
            halo::saved_games::file_reference_close(&ref);
        }
    }
}

}  // namespace halo::saved_games::player_profile

namespace halo::saved_games::playlist_profile {

/**
 * For each of the k_default_game_variant_count built-in variant builders: builds the default
 * game_variant, writes it under default_playlists_directory\NN\blam.lst (creating/emptying the
 * NN directory first), gives it a localized display name from the ui\\default_multiplayer_
 * game_setting_names ustr tag's string list (index i, falling back to default_ustr_fallback_
 * string if that index is out of range or empty), stamps the built-in index into the high byte
 * of variant_flags, and crcs and writes the file. Counts successful writes into
 * default_game_variant_count and marks the save-game index dirty once done. No-ops if the ustr
 * tag isn't found.
 *
 * @address 0x0053bc70
 */
void create_default_profiles_on_disk(void)
{
    datum_index tag_id;
    int16_t i;
    game_variant *defaults_result;
    game_variant variant_scratch;
    char path[256];
    UnicodeStringList *name_list;
    UnicodeStringListString *entry;
    uint16_t *source_name;
    uint32_t source_size;
    game_variant_file variant_file;
    file_reference_record ref;
    char *end;
    uint8_t written;

    tag_id = halo::cache::tag_lookup(groups::unicode_string_list, (char *)"ui\\default_multiplayer_game_setting_names");
    if (tag_id == k_datum_index_none) {
        return;
    }

    for (i = 0; i < (int16_t)k_default_game_variant_count; i++) {
        defaults_result = default_game_variant_procs[i](&variant_scratch);
        memcpy(&variant_file.variant, defaults_result, sizeof(variant_file.variant));
        memset(variant_file.padding_09c, 0, sizeof(variant_file.padding_09c));

        _snprintf(path, k_path_maximum_length, "%s\\%02d", default_playlists_directory, i);
        halo::saved_games::directory_ensure_empty(path);
        strncat(path, "\\blam.lst", k_path_maximum_length);

        source_name = missing_string_text;
        name_list = (UnicodeStringList *)halo::cache::globals().tag_instances[datum_slot(tag_id)].data;
        if (0 <= i && i < (int32_t)name_list->strings.count) {
            entry = (UnicodeStringListString *)name_list->strings.pointer + i;
            source_size = entry->string.size;
            if (0 < (int32_t)source_size) {
                source_name = (uint16_t *)entry->string.pointer;
                *(uint16_t *)((uint8_t *)source_name + ((source_size & 0xfffffffe) - 2)) = 0;
            }
        }

        wcsncpy((wchar_t *)variant_file.variant.name, (const wchar_t *)source_name, k_game_variant_name_length - 1);
        variant_file.variant.name[0x17] = 0;
        variant_file.variant.variant_flags =
            (int16_t)((uint16_t)variant_file.variant.variant_flags | ((uint16_t)(uint8_t)i << 8));

        variant_file.checksum = k_crc32_seed;
        halo::memory::crc32_update(&variant_file.checksum, &variant_file.variant, sizeof(variant_file.variant));

        memset(&ref, 0, sizeof(ref));
        ref.signature = k_file_reference_signature;
        ref.location = _file_location_absolute;
        if ((ref.flags & _file_reference_is_file_bit) != 0) {
            halo::saved_games::path_remove_last_component(ref.path);
        }
        if (path[0] != '\0') {
            end = ref.path + strlen(ref.path);
            if (end != ref.path) {
                *end = '\\';
                end++;
                *end = '\0';
            }
            strncpy(end, path, k_path_maximum_length - (uint32_t)strlen(ref.path));
            ref.path[0xff] = '\0';
        }
        ref.flags |= _file_reference_is_file_bit;

        written = 0;
        if (halo::saved_games::file_reference_create(&ref) != 0 && halo::saved_games::file_reference_open(&ref, 2) != 0 &&
            halo::saved_games::file_reference_seek(0, &ref) != 0) {
            written = halo::saved_games::file_reference_write(&ref, &variant_file, sizeof(variant_file));
            halo::saved_games::file_reference_close(&ref);
        }
        if (written != 0) {
            default_game_variant_count = default_game_variant_count + 1;
        }
    }
    savegame_index_dirty = 1;
}

}  // namespace halo::saved_games::playlist_profile

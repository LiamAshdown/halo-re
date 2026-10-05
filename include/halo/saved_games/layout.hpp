/**
 * @file include/halo/saved_games/layout.hpp
 * Named constants and layout checks for the saved games module (profiles, checkpoints, game state files).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/render/d3d9.hpp"

namespace halo::saved_games {

/** Seed of every crc32 the saved game files carry. */
inline constexpr uint32_t k_crc32_seed = 0xffffffffu;

/** Number of usable characters of the fixed path buffers (the buffers are one byte larger). */
inline constexpr uint32_t k_path_maximum_length = 0xff;

/** Byte size of the fixed path and last-used-file buffers (the usable length is k_path_maximum_length). */
inline constexpr uint32_t k_path_buffer_size = 0x100;

/** Byte size of the scratch file body the profile and variant files are built in. */
inline constexpr uint32_t k_file_body_buffer_size = 0x2000;

/** Size of the Win32 find-data buffer the disk space check hands to the directory scan. */
inline constexpr uint32_t k_find_data_buffer_size = 0x344;

/** Dwords cleared from the savegame index file global at start-up. */
inline constexpr int32_t k_savegame_index_file_clear_dwords = 0x2c7;

/** Wide characters of the display-name search buffer (the last one is the terminator). */
inline constexpr int32_t k_display_name_search_characters = 0x200;

/** Largest number of entries the savegames directory may hold before saving is refused. */
inline constexpr int32_t k_maximum_saved_game_entries = 0x3e6;

/** File and directory names. */
inline constexpr char k_checkpoints_directory[] = "checkpoints\\";
inline constexpr char k_autosave_name[] = "autosave";
inline constexpr char k_autosave1_name[] = "autosave1";
inline constexpr char k_savegame_name[] = "savegame";
inline constexpr char k_player_profile_file_name[] = "blam.sav";
inline constexpr char k_game_variant_file_name[] = "blam.lst";
inline constexpr char k_game_state_file_name[] = "savegame.bin";

/** Index of the slot a saved game handle names. */
constexpr uint32_t saved_game_handle_slot(uint32_t handle) noexcept {
    return (handle >> 16) & 0xfff;
}

/** String ids of the fatal error dialogs the module raises. */
inline constexpr uint32_t k_error_game_state_io_string = 0x8b;
inline constexpr uint32_t k_error_game_state_io_title = 0x8c;
inline constexpr uint32_t k_error_game_state_mismatch_string = 0x89;
inline constexpr uint32_t k_error_game_state_mismatch_title = 0x7e;

/** Quit confirm dialog strings for storage problems. */
inline constexpr int16_t k_quit_error_low_disk_space = 0x21;
inline constexpr int16_t k_quit_error_too_many_saves = 0x22;
inline constexpr int16_t k_quit_error_index_full = 0x24;

/** Default video mode written into a fresh profile. */
inline constexpr int16_t k_default_low_screen_width = 640;
inline constexpr int16_t k_default_low_screen_height = 480;
inline constexpr int16_t k_default_refresh_rate = 60;
inline constexpr uint16_t k_default_server_port = 2302;
inline constexpr uint16_t k_default_client_port = 2303;
/** Connection type (0 56k .. 4 T1/LAN): sent when joining and caps the host's send rate to us. Retail defaults to
    DSL/cable low (140 kbit/s); the browser build's games run through a server relay, so it is locked at T1/LAN
    (the menus show it dimmed) whatever a profile has saved. */
inline constexpr uint8_t k_connection_type_t1_lan = 4;
#if defined(__EMSCRIPTEN__)
inline constexpr bool k_connection_type_locked = true;
inline constexpr uint8_t k_default_connection_type = k_connection_type_t1_lan;
#else
inline constexpr bool k_connection_type_locked = false;
inline constexpr uint8_t k_default_connection_type = 1;
#endif

/** Byte size of one game state header or pool header block (data_array and memory_pool share it). */
inline constexpr int32_t k_game_state_block_header_size = 0x38;

/** Number of dwords cleared from the default profile global when the profile service shuts down. */
inline constexpr int32_t k_default_profile_clear_dwords = 0x1001;

/** Dwords cleared from the variant write request (also covers the thread pointer and default variant counter after it). */
inline constexpr int32_t k_variant_write_request_clear_dwords = 0x29;

/** Dwords between two entries of the input device slot table (one input_device record). */
inline constexpr int32_t k_input_device_stride_dwords = 0x90;

/** Machine class thresholds that choose between low and high default options. */
inline constexpr int32_t k_fast_machine_cpu_speed_mhz = 1000;
inline constexpr int32_t k_fast_machine_physical_memory_mb = 0x80;

inline constexpr uint32_t k_pixel_shader_version_1_1 = d3d9::k_pixel_shader_version_1_1;
inline constexpr uint32_t k_fast_machine_video_memory_bytes = 0x2000000;

/** Scale that converts a 0..255 color byte to 0..1. */
inline constexpr float k_color_byte_scale = 0.003921569f;

}  // namespace halo::saved_games

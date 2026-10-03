/**
 * @file include/halo/main/layout.hpp
 * Named constants and flag helpers for the main module (main loop, level control, movie player, console).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/render/d3d9.hpp"
#include "halo/tags/flags.hpp"

namespace halo::main {

/** Control word loaded by the optional x87 reinitialisation in the main loop (extended precision, all exceptions masked). */
inline constexpr uint16_t k_x87_control_word = 0x7e;

/** Ticks the user interface stays paused after a map load. */
inline constexpr int32_t k_ui_pause_pending_ticks = 0x1e;

/** Extra milliseconds before the loading screen address timers fire after a map load. */
inline constexpr int32_t k_loading_screen_extra_delay_ms = 0x6d6;
inline constexpr uint32_t k_loading_screen_maximum_delay_ms = 2000;

/** Smallest render skip threshold the loop accepts, in milliseconds. */
inline constexpr int32_t k_minimum_render_skip_threshold_ms = 0x14;

/** Frame time added per iteration while the clock is not running (one 30 Hz tick). */
inline constexpr uint32_t k_fallback_frame_time_ms = 0x21;

/** Size of a ban list record. */
inline constexpr int32_t k_ban_list_element_size = 0x38;

/** Method table index of IDirectInputDevice8::GetDeviceData. */
inline constexpr uint32_t k_directinput_get_device_data_slot = 0x28 / 4;

/** Salt of the scenario load request the front end builds. */
inline constexpr uint32_t k_scenario_load_request_salt = 0xdeadbeef;

/** Tag groups looked up by the main module. */
inline constexpr uint32_t k_looping_sound_group = fourcc('l', 's', 'n', 'd');
inline constexpr uint32_t k_ui_widget_definition_group = fourcc('D', 'e', 'L', 'a');
inline constexpr uint32_t k_bitmap_group = fourcc('b', 'i', 't', 'm');

/** Signature of a file reference record ('filo'). */
inline constexpr uint32_t k_file_reference_signature = fourcc('f', 'i', 'l', 'o');

/** Fixed names. */
inline constexpr char k_core_dump_file_name[] = "core.bin";
inline constexpr char k_default_scenario_path[] = "levels\\b30\\b30";
inline constexpr char k_ui_scenario_path[] = "levels\\ui\\ui";
inline constexpr char k_ban_list_file_name[] = "banned.txt";
inline constexpr char k_main_menu_widget_name[] = "the_main_menu";

/** Join error code shown when a connection attempt fails without a more specific reason. */
inline constexpr int16_t k_join_error_connection_failed = 0x35;

/** Offset of the first address pointer array inside a hostent record. */
inline constexpr size_t k_hostent_address_list_offset = 0xc;

/** Number of campaign progress records in the hud message table walked on bsp switches. */
inline constexpr int32_t k_hud_messaging_entry_count = 4;
inline constexpr size_t k_hud_messaging_entry_size = 0x8c;
inline constexpr size_t k_hud_messaging_active_offset = 0x82;

/** Count of dwords in the player profile cache and the active game variant copy. */
inline constexpr int32_t k_player_profile_cache_size = 0xc0 * 4;
inline constexpr int32_t k_game_engine_active_variant_size = 0x26 * 4;

}  // namespace halo::main

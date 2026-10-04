#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/shared_links.hpp"

#ifdef interface
#undef interface
#endif

/**
 * Link names of the engine globals the interface module reads and writes. The definitions live in the
 * standalone data layer under these original names; everything in the module reaches them through the
 * named references of halo::interface::state below.
 */
inline auto &console_tab_text = halo::link::ref<char [4]>(halo::ui::vars().console_tab_text);
inline auto &console_newline_text = halo::link::ref<char [4]>(halo::ui::vars().console_newline_text);
inline auto &console_newline_escape = halo::link::ref<char [4]>(halo::ui::vars().console_newline_escape);
inline auto &vehicle_options_team_page = halo::link::ref<int32_t>(halo::ui::vars().vehicle_options_team_page);
inline auto &escape_key_state = halo::link::ref<uint8_t>(halo::ui::vars().escape_key_state);
inline auto &profile_slot_flag = halo::link::ref<uint8_t>(halo::ui::vars().profile_slot_flag);
inline auto &current_campaign_level_path = halo::link::ref<char [255]>(halo::ui::vars().current_campaign_level_path);
inline auto &vehicle_options_respawn_time = halo::link::ref<uint32_t>(halo::ui::vars().vehicle_options_respawn_time);
inline auto &vehicle_options_red_set = halo::link::ref<uint32_t>(halo::ui::vars().vehicle_options_red_set);
inline auto &vehicle_options_blue_set = halo::link::ref<uint32_t>(halo::ui::vars().vehicle_options_blue_set);
inline auto &chat_window_unused_6b38f4 = halo::link::ref<int32_t>(halo::ui::vars().chat_window_unused_6b38f4);
inline auto &chat_window_unused_6b3914 = halo::link::ref<int32_t>(halo::ui::vars().chat_window_unused_6b3914);
inline auto &screen_fade_progress = halo::link::ref<float>(halo::ui::vars().screen_fade_progress);

namespace halo::interface::state {

/** Text the console writes to the Win32 terminal in place of the "|t" escape: a tab. @address 0x65fb2c */
inline char (&console_tab_text)[4] = ::console_tab_text;

/** Text the console writes in place of the "|n" escape, and after every line: a newline. @address 0x65fb14 */
inline char (&console_newline_text)[4] = ::console_newline_text;

/** The "|n" escape sequence the console replaces with a newline. @address 0x669ae0 */
inline char (&console_newline_escape)[4] = ::console_newline_escape;

/**
 * Index (0 or 1) of the autopatch download slot started by the server browser, or -1 when none is running.
 * Reset to -1 by the server browser on open and close and by the browser tick when the download ends.
 *
 * @address 0x695420
 */
inline int32_t &autopatch_active_slot = DAT_00695420;

/**
 * Which team's vehicle set the vehicle options screen is editing: 0 red, 1 blue.
 *
 * @address 0x692b0c
 */
inline int32_t &vehicle_options_team_page = ::vehicle_options_team_page;

/**
 * Object level-of-detail quality set by the video options: 0 low (radius scaled by 0.25), 1 medium (0.5),
 * 2 full. Read by the object level-of-detail pixel computation.
 *
 * @address 0x689450
 */
inline int16_t &object_lod_quality = unknown_00689450;

/**
 * Non-zero while decals and lens flares are rendered; the video options set it every time they apply.
 *
 * @address 0x6893ff
 */
inline uint8_t &decals_and_lens_flares_enabled = unknown_006893ff;

/**
 * The 30 fps frame limiter: set when the profile's frame rate mode is 2, forced off while a single tick is
 * being forced. Read by the main loop frame pacer.
 *
 * @address 0x6894ba
 */
inline uint8_t &frame_rate_limiter_enabled = unknown_006894ba;

/**
 * State of the escape key (system_key_states[1]); the pause menu only opens while it equals 1.
 *
 * @address 0x7127d1
 */
inline uint8_t &escape_key_state = ::escape_key_state;

/**
 * The flag byte of the first profile record, stashed by the profile save routine and read for the profile
 * status widget (records are 0x2004 bytes apart).
 *
 * @address 0x712f07
 */
inline uint8_t &profile_slot_flag = ::profile_slot_flag;

/**
 * Set when the main loop must reset the round of the running game (it disposes and re-creates the game
 * engine round) and cleared once that is done. Set when a network game is left or a revert completes.
 *
 * @address 0x719738
 */
inline uint8_t &round_reset_pending = unknown_00719738;

/**
 * Path of the campaign level the single player session is playing or about to play.
 *
 * @address 0x719779
 */
inline char (&current_campaign_level_path)[255] = ::current_campaign_level_path;

/**
 * Vehicle respawn time in ticks being edited on the vehicle options screen.
 *
 * @address 0x719208
 */
inline uint32_t &vehicle_options_respawn_time = ::vehicle_options_respawn_time;

/**
 * Packed red team vehicle set being edited on the vehicle options screen (game_variant::red_vehicle_set).
 *
 * @address 0x879f34
 */
inline uint32_t &vehicle_options_red_set = ::vehicle_options_red_set;

/**
 * Packed blue team vehicle set being edited on the vehicle options screen (game_variant::blue_vehicle_set).
 *
 * @address 0x879f38
 */
inline uint32_t &vehicle_options_blue_set = ::vehicle_options_blue_set;

/**
 * Chat window field zeroed by the window reset (next to the listbox rectangle). Nothing in the image reads it.
 *
 * @address 0x6b38f4
 */
inline int32_t &chat_window_unused_6b38f4 = ::chat_window_unused_6b38f4;

/**
 * Chat window field zeroed by the window reset. Nothing in the image reads it.
 *
 * @address 0x6b3914
 */
inline int32_t &chat_window_unused_6b3914 = ::chat_window_unused_6b3914;

/**
 * Progress of the screen fade used around level transitions and error dialogs: 0 to 1 while fading, -1 when
 * no fade is active. Scales the alpha of the full-screen fade quad.
 *
 * @address 0x718fa8
 */
inline float &screen_fade_progress = ::screen_fade_progress;

}
